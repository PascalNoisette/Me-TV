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
#include "data.h"
#include "devices_dialog.h"

#define GCONF_PATH					"/apps/me-tv"
#define CURRENT_DATABASE_VERSION	2

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
}

Application::Application(int argc, char *argv[], Glib::OptionContext& option_context) :
	Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv, option_context),
	application_dir(make_application_directory()),
	database_filename(Glib::build_filename(get_application_dir(), "/me-tv.db")),
	connection(get_database_filename())
{
	g_debug("Application constructor");

	if (current != NULL)
	{
		throw Exception(_("Application has already been initialised"));
	}
	
	g_static_rec_mutex_init(mutex.gobj());

	current					= this;
	main_window				= NULL;
	status_icon				= NULL;
	stream_thread			= NULL;
	timeout_source			= 0;
	record_state			= false;
	broadcast_state			= false;
	scheduled_recording_id	= 0;
	save_thread				= NULL;
	
	signal_quit().connect(sigc::mem_fun(this, &Application::on_quit));

	// Remove all other handlers first
	get_signal_error().clear();
	get_signal_error().connect(sigc::mem_fun(*this, &Application::on_error));

	g_debug("sqlite3_threadsafe() = %d", sqlite3_threadsafe());
	if (sqlite3_threadsafe() == 0)
	{
		throw Exception(_("The SQLite version is not thread-safe"));
	}
	
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
	set_boolean_configuration_default("fullscreen_bug_workaround", true);
	set_boolean_configuration_default("display_status_icon", true);
	set_int_configuration_default("x", 10);
	set_int_configuration_default("y", 10);
	set_int_configuration_default("width", 500);
	set_int_configuration_default("height", 500);
	set_int_configuration_default("epg_page_size", 20);
	set_string_configuration_default("screensaver_poke_command", "gnome-screensaver-command --poke");
	set_string_configuration_default ("gstreamer_command_line",
		"filesrc location=\"%1\" ! queue ! decodebin name=decoder " \
		"decoder. ! queue ! deinterlace name=deinterlace ! queue ! " \
		"xvimagesink name=videosink force-aspect-ratio=true " \
		"decoder. ! queue ! volume name=volume ! gconfaudiosink");

	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(application_dir);
	if (!file->query_exists())
	{
		file->make_directory();
	}
	
	Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
	Glib::ustring ui_path = current_directory + "/me-tv.ui";

	if (!Gio::File::create_for_path(ui_path)->query_exists())
	{
		ui_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.ui";
	}
	
	g_debug("Loading GtkBuilder file '%s' ...", ui_path.c_str());
	builder = Gtk::Builder::create_from_file(ui_path);
	
	g_debug("Application constructed");
}

Application::~Application()
{	
	g_debug("Application destructor started");
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
	stop_stream_thread();
	if (main_window != NULL)
	{
		delete main_window;
		main_window = NULL;
	}

	start_save_thread(true);
	while (!save_thread->is_terminated())
	{
		g_debug("Waiting for stream thread to exit");
		usleep(500000);
	}
	g_debug("Application destructor complete");
}

const Glib::ustring& Application::get_database_filename()
{
	return database_filename;
}

