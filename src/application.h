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
#include <gconfmm.h>
#include "device_manager.h"
#include "channel_manager.h"
#include "scheduled_recording_manager.h"
#include "main_window.h"
#include "status_icon.h"
#include "stream_manager.h"

class Application
{
private:
	static Application*					current;
	Glib::RefPtr<Gtk::Builder>			builder;
	MainWindow*							main_window;
	StatusIcon*							status_icon;
	Glib::ustring						preferred_language;
	Glib::StaticRecMutex				mutex;
	guint								timeout_source;
	Glib::ustring						application_dir;
	Data::Schema						schema;
	Glib::ustring						database_filename;
	gboolean							database_initialised;
	Glib::RefPtr<Gnome::Conf::Client>	gconf_client;

	void set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_default(const Glib::ustring& key, gint value);
	void set_boolean_configuration_default(const Glib::ustring& key, gboolean value);	
	Glib::ustring get_configuration_path(const Glib::ustring& key);

	void make_directory_with_parents(const Glib::ustring& path);
		
	void on_display_channel_changed(const Channel& channel);
	static gboolean on_timeout(gpointer data);
	gboolean on_timeout();

	void on_record();
	void on_quit();
	void on_next_channel();
	void on_previous_channel();
	
public:
	Application();
	~Application();
		
	void run();
	void quit();
	static Application& get_current();
	
	ChannelManager				channel_manager;
	ScheduledRecordingManager	scheduled_recording_manager;
	DeviceManager				device_manager;
	StreamManager				stream_manager;
	Data::Connection			connection;

	Glib::StaticRecMutex&	get_mutex();
	gboolean				initialise_database();
	Data::Schema			get_schema() const { return schema; }

	void show_error_dialog(const Glib::ustring& message);

	const Glib::ustring& get_database_filename();
	void update();
		
	StringList	get_string_list_configuration_value(const Glib::ustring& key);
	Glib::ustring	get_string_configuration_value(const Glib::ustring& key);
	gint			get_int_configuration_value(const Glib::ustring& key);
	gboolean		get_boolean_configuration_value(const Glib::ustring& key);

	void set_string_list_configuration_value(const Glib::ustring& key, const StringList& value);
	void set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_value(const Glib::ustring& key, gint value);
	void set_boolean_configuration_value(const Glib::ustring& key, gboolean value);
	
	Glib::RefPtr<Gtk::Builder> get_builder() { return builder; }
	
	void set_display_channel(const Channel& channel);
	void set_display_channel_by_id(guint channel_id);
	void set_display_channel_number(guint display_channel_number);
	
	void check_scheduled_recordings();
	void start_recording(Channel& channel);
	void start_recording(Channel& channel, const ScheduledRecording& scheduled_recording);
	void stop_recording(Channel& channel);
		
	const Glib::ustring& get_preferred_language() const { return preferred_language; }
	const Glib::ustring& get_application_dir() const { return application_dir; }
	
	MainWindow& get_main_window();

	void select_channel_to_play();
};

Application& get_application();

#endif
