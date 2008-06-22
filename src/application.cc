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

void set_default(Glib::RefPtr<Gnome::Conf::Client> client, const Glib::ustring& path, const Glib::ustring& value)
{
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%s'", path.c_str(), value.c_str());
		client->set(path, value);
	}
}

Application::Application(int argc, char *argv[]) :
	Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv)
{
	if (current != NULL)
	{
		throw Exception("Application has already been initialised");
	}
	
	current = this;
	
	Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
	set_default(client, GCONF_PATH"/video_output", "Xv");
	
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

	channel_manager.add_channels(profile_manager.get_current_profile().channels);
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
	TRY
	MainWindow* main_window = NULL;
	glade->get_widget_derived("window_main", main_window);

	Pipeline* existing_pipeline = pipeline_manager.find_pipeline("display");
	if (existing_pipeline != NULL)
	{
		pipeline_manager.remove(existing_pipeline);
	}
	
	Pipeline& pipeline = pipeline_manager.create("display", channel, main_window->get_drawing_area());
	pipeline.start();
	CATCH
}
