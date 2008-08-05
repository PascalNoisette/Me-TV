/*
 * Copyright (C) 2008 Michael Lamothe
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
#include "me-tv.h"
#include "application.h"

ScanDialog* ScanDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	ScanDialog* scan_dialog = NULL;
	glade->get_widget_derived("dialog_scan_wizard", scan_dialog);
	return scan_dialog;
}

Glib::ustring ScanDialog::get_initial_tuning_dir()
{
	Glib::ustring path = SCAN_DIRECTORY;
	switch(frontend.get_frontend_info().type)
	{
	case FE_OFDM:   path += "/dvb-t";       break;
	case FE_QAM:    path += "/dvb-c";       break;
	case FE_QPSK:   path += "/dvb-s";       break;
	case FE_ATSC:   path += "/atsc";        break;
	default:		throw Exception(_("Unknown frontend type"));
	}

	return path;
}

bool compare_countries (const Country& a, const Country& b)
{
	return a.name < b.name;
}

ScanDialog::ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::Dialog(cobject), glade(glade), frontend(get_application().get_device_manager().get_frontend())
{
	scan_thread = NULL;
	response = 0;

	notebook_scan_wizard = dynamic_cast<Gtk::Notebook*>(glade->get_widget("notebook_scan_wizard"));
	label_scan_information = dynamic_cast<Gtk::Label*>(glade->get_widget("label_scan_information"));
	glade->connect_clicked("button_scan_wizard_add", sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_add_clicked));
	glade->connect_clicked("button_scan_wizard_next", sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_next_clicked));
	glade->connect_clicked("button_scan_wizard_cancel", sigc::mem_fun(*this, &ScanDialog::on_button_scan_wizard_cancel_clicked));

	notebook_scan_wizard->set_show_tabs(false);
		
	Glib::ustring device_name = frontend.get_frontend_info().name;
	dynamic_cast<Gtk::Label*>(glade->get_widget("label_scan_device"))->set_label(device_name);
		
	progress_bar_scan = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_scan"));
	tree_view_scanned_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_scanned_channels"));
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_scanned_channels->set_model(list_store);
	tree_view_scanned_channels->append_column("Service Name", columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
	
	combo_box_select_country = NULL;
	glade->get_widget_derived("combo_box_select_country", combo_box_select_country);

	combo_box_select_region = NULL;
	glade->get_widget_derived("combo_box_select_region", combo_box_select_region);
	
	Glib::ustring scan_directory_path = get_initial_tuning_dir();

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
			if (name.substr(2,1) == "-")
			{
				Glib::ustring country_name = name.substr(0,2);
				Glib::ustring region_name = name.substr(3);
				Country& country = get_country(country_name);
				country.regions.push_back(region_name);
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
		combo_box_select_country->signal_changed().connect(sigc::mem_fun(*this, &ScanDialog::on_combo_box_select_country_changed));
		combo_box_select_country->set_active(0);
	}
}

ScanDialog::~ScanDialog()
{
	stop_scan();
}

gint ScanDialog::run()
{
	channel_count = 0;
	update_channel_count();
	progress_bar_scan->set_fraction(0);
	glade->get_widget("button_scan_wizard_add")->hide();
	glade->get_widget("button_scan_wizard_next")->show();
	notebook_scan_wizard->set_current_page(0);
	response = Gtk::RESPONSE_NONE;
	Gnome::Main::run(*this);
	return response;
}

void ScanDialog::on_hide()
{
	stop_scan();
	Widget::on_hide();
}

void ScanDialog::on_combo_box_select_country_changed()
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

void ScanDialog::stop_scan()
{
	if (scan_thread != NULL)
	{
		g_debug("Stopping scan");
		scan_thread->join(true);
		delete scan_thread;
		scan_thread = NULL;
	}
}

void ScanDialog::on_button_scan_wizard_cancel_clicked()
{
	response = Gtk::RESPONSE_CANCEL;
	hide();
}

void ScanDialog::on_button_scan_wizard_next_clicked()
{
	stop_scan();

	glade->get_widget("button_scan_wizard_next")->hide();
	notebook_scan_wizard->next_page();

	list_store->clear();

	Glib::ustring initial_tuning_file;
	
	Gtk::RadioButton* radio_button_scan_by_location = dynamic_cast<Gtk::RadioButton*>(glade->get_widget("radio_button_scan_by_location"));
	if (radio_button_scan_by_location->get_active())
	{
		Glib::ustring country_name = combo_box_select_country->get_active_text();
		Glib::ustring region_name = combo_box_select_region->get_active_text();
		Glib::ustring initial_tuning_dir = get_initial_tuning_dir();
		initial_tuning_file = initial_tuning_dir + "/" + country_name + "-" + region_name;
	}
	else
	{
		Gtk::FileChooserButton* file_chooser = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_select_file_to_scan"));
		initial_tuning_file = file_chooser->get_filename();
	}

	if (initial_tuning_file.empty())
	{
		Gtk::MessageDialog dialog(*this, "No tuning file has been selected");
		dialog.run();
	}
	else
	{
		g_debug("Initial tuning file: '%s'", initial_tuning_file.c_str());
		scan_thread = new ScanThread(frontend, initial_tuning_file);
		Dvb::Scanner& scanner = scan_thread->get_scanner();
		scanner.signal_service.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_service));
		scanner.signal_progress.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_progress));
		get_application().stop_stream_thread();
		scan_thread->start();
	}
}

void ScanDialog::on_button_scan_wizard_add_clicked()
{
	response = Gtk::RESPONSE_OK;
	hide();
}

void ScanDialog::on_signal_service(struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name)
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
}

void ScanDialog::update_channel_count()
{
	label_scan_information->set_text(Glib::ustring::compose("Found %1 channels", channel_count));
}

void ScanDialog::on_signal_progress(guint step, gsize total)
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

ScannedServiceList ScanDialog::get_scanned_services()
{
	std::list<ScannedService> result;
	std::list<Gtk::TreeModel::Path> selected_services = tree_view_scanned_channels->get_selection()->get_selected_rows();		
	std::list<Gtk::TreeModel::Path>::iterator iterator = selected_services.begin();
	while (iterator != selected_services.end())
	{
		Gtk::TreeModel::Row row(*list_store->get_iter(*iterator));
		ScannedService service;
		service.id = row.get_value(columns.column_id);
		service.name = row.get_value(columns.column_name);
		service.frontend_parameters = row.get_value(columns.column_frontend_parameters);
		result.push_back(service);

		iterator++;
	}
	return result;
}

Country& ScanDialog::get_country(const Glib::ustring& country_name)
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

Country* ScanDialog::find_country(const Glib::ustring& country_name)
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
