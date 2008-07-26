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
#include "dvb_demuxer.h"
#include "main_window.h"

class EpgThread : public Thread
{
public:
	EpgThread() : Thread("EPG Thread") {}
	void run();
};

class Application : public Gnome::Main
{
private:
	static Application*					current;
	Glib::RefPtr<Gnome::Glade::Xml>		glade;
	ProfileManager						profile_manager;
	DeviceManager						device_manager;
	DemuxerList							demuxers;
	MainWindow*							main_window;
	Engine*								engine;
	EpgThread							epg_thread;		
	Glib::RefPtr<Gnome::Conf::Client>	client;
		
	void on_display_channel_changed(const Channel& channel);
	void remove_all_demuxers();
	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);
	void setup_dvb(Dvb::Frontend& frontend, const Channel& channel);

	void set_configuration_default(const Glib::ustring& key, const Glib::ustring& value);
	void set_configuration_default(const Glib::ustring& key, gint value);

public:
	Application(int argc, char *argv[]);
	~Application();
		
	void run();
	static Application& get_current();
	
	ProfileManager&		get_profile_manager()	{ return profile_manager; }
	DeviceManager&		get_device_manager()	{ return device_manager; }

	void set_source(const Glib::ustring& source);
	void record(const Glib::ustring& filename);
	void mute(gboolean state);
		
	Glib::ustring get_string_configuration_value(const Glib::ustring& key);
	gint get_int_configuration_value(const Glib::ustring& key);
};

Application& get_application();

#endif
