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

#include "scheduled_recording_manager.h"
#include "application.h"

ScheduledRecordingManager::ScheduledRecordingManager()
{
	g_static_rec_mutex_init(mutex.gobj());
}

void ScheduledRecordingManager::load(Data::Connection& connection)
{
	Glib::RecMutex::Lock lock(mutex);
	
	g_debug("Loading scheduled recordings");

	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::TableAdapter adapter(connection, table);
	
	Glib::ustring where = Glib::ustring::compose(
		"((start_time + duration) > %1 OR recurring_type != 0)", time(NULL));
	Data::DataTable data_table = adapter.select_rows(where, "start_time");
	
	scheduled_recordings.clear();
	for (Data::Rows::iterator i = data_table.rows.begin(); i != data_table.rows.end(); i++)
	{
		ScheduledRecording scheduled_recording;
		Data::Row& row = *i;
		
		scheduled_recording.scheduled_recording_id	= row["scheduled_recording_id"].int_value;
		scheduled_recording.channel_id				= row["channel_id"].int_value;
		scheduled_recording.description				= row["description"].string_value;
		scheduled_recording.recurring_type			= row["recurring_type"].int_value;
		scheduled_recording.action_after			= row["action_after"].int_value;
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

	g_debug("Saving %d scheduled recordings", (int)scheduled_recordings.size());
	
	Data::Table table = get_application().get_schema().tables["scheduled_recording"];	
	Data::DataTable data_table(table);
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		guint now = time(NULL);
		if (scheduled_recording.get_end_time() > now || scheduled_recording.recurring_type != 0)
		{
			Data::Row row;
			row.auto_increment 						= &(scheduled_recording.scheduled_recording_id);
			row["scheduled_recording_id"].int_value	= scheduled_recording.scheduled_recording_id;
			row["channel_id"].int_value				= scheduled_recording.channel_id;
			row["description"].string_value			= scheduled_recording.description;
			row["recurring_type"].int_value			= scheduled_recording.recurring_type;
			row["action_after"].int_value			= scheduled_recording.action_after;
			row["start_time"].int_value				= scheduled_recording.start_time;
			row["duration"].int_value				= scheduled_recording.duration;
			row["device"].string_value				= scheduled_recording.device;

			data_table.rows.add(row);
		}

		g_debug("Scheduled recording '%s' (%d) saved", scheduled_recording.description.c_str(), scheduled_recording.scheduled_recording_id);
	}
	
	Data::TableAdapter adapter(connection, table);
	adapter.replace_rows(data_table);

	guint now = time(NULL);
	g_debug("Deleting old scheduled recordings ending before %d", now);
	Glib::ustring where = Glib::ustring::compose("recurring_type !=0 AND (start_time + duration) < %1", now);
	data_table = adapter.select_rows(where, "start_time");

	gboolean updated = false;
	for (Data::Rows::iterator i = data_table.rows.begin(); i != data_table.rows.end(); i++)
	{
		Data::Row& row = *i;
		g_message("ScheduledRecordingManager::save/clear ID: %d", row["scheduled_recording_id"].int_value); 

		if(row["recurring_type"].int_value == 1)
			row["start_time"].int_value += 86400;
		if(row["recurring_type"].int_value == 2)
			row["start_time"].int_value += 604800;
		if(row["recurring_type"].int_value == 3)
		{
			time_t tim = row["start_time"].int_value;
			struct tm *ts;
			char buf[80];
			ts = localtime(&tim);
			strftime(buf, sizeof(buf), "%w", ts);
			switch(atoi(buf))
			{
				case 5 : row["start_time"].int_value += 259200;break;
				case 6 : row["start_time"].int_value += 172800;break;
				default: row["start_time"].int_value +=  86400;break;
			}
		}  
		updated = true;
	}
	if(updated)
	{
		adapter.replace_rows(data_table);
		load(connection);
	}
	Glib::ustring clause = Glib::ustring::compose("(start_time + duration) < %1 AND recurring_type = 0", now);
	adapter.delete_rows(clause);

	g_debug("Scheduled recordings saved");
}

