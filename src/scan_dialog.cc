/*
 * Copyright (C) 2010 Michael Lamothe
 *
 * This file is part of Me TV
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "scan_dialog.h"
#include "dvb_scanner.h"
#include "thread.h"
#include "application.h"
#include "channels_conf_line.h"
#include "initial_scan_line.h"

void show_error(const Glib::ustring& message)
{
	GdkLock gdk_lock;
	Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.set_title(_("Me TV - Error"));
	dialog.run();
}

ScanThread::ScanThread(Dvb::Frontend& scan_frontend, Dvb::TransponderList& t) :
	Thread("Scan"), transponders(t), frontend(scan_frontend)
{
}

void ScanThread::run()
{
	scanner.start(frontend, transponders);
}

void ScanThread::stop()
{
	scanner.terminate();
}

ScanDialog& ScanDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ScanDialog* scan_dialog = NULL;
	builder->get_widget_derived("window_scan_wizard", scan_dialog);
	return *scan_dialog;
}

Glib::ustring ScanDialog::get_initial_tuning_dir()
{
	Glib::ustring result;
	gboolean done = false;
	guint i = 0;

	std::vector<Glib::ustring> parts;
	split_string(parts, SCAN_DIRECTORIES, ":", false, 100);

	while (!done)
	{
		if (i >= parts.size())
		{
			done = true;
		}
		else
		{
			Glib::ustring scan_directory = parts[i];
			
			g_debug("Checking '%s'", scan_directory.c_str());
		
			if (Gio::File::create_for_path(scan_directory)->query_exists())
			{
				done = true;
				result = scan_directory;

				switch(frontend->get_frontend_info().type)
				{
				case FE_OFDM:   result += "/dvb-t";       break;
				case FE_QAM:    result += "/dvb-c";       break;
				case FE_QPSK:   result += "/dvb-s";       break;
				case FE_ATSC:   result += "/atsc";        break;
				default:		throw Exception(_("Unknown frontend type"));
				}

				g_debug("Found '%s'", result.c_str());
			}

			i++;
		}
	}
	
	return result;
}

ScanDialog::ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Window(cobject), builder(builder)
{
	scan_thread = NULL;

	builder->get_widget("notebook_scan_wizard", notebook_scan_wizard);

	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_wizard_add", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_add_clicked));

	builder->get_widget("button_scan_wizard_next", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_next_clicked));

	builder->get_widget("button_scan_wizard_cancel", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_cancel_clicked));

	builder->get_widget("button_scan_stop", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScanDialog::on_button_scan_stop_clicked));

	notebook_scan_wizard->set_show_tabs(false);
		
	frontend = *(get_application().device_manager.get_frontends().begin());
	Glib::ustring device_name = frontend->get_frontend_info().name;

	Gtk::Label* label = NULL;
	builder->get_widget("label_scan_device", label);
	label->set_label(device_name);
		
	builder->get_widget("progress_bar_scan", progress_bar_scan);
	builder->get_widget("tree_view_scanned_channels", tree_view_scanned_channels);
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_scanned_channels->set_model(list_store);
	tree_view_scanned_channels->append_column(_("Service Name"), columns.column_name);
	tree_view_scanned_channels->append_column(_("Signal Strength"), columns.column_signal_strength);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);

	Gtk::FileChooserButton* file_chooser_button_scan = NULL;
	builder->get_widget("file_chooser_button_scan", file_chooser_button_scan);
	file_chooser_button_scan->set_current_folder(get_initial_tuning_dir());
	file_chooser_button_scan->signal_file_set().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_scan_file_set));
	
	Gtk::FileChooserButton* file_chooser_button_import = NULL;
	builder->get_widget("file_chooser_button_import", file_chooser_button_import);
	file_chooser_button_import->signal_file_set().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_import_file_set));

	ComboBoxText* combo_box_auto_scan_range = NULL;
	builder->get_widget_derived("combo_box_auto_scan_range", combo_box_auto_scan_range);

	combo_box_auto_scan_range->clear_items();
	combo_box_auto_scan_range->append_text(_("Australia"),		"AU");
	combo_box_auto_scan_range->append_text(_("Canada"),			"CA");
	combo_box_auto_scan_range->append_text(_("Finland"),		"FI");
	combo_box_auto_scan_range->append_text(_("France"),			"FR");
	combo_box_auto_scan_range->append_text(_("Germany"),		"DE");
	combo_box_auto_scan_range->append_text(_("Italy"),			"IT");
	combo_box_auto_scan_range->append_text(_("Lithuania"),		"LT");
	combo_box_auto_scan_range->append_text(_("New Zealand"),	"NZ");
	combo_box_auto_scan_range->append_text(_("Spain"),			"ES");
	combo_box_auto_scan_range->append_text(_("Slovenia"),		"SI");
	combo_box_auto_scan_range->append_text(_("United Kingdom"), "UK");
	combo_box_auto_scan_range->append_text(_("United States"),  "US");
	combo_box_auto_scan_range->set_active_value("AU");

	combo_box_auto_scan_range->signal_changed().connect(
	    sigc::mem_fun(*this, &ScanDialog::on_combo_box_auto_scan_range_changed));

	gboolean enable_auto_scan = false;
	switch (frontend->get_frontend_info().type)
	{
		case FE_OFDM:
		case FE_ATSC:
			enable_auto_scan = true;
			break;
		default:
			break;
	}		
	combo_box_auto_scan_range->set_sensitive(enable_auto_scan);

	Gtk::Label* label_auto_scan = NULL;
	builder->get_widget("label_auto_scan", label_auto_scan);
	label_auto_scan->set_sensitive(enable_auto_scan);

	Gtk::RadioButton* radio_button_auto_scan = NULL;
	builder->get_widget("radio_button_auto_scan", radio_button_auto_scan);
	radio_button_auto_scan->set_sensitive(enable_auto_scan);
	radio_button_auto_scan->set_active(enable_auto_scan);
}

ScanDialog::~ScanDialog()
{
	stop_scan();
}

void ScanDialog::on_show()
{
	channel_count = 0;
	progress_bar_scan->set_fraction(0);
	transponders.clear();
	
	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_stop", button);
	button->hide();

	builder->get_widget("button_scan_wizard_cancel", button);
	button->show();

	builder->get_widget("button_scan_wizard_add", button);
	button->hide();

	builder->get_widget("button_scan_wizard_next", button);
	button->show();
	
	notebook_scan_wizard->set_current_page(0);

	Window::on_show();
}

void ScanDialog::on_hide()
{
	stop_scan();
	Window::on_hide();
}

void ScanDialog::stop_scan()
{
	if (scan_thread != NULL)
	{
		g_debug("Stopping scan");
		
		Dvb::Scanner& scanner = scan_thread->get_scanner();
		scanner.signal_service.clear();
		scanner.signal_progress.clear();
		scanner.signal_complete.clear();
		
		scan_thread->stop();
		scan_thread->join(true);
		delete scan_thread;
		scan_thread = NULL;

		get_application().stream_manager.start();
		get_application().select_channel_to_play();
	}
}

void ScanDialog::on_file_chooser_button_scan_file_set()
{
	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);
	radio_button_scan->set_active();
}

void ScanDialog::on_file_chooser_button_import_file_set()
{
	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	radio_button_import->set_active();
}

void ScanDialog::on_button_scan_wizard_cancel_clicked()
{
	stop_scan();
	list_store->clear();
	hide();
}

void ScanDialog::on_button_scan_stop_clicked()
{	
	stop_scan();

	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_stop", button);
	button->hide();

	builder->get_widget("button_scan_wizard_add", button);
	button->show();

	builder->get_widget("button_scan_wizard_cancel", button);
	button->show();
}

void ScanDialog::import_channels_conf(const Glib::ustring& channels_conf_path)
{
	Glib::RefPtr<Glib::IOChannel> file = Glib::IOChannel::create_from_file(channels_conf_path, "r");
	Glib::ustring line;
	guint line_count = 0;

	progress_bar_scan->set_fraction(0);
	progress_bar_scan->set_text(_("Importing channels"));

	Gtk::Button* button = NULL;
	builder->get_widget("button_scan_wizard_next", button);
	button->hide();
	notebook_scan_wizard->next_page();

	while (file->read_line(line) == Glib::IO_STATUS_NORMAL)
	{
		ChannelsConfLine channels_conf_line(line);
		guint parameter_count = channels_conf_line.get_parameter_count();

		line_count++;
		
		g_debug("Line %d (%d parameters): '%s'", line_count, parameter_count, line.c_str());
	
		if (parameter_count > 1)
		{
			Channel channel;
			channel.transponder.frontend_type = frontend->get_frontend_info().type;
			
			switch(channel.transponder.frontend_type)
			{
				case FE_OFDM:
					if (parameter_count != 13)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = line_count;
			
					channel.transponder.frontend_parameters.frequency						= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion						= channels_conf_line.get_inversion(2);
					channel.transponder.frontend_parameters.u.ofdm.bandwidth				= channels_conf_line.get_bandwidth(3);
					channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= channels_conf_line.get_fec(4);
					channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= channels_conf_line.get_fec(5);
					channel.transponder.frontend_parameters.u.ofdm.constellation			= channels_conf_line.get_modulation(6);
					channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= channels_conf_line.get_transmit_mode(7);
					channel.transponder.frontend_parameters.u.ofdm.guard_interval			= channels_conf_line.get_guard_interval(8);
					channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= channels_conf_line.get_hierarchy(9);
					channel.service_id														= channels_conf_line.get_service_id(12);
					break;
				
				case FE_QAM:
					if (parameter_count != 9)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = line_count;
			
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion			= channels_conf_line.get_inversion(2);
					channel.transponder.frontend_parameters.u.qam.symbol_rate	= channels_conf_line.get_symbol_rate(3);
					channel.transponder.frontend_parameters.u.qam.fec_inner		= channels_conf_line.get_fec(4);
					channel.transponder.frontend_parameters.u.qam.modulation	= channels_conf_line.get_modulation(5);
					channel.service_id											= channels_conf_line.get_service_id(8);
					break;

				case FE_QPSK:
					if (parameter_count != 8)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = line_count;
			
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1)*1000;
					channel.transponder.polarisation							= channels_conf_line.get_polarisation(2);
					channel.transponder.satellite_number						= channels_conf_line.get_int(3);
					channel.transponder.frontend_parameters.u.qpsk.symbol_rate	= channels_conf_line.get_int(4) * 1000;
					channel.transponder.frontend_parameters.u.qpsk.fec_inner	= FEC_AUTO;
					channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
					channel.service_id											= channels_conf_line.get_service_id(7);
					break;

				case FE_ATSC:
					if (parameter_count != 6)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = line_count;
			
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
					channel.transponder.frontend_parameters.u.vsb.modulation	= channels_conf_line.get_modulation(2);
					channel.service_id											= channels_conf_line.get_service_id(5);
					break;
				
				default:
					throw Exception(_("Failed to import: importing a channels.conf is only supported with DVB-T, DVB-C, DVB-S and ATSC"));
			}

			add_channel_row(channel, 0);
		}		
	}

	builder->get_widget("button_scan_wizard_add", button);
	button->show();

	g_debug("Finished importing channels");
}

void ScanDialog::add_channel_row(const Channel& channel, guint signal_strength)
{	
	Gtk::TreeModel::iterator iterator = list_store->append();
	
	Gtk::TreeModel::Row row					= *iterator;
	row[columns.column_id]					= channel.service_id;
	row[columns.column_name]				= channel.name;
	row[columns.column_frontend_parameters]	= channel.transponder.frontend_parameters;
	row[columns.column_polarisation]		= channel.transponder.polarisation;
	row[columns.column_signal_strength]		= Glib::ustring::compose("%1%%", ((0xFFFF & signal_strength) * 100)/65356);
	tree_view_scanned_channels->get_selection()->select(row);

	channel_count++;
	g_debug("Added channel %d : %s", channel.service_id, channel.name.c_str());
}

void ScanDialog::load_initial_tuning_file(const Glib::ustring& initial_tuning_file_path)
{
	Glib::RefPtr<Glib::IOChannel> initial_tuning_file = Glib::IOChannel::create_from_file(initial_tuning_file_path, "r");

	std::list<Glib::ustring> lines;
	Glib::ustring line;
	Glib::IOStatus status = initial_tuning_file->read_line(line);
	while (status == Glib::IO_STATUS_NORMAL)
	{
		// Remove comments
		Glib::ustring::size_type index = line.find("#");
		if (index != Glib::ustring::npos)
		{
			line = line.substr(0, index);
		}

		// Remove trailing whitespace
		index = line.find_last_not_of(" \t\r\n");
		if (index == Glib::ustring::npos)
		{
			line.clear();
		}
		else
		{
			line = line.substr(0, index + 1);
		}

		// Ignore empty lines or comments
		if (!line.empty())
		{
			lines.push_back(line);
		}

		status = initial_tuning_file->read_line(line);
	}

	guint size = lines.size();

	for (StringList::iterator iterator = lines.begin(); iterator != lines.end(); iterator++)
	{
		Glib::ustring process_line = *iterator;

		if (!process_line.empty())
		{
			g_debug("Processing line: '%s'", process_line.c_str());

			if (Glib::str_has_prefix(process_line, "T "))
			{
				process_terrestrial_line(process_line);
			}
			else if (Glib::str_has_prefix(process_line, "C "))
			{
				process_cable_line(process_line);
			}
			else if (Glib::str_has_prefix(process_line, "S "))
			{
				process_satellite_line(process_line);
			}
			else if (Glib::str_has_prefix(process_line, "A "))
			{
				process_atsc_line(process_line);
			}
			else
			{
				throw Exception(_("Me TV cannot process a line in the initial tuning file"));
			}
		}
	}
}

void ScanDialog::on_button_scan_wizard_next_clicked()
{
	stop_scan();

	list_store->clear();

	Glib::ustring initial_tuning_file;
	
	Gtk::RadioButton* radio_button_auto_scan = NULL;
	builder->get_widget("radio_button_auto_scan", radio_button_auto_scan);

	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);

	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	
	if (radio_button_scan->get_active() || radio_button_auto_scan->get_active())
	{
		if (radio_button_scan->get_active())
		{
			Gtk::FileChooserButton* file_chooser_button = NULL;
			builder->get_widget("file_chooser_button_scan", file_chooser_button);
			initial_tuning_file = file_chooser_button->get_filename();

			if (initial_tuning_file.empty())
			{
				throw Exception(_("No tuning file has been selected"));
			}

			g_debug("Initial tuning file: '%s'", initial_tuning_file.c_str());

			load_initial_tuning_file(initial_tuning_file);
		}
		else
		{
			ComboBoxText* combo_box_auto_scan_range = NULL;
			builder->get_widget_derived("combo_box_auto_scan_range", combo_box_auto_scan_range);
			add_auto_scan_range(frontend->get_frontend_info().type, combo_box_auto_scan_range->get_active_value());
		}
		
		Gtk::Button* button = NULL;

		builder->get_widget("button_scan_wizard_cancel", button);
		button->hide();

		builder->get_widget("button_scan_stop", button);
		button->show();

		builder->get_widget("button_scan_wizard_next", button);
		button->hide();
		
		notebook_scan_wizard->next_page();

		progress_bar_scan->set_text(_("Starting scanner"));
		scan_thread = new ScanThread(*frontend, transponders);
		Dvb::Scanner& scanner = scan_thread->get_scanner();
		scanner.signal_service.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_service));
		scanner.signal_progress.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_progress));
		scanner.signal_complete.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_complete));
		get_application().stream_manager.stop();
		scan_thread->start();
	}
	else if (radio_button_import->get_active())
	{
		Gtk::FileChooserButton* file_chooser_button = NULL;
		builder->get_widget("file_chooser_button_import", file_chooser_button);
		Glib::ustring channels_conf_path = file_chooser_button->get_filename();
		import_channels_conf(channels_conf_path);
	}	
}

void ScanDialog::on_button_scan_wizard_add_clicked()
{
	hide();
}

void ScanDialog::on_signal_service(const struct dvb_frontend_parameters& frontend_parameters, guint service_id, const Glib::ustring& name, const guint polarisation, guint signal_strength)
{
	if (scan_thread != NULL && !scan_thread->is_terminated())
	{
		GdkLock gdk_lock;

		Gtk::TreeModel::Children children = list_store->children();
		Gtk::TreeModel::Children::iterator iterator = children.begin();
		while (iterator != children.end())
		{
			Gtk::TreeModel::Row row  = *iterator;

			guint							existing_service_id;
			struct dvb_frontend_parameters	existing_frontend_parameters;

			existing_service_id = row.get_value(columns.column_id);
			existing_frontend_parameters = row.get_value(columns.column_frontend_parameters);

			if (abs(frontend_parameters.frequency - existing_frontend_parameters.frequency) < 500000 &&
			    service_id == existing_service_id)
			{
				g_debug("Already got service ID %d at %d Hz, ignoring service at %d Hz",
				    existing_service_id,
				    existing_frontend_parameters.frequency,
				 	frontend_parameters.frequency);
			}
			
			iterator++;
		}

		Channel channel;
		channel.service_id						= service_id;
		channel.name							= name;
		channel.transponder.frontend_parameters = frontend_parameters;
		channel.transponder.polarisation		= polarisation;

		add_channel_row(channel, signal_strength);
	}
}

void ScanDialog::on_signal_progress(guint step, gsize total)
{
	if (scan_thread != NULL && !scan_thread->is_terminated())
	{
		GdkLock gdk_lock;
		gdouble fraction = total == 0 ? 0 : step/(gdouble)total;
		
		if (fraction < 0 || fraction > 1.0)
		{
			g_debug("Invalid fraction: %f", fraction);
			g_debug("STEP: %d", step);
			g_debug("TOTAL: %d", (guint)total);
		}
		else
		{
			progress_bar_scan->set_fraction(fraction);
			Glib::ustring text = Glib::ustring::compose(_("%1/%2 (%3 channels)"), step, total, channel_count);
			progress_bar_scan->set_text(text);
		}
	}
}

void ScanDialog::on_signal_complete()
{
	g_debug("Scan completed");
	
	if (scan_thread != NULL && !scan_thread->is_terminated())
	{
		GdkLock gdk_lock;

		Gtk::Button* button_scan_stop = NULL;
		builder->get_widget("button_scan_stop", button_scan_stop);
		button_scan_stop->hide();

		Gtk::Button* button_scan_wizard_cancel = NULL;
		builder->get_widget("button_scan_wizard_cancel", button_scan_wizard_cancel);
		button_scan_wizard_cancel->show();

		Gtk::Button* button_scan_wizard_add = NULL;
		builder->get_widget("button_scan_wizard_add", button_scan_wizard_add);
		button_scan_wizard_add->show();
		
		progress_bar_scan->set_fraction(1);
		progress_bar_scan->set_text(_("Scan complete"));
	}
}

void ScanDialog::on_combo_box_auto_scan_range_changed()
{
	Gtk::RadioButton* radio_button_auto_scan = NULL;
	builder->get_widget("radio_button_auto_scan", radio_button_auto_scan);
	radio_button_auto_scan->set_active();
}

ChannelArray ScanDialog::get_selected_channels()
{
	ChannelArray result;
	std::list<Gtk::TreeModel::Path> selected_services = tree_view_scanned_channels->get_selection()->get_selected_rows();		
	std::list<Gtk::TreeModel::Path>::iterator iterator = selected_services.begin();
	while (iterator != selected_services.end())
	{
		Gtk::TreeModel::Row row(*list_store->get_iter(*iterator));

		Channel channel;

		channel.service_id						= row.get_value(columns.column_id);
		channel.name							= row.get_value(columns.column_name);
		channel.transponder.frontend_type		= frontend->get_frontend_info().type;
		channel.transponder.frontend_parameters = row.get_value(columns.column_frontend_parameters);
		channel.transponder.polarisation		= row.get_value(columns.column_polarisation);
		
		result.push_back(channel);
		
		iterator++;
	}
	return result;
}

void ScanDialog::add_scan_range(guint start, guint end, guint step, struct dvb_frontend_parameters frontend_parameters)
{
	for (guint frequency = start; frequency <= end; frequency += step)
	{
		g_debug("Adding %d Hz", frequency);
		frontend_parameters.frequency = frequency;
		add_transponder(frontend_parameters);
	}
}

void ScanDialog::add_scan_list(const int *si, int length, struct dvb_frontend_parameters frontend_parameters)
{
	for (int index = 0; index < length; ++index)
	{
		frontend_parameters.frequency = si[index];
		add_transponder(frontend_parameters);
	}
}

void ScanDialog::add_auto_scan_range(fe_type_t frontend_type, const Glib::ustring& range)
{
	if (range.empty())
	{
		throw Exception(_("No auto scan range was specified"));
	}
	
	struct dvb_frontend_parameters frontend_parameters;

	if (frontend_type == FE_OFDM)
	{

		frontend_parameters.inversion						= INVERSION_AUTO;
		frontend_parameters.u.ofdm.hierarchy_information	= HIERARCHY_NONE;
		frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_AUTO;
		frontend_parameters.u.ofdm.code_rate_HP				= FEC_AUTO;
		frontend_parameters.u.ofdm.code_rate_LP				= FEC_AUTO;
		frontend_parameters.u.ofdm.constellation			= QAM_AUTO;
		frontend_parameters.u.ofdm.transmission_mode		= TRANSMISSION_MODE_AUTO;
		frontend_parameters.u.ofdm.guard_interval			= GUARD_INTERVAL_AUTO;

		if (range == "AU")
		{
			frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_7_MHZ;
			frontend_parameters.u.ofdm.transmission_mode		= TRANSMISSION_MODE_8K;
			frontend_parameters.u.ofdm.constellation			= QAM_64;
			
			add_scan_range(177500000, 226500000, 7000000, frontend_parameters);
			add_scan_range(529500000, 816500000, 7000000, frontend_parameters);
		}
		else if (range == "DE" || range == "IT")
		{
			frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_7_MHZ;
			frontend_parameters.u.ofdm.transmission_mode		= TRANSMISSION_MODE_8K;
			
			add_scan_range(177500000, 226500000, 7000000, frontend_parameters);

			frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_8_MHZ;
			add_scan_range(474000000, 858000000, 8000000, frontend_parameters);
		}
		else if (range == "FR")
		{
			frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_8_MHZ;
			frontend_parameters.u.ofdm.transmission_mode		= TRANSMISSION_MODE_8K;
			add_scan_range(474000000, 850000000, 8000000, frontend_parameters);
		}
		else if (range == "ES" || range == "LT")
		{
			frontend_parameters.u.ofdm.bandwidth		= BANDWIDTH_8_MHZ;
			add_scan_range(474000000, 858000000, 8000000, frontend_parameters);
		}
		else if (range == "FI" || range == "SE")
		{
			frontend_parameters.u.ofdm.bandwidth		= BANDWIDTH_8_MHZ;
			frontend_parameters.u.ofdm.constellation	= QAM_64;
			add_scan_range(474000000, 850000000, 8000000, frontend_parameters);
		}
		else if (range == "NZ")
		{
			frontend_parameters.u.ofdm.bandwidth				= BANDWIDTH_8_MHZ;
			frontend_parameters.u.ofdm.code_rate_HP				= FEC_3_4;
			frontend_parameters.u.ofdm.code_rate_LP				= FEC_3_4;
			frontend_parameters.u.ofdm.transmission_mode		= TRANSMISSION_MODE_8K;
			frontend_parameters.u.ofdm.guard_interval			= GUARD_INTERVAL_1_16;
			add_scan_range(474000000, 858000000, 8000000, frontend_parameters);
		}
		else if (range == "SI")
		{
			static const int si[] = { 514000000, 602000000 };
			add_scan_list(si, sizeof(si) / sizeof(si[0]), frontend_parameters);
		}
		else if (range == "UK")
		{
			frontend_parameters.u.ofdm.guard_interval		= GUARD_INTERVAL_1_32;
			frontend_parameters.u.ofdm.bandwidth			= BANDWIDTH_8_MHZ;
			frontend_parameters.u.ofdm.transmission_mode	= TRANSMISSION_MODE_2K;
			frontend_parameters.inversion					= INVERSION_OFF;
			add_scan_range(474000000, 850000000, 8000000, frontend_parameters);
		}
		else
		{
			throw Exception(Glib::ustring::compose(_("Unknown scan range '%1'"), range));
		}
 	}
	else if (frontend_type == FE_ATSC)
	{
		frontend_parameters.inversion			= INVERSION_AUTO;
		frontend_parameters.u.vsb.modulation	= VSB_8;
		add_scan_range(57028615, 70000000, 6000000, frontend_parameters);
		add_scan_range(79028615, 90000000, 6000000, frontend_parameters);
		add_scan_range(177028615, 215000000, 6000000, frontend_parameters);
		add_scan_range(473028615, 805000000, 6000000, frontend_parameters);
			
	}
	else
	{
		throw Exception(_("Auto scanning is only supported on DVB-T and ATSC devices"));
	}
}

void ScanDialog::add_transponder(struct dvb_frontend_parameters frontend_parameters)
{
	Dvb::Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;
	transponders.push_back(transponder);
}

void ScanDialog::process_terrestrial_line(const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;

	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency						= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion						= INVERSION_AUTO;
	frontend_parameters.u.ofdm.bandwidth				= initial_scan_line.get_bandwidth(2);
	frontend_parameters.u.ofdm.code_rate_HP				= initial_scan_line.get_fec(3);
	frontend_parameters.u.ofdm.code_rate_LP				= initial_scan_line.get_fec(4);
	frontend_parameters.u.ofdm.constellation			= initial_scan_line.get_modulation(5);
	frontend_parameters.u.ofdm.transmission_mode		= initial_scan_line.get_transmit_mode(6);
	frontend_parameters.u.ofdm.guard_interval			= initial_scan_line.get_guard_interval(7);
	frontend_parameters.u.ofdm.hierarchy_information	= initial_scan_line.get_hierarchy(8);

	Dvb::Transponder transponder;
	transponder.frontend_type				= FE_OFDM;
	transponder.frontend_parameters = frontend_parameters;

	transponders.add(transponder);
}

void ScanDialog::process_atsc_line(const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;

	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.u.vsb.modulation	= initial_scan_line.get_modulation(2);
	frontend_parameters.inversion			= INVERSION_AUTO;

	Dvb::Transponder transponder;
	transponder.frontend_type		= FE_ATSC;
	transponder.frontend_parameters = frontend_parameters;
	
	transponders.add(transponder);
}

void ScanDialog::process_satellite_line(const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;
	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion			= INVERSION_AUTO;
	
	frontend_parameters.u.qpsk.symbol_rate	= initial_scan_line.get_symbol_rate(3);
	frontend_parameters.u.qpsk.fec_inner	= initial_scan_line.get_fec(4);

	Dvb::Transponder transponder;
	transponder.frontend_type		= FE_QPSK;
	transponder.frontend_parameters = frontend_parameters;
	transponder.polarisation		= initial_scan_line.get_polarisation(2);
	
	g_debug("Frequency %d, Symbol rate %d, FEC %d, polarisation %d", frontend_parameters.frequency, frontend_parameters.u.qpsk.symbol_rate, frontend_parameters.u.qpsk.fec_inner, transponder.polarisation);
	
	transponders.add(transponder);
}

void ScanDialog::process_cable_line(const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;
	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion			= INVERSION_AUTO;
	
	frontend_parameters.u.qam.symbol_rate	= initial_scan_line.get_symbol_rate(2);
	frontend_parameters.u.qam.fec_inner		= initial_scan_line.get_fec(3);
	frontend_parameters.u.qam.modulation	= initial_scan_line.get_modulation(4);

	Dvb::Transponder transponder;
	transponder.frontend_type				= FE_QAM;
	transponder.frontend_parameters = frontend_parameters;
	
	transponders.add(transponder);
}
