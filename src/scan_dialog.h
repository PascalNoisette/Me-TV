/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SCAN_DIALOG_H__
#define __SCAN_DIALOG_H__

#include <gtkmm.h>
#include "device_manager.h"
#include "dvb_scanner.h"
#include "thread.h"
#include "me-tv-ui.h"
#include "dvb_transponder.h"

#ifndef SCAN_DIRECTORIES
#define SCAN_DIRECTORIES "/usr/share/dvb:/usr/share/doc/dvb-utils/examples/scan:/usr/share/dvb-apps"
#endif

class ScanThread: public Thread {
public:
	Dvb::Scanner scanner;
	Dvb::TransponderList transponders;
	Dvb::Frontend & frontend;
public:
	ScanThread(Dvb::Frontend & scan_frontend, Dvb::TransponderList & transponders);
	void run();
	void stop();
	Dvb::Scanner & get_scanner() { return scanner; }
};

class ScanDialog: public Gtk::Window {
private:
	const Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Notebook * notebook_scan_wizard;
	Gtk::ProgressBar * progress_bar_scan;
	Gtk::TreeView * tree_view_scanned_channels;
	ScanThread * scan_thread;
	StringList system_files;
	guint channel_count;
	Glib::ustring scan_directory_path;
	Dvb::TransponderList transponders;
	Dvb::Frontend * frontend;
	class ModelColumns: public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(column_id);
			add(column_name);
			add(column_frontend_parameters);
			add(column_polarisation); // for DVB-S only
			add(column_signal_strength); // for DVB-S only
		}
		Gtk::TreeModelColumn<guint> column_id;
		Gtk::TreeModelColumn<Glib::ustring> column_name;
		Gtk::TreeModelColumn<dvb_frontend_parameters> column_frontend_parameters;
		Gtk::TreeModelColumn<guint> column_polarisation;
		Gtk::TreeModelColumn<Glib::ustring> column_signal_strength;
	};
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::ustring get_initial_tuning_dir();
	void import_channels_conf(Glib::ustring const & channels_conf_path);
	void load_initial_tuning_file(Glib::ustring const & initial_tuning_file);
	void on_button_scan_wizard_next_clicked();
	void on_button_scan_wizard_cancel_clicked();
	void on_button_scan_wizard_add_clicked();
	void on_button_scan_stop_clicked();
	void on_signal_service(dvb_frontend_parameters const & frontend_parameters,
		guint id, Glib::ustring const & name, guint const polarisation, guint signal_strength);
	void on_signal_progress(guint step, gsize total);
	void on_signal_complete();
	void on_file_chooser_button_scan_file_set();
	void on_file_chooser_button_import_file_set();
	void on_combo_box_auto_scan_range_changed();
	void on_hide();
	void stop_scan();
	void update_channel_count();
	void add_channel_row(Channel const & channel, guint signal_strength);
	guint convert_string_to_value(StringTable const * table, gchar const * text);
	void process_terrestrial_line(Glib::ustring const & line);
	void process_satellite_line(Glib::ustring const & line);
	void process_cable_line(Glib::ustring const & line);
	void process_atsc_line(Glib::ustring const & line);
	void add_transponder(dvb_frontend_parameters frontend_parameters);
	void add_scan_range(guint start, guint end, guint step, dvb_frontend_parameters frontend_parameters);
	void add_scan_list(int const  * si, int length, dvb_frontend_parameters frontend_parameters);
	void add_auto_scan_range(fe_type_t frontend_type, Glib::ustring const & range);
	void on_show();
public:
	ScanDialog(BaseObjectType* cobject, Glib::RefPtr<Gtk::Builder> const & builder);
	~ScanDialog();
	ChannelArray get_selected_channels();
	static ScanDialog & create(Glib::RefPtr<Gtk::Builder> builder);
};

#endif
