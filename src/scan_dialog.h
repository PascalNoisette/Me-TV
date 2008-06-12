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
		scanner.start(frontend, initial_tuning_file, 1);
	}

	Dvb::Scanner& get_scanner() { return scanner; }
};

class ScanDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>&	glade;
	Gtk::ProgressBar*						progress_bar_scan;
	Gtk::TreeView*							tree_view_scanned_channels;
	ComboBoxText*							combo_box_scan_device;
	ComboBoxText*							combo_box_select_country;
	ComboBoxText*							combo_box_select_region;
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
		
	Glib::ustring get_initial_tuning_dir(Dvb::Frontend& frontend);

public:
	ScanDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade);
	~ScanDialog();
		
	void on_hide();	
	void stop_scan();
	void on_button_start_scan_clicked();
	void on_signal_service(struct dvb_frontend_parameters& frontend_parameters, guint id, const Glib::ustring& name);
	void on_signal_progress(guint step, gsize total);
	std::list<ScannedService> get_scanned_services();
	void on_button_scan_wizard_ok_clicked();
};

#endif