void ScheduledRecordingManager::set_scheduled_recording(ScheduledRecording& scheduled_recording)
{
	Glib::RecMutex::Lock lock(mutex);
	ScheduledRecordingList::iterator iupdated;
	gboolean updated  = false;
	gboolean is_same  = false;
	gboolean conflict = false;

	g_debug("Setting scheduled recording");
	
	Channel& channel = get_application().channel_manager.get_channel_by_id(scheduled_recording.channel_id);

	if (scheduled_recording.device.empty())
	{
		g_debug("Looking for an available frontend for scheduled recording");
		FrontendList& frontends = get_application().device_manager.get_frontends();
		for (FrontendList::iterator j = frontends.begin(); j != frontends.end(); j++)
		{
			Glib::ustring device = (*j)->get_path();
			gboolean device_conflict = false;

			for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end() && !found; i++)
			{
				ScheduledRecording& current = *i;

				Channel& current_channel = get_application().channel_manager.get_channel_by_id(current.channel_id);

				// Check for conflict
				if (current.scheduled_recording_id != scheduled_recording.scheduled_recording_id &&
					scheduled_recording.overlaps(current) &&
					device == current.device)
				{
					g_debug("'%s' is busy at that time", device.c_str());
					device_conflict = true;
				}
			}

			if (!device_conflict)
			{
				g_debug("Found available frontend '%s'", device.c_str());
				scheduled_recording.device = device;
			}
		}
	}
		
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& current = *i;

		Channel& current_channel = get_application().channel_manager.get_channel_by_id(current.channel_id);

		// Check for conflict
		if (current.scheduled_recording_id != scheduled_recording.scheduled_recording_id &&
		    current_channel.transponder != channel.transponder &&
		    scheduled_recording.device == current.device &&
		    scheduled_recording.overlaps(current))
		{
			conflict = true;
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'."),
				current.description);
			throw Exception(message);
		}

		// Check if its an existing scheduled recording
		if (scheduled_recording.scheduled_recording_id != 0 &&
		    scheduled_recording.scheduled_recording_id == current.scheduled_recording_id)
		{
			g_debug("Updating scheduled recording");
			updated = true;
			iupdated = i;
		}

		// Check if we are scheduling the same program
		if (scheduled_recording.scheduled_recording_id == 0 &&
		    current.channel_id 		== scheduled_recording.channel_id &&
		    current.start_time 		== scheduled_recording.start_time &&
		    current.duration 		== scheduled_recording.duration)
		{
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because you have already have a scheduled recording called '%1' which is scheduled for the same time on the same channel."),
				current.description);
			throw Exception(message);
		}

		if (current.scheduled_recording_id == scheduled_recording.scheduled_recording_id &&
			current.recurring_type	== scheduled_recording.recurring_type &&
			current.action_after	== scheduled_recording.action_after &&
			current.channel_id 		== scheduled_recording.channel_id &&
			current.start_time 		== scheduled_recording.start_time &&
			current.duration 		== scheduled_recording.duration)
		{
			is_same = true;
		}
	}
	
	// If there is an update an not conflict on scheduled recording, update it.
	if (updated && !conflict && !is_same)
	{
		ScheduledRecording& current = *iupdated;

		current.device = scheduled_recording.device;
		current.recurring_type = scheduled_recording.recurring_type;
		current.action_after = scheduled_recording.action_after;
		current.description = scheduled_recording.description;
		current.channel_id = scheduled_recording.channel_id;
		current.start_time = scheduled_recording.start_time;
		current.duration = scheduled_recording.duration;
	}
	
	// If the scheduled recording is new then add it
	if (scheduled_recording.scheduled_recording_id == 0)
	{
		g_debug("Adding scheduled recording");
		scheduled_recordings.push_back(scheduled_recording);
	}

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

guint scheduled_recording_now = 0;

guint is_old(ScheduledRecording& scheduled_recording)
{
	return (scheduled_recording.get_end_time() < scheduled_recording_now && scheduled_recording.recurring_type == 0);
}

ScheduledRecordingList ScheduledRecordingManager::check_scheduled_recordings()
{
	ScheduledRecordingList results;
	
	g_debug("Checking scheduled recordings");
	Glib::RecMutex::Lock lock(mutex);

	Application& application = get_application();

	scheduled_recording_now = time(NULL);
	g_debug("Removing scheduled recordings older than %d", scheduled_recording_now);
	scheduled_recordings.remove_if(is_old);

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
				application.channel_manager.get_channel_by_id(scheduled_recording.channel_id).name.c_str(),
				scheduled_recording.device.c_str(),
				scheduled_recording.description.c_str());
			
			if (record)
			{
				gboolean conflict = false;
				for (ScheduledRecordingList::iterator i = results.begin(); i != results.end(); i++)
				{
					ScheduledRecording& scheduled_recording_test = *i;

					Dvb::Transponder& t1 = get_application().channel_manager.get_channel_by_id(scheduled_recording_test.channel_id).transponder;
					Dvb::Transponder& t2 = get_application().channel_manager.get_channel_by_id(scheduled_recording.channel_id).transponder;

					if (t1 != t2)
					{
						conflict = true;
						g_debug("Conflict!");
						break;
					}
				}
				
				if (!conflict)
				{
					results.push_back(scheduled_recording);
				}
			}
		}
	}
		
	return results;
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

guint ScheduledRecordingManager::is_recording(const Channel& channel)
{
	Glib::RecMutex::Lock lock(mutex);

	guint now = time(NULL);
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{			
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.is_in(now) && scheduled_recording.channel_id == channel.channel_id)
			return -1;
		else if (scheduled_recording.is_in(now-60) && scheduled_recording.channel_id == channel.channel_id)
			return scheduled_recording.action_after;
	}
	return -2;
}

gboolean ScheduledRecordingManager::is_recording(const EpgEvent& epg_event)
{
	Glib::RecMutex::Lock lock(mutex);

	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording.channel_id == epg_event.channel_id &&
			scheduled_recording.is_in(
			    convert_to_utc_time(epg_event.start_time),
			    convert_to_utc_time(epg_event.get_end_time())))
		{
			return true;
		}
	}
	
	return false;
}
