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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "me-tv.h"
#include <gtkmm.h>
#include <dbus/dbus-glib.h>
#include "status_icon.h"

#include "device_manager.h"
#include "channel_manager.h"
#include "scheduled_recording_manager.h"
#include "stream_manager.h"
#include "configuration_manager.h"
#include "web_manager.h"

extern ChannelManager channel_manager;
extern ScheduledRecordingManager scheduled_recording_manager;
extern DeviceManager device_manager;
extern StreamManager stream_manager;
extern ConfigurationManager configuration_manager;
extern WebManager web_manager;

class Application {
private:
	static Application * current;
	Glib::RefPtr<Gtk::Builder> builder;
	StatusIcon * status_icon;
	Glib::Threads::RecMutex mutex;
	guint timeout_source;
	Glib::ustring application_dir;
	Data::Schema schema;
	Glib::ustring database_filename;
	gboolean database_initialised;
	DBusGConnection * dbus_connection;
	void make_directory_with_parents(Glib::ustring const & path);
	void on_display_channel_changed(Channel const & channel);
	static gboolean on_timeout(gpointer data);
	gboolean on_timeout();
	void on_record_current();
	void on_quit();
public:
	Application();
	~Application();
	void run();
	void quit();
	static Application & get_current();
	Data::Connection connection;
	Glib::Threads::RecMutex & get_mutex();
	gboolean initialise_database();
	Data::Schema get_schema() const { return schema; }
	Glib::ustring const & get_database_filename() const { return database_filename; };
	Glib::RefPtr<Gtk::Builder> get_builder() const { return builder; }
	void check_auto_record();
	void check_scheduled_recordings();
	void start_recording(Channel & channel);
	void start_recording(Channel & channel, ScheduledRecording const & scheduled_recording);
	void stop_recording(Channel & channel);
	Glib::ustring const & get_application_dir() const { return application_dir; }
	DBusGConnection * get_dbus_connection() const { return dbus_connection; }
};

Application & get_application();

#endif
