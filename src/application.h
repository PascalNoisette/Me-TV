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

#include <libgnomeuimm.h>
#include <libglademm.h>
#include <giomm.h>
#include "config.h"
#include "device_manager.h"
#include "profile_manager.h"

class Application : public Gnome::Main
{
private:
	static Application* current;
	Glib::RefPtr<Gnome::Glade::Xml> glade;
	ProfileManager profile_manager;
	Dvb::DeviceManager device_manager;

public:
	Application(int argc, char *argv[]);
	void run();
	static Application& get_current();
	
	ProfileManager& get_profile_manager() { return profile_manager; }
	Dvb::DeviceManager& get_device_manager() { return device_manager; }
};

#endif
