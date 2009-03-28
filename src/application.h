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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "me-tv.h"
#include <libglademm.h>
#include <libgnomemm.h>
#include "device_manager.h"
#include "channel_manager.h"
#include "scheduled_recording_manager.h"
#include "main_window.h"
#include "status_icon.h"
#include "stream_thread.h"

class Application : public Gnome::Main
{
private:
	static Application*					current;
	Glib::RefPtr<Gnome::Glade::Xml>		glade;
	MainWindow*							main_window;
	StatusIcon*							status_icon;
	Glib::RefPtr<Gnome::Conf::Client>	client;
	Glib::ustring						preferred_language;
	StreamThread*						stream_thread;
	Glib::StaticRecMutex				mutex;
	guint								timeout_source;
	gboolean							record_state;
	gboolean							broadcast_state;
	bool								on_quit();
	Glib::ustring						application_dir;
	Data::Schema						schema;

	void set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_default(const Glib::ustring& key, gint value);
	void set_boolean_configuration_default(const Glib::ustring& key, gboolean value);	
	Glib::ustring get_configuration_path(const Glib::ustring& key);
		
	void on_display_channel_changed(const Channel& channel);
	static gboolean on_timeout(gpointer data);
	gboolean on_timeout();
	void on_error(const Glib::ustring& message);

public:
	Application(int argc, char *argv[], Glib::OptionContext& option_context);
	~Application();
		
	void run();
	static Application& get_current();
	
	ChannelManager						channel_manager;
	ScheduledRecordingManager			scheduled_recording_manager;
	DeviceManager						device_manager;
	Data::Connection					connection;

	Glib::StaticRecMutex&	get_mutex();
	StreamThread*			get_stream_thread();
	void					stop_stream_thread();
	void					restart_stream();
	void					set_source(const Channel& channel);
	void					initialise_database();
	Data::Schema			get_schema() const { return schema; }

	void update();
		
	Glib::ustring	get_string_configuration_value(const Glib::ustring& key);
	gint			get_int_configuration_value(const Glib::ustring& key);
	gboolean		get_boolean_configuration_value(const Glib::ustring& key);

	void set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_value(const Glib::ustring& key, gint value);
	void set_boolean_configuration_value(const Glib::ustring& key, gboolean value);
	
	Glib::RefPtr<Gnome::Glade::Xml> get_glade() { return glade; }
	
	sigc::signal<void> signal_configuration_changed;

	gboolean is_recording();
	void start_recording(const Glib::ustring& filename = "");
	void stop_recording();
	void toggle_recording();
	void set_record_state(gboolean state);
	void check_scheduled_recordings();
	
	gboolean is_broadcasting();
	void set_broadcast_state(gboolean state);
	void toggle_broadcast();
	
	const Glib::ustring& get_preferred_language() const { return preferred_language; }
	Glib::ustring make_recording_filename(const Glib::ustring& description = "");
	const Glib::ustring& get_application_dir() const { return application_dir; }
	
	MainWindow& get_main_window();
};

Application& get_application();

#endif
