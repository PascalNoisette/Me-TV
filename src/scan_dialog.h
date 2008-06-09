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

#ifndef __SCAN_DIALOG_H__
#define __SCAN_DIALOG_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "device_manager.h"
#include "dvb_scanner.h"
#include "thread.h"
#include "me-tv.h"
#include <ext/hash_map>

#ifndef SCAN_DIRECTORY
#define SCAN_DIRECTORY "/usr/share/doc/dvb-utils/examples/scan"
#endif

class ScannedService
{
public:
	guint id;
	Glib::ustring name;
	struct dvb_frontend_parameters frontend_parameters;
};

class ScanThread : public Thread
{
public:
	Dvb::Scanner scanner;
	Glib::ustring initial_tuning_file;
	Dvb::Frontend& frontend;
public:
	ScanThread(Dvb::Frontend& frontend, const Glib::ustring& file) : frontend(frontend), initial_tuning_file(file)
	{
	}
		
	void run()
	{
		scanner.start(frontend, initial_tuning_file, 2);
	}

	Dvb::Scanner& get_scanner() { return scanner; }
};

class ScanDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>&	glade;
	ComboBoxText*							combo_box_scan_device;
	Gtk::ProgressBar*						progress_bar_scan;
	Gtk::TreeView*							tree_view_scanned_channels;
	Gtk::ComboBox*							combo_box_select_country;
	Gtk::ComboBox*							combo_box_select_region;
	ScanThread*								scan_thread;

	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_id);
			add(column_name);
			add(column_frontend_parameters);
		}

		Gtk::TreeModelColumn<guint>								column_id;
		Gtk::TreeModelColumn<Glib::ustring>						column_name;
		Gtk::TreeModelColumn<struct dvb_frontend_parameters>	column_frontend_parameters;
	};

	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
		
public:
	ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Dialog(cobject), glade(glade)
	{
		scan_thread = NULL;
		
		glade->connect_clicked("button_start_scan", sigc::mem_fun(*this, &ScanDialog::on_button_start_scan_clicked));
		progress_bar_scan = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_scan"));
		tree_view_scanned_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_scanned_channels"));
		
		progress_bar_scan->hide();
		
		list_store = Gtk::ListStore::create(columns);
		tree_view_scanned_channels->set_model(list_store);
		tree_view_scanned_channels->append_column("Service Name", columns.column_name);
		
		Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scanned_channels->get_selection();
		selection->set_mode(Gtk::SELECTION_MULTIPLE);
		
		Dvb::DeviceManager& device_manager = Application::get_current().get_device_manager();
		const std::list<Dvb::Frontend*> frontends = device_manager.get_frontends();
		
		combo_box_scan_device = NULL;
		glade->get_widget_derived("combo_box_scan_device", combo_box_scan_device);
		
		std::list<Dvb::Frontend*>::const_iterator frontend_iterator = frontends.begin();
		while (frontend_iterator != frontends.end())
		{
			Dvb::Frontend* frontend = *frontend_iterator;
			combo_box_scan_device->append_text(frontend->get_frontend_info().name);
			frontend_iterator++;
		}
		combo_box_scan_device->set_active(0);
		
		combo_box_select_country = dynamic_cast<Gtk::ComboBox*>(glade->get_widget("combo_box_select_country"));
		combo_box_select_region = dynamic_cast<Gtk::ComboBox*>(glade->get_widget("combo_box_select_region"));
/*
		Glib::ustring frontend_directory = "/dvb-t";
		Glib::RefPtr<Gio::File> scan_directory = Gio::File::create_for_path(SCAN_DIRECTORY + frontend_directory);
		Glib::RefPtr<Gio::FileEnumerator> children = scan_directory->enumerate_children();
		Glib::RefPtr<Gio::FileInfo> current_file = children->next_file();
		while (current_file)
		{
			Glib::ustring name = current_file->get_name();
			if (name.substr(3,1) == "-")
			{
				Glib::ustring country = name.substr(0,2);
				Glib::ustring region = name.substr(3);
				g_debug("COUNTRY: '%s', REGION '%s'", country.c_str(), region.c_str());
			}
			current_file = children->next_file();
		}
		*/
	}
		
	~ScanDialog()
	{
		stop_scan();
	}
	
	void stop_scan()
	{
		progress_bar_scan->hide();
		if (scan_thread != NULL)
		{
			g_debug("Stopping scan");
			scan_thread->join();
			delete scan_thread;
			scan_thread = NULL;
		}
	}

	void on_button_start_scan_clicked()
	{
		progress_bar_scan->show();
		
		stop_scan();
		
		list_store->clear();

		Glib::ustring device_name = combo_box_scan_device->get_active_text();
		Dvb::DeviceManager& device_manager = Application::get_current().get_device_manager();
		Dvb::Frontend& frontend = device_manager.get_frontend_by_name(device_name);
		
		Gtk::FileChooserButton* file_chooser = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_select_file_to_scan"));
		Glib::ustring initial_tuning_file = file_chooser->get_filename();

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
			scan_thread->start();
		}
	}

	void on_signal_service(struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name)
	{
		GdkLock gdk_lock;
		Gtk::TreeModel::iterator iterator = list_store->append();
		Gtk::TreeModel::Row row = *iterator;
		row[columns.column_id] = id;
		row[columns.column_name] = name;
		row[columns.column_frontend_parameters] = frontend_parameters;
		tree_view_scanned_channels->get_selection()->select(row);
	}
		
	void on_signal_progress(guint step, gsize total)
	{
		GdkLock gdk_lock;
		progress_bar_scan->set_fraction(step/(gdouble)total);
	}
		
	std::list<ScannedService> get_scanned_services()
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
};

#endif
