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

#include "scan_window.h"
#include "dvb_scanner.h"
#include "thread.h"
#include "application.h"
#include "channels_conf_line.h"
#include "profile.h"

ScanWindow* ScanWindow::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	ScanWindow* scan_window = NULL;
	glade->get_widget_derived("window_scan_wizard", scan_window);
	return scan_window;
}

Glib::ustring ScanWindow::get_initial_tuning_dir()
{
	Glib::ustring result;
	gboolean done = false;
	guint i = 0;
	
	StringSplitter splitter(SCAN_DIRECTORIES, ":", 100);

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

bool compare_countries (const Country& a, const Country& b)
{
	return a.name < b.name;
}

ScanWindow::ScanWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml) :
	Gtk::Window(cobject), glade(glade_xml), frontend(get_application().get_device_manager().get_frontend())
{
	scan_thread = NULL;

	notebook_scan_wizard = dynamic_cast<Gtk::Notebook*>(glade->get_widget("notebook_scan_wizard"));
	label_scan_information = dynamic_cast<Gtk::Label*>(glade->get_widget("label_scan_information"));
	glade->connect_clicked("button_scan_wizard_add", sigc::mem_fun(*this, &ScanWindow::on_button_scan_wizard_add_clicked));
	glade->connect_clicked("button_scan_wizard_next", sigc::mem_fun(*this, &ScanWindow::on_button_scan_wizard_next_clicked));
	glade->connect_clicked("button_scan_wizard_cancel", sigc::mem_fun(*this, &ScanWindow::on_button_scan_wizard_cancel_clicked));

	notebook_scan_wizard->set_show_tabs(false);
		
	Glib::ustring device_name = frontend.get_frontend_info().name;
	dynamic_cast<Gtk::Label*>(glade->get_widget("label_scan_device"))->set_label(device_name);
		
	progress_bar_scan = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_scan"));
	tree_view_scanned_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_scanned_channels"));
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_scanned_channels->set_model(list_store);
	tree_view_scanned_channels->append_column(_("Service Name"), columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
	
	combo_box_select_country = NULL;
	glade->get_widget_derived("combo_box_select_country", combo_box_select_country);

	combo_box_select_region = NULL;
	glade->get_widget_derived("combo_box_select_region", combo_box_select_region);
	
	scan_directory_path = get_initial_tuning_dir();

	if (scan_directory_path.size() > 0)
	{
		Glib::RefPtr<Gio::File> scan_directory = Gio::File::create_for_path(scan_directory_path);
		g_debug("Scanning directory: %s", scan_directory_path.c_str());
			
		// This is a hack because I can't get scan_directory->enumerate_children() to work
		GFileEnumerator* children = g_file_enumerate_children(scan_directory->gobj(),
			"*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
		if (children != NULL)
		{
			GFileInfo* file_info = g_file_enumerator_next_file(children, NULL, NULL);
			while (file_info != NULL)
			{
				Glib::ustring name = g_file_info_get_name(file_info);
				if (name.substr(2,1) == "-" && frontend.get_frontend_info().type != FE_QPSK)
				{
					Glib::ustring country_name = name.substr(0,2);
					Glib::ustring region_name = name.substr(3);
					Country& country = get_country(country_name);
					country.regions.push_back(region_name);
				}
				else if(name.find("-") > 0 && frontend.get_frontend_info().type == FE_QPSK) // This is a DVB-S satellite file which contains the satellite name (>2 chars) in front of "-".
				{
					Glib::ustring satellite_name = name.substr(0, name.find("-"));
					Glib::ustring position = name.substr(name.find("-")+1);
					Country& satellite = get_country(satellite_name); // The satellite name is not actually a region name, but the controls are named that way.
					satellite.regions.push_back(position); // Likewise, the satellite position is not actually a region name.
				}
				file_info = g_file_enumerator_next_file(children, NULL, NULL);
			}

			// Populate controls
			countries.sort(compare_countries);
			CountryList::iterator country_iterator = countries.begin();
			while (country_iterator != countries.end())
			{
				Country& country = *country_iterator;
				country.regions.sort();
				combo_box_select_country->append_text(country.name);
				country_iterator++;
			}
			combo_box_select_country->signal_changed().connect(sigc::mem_fun(*this, &ScanWindow::on_combo_box_select_country_changed));
			combo_box_select_country->set_active(0);
		}
	}
}

ScanWindow::~ScanWindow()
{
	stop_scan();
}

void ScanWindow::on_show()
{
	channel_count = 0;
	update_channel_count();
	progress_bar_scan->set_fraction(0);
	glade->get_widget("button_scan_wizard_add")->hide();
	glade->get_widget("button_scan_wizard_next")->show();
	notebook_scan_wizard->set_current_page(0);
	Window::on_show();
}

void ScanWindow::on_hide()
{
	stop_scan();
	Window::on_hide();
}

void ScanWindow::on_combo_box_select_country_changed()
{
	combo_box_select_region->clear_items();
	Country& country = get_country(combo_box_select_country->get_active_text());
	StringList::iterator region_iterator = country.regions.begin();
	while (region_iterator != country.regions.end())
	{
		combo_box_select_region->append_text(*region_iterator);
		region_iterator++;
	}
	combo_box_select_region->set_active(0);
}

void ScanWindow::stop_scan()
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

void ScanWindow::on_button_scan_wizard_cancel_clicked()
{
	hide();
}

void ScanWindow::import_channels_conf(const Glib::ustring& channels_conf_path)
{
	Profile& current_profile = get_application().get_profile_manager().get_current_profile();
	Glib::RefPtr<Glib::IOChannel> file = Glib::IOChannel::create_from_file(channels_conf_path, "r");
	Glib::ustring line;
	guint line_count = 0;
	
	while (file->read_line(line) == Glib::IO_STATUS_NORMAL)
	{
		ChannelsConfLine channels_conf_line(line);
		guint parameter_count = channels_conf_line.get_parameter_count();

		line_count++;
		
		g_debug("Line %d (%d parameters): '%s'", line_count, parameter_count, line.c_str());
	
		if (parameter_count > 1)
		{
			switch(frontend.get_frontend_info().type)
			{
				case FE_OFDM:
					if (parameter_count != 13)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					{
						Channel channel;

						channel.name = channels_conf_line.get_name(0);
						channel.sort_order = line_count;
						channel.flags = CHANNEL_FLAG_DVB_T;
				
						channel.transponder.frontend_parameters.frequency						= channels_conf_line.get_frequency(1);
						channel.transponder.frontend_parameters.inversion						= channels_conf_line.get_inversion(2);
						channel.transponder.frontend_parameters.u.ofdm.bandwidth				= channels_conf_line.get_bandwidth(3);
						channel.transponder.frontend_parameters.u.ofdm.code_rate_HP				= channels_conf_line.get_fec(4);
						channel.transponder.frontend_parameters.u.ofdm.code_rate_LP				= channels_conf_line.get_fec(5);
						channel.transponder.frontend_parameters.u.ofdm.constellation			= channels_conf_line.get_modulation(6);
						channel.transponder.frontend_parameters.u.ofdm.transmission_mode		= channels_conf_line.get_transmit_mode(7);
						channel.transponder.frontend_parameters.u.ofdm.guard_interval			= channels_conf_line.get_guard_interval(8);
						channel.transponder.frontend_parameters.u.ofdm.hierarchy_information	= channels_conf_line.get_hierarchy(9);
						channel.service_id														= channels_conf_line.get_service_id(12);
				
						current_profile.add_channel(channel);
					}
					break;
				
				case FE_QAM:
					if (parameter_count != 9)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					{
						Channel channel;

						channel.name = channels_conf_line.get_name(0);
						channel.sort_order = line_count;
						channel.flags = CHANNEL_FLAG_DVB_C;
				
						channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_frequency(1);
						channel.transponder.frontend_parameters.inversion			= channels_conf_line.get_inversion(2);
						channel.transponder.frontend_parameters.u.qam.symbol_rate	= channels_conf_line.get_symbol_rate(3);
						channel.transponder.frontend_parameters.u.qam.fec_inner		= channels_conf_line.get_fec(4);
						channel.transponder.frontend_parameters.u.qam.modulation	= channels_conf_line.get_modulation(5);
						channel.service_id											= channels_conf_line.get_service_id(8);
				
						current_profile.add_channel(channel);
					}
					break;

				case FE_ATSC:
					if (parameter_count != 6)
					{
						Glib::ustring message = Glib::ustring::compose(_("Invalid parameter count on line %1"), line_count);
						throw Exception(message);
					}

					{
						Channel channel;

						channel.name = channels_conf_line.get_name(0);
						channel.sort_order = line_count;
						channel.flags = CHANNEL_FLAG_ATSC;
				
						channel.transponder.frontend_parameters.frequency			= channels_conf_line.get_frequency(1);
						channel.transponder.frontend_parameters.inversion			= INVERSION_AUTO;
						channel.transponder.frontend_parameters.u.vsb.modulation	= channels_conf_line.get_modulation(2);
						channel.service_id											= channels_conf_line.get_service_id(5);
				
						current_profile.add_channel(channel);
					}
					break;
				
				default:
					throw Exception(_("Failed to import: importing a channels.conf is only supported with DVB-T, DVB-C and ATSC"));
					break;
			}
		}		
	}
	g_debug("Finished importing channels");
	
	Data data;
	data.replace_profile(current_profile);
	hide();
}

void ScanWindow::on_button_scan_wizard_next_clicked()
{
	TRY
	gboolean do_scan = true;
	
	stop_scan();

	list_store->clear();

	Glib::ustring initial_tuning_file;
	
	Gtk::RadioButton* radio_button_scan_by_location = dynamic_cast<Gtk::RadioButton*>(glade->get_widget("radio_button_scan_by_location"));
	Gtk::RadioButton* radio_button_scan_by_file = dynamic_cast<Gtk::RadioButton*>(glade->get_widget("radio_button_scan_by_file"));
	Gtk::RadioButton* radio_button_scan_import_channels_conf = dynamic_cast<Gtk::RadioButton*>(glade->get_widget("radio_button_scan_import_channels_conf"));
		
	if (radio_button_scan_by_location->get_active())
	{
		Glib::ustring country_name = combo_box_select_country->get_active_text();
		Glib::ustring region_name = combo_box_select_region->get_active_text();
		initial_tuning_file = scan_directory_path + "/" + country_name + "-" + region_name;
	}
	else if (radio_button_scan_by_file->get_active())
	{
		Gtk::FileChooserButton* file_chooser = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_select_file_to_scan"));
		initial_tuning_file = file_chooser->get_filename();
	}
	else if (radio_button_scan_import_channels_conf->get_active())
	{
		Gtk::FileChooserButton* file_chooser = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_channels_conf"));
		Glib::ustring channels_conf_path = file_chooser->get_filename();
		import_channels_conf(channels_conf_path);
		do_scan = false;
	}
	else
	{
		throw Exception(_("No scan/import option specified"));
	}
	
	if (do_scan)
	{
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
			Gtk::MessageDialog dialog(*this, _("No tuning file has been selected"));
			dialog.run();
		}
		else
		{
			glade->get_widget("button_scan_wizard_next")->hide();
			notebook_scan_wizard->next_page();

			g_debug("Initial tuning file: '%s'", initial_tuning_file.c_str());
			scan_thread = new ScanThread(frontend, initial_tuning_file);
			Dvb::Scanner& scanner = scan_thread->get_scanner();
			scanner.signal_service.connect(sigc::mem_fun(*this, &ScanWindow::on_signal_service));
			scanner.signal_progress.connect(sigc::mem_fun(*this, &ScanWindow::on_signal_progress));
			get_application().stop_stream_thread();
			scan_thread->start();
		}
	}
	CATCH
}

void ScanWindow::on_button_scan_wizard_add_clicked()
{
	Profile& current_profile = get_application().get_profile_manager().get_current_profile();

	std::list<Gtk::TreeModel::Path> selected_services = tree_view_scanned_channels->get_selection()->get_selected_rows();		
	std::list<Gtk::TreeModel::Path>::iterator iterator = selected_services.begin();
	while (iterator != selected_services.end())
	{
		Gtk::TreeModel::Row row(*list_store->get_iter(*iterator));

		Channel channel;

		channel.service_id			= row.get_value(columns.column_id);
		channel.name				= row.get_value(columns.column_name);
		channel.transponder.frontend_parameters = row.get_value(columns.column_frontend_parameters);

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
		
		current_profile.add_channel(channel);
		
		iterator++;
	}

	Data data;
	data.replace_profile(current_profile);
	hide();
}

void ScanWindow::on_signal_service(struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name)
{
	GdkLock gdk_lock;
	Gtk::TreeModel::iterator iterator = list_store->append();
	Gtk::TreeModel::Row row = *iterator;
	row[columns.column_id] = id;
	row[columns.column_name] = name;
	row[columns.column_frontend_parameters] = frontend_parameters;
	tree_view_scanned_channels->get_selection()->select(row);

	channel_count++;
	update_channel_count();
	g_debug("Found channel #%d : %s", id, name.c_str());
}

void ScanWindow::update_channel_count()
{
	label_scan_information->set_text(Glib::ustring::compose(ngettext("Found 1 channel", "Found %1 channels", channel_count), channel_count));
}

void ScanWindow::on_signal_progress(guint step, gsize total)
{
	GdkLock gdk_lock;
	gdouble fraction = total == 0 ? 0 : step/(gdouble)total;
	progress_bar_scan->set_fraction(fraction);
	if (step == total)
	{
		glade->get_widget("button_scan_wizard_add")->show();
		notebook_scan_wizard->next_page();
	}
}

Country& ScanWindow::get_country(const Glib::ustring& country_name)
{
	Country* result = find_country(country_name);

	// If we don't have the country already then create one
	if (result == NULL)
	{
		Country country;
		country.name = country_name;
		countries.push_back(country);
		result = find_country(country_name);
	}
	
	return *result;
}

Country* ScanWindow::find_country(const Glib::ustring& country_name)
{
	Country* result = NULL;
	
	CountryList::iterator country_iterator = countries.begin();
	while (country_iterator != countries.end() && result == NULL)
	{
		Country& country = *country_iterator;
		if (country.name == country_name)
		{
			result = &(*country_iterator);
		}
		country_iterator++;
	}
	
	return result;
}
