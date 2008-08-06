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
#include "config.h"
#include "data.h"

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
	main_window = NULL;
	stream_thread = NULL;

	client = Gnome::Conf::Client::get_default_client();
	
	set_int_configuration_default("epg_span_hours", 3);
	set_int_configuration_default("last_channel", -1);
	set_string_configuration_default("recording_directory", Glib::get_home_dir());
	set_string_configuration_default("engine_type", "gstreamer");
	set_boolean_configuration_default("keep_above", true);
	set_int_configuration_default("record_extra_before", 5);
	set_int_configuration_default("record_extra_after", 10);
	set_string_configuration_default("broadcast_address", "192.168.0.255");
	set_int_configuration_default("broadcast_port", 2005);
	set_boolean_configuration_default("show_epg_header", true);
	set_boolean_configuration_default("show_epg_time", true);
	
	Glib::ustring path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
	if (!file->query_exists())
	{
		file->make_directory();
	}
	
	// Initialise database
	Data data(true);
	
	Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
	Glib::ustring glade_path = current_directory + "/me-tv.glade";

	if (!Gio::File::create_for_path(glade_path)->query_exists())
	{
		glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
	}
	
	g_debug("Using glade file '%s'", glade_path.c_str());
	
	glade = Gnome::Glade::Xml::create(glade_path);
	
	profile_manager.load();
	Profile& profile = profile_manager.get_current_profile();
	profile.signal_display_channel_changed.connect(
		sigc::mem_fun(*this, &Application::on_display_channel_changed));	
}

Application::~Application()
{
	stop_stream_thread();
	if (main_window != NULL)
	{
		delete main_window;
		main_window = NULL;
	}
}

Glib::ustring Application::get_configuration_path(const Glib::ustring& key)
{
	return Glib::ustring::compose(GCONF_PATH"/%1", key);
}

void Application::set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%s'", key.c_str(), value.c_str());
		client->set(path, value);
	}
}

void Application::set_int_configuration_default(const Glib::ustring& key, gint value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%d'", path.c_str(), value);
		client->set(path, value);
	}
}

void Application::set_boolean_configuration_default(const Glib::ustring& key, gboolean value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%s'", path.c_str(), value ? "true" : "false");
		client->set(path, (bool)value);
	}
}

Glib::ustring Application::get_string_configuration_value(const Glib::ustring& key)
{
	return client->get_string(get_configuration_path(key));
}

gint Application::get_int_configuration_value(const Glib::ustring& key)
{
	return client->get_int(get_configuration_path(key));
}

gint Application::get_boolean_configuration_value(const Glib::ustring& key)
{
	return client->get_bool(get_configuration_path(key));
}

void Application::set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value)
{
	client->set(get_configuration_path(key), value);
}

void Application::set_int_configuration_value(const Glib::ustring& key, gint value)
{
	client->set(get_configuration_path(key), (gint)value);
}

void Application::set_boolean_configuration_value(const Glib::ustring& key, gboolean value)
{
	client->set(get_configuration_path(key), (bool)value);
}

void Application::run()
{
	TRY
	GdkLock gdk_lock;
	status_icon = new StatusIcon(glade);
	main_window = MainWindow::create(glade);
	main_window->show();
	Gnome::Main::run();
	CATCH
}

Application& Application::get_current()
{
	if (current == NULL)
	{
		throw Exception(_("Application has not been initialised"));
	}
	
	return *current;
}

void Application::stop_stream_thread()
{	
	if (stream_thread != NULL)
	{
		delete stream_thread;
		stream_thread = NULL;
	}
}

void Application::set_source(const Channel& channel)
{
	stop_stream_thread();
	stream_thread = new StreamThread(channel);
	stream_thread->start();
	update_ui();
}

void Application::update_ui()
{
	main_window->update();
	status_icon->update();
}

void Application::on_display_channel_changed(const Channel& channel)
{
	TRY
	set_source(channel);
	set_int_configuration_value("last_channel", channel.channel_id);
	CATCH
}

void Application::update_epg_time()
{
	last_epg_update_time = time(NULL);
}

guint Application::get_last_epg_update_time() const
{
	return last_epg_update_time;
}

void Application::toggle_visibility()
{
	if (main_window != NULL)
	{
		main_window->property_visible() = !main_window->property_visible();
	}
}

StreamThread& Application::get_stream_thread()
{
	if (stream_thread == NULL)
	{
		throw Exception("Stream thread has not been created");
	}
	
	return *stream_thread;
}

MainWindow& Application::get_main_window()
{
	if (main_window == NULL)
	{
		throw Exception("Main window has not been created");
	}
	
	return *main_window;
}
