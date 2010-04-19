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

#define GCONF_PATH					"/apps/me-tv"
#define CURRENT_DATABASE_VERSION	5

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
	main_window				= NULL;
	status_icon				= NULL;
	timeout_source			= 0;
	database_initialised	= false;
	
#ifndef IGNORE_SQLITE3_THREADSAFE_CHECK
	g_debug("sqlite3_threadsafe() = %d", sqlite3_threadsafe());
	if (sqlite3_threadsafe() == 0)
	{
		throw Exception(_("The SQLite version is not thread-safe"));
	}
#endif

	Crc32::init();
	
	gconf_client = Gnome::Conf::Client::get_default_client();

	if (get_int_configuration_value("epg_span_hours") == 0)
	{
		set_int_configuration_value("epg_span_hours", 1);
	}

	if (get_int_configuration_value("epg_page_size") == 0)
	{
		set_int_configuration_value("epg_page_size", 1);
	}
	
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
	toggle_action_record = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(builder->get_object("toggle_action_record"));

	action_about = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_about"));
	action_channels = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_channels"));
	action_epg_event_search = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_epg_event_search"));
	action_meters = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_meters"));
	action_next_channel = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_next_channel"));
	action_preferences = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_preferences"));
	action_previous_channel = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_previous_channel"));
	action_quit = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_quit"));
	action_scheduled_recordings = Glib::RefPtr<Gtk::Action>::cast_dynamic(builder->get_object("action_scheduled_recordings"));

	Glib::RefPtr<Gtk::ActionGroup> action_group = Gtk::ActionGroup::create();
	action_group->add(toggle_action_record, Gtk::AccelKey("R"));
	action_group->add(toggle_action_fullscreen, Gtk::AccelKey("F"));
	action_group->add(toggle_action_mute, Gtk::AccelKey("M"));

	action_group->add(action_about, Gtk::AccelKey("F1"));
	action_group->add(action_channels);
	action_group->add(action_epg_event_search);
	action_group->add(action_meters);
	action_group->add(action_next_channel, Gtk::AccelKey("<Ctrl>Down"));
	action_group->add(action_preferences);
	action_group->add(action_previous_channel, Gtk::AccelKey("<Ctrl>Up"));
	action_group->add(action_quit);
	action_group->add(action_scheduled_recordings);

	action_group->add(Gtk::Action::create("action_file", Gtk::Stock::FILE, _("_File")));
	action_group->add(Gtk::Action::create("action_view", _("_View")));
	action_group->add(Gtk::Action::create("action_video", _("_Video")));
	action_group->add(Gtk::Action::create("action_audio", _("_Audio")));
	action_group->add(Gtk::Action::create("action_help", Gtk::Stock::HELP));
	action_group->add(Gtk::Action::create("action_change_view_mode", _("_Change View Mode")));
	action_group->add(Gtk::Action::create("action_subtitle_streams", _("Subtitles")));
	action_group->add(Gtk::Action::create("action_audio_streams", _("_Streams")));
	action_group->add(Gtk::Action::create("action_audio_channels", _("_Channels")));

	Gtk::RadioButtonGroup radio_button_group_audio_channel;
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_both", _("_Both")));
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_left", _("_Left")));
	action_group->add(Gtk::RadioAction::create(radio_button_group_audio_channel, "action_audio_channel_right", _("_Right")));
	
	action_quit->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));
	toggle_action_record->signal_activate().connect(sigc::mem_fun(*this, &Application::on_record));
	action_next_channel->signal_activate().connect(sigc::mem_fun(*this, &Application::on_next_channel));
	action_previous_channel->signal_activate().connect(sigc::mem_fun(*this, &Application::on_previous_channel));

	ui_manager = Gtk::UIManager::create();
	ui_manager->insert_action_group(action_group);

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

	if (main_window != NULL)
	{
		delete main_window;
		main_window = NULL;
	}

	if (database_initialised)
	{
		scheduled_recording_manager.save(connection);
		channel_manager.save(connection);
	}

	g_debug("Application destructor complete");
}

void Application::on_record()
{
	Glib::RecMutex::Lock lock(mutex);

	if (toggle_action_record->get_active())
	{
		try
		{
			start_recording(channel_manager.get_display_channel());
		}
		catch (const Glib::Exception& exception)
		{
			toggle_action_record->set_active(false);
			throw Exception(exception.what());
		}
		catch (...)
		{
			toggle_action_record->set_active(false);
			throw Exception(_("Failed to start recording"));
		}
	}
	else
	{
		stream_manager.stop_recording(channel_manager.get_display_channel());			
		g_debug("Recording stopped");
	}

	update();
}

void Application::on_previous_channel()
{
	Channel* channel = channel_manager.get_previous_channel();
	if (channel != NULL)
	{
		set_display_channel(*channel);
	}
}

void Application::on_next_channel()
{
	Channel* channel = channel_manager.get_next_channel();
	if (channel != NULL)
	{
		set_display_channel(*channel);
	}
}

void Application::on_quit()
{
	get_application().get_main_window().hide();
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

	database_initialised = true;
		
	return result;
}

Glib::ustring Application::get_configuration_path(const Glib::ustring& key)
{
	return Glib::ustring::compose(GCONF_PATH"/%1", key);
}

void Application::set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%s'", key.c_str(), value.c_str());
		gconf_client->set(path, value);
	}
}

void Application::set_int_configuration_default(const Glib::ustring& key, gint value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%d'", path.c_str(), value);
		gconf_client->set(path, value);
	}
}

void Application::set_boolean_configuration_default(const Glib::ustring& key, gboolean value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%s'", path.c_str(), value ? "true" : "false");
		gconf_client->set(path, (bool)value);
	}
}