gboolean Application::initialise_database()
{
	gboolean result = false;
	
	Data::Table table_channel;
	table_channel.name = "channel";
	table_channel.columns.add("channel_id",				Data::DATA_TYPE_INTEGER, 0, false);
	table_channel.columns.add("name",					Data::DATA_TYPE_STRING, 50, false);
	table_channel.columns.add("flags",					Data::DATA_TYPE_INTEGER, 0, false);
	table_channel.columns.add("sort_order",				Data::DATA_TYPE_INTEGER, 0, false);
	table_channel.columns.add("mrl",					Data::DATA_TYPE_STRING, 1024, true);
	table_channel.columns.add("service_id",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("frequency",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("inversion",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("bandwidth",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("code_rate_hp",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("code_rate_lp",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("constellation",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("transmission_mode",		Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("guard_interval",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("hierarchy_information",	Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("symbol_rate",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("fec_inner",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("modulation",				Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.columns.add("polarisation",			Data::DATA_TYPE_INTEGER, 0, true);
	table_channel.primary_key = "channel_id";
	StringList table_channel_unique_columns;
	table_channel_unique_columns.push_back("name");
	table_channel.constraints.add_unique(table_channel_unique_columns);
	schema.tables.add(table_channel);
	
	Data::Table table_epg_event;
	table_epg_event.name = "epg_event";
	table_epg_event.columns.add("epg_event_id",	Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("channel_id",	Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("event_id",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("start_time",	Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("duration",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.primary_key = "epg_event_id";
	StringList table_epg_event_unique_columns;
	table_epg_event_unique_columns.push_back("channel_id");
	table_epg_event_unique_columns.push_back("event_id");
	table_epg_event.constraints.add_unique(table_epg_event_unique_columns);
	schema.tables.add(table_epg_event);
			
	Data::Table table_epg_event_text;
	table_epg_event_text.name = "epg_event_text";
	table_epg_event_text.columns.add("epg_event_text_id",	Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event_text.columns.add("epg_event_id",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event_text.columns.add("language",			Data::DATA_TYPE_STRING, 3, false);
	table_epg_event_text.columns.add("title",				Data::DATA_TYPE_STRING, 200, false);
	table_epg_event_text.columns.add("description",			Data::DATA_TYPE_STRING, 1000, false);
	table_epg_event_text.primary_key = "epg_event_text_id";
	StringList table_epg_event_text_unique_columns;
	table_epg_event_text_unique_columns.push_back("epg_event_id");
	table_epg_event_text_unique_columns.push_back("language");
	table_epg_event_text.constraints.add_unique(table_epg_event_text_unique_columns);
	schema.tables.add(table_epg_event_text);

	Data::Table table_scheduled_recording;
	table_scheduled_recording.name = "scheduled_recording";
	table_scheduled_recording.columns.add("scheduled_recording_id",	Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("description",			Data::DATA_TYPE_STRING, 200, false);
	table_scheduled_recording.columns.add("type",					Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("channel_id",				Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("start_time",				Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("duration",				Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("device",					Data::DATA_TYPE_STRING, 200, false);
	table_scheduled_recording.primary_key = "scheduled_recording_id";
	schema.tables.add(table_scheduled_recording);

	Data::Table table_version;
	table_version.name = "version";
	table_version.columns.add("value",	Data::DATA_TYPE_INTEGER, 0, false);
	schema.tables.add(table_version);
	
	Data::SchemaAdapter adapter(connection, schema);
	adapter.initialise_table(table_version);
	
	Data::TableAdapter adapter_version(connection, table_version);
	
	if (connection.get_database_created())
	{
		adapter.initialise_schema();
		
		Data::DataTable data_table_version(table_version);
		Data::Row row;
		row["value"].int_value = CURRENT_DATABASE_VERSION;
		data_table_version.rows.add(row);
		adapter_version.replace_rows(data_table_version);
		
		result = true;
	}
	else
	{
		guint database_version = 0;

		Data::DataTable data_table = adapter_version.select_rows();
		if (!data_table.rows.empty())
		{
			database_version = data_table.rows[0]["value"].int_value;
		}
		
		g_debug("Required Database version: %d", CURRENT_DATABASE_VERSION);
		g_debug("Actual Database version: %d", database_version);

		if (database_version == CURRENT_DATABASE_VERSION)
		{
			result = true;
		}
		else
		{
			Gtk::Dialog* dialog_database_version = NULL;
			builder->get_widget("dialog_database_version", dialog_database_version);
			int response = dialog_database_version->run();
			dialog_database_version->hide();
			if (response == 0)
			{
				g_debug("Dropping Me TV schema");

				adapter.drop_schema();
				g_debug("Vacuuming database");
				connection.vacuum();
				adapter.initialise_schema();

				Data::DataTable data_table_version(table_version);
				Data::Row row;
				row["value"].int_value = CURRENT_DATABASE_VERSION;
				data_table_version.rows.add(row);
				adapter_version.replace_rows(data_table_version);

				result = true;
			}
			else
			{
				result = false;
			}
		}
	}
		
	return result;
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

	if (initialise_database())
	{
		g_debug("Me TV database initialised successfully");
				
		const FrontendList& frontends = device_manager.get_frontends();

		if (!default_device.empty())
		{
			Dvb::Frontend* default_frontend = device_manager.find_frontend_by_path(default_device);
			
			if (default_frontend == NULL)
			{
				Glib::ustring message = Glib::ustring::compose(
					_("Failed to load default device '%1'"), default_device);
				throw Exception(message);
			}
			
			device_manager.set_frontend(*default_frontend);
		}
		else
		{
			if (frontends.size() > 0)
			{
				device_manager.set_frontend(**frontends.begin());
			}
		}

		channel_manager.load(connection);
//		scheduled_recording_manager.load(connection);

		status_icon = new StatusIcon(builder);
		main_window = MainWindow::create(builder);

		if (!minimised_mode)
		{
			main_window->show();
		
			if (safe_mode)
			{
				main_window->show_preferences_dialog();
			}
		}

		timeout_source = gdk_threads_add_timeout(1000, &Application::on_timeout, this);

		TRY
		//device_manager.get_frontend();
		
		const ChannelList& channels = channel_manager.get_channels();
		if (channels.empty())
		{
			main_window->show_channels_dialog();
		}
		if (channels.size() > 0)
		{
			gint last_channel = get_application().get_int_configuration_value("last_channel");
			if (channel_manager.find_channel(last_channel) == NULL)
			{
				g_debug("Last channel '%d' not found", last_channel);
				last_channel = -1;
			}
			if (last_channel == -1)
			{
				last_channel = (*channels.begin()).channel_id;
			}
			set_display_channel(last_channel);
		}
		CATCH
		
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

Glib::ustring Application::make_application_directory()
{
	Glib::ustring path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
	if (!file->query_exists())
	{
		g_debug("Creating directory '%s'", path.c_str());
		file->make_directory();
	}
	return path;
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

void Application::set_display_channel(const Channel& channel)
{
	if (is_recording())
	{
		throw Exception(_("You cannot change channels because you are recording."));
	}

	main_window->stop_engine();
	stop_stream_thread();

	channel_manager.set_display_channel(channel);
	
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
		
	set_int_configuration_value("last_channel", channel.channel_id);

	update();
}

void Application::set_display_channel(guint channel_id)
{
	set_display_channel(channel_manager.get_channel(channel_id));
}

MainWindow& Application::get_main_window()
{
	if (main_window == NULL)
	{
		throw Exception(_("Main window has not been created"));
	}
	
	return *main_window;
}

void Application::check_scheduled_recordings()
{
	guint id = scheduled_recording_manager.check_scheduled_recordings();
	if (id != 0)
	{
		scheduled_recording_id = id;
		ScheduledRecording scheduled_recording = scheduled_recording_manager.get_scheduled_recording(id);

		const Channel* channel = channel_manager.get_display_channel();
		if (channel == NULL || channel->channel_id == scheduled_recording.channel_id)
		{
			g_debug("Already tuned to correct channel");
		}
		else
		{			
			g_debug("Changing channel for scheduled recording");
			set_display_channel(scheduled_recording.channel_id);
		}
		
		if (record_state == true)
		{
			g_debug("Already recording");
		}
		else
		{
			g_debug("Starting recording due to scheduled recording");
			Glib::ustring filename = make_recording_filename(scheduled_recording.description);
			start_recording(filename);
		}
	}

	// Check if the SR has just finished
	if (stream_thread != NULL &&			// If there's a stream
		record_state &&						// and it's recording
		scheduled_recording_id != 0 &&		// and there is an existing SR running
		id == 0)							// but the recording manager has just told us that there's no SR
	{										// then we need to stop recording
		g_debug("Record stopped by scheduled recording");
		stop_recording();
	}
}

gboolean Application::on_timeout(gpointer data)
{
	return ((Application*)data)->on_timeout();
}

gboolean Application::on_timeout()
{
	TRY
	static guint last_seconds = 60;
	
	guint now = time(NULL);
	
	guint seconds = now % 60;
	if (last_seconds > seconds)
	{
		check_scheduled_recordings();
		
		if (save_thread == NULL)
		{
			start_save_thread(false);
		}
		else
		{
			g_debug("Save thread already running");
			if (save_thread->is_terminated())
			{
				g_debug("Save thread has terminated, deleting");

				delete save_thread;
				save_thread = NULL;

				g_debug("Save thread deleted");
				
				start_save_thread(false);
			}
		}
		
		update();
	}
	last_seconds = seconds;
	
	CATCH
	
	return true;
}

Glib::ustring Application::make_recording_filename(const Glib::ustring& description)
{
	Channel* channel = channel_manager.get_display_channel();
		
	if (channel == NULL)
	{
		throw Exception(_("No channel to make recording filename"));
	}
	
	Glib::ustring start_time = get_local_time_text("%c");
	Glib::ustring filename;
	Glib::ustring title = description;

	if (title.empty())
	{
		EpgEvent epg_event;
		if (channel->epg_events.get_current(epg_event))
		{
			title = epg_event.get_title();
		}
	}
	
	if (title.empty())
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2.mpeg",
			channel->name,
			start_time
		);
	}
	else
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2 - %3.mpeg",
			title,
			channel->name,
			start_time
		);
	}

	// Remove forward slashes in the filename, if any
	Glib::ustring::size_type position = Glib::ustring::npos;
	while ((position = filename.find('/')) != Glib::ustring::npos)
	{
		filename.erase(position, 1);
	}
	
	Glib::ustring fixed_filename = Glib::filename_from_utf8(filename);
	
	return Glib::build_filename(get_string_configuration_value("recording_directory"), fixed_filename);
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

void Application::start_recording(const Glib::ustring& filename)
{
	if (record_state == false)
	{
		Glib::RecMutex::Lock lock(mutex);
		
		if (stream_thread == NULL)
		{
			throw Exception(_("There is no stream to record"));
		}
		
		Glib::ustring recording_filename = filename;

		if (recording_filename.empty())
		{
			recording_filename = make_recording_filename();
		}
		
		stream_thread->start_recording(recording_filename);
		record_state = true;
		update();
		
		g_debug("Recording started");
	}
}

void Application::stop_recording()
{
	if (record_state == true)
	{
		Glib::RecMutex::Lock lock(mutex);

		stream_thread->stop_recording();
		record_state = false;
		
		if (scheduled_recording_id != 0)
		{
			scheduled_recording_manager.remove_scheduled_recording(scheduled_recording_id);
		}
		scheduled_recording_id = 0;
	
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
		FullscreenBugWorkaround fullscreen_bug_workaround;
		Gtk::MessageDialog dialog(*main_window, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.set_title(_("Me TV - Error Message"));
		dialog.run();
	}
	else
	{
		Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.set_title(_("Me TV - Error Message"));
		dialog.run();
	}
}

void Application::restart_stream()
{
	Channel* channel = channel_manager.get_display_channel();
	if (channel != NULL)
	{
		set_display_channel(channel->channel_id);
	}
}

void Application::start_save_thread(gboolean block)
{	
	if (block)
	{
		while (save_thread != NULL)
		{
			if (save_thread->is_terminated())
			{
				delete save_thread;
				save_thread = NULL;
			}
			else
			{
				g_debug("Waiting for existing save thread to exit");
				usleep(500000);
			}
		}
	}
	else
	{
		if (save_thread != NULL)
		{
			throw Exception(_("Save thread is already running"));
		}
	}

	g_debug("Creating save thread");
	save_thread = new SaveThread();
	save_thread->start();
}
