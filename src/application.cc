/*
 * Copyright (C) 2009 Michael Lamothe
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

//#define TEST_VIDEO_SOURCE "/usr/share/xine/visuals/default.avi"

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
}

Application::Application(int argc, char *argv[], Glib::OptionContext& option_context) :
	Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv, option_context)
{
	if (current != NULL)
	{
		throw Exception(_("Application has already been initialised"));
	}
	application_dir = path;
	
	g_static_rec_mutex_init(mutex.gobj());

	current = this;
	main_window = NULL;
	status_icon = NULL;
	stream_thread = NULL;
	update_epg_time();
	timeout_source = 0;
	scheduled_recording_id = 0;
	record_state = false;
	broadcast_state = false;

#ifndef TEST_VIDEO_SOURCE
	device_manager.get_frontend();
#endif
	
	signal_quit().connect(sigc::mem_fun(this, &Application::on_quit));

	// Remove all other handlers first
	get_signal_error().clear();
	get_signal_error().connect(sigc::mem_fun(*this, &Application::on_error));

	client = Gnome::Conf::Client::get_default_client();
	
	set_int_configuration_default("epg_span_hours", 3);
	set_int_configuration_default("last_channel", -1);
	set_string_configuration_default("recording_directory", Glib::get_home_dir());
	set_string_configuration_default("engine_type", "xine");
	set_boolean_configuration_default("keep_above", true);
	set_int_configuration_default("record_extra_before", 5);
	set_int_configuration_default("record_extra_after", 10);
	set_string_configuration_default("broadcast_address", "192.168.0.255");
	set_int_configuration_default("broadcast_port", 2005);
	set_boolean_configuration_default("show_epg_header", true);
	set_boolean_configuration_default("show_epg_time", true);
	set_boolean_configuration_default("show_epg_tooltips", false);
	set_string_configuration_default("xine.video_driver", "xv");
	set_string_configuration_default("xine.audio_driver", "alsa");
	set_string_configuration_default("mplayer.video_driver", "xv");
	set_string_configuration_default("mplayer.audio_driver", "alsa");
	set_string_configuration_default("vlc.vout", "xvideo");
	set_string_configuration_default("vlc.aout", "alsa");
	set_string_configuration_default("xine.deinterlace_type", "tvtime");
	set_string_configuration_default("preferred_language", "");
	set_string_configuration_default("text_encoding", "auto");
	set_boolean_configuration_default("use_24_hour_workaround", true);
	set_boolean_configuration_default("display_status_icon", true);
	set_int_configuration_default("x", 10);
	set_int_configuration_default("y", 10);
	set_int_configuration_default("width", 500);
	set_int_configuration_default("height", 500);

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

	timeout_source = gdk_threads_add_timeout(10000, &Application::on_timeout, this);
}

Application::~Application()
{
	if (timeout_source == 0)
	{
		g_source_remove(timeout_source);
	}
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
	if (get_boolean_configuration_value("display_status_icon"))
	{
		status_icon = new StatusIcon(glade);
	}
	main_window = MainWindow::create(glade);
	
	
	if (!minimised_mode)
	{
		main_window->show();
	
		if (safe_mode)
		{
			main_window->show_preferences_dialog();
		}
	}

#ifndef TEST_VIDEO_SOURCE
	Profile& current_profile = get_profile_manager().get_current_profile();
	ChannelList& channels = current_profile.get_channels();
	if (channels.size() == 0)
	{
		main_window->show_channels_dialog();
	}
	channels = current_profile.get_channels();
	if (channels.size() > 0)
	{
		gint last_channel = get_application().get_int_configuration_value("last_channel");
		if (current_profile.find_channel(last_channel) == NULL)
		{
			g_debug("Last channel '%d' not found", last_channel);
			last_channel = -1;
		}
		if (last_channel == -1)
		{
			last_channel = (*channels.begin()).channel_id;
		}
		current_profile.set_display_channel(last_channel);
	}
#else
	main_window->play(TEST_VIDEO_SOURCE);
#endif
	
	Gnome::Main::run();
	
	if (status_icon != NULL)
	{
		delete status_icon;
		status_icon = NULL;
	}
	
	if (main_window != NULL)
	{
		delete main_window;
		main_window = NULL;
	}

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
	Glib::RecMutex::Lock lock(mutex);
	if (stream_thread != NULL)
	{
		delete stream_thread;
		stream_thread = NULL;
	}
}

void Application::set_source(const Channel& channel)
{
	Glib::RecMutex::Lock lock(mutex);

#ifndef TEST_VIDEO_SOURCE
	main_window->stop_engine();
	stop_stream_thread();
	stream_thread = new StreamThread(channel);
	try
	{
		stream_thread->start();

		try
		{
			main_window->start_engine();
		}
		catch(const Glib::Exception& exception)
		{
			main_window->stop_engine();
			get_signal_error().emit(exception.what().c_str());
		}	
	}
	catch(const Glib::Exception& exception)
	{
		stop_stream_thread();
		get_signal_error().emit(exception.what().c_str());
	}
#else
	main_window->play(TEST_VIDEO_SOURCE);
#endif
	
	update();
}

void Application::update()
{
	preferred_language = get_string_configuration_value("preferred_language");	

	if (main_window != NULL)
	{
		main_window->update();
	}
	
	if (status_icon != NULL)
	{
		status_icon->update();
	}
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

MainWindow& Application::get_main_window()
{
	if (main_window == NULL)
	{
		throw Exception(_("Main window has not been created"));
	}
	
	return *main_window;
}

gboolean Application::on_timeout(gpointer data)
{
	return ((Application*)data)->on_timeout();
}

gboolean Application::on_timeout()
{
	TRY
		
	Profile& profile = profile_manager.get_current_profile();
	gboolean got_recording = false;
	
	Data data;
	data.delete_old_scheduled_recordings();
	ScheduledRecordingList scheduled_recording_list = data.get_scheduled_recordings();
	if (scheduled_recording_list.size() > 0)
	{
		guint now = time(NULL);
		g_debug(" ");
		g_debug("======================================================================");
		g_debug("Now: %d", now);
		g_debug("======================================================================");
		g_debug("#ID | Start Time | Duration | Record | Channel    | Description");
		g_debug("======================================================================");
		for (ScheduledRecordingList::iterator i = scheduled_recording_list.begin(); i != scheduled_recording_list.end(); i++)
		{
			ScheduledRecording& scheduled_recording = *i;
			gboolean record = scheduled_recording.is_in(now);
			g_debug("%3d | %d | %8d | %s | %10s | %s",
				scheduled_recording.scheduled_recording_id,
				scheduled_recording.start_time,
				scheduled_recording.duration,
				record ? "true  " : "false ",
				profile.get_channel(scheduled_recording.channel_id).name.c_str(),
				scheduled_recording.description.c_str());
			
			if (record)
			{
				if (got_recording)
				{
					g_debug("Conflict!");
				}
				else
				{
					got_recording = true;
					
					const Channel* channel = profile.get_display_channel();
					if (channel == NULL || channel->channel_id == scheduled_recording.channel_id)
					{
						g_debug("Already tuned to correct channel");
						
						if (record_state == true)
						{
							g_debug("Already recording");
						}
						else
						{
							g_debug("Starting recording due to scheduled recording");
							Glib::ustring filename = make_recording_filename(scheduled_recording.description);
							start_recording(filename, scheduled_recording.scheduled_recording_id);
						}
					}
					else
					{
						scheduled_recording_id = 0;
						
						g_debug("Recording stopped by scheduled recording");
						stop_recording();
						
						g_debug("Changing channel for scheduled recording");
						profile.set_display_channel(scheduled_recording.channel_id);

						g_debug("Starting recording due to scheduled recording");
						Glib::ustring filename = make_recording_filename(scheduled_recording.description);
						start_recording(filename, scheduled_recording.scheduled_recording_id);
					}
				}
			}
		}
	}

	Glib::RecMutex::Lock lock(mutex);
	if (stream_thread != NULL && record_state == true && !got_recording && scheduled_recording_id != 0)
	{
		scheduled_recording_id = 0;
 
		g_debug("Record stopped by scheduled recording");
		stop_recording();
	}

	CATCH
	
	return true;
}

Glib::ustring Application::make_recording_filename(const Glib::ustring& description)
{
	const Channel* channel = profile_manager.get_current_profile().get_display_channel();
		
	if (channel == NULL)
	{
		throw Exception(_("No channel to make recording filename"));
	}
	
	Glib::ustring start_time = get_local_time_text("%c");
	Glib::ustring filename;

	if (description.size() == 0)
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2.mpeg",
			channel->get_text(),
			start_time
		);
	}
	else
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2 - %3.mpeg",
			channel->name,
			description,
			start_time
		);
	}
	
	return Glib::build_filename(get_string_configuration_value("recording_directory"), filename);
}

StreamThread* Application::get_stream_thread()
{
	return stream_thread;
}

gboolean Application::is_recording()
{
	return record_state;
}

void Application::set_record_state(gboolean state)
{
	if (record_state != state)
	{
		if (state == false)
		{			
			stop_recording();
		}
		else
		{
			start_recording();
		}
	}
}

void Application::start_recording(const Glib::ustring& filename, guint id)
{
	if (record_state == false)
	{
		Glib::RecMutex::Lock lock(mutex);
		
		if (stream_thread == NULL)
		{
			throw Exception(_("Stream thread has not been created"));
		}
		
		Glib::ustring recording_filename = filename;

		if (recording_filename.size() == 0)
		{
			recording_filename = make_recording_filename();
		}
		
		stream_thread->start_recording(recording_filename);
		scheduled_recording_id = id;
		record_state = true;
		update();
		
		g_debug("Recording started");
	}
}

void Application::stop_recording()
{
	if (record_state == true)
	{
		if (scheduled_recording_id != 0)
		{
			Glib::ustring message = _("You are trying to stop a scheduled recording.  Would you like Me TV to delete the scheduled recording?");
			Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
			if (dialog.run() == Gtk::RESPONSE_YES)
			{
				Data data;
				data.delete_scheduled_recording(scheduled_recording_id);
			}
		}

		Glib::RecMutex::Lock lock(mutex);
		stream_thread->stop_recording();
		scheduled_recording_id = 0;
		record_state = false;
		update();
		g_debug("Recording stopped");
	}
}

void Application::toggle_recording()
{
	set_record_state(!record_state);
}

void Application::toggle_broadcast()
{
	set_broadcast_state(!broadcast_state);
}

gboolean Application::is_broadcasting()
{
	return broadcast_state;
}

void Application::set_broadcast_state(gboolean state)
{
	if (broadcast_state != state)
	{
		broadcast_state = state;
		
		Glib::RecMutex::Lock lock(mutex);
		if (stream_thread != NULL)
		{
			if (broadcast_state)
			{
				stream_thread->start_broadcasting();
			}
			else
			{
				stream_thread->stop_broadcasting();
			}
		}
		update();
	}
}

Glib::StaticRecMutex& Application::get_mutex()
{
	return mutex;
}

bool Application::on_quit()
{
	if (main_window != NULL)
	{
		main_window->stop_engine();
	}
	return true;
}

void Application::on_error(const Glib::ustring& message)
{
	if (main_window != NULL)
	{
		Gtk::MessageDialog dialog(*main_window, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.run();
	}
	else
	{
		Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.run();
	}
}
