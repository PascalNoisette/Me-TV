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
#include "data.h"

ScheduledRecordingManager::ScheduledRecordingManager()
{
	g_static_rec_mutex_init(mutex.gobj());
}

void ScheduledRecordingManager::load()
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("Loading scheduled recordings");

	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::TableAdapter adapter(get_application().connection, table);
	Data::DataTable data_table = adapter.select_rows("", "start_time");
	
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

		scheduled_recordings[scheduled_recording.scheduled_recording_id] = scheduled_recording;
	}
	g_debug("Scheduled recordings loaded");
}

void ScheduledRecordingManager::save()
{
	Glib::RecMutex::Lock lock(mutex);

	g_debug("Saving scheduled recordings");

	Data::Table table = get_application().get_schema().tables["scheduled_recording"];	
	Data::DataTable data_table(table);
	for (ScheduledRecordingMap::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = scheduled_recordings[i->first];

		Data::Row row;
		row.auto_increment = &(scheduled_recording.scheduled_recording_id);
		row["scheduled_recording_id"].int_value	= scheduled_recording.scheduled_recording_id;
		row["channel_id"].int_value				= scheduled_recording.channel_id;
		row["description"].string_value			= scheduled_recording.description;
		row["start_time"].int_value				= scheduled_recording.start_time;
		row["duration"].int_value				= scheduled_recording.duration;
				
		data_table.rows.add(row);
	}
	
	Data::TableAdapter adapter(get_application().connection, table);
	adapter.replace_rows(data_table);
	
	g_debug("Scheduled recordings saved");
}

void ScheduledRecordingManager::add_scheduled_recording(ScheduledRecording& scheduled_recording)
{
	Glib::RecMutex::Lock lock(mutex);
	
	for (ScheduledRecordingMap::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& current = scheduled_recordings[i->first];
		if (current.channel_id != scheduled_recording.channel_id && scheduled_recording.overlaps(current))
		{
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'"),
				current.description);
			throw Exception(message);
		}
	}	
	
	Data::Table table = get_application().get_schema().tables["scheduled_recording"];	
	Data::DataTable data_table(table);
	Data::Row row;
	row.auto_increment = &(scheduled_recording.scheduled_recording_id);
	row["scheduled_recording_id"].int_value	= scheduled_recording.scheduled_recording_id;
	row["channel_id"].int_value				= scheduled_recording.channel_id;
	row["description"].string_value			= scheduled_recording.description;
	row["start_time"].int_value				= scheduled_recording.start_time;
	row["duration"].int_value				= scheduled_recording.duration;
	data_table.rows.add(row);

	Data::TableAdapter adapter(get_application().connection, table);
	adapter.replace_rows(data_table);
	
	scheduled_recordings[scheduled_recording.scheduled_recording_id] = scheduled_recording;
	g_debug("Scheduled recording '%s' (%d)", scheduled_recording.description.c_str(), scheduled_recording.scheduled_recording_id);
}

void ScheduledRecordingManager::delete_scheduled_recording(guint scheduled_recording_id)
{
	Glib::RecMutex::Lock lock(mutex);

	ScheduledRecording& scheduled_recording = scheduled_recordings[scheduled_recording_id];
	g_debug("Deleteing scheduled recording '%s' (%d)", scheduled_recording.description.c_str(), scheduled_recording.scheduled_recording_id);

	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::TableAdapter adapter(get_application().connection, table);
	adapter.delete_row(scheduled_recording_id);
	scheduled_recordings.erase(scheduled_recording_id);

	g_debug("Scheduled recording deleted");
}
