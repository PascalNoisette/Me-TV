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
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
	ComboBoxText* combo_box_scan_device;
	Gtk::ProgressBar* progress_bar_scan;
	Gtk::TreeView* treeview_scanned_channels;
	ScanThread* scan_thread;
		
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_text);
		}

		Gtk::TreeModelColumn<Glib::ustring> column_text;
	};

	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
		
public:
	ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Dialog(cobject), glade(glade)
	{
		scan_thread = NULL;
		
		glade->connect_clicked("button_device_scan", sigc::mem_fun(*this, &ScanDialog::on_button_device_scan_clicked));
		progress_bar_scan = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_scan"));
		treeview_scanned_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("treeview_scanned_channels"));
		
		list_store = Gtk::ListStore::create(columns);
		treeview_scanned_channels->set_model(list_store);
		treeview_scanned_channels->append_column("Service Name", columns.column_text);
				
		combo_box_scan_device = NULL;
		glade->get_widget_derived("combo_box_scan_device", combo_box_scan_device);

		Dvb::DeviceManager& device_manager = Application::get_current().get_device_manager();
		const std::list<Dvb::Frontend*> frontends = device_manager.get_frontends();
		
		std::list<Dvb::Frontend*>::const_iterator frontend_iterator = frontends.begin();
		while (frontend_iterator != frontends.end())
		{
			Dvb::Frontend* frontend = *frontend_iterator;
			combo_box_scan_device->append_text(frontend->get_frontend_info().name);
			frontend_iterator++;
		}
		combo_box_scan_device->set_active(0);
	}
		
	~ScanDialog()
	{
		stop_scan();
	}
	
	void stop_scan()
	{
		if (scan_thread != NULL)
		{
			g_debug("Stopping scan");
			scan_thread->join();
			delete scan_thread;
			scan_thread = NULL;
		}
	}

	void on_button_device_scan_clicked()
	{
		stop_scan();
		
		Glib::ustring device_name = combo_box_scan_device->get_active_text();
		Dvb::DeviceManager& device_manager = Application::get_current().get_device_manager();
		Dvb::Frontend& frontend = device_manager.get_frontend_by_name(device_name);
		
		Gtk::FileChooserButton* file_chooser = dynamic_cast<Gtk::FileChooserButton*>(glade->get_widget("file_chooser_button_select_file_to_scan"));
		Glib::ustring initial_tuning_file = file_chooser->get_filename();
		g_debug("Initial tuning file: '%s'", initial_tuning_file.c_str());

		scan_thread = new ScanThread(frontend, initial_tuning_file);
		Dvb::Scanner& scanner = scan_thread->get_scanner();
		scanner.signal_service.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_service));
		scanner.signal_progress.connect(sigc::mem_fun(*this, &ScanDialog::on_signal_progress));
		scan_thread->start();
	}
		
	void on_signal_service(Dvb::Service& service)
	{
		GdkLock gdk_lock;
		Gtk::TreeModel::iterator iterator = list_store->append();
		Gtk::TreeModel::Row row = *iterator;
		row[columns.column_text] = service.name;
	}
		
	void on_signal_progress(gdouble progress)
	{
		GdkLock gdk_lock;
		progress_bar_scan->set_fraction(progress);
	}
};

#endif
