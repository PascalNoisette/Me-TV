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
#include "channel_manager.h"
#include "dvb_demuxer.h"
#include "main_window.h"

class Application : public Gnome::Main
{
private:
	static Application*				current;
	Glib::RefPtr<Gnome::Glade::Xml>	glade;
	ProfileManager					profile_manager;
	DeviceManager					device_manager;
	ChannelManager					channel_manager;
	DemuxerList						demuxers;
	MainWindow*						main_window;
	Engine*							engine;

	void on_display_channel_changed(Channel& channel);
	void remove_all_demuxers();
	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);
	void setup_dvb(Dvb::Frontend& frontend, const Channel& channel);

public:
	Application(int argc, char *argv[]);
	void run();
	static Application& get_current();
	Engine& get_engine();
	
	ProfileManager&		get_profile_manager()	{ return profile_manager; }
	DeviceManager&		get_device_manager()	{ return device_manager; }
	ChannelManager&		get_channel_manager()	{ return channel_manager; }
};

Application& get_application();

#endif
