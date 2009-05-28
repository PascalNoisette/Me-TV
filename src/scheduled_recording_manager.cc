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

#include "scheduled_recording_manager.h"
#include "application.h"

ScheduledRecordingManager::ScheduledRecordingManager()
{
	g_static_rec_mutex_init(mutex.gobj());
}

void ScheduledRecordingManager::load(Data::Connection& connection)
{
	Glib::RecMutex::Lock lock(mutex);

	Glib::ustring frontend_path = get_application().device_manager.get_frontend().get_path();
	
	g_debug("Loading scheduled recordings");

	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::TableAdapter adapter(connection, table);
	
	Glib::ustring where = Glib::ustring::compose(
		"device = '%1' AND (start_time + duration) > %2",
		frontend_path, time(NULL));
	Data::DataTable data_table = adapter.select_rows(where, "start_time");
	
	scheduled_recordings.clear();
	for (Data::Rows::iterator i = data_table.rows.begin(); i != data_table.rows.end(); i++)
	{
		ScheduledRecording scheduled_recording;
		Data::Row& row = *i;
		
		scheduled_recording.scheduled_recording_id	= row["scheduled_recording_id"].int_value;
		scheduled_recording.channel_id				= row["channel_id"].int_value;
		scheduled_recording.description				= row["description"].string_value;
		scheduled_recording.start_time				= row["start_time"].int_value;
		scheduled_recording.duration				= row["duration"].int_value;
		scheduled_recording.device					= row["device"].string_value;

		scheduled_recordings.push_back(scheduled_recording);
	}
	g_debug("Scheduled recordings loaded");
}

void ScheduledRecordingManager::save(Data::Connection& connection)
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("Saving %d scheduled recordings", scheduled_recordings.size());
	
	Data::Table table = get_application().get_schema().tables["scheduled_recording"];	
	Data::DataTable data_table(table);
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;

		guint now = convert_to_local_time(time(NULL));
		if (scheduled_recording.get_end_time() > now)
		{
			Data::Row row;
			row.auto_increment = &(scheduled_recording.scheduled_recording_id);
			row["scheduled_recording_id"].int_value	= scheduled_recording.scheduled_recording_id;
			row["channel_id"].int_value				= scheduled_recording.channel_id;
			row["description"].string_value			= scheduled_recording.description;
			row["start_time"].int_value				= scheduled_recording.start_time;
			row["duration"].int_value				= scheduled_recording.duration;
			row["device"].string_value				= scheduled_recording.device;
					
			data_table.rows.add(row);
		}
	
		g_debug("Scheduled recording '%s' (%d) saved", scheduled_recording.description.c_str(), scheduled_recording.scheduled_recording_id);
	}
	
	Data::TableAdapter adapter(connection, table);
	adapter.replace_rows(data_table);
	
	g_debug("Deleting old scheduled recordings");
	Glib::ustring clause = Glib::ustring::compose("(start_time + duration) < %1", time(NULL));
	adapter.delete_rows(clause);

	g_debug("Scheduled recordings saved");
}

void ScheduledRecordingManager::add_scheduled_recording(ScheduledRecording& scheduled_recording)
{
	Glib::RecMutex::Lock lock(mutex);
	
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& current = *i;
		if (current.channel_id != scheduled_recording.channel_id && scheduled_recording.overlaps(current))
		{
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'."),
				current.description);
			throw Exception(message);
		}

		if (current.channel_id == scheduled_recording.channel_id && current.start_time == scheduled_recording.start_time)
		{
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because you have already have a scheduled recording called '%1' which starts at the same time on the same channel."),
				current.description);
			throw Exception(message);
		}
	}	
	
	scheduled_recordings.push_back(scheduled_recording);

	// Have to save to update the scheduled recording ID
	Application& application = get_application();
	Data::Connection connection(application.get_database_filename());
	save(connection);

	application.check_scheduled_recordings();
}

void ScheduledRecordingManager::remove_scheduled_recording(guint scheduled_recording_id)
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("Deleting scheduled recording %d", scheduled_recording_id);
	
	gboolean found = false;
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end() && !found; i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording_id == scheduled_recording.scheduled_recording_id)
		{
			g_debug("Deleting scheduled recording '%s' (%d)",
				scheduled_recording.description.c_str(),
				scheduled_recording.scheduled_recording_id);
			scheduled_recordings.erase(i);

			Data::Connection connection(get_application().get_database_filename());
			Data::Table table = get_application().get_schema().tables["scheduled_recording"];
			Data::TableAdapter adapter(connection, table);
			adapter.delete_row(scheduled_recording_id);
			
			found = true;
		}
	}	
	g_debug("Scheduled recording deleted");

	get_application().check_scheduled_recordings();
}

guint ScheduledRecordingManager::check_scheduled_recordings()
{
	g_debug("Checking scheduled recordings");
	guint active_scheduled_recording_id = NULL;
	Glib::RecMutex::Lock lock(mutex);

	Application& application = get_application();

	guint now = convert_to_local_time(time(NULL));
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.get_end_time() < now)
		{
			scheduled_recordings.erase(i);
		}
	}
	
	gboolean got_recording = false;

	if (!scheduled_recordings.empty())
	{
		guint now = time(NULL);
		g_debug("Now: %d", now);
		g_debug("=============================================================================================");
		g_debug("#ID | Start Time | Duration | Record | Channel    | Device                      | Description");
		g_debug("=============================================================================================");
		for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
		{			
			ScheduledRecording& scheduled_recording = *i;
				
			gboolean record = scheduled_recording.is_in(now);
			g_debug("%3d | %d | %8d | %s | %10s | %27s | %s",
				scheduled_recording.scheduled_recording_id,
				scheduled_recording.start_time,
				scheduled_recording.duration,
				record ? "true  " : "false ",
				application.channel_manager.get_channel(scheduled_recording.channel_id).name.c_str(),
				scheduled_recording.device.c_str(),
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
					active_scheduled_recording_id = scheduled_recording.scheduled_recording_id;
				}
			}
		}
	}
		
	return active_scheduled_recording_id;
}

ScheduledRecording ScheduledRecordingManager::get_scheduled_recording(guint scheduled_recording_id)
{
	Glib::RecMutex::Lock lock(mutex);

	ScheduledRecording* result = NULL;
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end() && result == NULL; i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.scheduled_recording_id == scheduled_recording_id)
		{
			result = &scheduled_recording;			
		}
	}
	
	if (result == NULL)
	{
		Glib::ustring message = Glib::ustring::compose(
			_("Scheduled recording '%1' not found"), scheduled_recording_id);
		throw Exception(message);
	}
	
	return *result;
}