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

#ifndef __SCAN_WINDOW_H__
#define __SCAN_WINDOW_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "device_manager.h"
#include "dvb_scanner.h"
#include "thread.h"
#include "me-tv-ui.h"

#ifndef SCAN_DIRECTORY
#define SCAN_DIRECTORY "/usr/share/dvb"
#endif

#ifndef ALTERNATE_SCAN_DIRECTORY
#define ALTERNATE_SCAN_DIRECTORY "/usr/share/doc/dvb-utils/examples/scan"
#endif

class Country
{
public:
	Glib::ustring name;
	StringList regions;
};

typedef std::list<Country> CountryList;

class ScanThread : public Thread
{
public:
	Dvb::Scanner scanner;
	Glib::ustring initial_tuning_file;
	Dvb::Frontend& frontend;
				
public:
	ScanThread(Dvb::Frontend& frontend, const Glib::ustring& file) :
		Thread("Scan"), frontend(frontend), initial_tuning_file(file), scanner(2) {}
		
	void run()
	{
		scanner.start(frontend, initial_tuning_file);
	}
		
	void stop()
	{
		scanner.terminate();
	}

	Dvb::Scanner& get_scanner() { return scanner; }
};

class ScanWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>	glade;
	Gtk::Notebook*							notebook_scan_wizard;
	Gtk::Label*								label_scan_information;
	Gtk::ProgressBar*						progress_bar_scan;
	Gtk::TreeView*							tree_view_scanned_channels;
	ComboBoxText*							combo_box_select_country;
	ComboBoxText*							combo_box_select_region;
	ScanThread*								scan_thread;
	CountryList								countries;
	guint									channel_count;
	Dvb::Frontend&							frontend;

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
		
	Glib::ustring get_initial_tuning_dir();
	Country* find_country(const Glib::ustring& country_name);
	Country& get_country(const Glib::ustring& country);

	void on_file_chooser_button_select_file_to_scan_clicked();
	void on_combo_box_select_country_changed();
	void on_button_scan_wizard_next_clicked();
	void on_button_scan_wizard_cancel_clicked();
	void on_button_scan_wizard_add_clicked();
	void on_signal_service(struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name);
	void on_signal_progress(guint step, gsize total);
	void on_hide();	
	void stop_scan();
	void update_channel_count();
		
public:
	ScanWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	~ScanWindow();
		
	void on_show();

	static ScanWindow* create(Glib::RefPtr<Gnome::Glade::Xml> glade);
};

#endif