StringList Application::get_string_list_configuration_value(const Glib::ustring& key)
{
	return gconf_client->get_string_list(get_configuration_path(key));
}

Glib::ustring Application::get_string_configuration_value(const Glib::ustring& key)
{
	return gconf_client->get_string(get_configuration_path(key));
}

gint Application::get_int_configuration_value(const Glib::ustring& key)
{
	return gconf_client->get_int(get_configuration_path(key));
}

gint Application::get_boolean_configuration_value(const Glib::ustring& key)
{
	return gconf_client->get_bool(get_configuration_path(key));
}

void Application::set_string_list_configuration_value(const Glib::ustring& key, const StringList& value)
{
	gconf_client->set_string_list(get_configuration_path(key), value);
}

void Application::set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value)
{
	gconf_client->set(get_configuration_path(key), value);
}

void Application::set_int_configuration_value(const Glib::ustring& key, gint value)
{
	gconf_client->set(get_configuration_path(key), (gint)value);
}

void Application::set_boolean_configuration_value(const Glib::ustring& key, gboolean value)
{
	gconf_client->set(get_configuration_path(key), (bool)value);
}

void Application::select_channel_to_play()
{
	const ChannelArray& channels = channel_manager.get_channels();
	if (channels.size() > 0)
	{
		gint last_channel_id = get_application().get_int_configuration_value("last_channel");
		Channel* last_channel = channel_manager.find_channel(last_channel_id);
		if (last_channel != NULL)
		{
			g_debug("Last channel '%d' found", last_channel_id);
			set_display_channel_by_id(last_channel_id);
		}
		else
		{
			g_debug("Last channel '%d' not found", last_channel_id);
			channel_manager.select_display_channel();
		}
	}
}

void Application::run()
{
	GdkLock gdk_lock;

	if (!initialise_database())
	{
		throw Exception(_("Failed to initialise database"));
	}

	g_debug("Me TV database initialised");

	status_icon = new StatusIcon();
	main_window = MainWindow::create(builder);

	try
	{	
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
	
		timeout_source = gdk_threads_add_timeout(1000, &Application::on_timeout, this);
	
		if (!device_manager.get_frontends().empty())	
		{	
			scheduled_recording_manager.load(connection);	
		}	
	
		if (!minimised_mode)	
		{	
			main_window->show();	
	
			if (safe_mode)	
			{	
				main_window->show_preferences_dialog();	
			}	
		}
	
		if (device_manager.get_frontends().empty())
		{
			throw Exception(_("There are no DVB devices available"));
		}	

		stream_manager.start();

		ChannelArray& channels = channel_manager.get_channels();	
		if (channels.empty())
		{	
			main_window->show_channels_dialog();	
		}
	
		if (!channels.empty())	
		{	
			select_channel_to_play();	
		}
	}
	catch(...)
	{
		main_window->on_exception();
	}
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

void Application::set_display_channel_by_id(guint channel_id)
{
	set_display_channel(channel_manager.get_channel_by_id(channel_id));
}

void Application::set_display_channel_number(guint channel_index)
{
	set_display_channel(channel_manager.get_channel_by_index(channel_index));
}

void Application::set_display_channel(const Channel& channel)
{
	g_message(_("Changing channel to '%s'"), channel.name.c_str());

	if (channel_manager.has_display_channel())
	{
		Channel& current_channel = channel_manager.get_display_channel();
		if (current_channel.transponder == channel.transponder)
		{
			g_message(_("Already tuned to correct frequency"));
		}
		else
		{
			if (stream_manager.is_recording())
			{
				Glib::ustring message = Glib::ustring::compose(
					_("You cannot tune to channel '%1' because you are recording."),
					channel.name);
				throw Exception(message);
			}
		}

		if (current_channel != channel)
		{
			main_window->stop_engine();
		}
	}

	channel_manager.set_display_channel(channel);
	stream_manager.set_display_stream(channel);
	main_window->start_engine();
	
	set_int_configuration_value("last_channel", channel.channel_id);

	update();

	g_message(_("Channel changed to %s"), channel.name.c_str());
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

		std::list<ChannelStream> streams = stream_manager.get_streams();
		for (std::list<ChannelStream>::iterator i = streams.begin(); i != streams.end(); i++)
		{
			ChannelStream& channel_stream = *i;
			if (
			    channel_stream.type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING &&
				!scheduled_recording_manager.is_recording(channel_stream.channel))
			{
				stream_manager.stop_recording(channel_stream.channel);
				check = true;
				break;
			}
		}
	}
}

gboolean Application::on_timeout(gpointer data)
{
	return ((Application*)data)->on_timeout();
}

gboolean Application::on_timeout()
{
	static guint last_seconds = 60;
	
	guint now = time(NULL);
	
	guint seconds = now % 60;
	if (last_seconds > seconds)
	{
		check_scheduled_recordings();
		scheduled_recording_manager.save(connection);
		channel_manager.prune_epg();
		channel_manager.save(connection);
		update();
	}
	last_seconds = seconds;
	
	return true;
}

Glib::StaticRecMutex& Application::get_mutex()
{
	return mutex;
}

void Application::start_recording(Channel& channel)
{
	stream_manager.start_recording(channel);
	update();
	g_debug("Recording started");
}

void Application::start_recording(Channel& channel, const ScheduledRecording& scheduled_recording)
{
	stream_manager.start_recording(channel, scheduled_recording);
	update();
	g_debug("Scheduled recording started");
}

void Application::stop_recording(Channel& channel)
{
	stream_manager.stop_recording(channel);
	update();
}
