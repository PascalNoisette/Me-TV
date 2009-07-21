	/*
 * Copyright (C) 2009 Michael Lamothe
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

void show_error(const Glib::ustring& message)
{
	GdkLock gdk_lock;
	Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	dialog.set_title(_("Me TV - Error"));
	dialog.run();
}

ScanThread::ScanThread(Dvb::Frontend& scan_frontend, const Glib::ustring& file) :
	Thread("Scan"), scanner(), initial_tuning_file(file), frontend(scan_frontend)
{
}

void ScanThread::run()
{
	try
	{
		scanner.start(frontend, initial_tuning_file);
	}
	catch(const Exception& exception)
	{
		show_error(exception.what());
	}
	catch(...)
	{
		show_error(_("An unhandled error occurred"));
	}
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
	
	StringSplitter splitter(SCAN_DIRECTORIES, ":", false, 100);

	while (!done)
	{
		if (i >= splitter.get_count())
		{
			done = true;
		}
		else
		{
			Glib::ustring scan_directory = splitter.get_value(i);
			
			g_debug("Checking '%s'", scan_directory.c_str());
		
			if (Gio::File::create_for_path(scan_directory)->query_exists())
			{
				done = true;
				result = scan_directory;

				switch(frontend.get_frontend_info().type)
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
	Gtk::Window(cobject), builder(builder), frontend(get_application().device_manager.get_frontend())
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

	notebook_scan_wizard->set_show_tabs(false);
		
	Glib::ustring device_name = frontend.get_frontend_info().name;

	Gtk::Label* label = NULL;
	builder->get_widget("label_scan_device", label);
	label->set_label(device_name);
		
	builder->get_widget("progress_bar_scan", progress_bar_scan);
	builder->get_widget("tree_view_scanned_channels", tree_view_scanned_channels);
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_scanned_channels->set_model(list_store);
	tree_view_scanned_channels->append_column(_("Service Name"), columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);

	Gtk::FileChooserButton* file_chooser_button_scan = NULL;
	builder->get_widget("file_chooser_button_scan", file_chooser_button_scan);

	Gtk::FileChooserButton* file_chooser_button_import = NULL;
	builder->get_widget("file_chooser_button_import", file_chooser_button_import);

	file_chooser_button_scan->signal_selection_changed().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_scan_file_changed));
	file_chooser_button_import->signal_selection_changed().connect(sigc::mem_fun(*this, &ScanDialog::on_file_chooser_button_import_file_changed));
}

ScanDialog::~ScanDialog()
{
	stop_scan();
}

void ScanDialog::on_show()
{
	channel_count = 0;
	progress_bar_scan->set_fraction(0);

	Gtk::Button* button = NULL;

	builder->get_widget("button_scan_wizard_add", button);
	button->hide();

	builder->get_widget("button_scan_wizard_next", button);
	button->show();
	
	notebook_scan_wizard->set_current_page(0);

	Gtk::FileChooserButton* file_chooser_button = NULL;
	builder->get_widget("file_chooser_button_scan", file_chooser_button);
	
	Glib::ustring initial_tuning_file = get_initial_tuning_dir();
	file_chooser_button->set_current_folder(initial_tuning_file);

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
		scan_thread->stop();
		scan_thread->join(true);
		delete scan_thread;
		scan_thread = NULL;
	}
}

void ScanDialog::on_file_chooser_button_scan_file_changed()
{
	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);
	radio_button_scan->set_active();
}

void ScanDialog::on_file_chooser_button_import_file_changed()
{
	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	radio_button_import->set_active();
}

void ScanDialog::on_button_scan_wizard_cancel_clicked()
{
	list_store->clear();
	hide();
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
			
			switch(frontend.get_frontend_info().type)
			{
				case FE_OFDM:
					if (parameter_count != 13)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					channel.name = channels_conf_line.get_name(0);
					channel.sort_order = line_count;
					channel.flags = CHANNEL_FLAG_DVB_T;
			
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
					channel.flags = CHANNEL_FLAG_DVB_C;
			
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
					channel.flags = CHANNEL_FLAG_DVB_S;
			
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
					channel.flags = CHANNEL_FLAG_ATSC;
			
					channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_int(1);
					channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
					channel.transponder.frontend_parameters.u.vsb.modulation	= channels_conf_line.get_modulation(2);
					channel.service_id											= channels_conf_line.get_service_id(5);
					break;
				
				default:
					throw Exception(_("Failed to import: importing a channels.conf is only supported with DVB-T, DVB-C, DVB-S and ATSC"));
					break;
			}

			add_channel_row(channel);
		}		
	}

	builder->get_widget("button_scan_wizard_add", button);
	button->show();

	g_debug("Finished importing channels");
}

void ScanDialog::add_channel_row(const Channel& channel)
{	
	Gtk::TreeModel::iterator iterator = list_store->append();
	
	Gtk::TreeModel::Row row					= *iterator;
	row[columns.column_id]					= channel.service_id;
	row[columns.column_name]				= channel.name;
	row[columns.column_frontend_parameters]	= channel.transponder.frontend_parameters;
	row[columns.column_polarisation]		= channel.transponder.polarisation;
	tree_view_scanned_channels->get_selection()->select(row);

	channel_count++;
	g_debug("Added channel %d : %s", channel.service_id, channel.name.c_str());
}

void ScanDialog::on_button_scan_wizard_next_clicked()
{
	TRY
	stop_scan();

	list_store->clear();

	Glib::ustring initial_tuning_file;
	
	Gtk::RadioButton* radio_button_scan = NULL;
	builder->get_widget("radio_button_scan", radio_button_scan);

	Gtk::RadioButton* radio_button_import = NULL;
	builder->get_widget("radio_button_import", radio_button_import);
	
	if (radio_button_scan->get_active())
	{
		Gtk::FileChooserButton* file_chooser_button = NULL;
		builder->get_widget("file_chooser_button_scan", file_chooser_button);
		initial_tuning_file = file_chooser_button->get_filename();

		switch(frontend.get_frontend_info().type)
		{
			case FE_OFDM:
			case FE_QAM:
			case FE_QPSK:
				break;
			default:
				throw Exception(_("Failed to scan: scanning is only supported for DVB-T, DVB-S and DVB-C devices"));
		}

		if (initial_tuning_file.empty())
		{
			throw Exception(_("No tuning file has been selected"));
		}

		Gtk::Button* button = NULL;
		builder->get_widget("button_scan_wizard_next", button);
		button->hide();
		notebook_scan_wizard->next_page();

		g_debug("Initial tuning file: '%s'", initial_tuning_file.c_str());
		scan_thread = new ScanThread(frontend, initial_tuning_file);
		Dvb::Scanner& scanner = scan_thread->get_scanner();
		scanner.signal_service.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_service));
		scanner.signal_progress.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_progress));
		scanner.signal_complete.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_complete));
		get_application().stop_stream_thread();
		scan_thread->start();
	}
	else if (radio_button_import->get_active())
	{
		Gtk::FileChooserButton* file_chooser_button = NULL;
		builder->get_widget("file_chooser_button_import", file_chooser_button);
		Glib::ustring channels_conf_path = file_chooser_button->get_filename();
		import_channels_conf(channels_conf_path);
	}	
	CATCH
}

void ScanDialog::on_button_scan_wizard_add_clicked()
{
	TRY
	hide();
	CATCH;
}

void ScanDialog::on_signal_service(const struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name, const guint polarisation)
{
	if (!scan_thread->is_terminated())
	{
		GdkLock gdk_lock;
		
		Channel channel;
		channel.service_id						= id;
		channel.name							= name;
		channel.transponder.frontend_parameters = frontend_parameters;
		channel.transponder.polarisation		= polarisation;		

		add_channel_row(channel);
	}
}

void ScanDialog::on_signal_progress(guint step, gsize total)
{
	if (!scan_thread->is_terminated())
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
	if (!scan_thread->is_terminated())
	{
		GdkLock gdk_lock;

		Gtk::Button* button_scan_wizard_add = NULL;
		builder->get_widget("button_scan_wizard_add", button_scan_wizard_add);
		button_scan_wizard_add->show();
		progress_bar_scan->set_fraction(1);
		progress_bar_scan->set_text(_("Scan complete"));
	}
}

ChannelArray ScanDialog::get_channels()
{
	ChannelArray result;
	std::list<Gtk::TreeModel::Path> selected_services = tree_view_scanned_channels->get_selection()->get_selected_rows();		
	std::list<Gtk::TreeModel::Path>::iterator iterator = selected_services.begin();
	while (iterator != selected_services.end())
	{
		Gtk::TreeModel::Row row(*list_store->get_iter(*iterator));

		Channel channel;

		channel.service_id			= row.get_value(columns.column_id);
		channel.name				= row.get_value(columns.column_name);
		channel.transponder.frontend_parameters = row.get_value(columns.column_frontend_parameters);
		channel.transponder.polarisation = row.get_value(columns.column_polarisation);

		switch(frontend.get_frontend_info().type)
		{
			case FE_OFDM:
				channel.flags = CHANNEL_FLAG_DVB_T;
				break;
				
			case FE_QPSK:
				channel.flags = CHANNEL_FLAG_DVB_S;
				break;

			case FE_QAM:
				channel.flags = CHANNEL_FLAG_DVB_C;
				break;
				
			case FE_ATSC:
				channel.flags = CHANNEL_FLAG_ATSC;
				break;

			default:
				throw Exception(_("Invalid frontend type"));
				break;
		}
		
		result.push_back(channel);
		
		iterator++;
	}
	return result;	
}
