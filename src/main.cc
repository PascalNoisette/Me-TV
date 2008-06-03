/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.cc
 * Copyright (C) Michael Lamothe 2008 <michael.lamothe@gmail.com>
 * 
 * main.cc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.cc is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libgnomeuimm.h>
#include <libglademm.h>
#include <giomm.h>
#include "config.h"

int main (int argc, char *argv[])
{	
	try
	{
		g_message("Me TV %s", VERSION);

		Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
		Glib::ustring glade_path = current_directory + "/me-tv.glade"; 

		Gnome::Main gnome_main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv);

		if (!Gio::File::create_for_path(glade_path)->query_exists())
		{
			glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
		}
		
		g_debug("Using glade file '%s'", glade_path.c_str());
		
		Glib::RefPtr<Gnome::Glade::Xml> glade = Gnome::Glade::Xml::create(glade_path);

		Gtk::Window* window_main = NULL;
		Gtk::DrawingArea* drawing_area_video = NULL;
		
		glade->get_widget("window_main", window_main);
		glade->get_widget("drawing_area_video", drawing_area_video);
		
		drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
		
		gnome_main.run(*window_main);
	}
	catch(const Glib::Error& error)
	{
		g_error(error.what().c_str());
	}
	g_message("Me TV terminated normally");
	
	return 0;
}
