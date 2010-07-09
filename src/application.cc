/*
 * Copyright (C) 2010 Michael Lamothe
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
#include "crc32.h"
#include "me-tv-ui.h"
#include "main_window.h"
#include <dbus/dbus-glib-lowlevel.h>

#define CURRENT_DATABASE_VERSION	6

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
}

Application::Application()
{
	g_debug("Application constructor");

	if (current != NULL)
	{
		throw Exception(_("Application has already been initialised"));
	}
	
	g_static_rec_mutex_init(mutex.gobj());

	current					= this;
	status_icon				= NULL;
	timeout_source			= 0;
	database_initialised	= false;
	dbus_connection			= NULL;
	
#ifndef IGNORE_SQLITE3_THREADSAFE_CHECK
	g_debug("sqlite3_threadsafe() = %d", sqlite3_threadsafe());
	if (sqlite3_threadsafe() == 0)
	{
		throw Exception(_("The SQLite version is not thread-safe"));
	}
#endif

	Crc32::init();
		
	application_dir = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	make_directory_with_parents (application_dir);

	Glib::ustring data_directory = Glib::get_home_dir() + "/.local/share/me-tv";
	make_directory_with_parents (data_directory);
	
	database_filename = Glib::build_filename(data_directory, "me-tv.db");
	connection.open(database_filename);
	
	g_debug("Loading UI files");
	
	builder = Gtk::Builder::create_from_file(PACKAGE_DATA_DIR"/me-tv/glade/me-tv.ui");
	
	toggle_action_fullscreen = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_fullscreen"));
	toggle_action_mute = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_mute"));
	toggle_action_record_current = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_record_current"));
	toggle_action_visibility = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_visibility"));

	action_about = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_about"));
	action_auto_record = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_auto_record"));
	action_channels = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_channels"));
	action_change_view_mode = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_change_view_mode"));
	action_epg_event_search = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_epg_event_search"));
	action_preferences = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_preferences"));
	action_present = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_present"));
	action_quit = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_quit"));
	action_scheduled_recordings = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_scheduled_recordings"));
	action_increase_volume = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_increase_volume"));
	action_decrease_volume = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_decrease_volume"));

	Glib::RefPtr<Gtk::ActionGroup> action_group = Gtk::ActionGroup::create();
	action_group->add(toggle_action_record_current, Gtk::AccelKey("R"));
	action_group->add(toggle_action_fullscreen, Gtk::AccelKey("F"));
	action_group->add(toggle_action_mute, Gtk::AccelKey("M"));
	action_group->add(toggle_action_visibility);

	action_group->add(action_about, Gtk::AccelKey("F1"));
	action_group->add(action_auto_record);
	action_group->add(action_channels);
	action_group->add(action_change_view_mode, Gtk::AccelKey("V"));
	action_group->add(action_epg_event_search);
	action_group->add(action_preferences);
	action_group->add(action_present);
	action_group->add(action_quit);
	action_group->add(action_scheduled_recordings);
	action_group->add(action_increase_volume, Gtk::AccelKey("plus"));
	action_group->add(action_decrease_volume, Gtk::AccelKey("minus"));

	action_group->add(Gtk::Action::create("action_file", _("_File")));
	action_group->add(Gtk::Action::create("action_view", _("_View")));
	action_group->add(Gtk::Action::create("action_video", _("_Video")));
	action_group->add(Gtk::Action::create("action_audio", _("_Audio")));
	action_group->add(Gtk::Action::create("action_help", _("_Help")));
	
	action_group->add(Gtk::Action::create("action_subtitle_streams", _("Subtitles")));
	action_group->add(Gtk::Action::create("action_audio_streams", _("_Streams")));
	action_group->add(Gtk::Action::create("action_audio_channels", _("_Channels")));

	Gtk::RadioButtonGroup radio_button_group_audio_channel;
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_both", _("_Both")));
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_left", _("_Left")));
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_right", _("_Right")));
	
	action_quit->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));
	toggle_action_record_current->signal_activate().connect(sigc::mem_fun(*this, &Application::on_record_current));

	ui_manager = Gtk::UIManager::create();
	ui_manager->insert_action_group(action_group);
	
	GError *error = NULL;
	dbus_connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (dbus_connection == NULL)
	{
		g_message(_("Failed to get DBus session"));
	}
	
	g_debug("Application constructed");
}

Application::~Application()
{	
	g_debug("Application destructor started");
	if (timeout_source != 0)
	{
		g_source_remove(timeout_source);
	}
	
	if (status_icon != NULL)
	{
		delete status_icon;
		status_icon = NULL;
	}

	if (database_initialised)
	{
		scheduled_recording_manager.save(connection);
		channel_manager.save(connection);
	}

	g_debug("Application destructor complete");
}

void Application::on_record_current()
{
	Glib::RecMutex::Lock lock(mutex);

	if (toggle_action_record_current->get_active())
	{
		device_manager.check_frontend();
		
		try
		{
			start_recording(stream_manager.get_display_channel());
		}
		catch (const Glib::Exception& exception)
		{
			toggle_action_record_current->set_active(false);
			throw Exception(exception.what());
		}
		catch (...)
		{
			toggle_action_record_current->set_active(false);
			throw Exception(_("Failed to start recording"));
		}
	}
	else
	{
		stream_manager.stop_recording(stream_manager.get_display_channel());			
		g_debug("Recording stopped");
	}

	signal_update();
}

void Application::on_quit()
{
	Gtk::Main::quit();
}

void Application::make_directory_with_parents(const Glib::ustring& path)
{
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
	if (!file->query_exists())
	{
		Glib::RefPtr<Gio::File> parent = file->get_parent();
		if (!parent->query_exists())
		{
			make_directory_with_parents(parent->get_path());
		}

		g_debug("Creating directory '%s'", path.c_str());
		file->make_directory();
	}
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
	table_channel.columns.add("type",					Data::DATA_TYPE_INTEGER, 0, false);
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
	table_epg_event.columns.add("epg_event_id",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("channel_id",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("version_number",	Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("event_id",			Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("start_time",		Data::DATA_TYPE_INTEGER, 0, false);
	table_epg_event.columns.add("duration",			Data::DATA_TYPE_INTEGER, 0, false);
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
	table_epg_event_text.columns.add("subtitle",			Data::DATA_TYPE_STRING, 200, false);
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
	table_scheduled_recording.columns.add("recurring_type",			Data::DATA_TYPE_INTEGER, 0, false);
	table_scheduled_recording.columns.add("action_after",			Data::DATA_TYPE_INTEGER, 0, false);
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

	database_initialised = true;
		
	return result;
}

void Application::run()
{
	GdkLock gdk_lock;

	if (!initialise_database())
	{
		throw Exception(_("Failed to initialise database"));
	}

	g_debug("Me TV database initialised");

	configuration_manager.initialise();
	preferred_language = configuration_manager.get_string_value("preferred_language");

	status_icon = new StatusIcon();
	MainWindow::create(builder);

	try
	{
		device_manager.initialise();
		channel_manager.initialise();
		channel_manager.load(connection);
		scheduled_recording_manager.initialise();
		stream_manager.initialise();
		stream_manager.start();

		ChannelArray& channels = channel_manager.get_channels();	

		const FrontendList& frontends = device_manager.get_frontends();	
		if (!frontends.empty())
		{
			scheduled_recording_manager.load(connection);	
		}	

		if (!minimised_mode)	
		{
			action_present->activate();
	
			if (safe_mode)	
			{
				action_preferences->activate();
			}	
		}

		// Check that there's a device
		device_manager.check_frontend();
		
		if (channels.empty())
		{
			action_channels->activate();
		}
	}
	catch(...)
	{
		on_error();
	}

	timeout_source = gdk_threads_add_timeout(1000, &Application::on_timeout, this);
	Gtk::Main::run();
}

Application& Application::get_current()
{
	if (current == NULL)
	{
		throw Exception(_("Application has not been initialised"));
	}
	
	return *current;
}

void Application::check_scheduled_recordings()
{
	ScheduledRecordingList scheduled_recordings = scheduled_recording_manager.check_scheduled_recordings();
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		const ScheduledRecording& scheduled_recording = *i;
		Channel* channel = channel_manager.find_channel(scheduled_recording.channel_id);
		if (channel != NULL)
		{
			start_recording(*channel, scheduled_recording);
		}
	}

	gboolean check = true;
	while (check)
	{
		check = false;

		FrontendThreadList& frontend_threads = stream_manager.get_frontend_threads();
		for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
		{
			FrontendThread& frontend_thread = **i;
			ChannelStreamList& streams = frontend_thread.get_streams();
			for (ChannelStreamList::iterator j = streams.begin(); j != streams.end(); j++)
			{
				ChannelStream& channel_stream = **j;
				guint scheduled_recording_id = scheduled_recording_manager.is_recording(channel_stream.channel);
				if (channel_stream.type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING &&
					(signed)scheduled_recording_id >= 0)
				{
					stream_manager.stop_recording(channel_stream.channel);
					action_after(scheduled_recording_id);
					check = true;
					break;
				}
			}
		}
	}
}

void Application::action_after(guint action)
{
	if (action == SCHEDULED_RECORDING_ACTION_AFTER_CLOSE)
	{
		g_message("Me TV closed by Scheduled Recording");
		action_quit->activate();
	}
	else if (action == SCHEDULED_RECORDING_ACTION_AFTER_SHUTDOWN)
	{
		if (dbus_connection == NULL)
		{
			throw Exception(_("DBus connection not available"));
		}
		
		g_message("Computer shutdown by scheduled recording");
		
		DBusGProxy* proxy = dbus_g_proxy_new_for_name(dbus_connection,
			"org.gnome.SessionManager",
			"/org/gnome/SessionManager",
			"org.gnome.SessionManager");
		if (proxy == NULL)
		{
			throw Exception(_("Failed to get org.gnome.SessionManager proxy"));
		}
		
		GError* error = NULL;
		if (!dbus_g_proxy_call(proxy, "Shutdown", &error, G_TYPE_INVALID, G_TYPE_INVALID))
		{
			throw Exception(_("Failed to call Shutdown method"));
		}

		g_message("Shutdown requested");
	}
}

gboolean Application::on_timeout(gpointer data)
{
	return ((Application*)data)->on_timeout();
}

gboolean Application::on_timeout()
{
	static guint last_seconds = 60;

	try
	{
		guint now = time(NULL);
		guint seconds = now % 60;
		if (last_seconds > seconds)
		{
			if (channel_manager.is_dirty())
			{
				channel_manager.save(connection);
				check_auto_record();
			}
			
			check_scheduled_recordings();
			scheduled_recording_manager.save(connection);
			channel_manager.prune_epg();

			signal_update();
		}
		last_seconds = seconds;
	}
	catch(...)
	{
		on_error();
	}
	
	return true;
}

void Application::check_auto_record()
{
	StringList auto_record_list = configuration_manager.get_string_list_value("auto_record");
	ChannelArray& channels = channel_manager.get_channels();

	g_debug("Searching for auto record EPG events");
	for (StringList::iterator iterator = auto_record_list.begin(); iterator != auto_record_list.end(); iterator++)
	{
		Glib::ustring title = (*iterator).uppercase();
		g_debug("Searching for '%s'", title.c_str());

		for (ChannelArray::iterator i = channels.begin(); i != channels.end(); i++)
		{
			Channel& channel = *i;

			EpgEventList list = channel.epg_events.search(title, false);
			for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
			{
				EpgEvent& epg_event = *j;

				gboolean record = scheduled_recording_manager.is_recording(epg_event);
				if (!record)
				{
					try
					{
						g_debug("Trying to auto record '%s' (%d)", epg_event.get_title().c_str(), epg_event.event_id);
						scheduled_recording_manager.set_scheduled_recording(epg_event);
					}
					catch(...)
					{
						on_error();
					}			
				}
			}
		}
	}
}

Glib::StaticRecMutex& Application::get_mutex()
{
	return mutex;
}

void Application::start_recording(Channel& channel)
{
	stream_manager.start_recording(channel);
	signal_update();
}

void Application::start_recording(Channel& channel, const ScheduledRecording& scheduled_recording)
{
	stream_manager.start_recording(channel, scheduled_recording);
	signal_update();
}

void Application::stop_recording(Channel& channel)
{
	stream_manager.stop_recording(channel);
	signal_update();
}
