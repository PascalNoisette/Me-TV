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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <libglademm.h>
#include <libgnomemm.h>
#include "device_manager.h"
#include "profile_manager.h"
#include "main_window.h"
#include "status_icon.h"
#include "stream_thread.h"

class Application : public Gnome::Main
{
private:
	static Application*					current;
	Glib::RefPtr<Gnome::Glade::Xml>		glade;
	ProfileManager						profile_manager;
	DeviceManager						device_manager;
	MainWindow*							main_window;
	StatusIcon*							status_icon;
	StreamThread*						stream_thread;
	Glib::RefPtr<Gnome::Conf::Client>	client;
	guint								last_epg_update_time;
		
	void on_display_channel_changed(const Channel& channel);
		
	void set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_default(const Glib::ustring& key, gint value);
	void set_boolean_configuration_default(const Glib::ustring& key, gboolean value);
	
	Glib::ustring get_configuration_path(const Glib::ustring& key);
public:
	Application(int argc, char *argv[]);
	~Application();
		
	void run();
	static Application& get_current();
	
	ProfileManager&		get_profile_manager()	{ return profile_manager; }
	DeviceManager&		get_device_manager()	{ return device_manager; }

	StreamThread& get_stream_thread();
	void stop_stream_thread();
	void set_source(const Channel& channel);
		
	void update_ui();
	void mute(gboolean state);
		
	Glib::ustring get_string_configuration_value(const Glib::ustring& key);
	gint get_int_configuration_value(const Glib::ustring& key);
	gboolean get_boolean_configuration_value(const Glib::ustring& key);

	void set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value);
	void set_int_configuration_value(const Glib::ustring& key, gint value);
	void set_boolean_configuration_value(const Glib::ustring& key, gboolean value);
	
	void update_epg_time();
	guint get_last_epg_update_time() const;
	void toggle_visibility();
		
	MainWindow& get_main_window();
};

Application& get_application();

#endif
