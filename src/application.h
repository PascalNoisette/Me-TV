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

extern ChannelManager				channel_manager;
extern ScheduledRecordingManager	scheduled_recording_manager;
extern DeviceManager				device_manager;
extern StreamManager				stream_manager;
extern ConfigurationManager			configuration_manager;

class Application
{
private:
	static Application*					current;
	Glib::RefPtr<Gtk::Builder>			builder;
	StatusIcon*							status_icon;
	Glib::ustring						preferred_language;
	Glib::StaticRecMutex				mutex;
	guint								timeout_source;
	Glib::ustring						application_dir;
	Data::Schema						schema;
	Glib::ustring						database_filename;
	gboolean							database_initialised;
	DBusGConnection*					dbus_connection;
	
	void make_directory_with_parents(const Glib::ustring& path);
		
	void on_display_channel_changed(const Channel& channel);
	static gboolean on_timeout(gpointer data);
	gboolean on_timeout();
	void action_after(guint action);

	void on_record();
	void on_quit();
	
public:
	Application();
	~Application();

	void run();
	void quit();
	static Application& get_current();
	
	Data::Connection			connection;

	Glib::StaticRecMutex&	get_mutex();
	gboolean				initialise_database();
	Data::Schema			get_schema() const { return schema; }

	const Glib::ustring& get_database_filename();
	void update();
	
	Glib::RefPtr<Gtk::Builder> get_builder() { return builder; }
	
	void check_auto_record();
	void check_scheduled_recordings();
	void start_recording(Channel& channel);
	void start_recording(Channel& channel, const ScheduledRecording& scheduled_recording);
	void stop_recording(Channel& channel);
		
	const Glib::ustring& get_preferred_language() const { return preferred_language; }
	const Glib::ustring& get_application_dir() const { return application_dir; }
	DBusGConnection* get_dbus_connection() const { return dbus_connection; }
};

Application& get_application();

#endif
