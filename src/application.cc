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

#include "application.h"
#include "main_window.h"
#include "config.h"

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
}

Application::Application(int argc, char *argv[]) :
	Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv)
{
	if (current != NULL)
	{
		throw Exception("Application has already been initialised");
	}
	
	current = this;
	
	Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
	Glib::ustring glade_path = current_directory + "/me-tv.glade";

	if (!Gio::File::create_for_path(glade_path)->query_exists())
	{
		glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
	}
	
	g_debug("Using glade file '%s'", glade_path.c_str());
	
	glade = Gnome::Glade::Xml::create(glade_path);
	
	channel_manager.signal_display_channel_changed.connect(
		sigc::mem_fun(*this, &Application::on_display_channel_changed));
}

void Application::run()
{
	MainWindow* main_window = NULL;
	glade->get_widget_derived("window_main", main_window);
	Gnome::Main::run(*main_window);
}

Application& Application::get_current()
{
	if (current == NULL)
	{
		throw Exception("Application has not been initialised");
	}
	
	return *current;
}

void Application::on_display_channel_changed(Channel& channel)
{
	MainWindow* main_window = NULL;
	glade->get_widget_derived("window_main", main_window);

	Pipeline& pipeline = pipeline_manager.create("main_window");
	pipeline.set_source(channel);
	pipeline.add_sink(main_window->get_drawing_area());
	pipeline.start();
}
