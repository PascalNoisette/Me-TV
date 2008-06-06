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
#include "main_window.h"

class Application : public Gnome::Main
{
private:
	Glib::RefPtr<Gnome::Glade::Xml> glade;

public:
	Application(int argc, char *argv[])
		: Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv)
	{
		if (!Glib::thread_supported())
		{
			Glib::thread_init();
		}
		gdk_threads_init();
		
		Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
		Glib::ustring glade_path = current_directory + "/me-tv.glade"; 

		if (!Gio::File::create_for_path(glade_path)->query_exists())
		{
			glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
		}
		
		g_debug("Using glade file '%s'", glade_path.c_str());
		
		glade = Gnome::Glade::Xml::create(glade_path);
	}
	
	void run()
	{
		MainWindow* main_window = NULL;
		glade->get_widget_derived("window_main", main_window);
		Gnome::Main::run(*main_window);
	}
};

#endif
