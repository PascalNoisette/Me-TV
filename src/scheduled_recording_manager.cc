/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scheduled_recording_manager.h"
#include "application.h"

void ScheduledRecordingManager::initialise() { }

void ScheduledRecordingManager::load(Data::Connection & connection) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	g_debug("Loading scheduled recordings");
	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::TableAdapter adapter(connection, table);
	Glib::ustring where = Glib::ustring::compose("((start_time + duration) > %1 OR recurring_type != 0)", time(NULL));
	Data::DataTable data_table = adapter.select_rows(where, "start_time");
	dirty = false;
	scheduled_recordings.clear();
	for (auto && row: data_table.rows) {
		ScheduledRecording scheduled_recording;
		scheduled_recording.scheduled_recording_id = row["scheduled_recording_id"].int_value;
		scheduled_recording.channel_id = row["channel_id"].int_value;
		scheduled_recording.description = row["description"].string_value;
		scheduled_recording.recurring_type = row["recurring_type"].int_value;
		scheduled_recording.action_after = row["action_after"].int_value;
		scheduled_recording.start_time = row["start_time"].int_value;
		scheduled_recording.duration = row["duration"].int_value;
		scheduled_recording.device = row["device"].string_value;
		scheduled_recordings.push_back(scheduled_recording);
		guint now = time(NULL);
		if (scheduled_recording.start_time + scheduled_recording.duration < now) { dirty = true; }
	}
	g_debug("Scheduled recordings loaded");
}

void ScheduledRecordingManager::save(Data::Connection & connection) {
	if (!dirty) {
		g_debug("Scheduled recordings are not dirty, not saving");
		return;
	}
	g_debug("Scheduled recordings are dirty, saving");
	Glib::Threads::RecMutex::Lock lock(mutex);
	g_debug("Saving %d scheduled recordings", (int)scheduled_recordings.size());
	Data::Table table = get_application().get_schema().tables["scheduled_recording"];
	Data::DataTable data_table(table);
	for (auto && scheduled_recording: scheduled_recordings) {
		time_t now = time(NULL);
		if (scheduled_recording.get_end_time() > now || scheduled_recording.recurring_type != 0) {
			Data::Row row;
			row.auto_increment = &(scheduled_recording.scheduled_recording_id);
			row["scheduled_recording_id"].int_value = scheduled_recording.scheduled_recording_id;
			row["channel_id"].int_value = scheduled_recording.channel_id;
			row["description"].string_value = scheduled_recording.description;
			row["recurring_type"].int_value = scheduled_recording.recurring_type;
			row["action_after"].int_value = scheduled_recording.action_after;
			row["start_time"].int_value = scheduled_recording.start_time;
			row["duration"].int_value = scheduled_recording.duration;
			row["device"].string_value = scheduled_recording.device;
			data_table.rows.add(row);
		}
		g_debug("Scheduled recording '%s' (%d) saved", scheduled_recording.description.c_str(), scheduled_recording.scheduled_recording_id);
	}
	Data::TableAdapter adapter(connection, table);
	adapter.replace_rows(data_table);
	guint now = time(NULL);
	g_debug("Deleting old scheduled recordings ending before %d", now);
	Glib::ustring where = Glib::ustring::compose("recurring_type != %1 AND (start_time + duration) < %2", SCHEDULED_RECORDING_RECURRING_TYPE_ONCE, now);
	data_table = adapter.select_rows(where, "start_time");
	gboolean updated = false;
	for (auto && row: data_table.rows) {
		g_debug("ScheduledRecordingManager::save/clear ID: %d", row["scheduled_recording_id"].int_value);
		if (row["recurring_type"].int_value == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYDAY) {
			row["start_time"].int_value += 86400;
		}
		else if (row["recurring_type"].int_value == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEK) {
			row["start_time"].int_value += 604800;
		}
		else if (row["recurring_type"].int_value == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEKDAY) {
			time_t tim = row["start_time"].int_value;
			tm *ts;
			char buf[80];
			ts = localtime(&tim);
			strftime(buf, sizeof(buf), "%w", ts);
			switch (atoi(buf)) {
				case 5: row["start_time"].int_value += 259200; break;
				case 6: row["start_time"].int_value += 172800; break;
				default: row["start_time"].int_value +=  86400; break;
			}
		}
		updated = true;
	}
	if (updated) {
		adapter.replace_rows(data_table);
		load(connection);
	}
	Glib::ustring clause = Glib::ustring::compose("(start_time + duration) < %1 AND recurring_type = %2", now, SCHEDULED_RECORDING_RECURRING_TYPE_ONCE);
	adapter.delete_rows(clause);
	dirty = false;
	g_debug("Scheduled recordings saved");
}

void ScheduledRecordingManager::set_scheduled_recording(EpgEvent & epg_event) {
	ScheduledRecording scheduled_recording;
	Application & application = get_application();
	guint before = configuration_manager.get_int_value("record_extra_before");
	guint after = configuration_manager.get_int_value("record_extra_after");
	guint now = get_local_time();
	scheduled_recording.channel_id = epg_event.channel_id;
	scheduled_recording.description = epg_event.get_title();
	scheduled_recording.recurring_type = SCHEDULED_RECORDING_RECURRING_TYPE_ONCE;
	scheduled_recording.action_after = SCHEDULED_RECORDING_ACTION_AFTER_NONE;
	scheduled_recording.start_time = convert_to_utc_time(epg_event.start_time - (before * 60));
 	scheduled_recording.duration = epg_event.duration + ((before + after) * 60);
	scheduled_recording.device = "";
	set_scheduled_recording(scheduled_recording);
}

gboolean ScheduledRecordingManager::is_device_available(const Glib::ustring & device, ScheduledRecording const & scheduled_recording) {
	Channel & channel = channel_manager.get_channel_by_id(scheduled_recording.channel_id);
	for (auto const & current: scheduled_recordings) {
		Channel & current_channel = channel_manager.get_channel_by_id(current.channel_id);
		if (
			channel.transponder != current_channel.transponder &&
			scheduled_recording.overlaps(current) &&
			device == current.device
				) {
			g_debug("Frontend '%s' is busy recording '%s'", device.c_str(), current.description.c_str());
			return false;
		}
	}
	g_debug("Found available frontend '%s'", device.c_str());
	return true;
}

void ScheduledRecordingManager::select_device(ScheduledRecording & scheduled_recording) {
	g_debug("Looking for an available device for scheduled recording");
	Channel & channel = channel_manager.get_channel_by_id(scheduled_recording.channel_id);
	FrontendList & frontends = device_manager.get_frontends();
	for (auto const device: frontends) {
		const Glib::ustring& device_path = device->get_path();
		if (device->get_frontend_type() != channel.transponder.frontend_type) {
			g_debug("Device %s is the wrong type", device_path.c_str());
		}
		else {
			if (is_device_available(device_path, scheduled_recording)) {
				scheduled_recording.device = device_path;
				if (stream_manager.has_display_stream()) {
					Channel& display_channel = stream_manager.get_display_channel();
					if (channel.transponder == display_channel.transponder) { return; }
				}
				else { return; }
				g_debug("Display channel is on a different transponder for '%s', looking for something better", device_path.c_str());
			}
		}
	}
}

void ScheduledRecordingManager::set_scheduled_recording(ScheduledRecording & scheduled_recording) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	ScheduledRecordingList::iterator iupdated;
	gboolean updated  = false;
	gboolean is_same  = false;
	gboolean conflict = false;
	g_debug("Setting scheduled recording");
	Channel & channel = channel_manager.get_channel_by_id(scheduled_recording.channel_id);
	if (scheduled_recording.device.empty()) {
		select_device(scheduled_recording);
		if (scheduled_recording.device.empty()) {
			Glib::ustring message = Glib::ustring::compose(_(
				"Failed to set scheduled recording for '%1' at %2: There are no devices available at that time"),
				scheduled_recording.description, scheduled_recording.get_start_time_text());
			throw Exception(message);
		}
		g_debug("Device selected: '%s'", scheduled_recording.device.c_str());
	}
	for (auto i = scheduled_recordings.begin(); i != scheduled_recordings.end(); ++i) {
		auto const & current = *i;
		Channel & current_channel = channel_manager.get_channel_by_id(current.channel_id);
		// Check for conflict
		if (current.scheduled_recording_id != scheduled_recording.scheduled_recording_id &&
		    current_channel.transponder != channel.transponder &&
		    scheduled_recording.device == current.device &&
		    scheduled_recording.overlaps(current)) {
			conflict = true;
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'."),
				current.description);
			throw Exception(message);
		}
		// Check if its an existing scheduled recording
		if (scheduled_recording.scheduled_recording_id != 0 &&
		    scheduled_recording.scheduled_recording_id == current.scheduled_recording_id) {
			g_debug("Updating scheduled recording");
			updated = true;
			iupdated = i;
		}
		// Check if we are scheduling the same program
		if (scheduled_recording.scheduled_recording_id == 0 &&
		    current.channel_id  == scheduled_recording.channel_id &&
		    current.start_time  == scheduled_recording.start_time &&
		    current.duration  == scheduled_recording.duration) {
			Glib::ustring message =  Glib::ustring::compose(
				_("Failed to save scheduled recording because you have already have a scheduled recording called '%1' which is scheduled for the same time on the same channel."),
				current.description);
			throw Exception(message);
		}
		if (current.scheduled_recording_id == scheduled_recording.scheduled_recording_id &&
			current.recurring_type == scheduled_recording.recurring_type &&
			current.action_after == scheduled_recording.action_after &&
			current.channel_id  == scheduled_recording.channel_id &&
			current.start_time  == scheduled_recording.start_time &&
			current.duration  == scheduled_recording.duration) {
			is_same = true;
		}
	}
	// If there is an update an not conflict on scheduled recording, update it.
	if (updated && !conflict && !is_same) {
		ScheduledRecording & current = *iupdated;
		current.device = scheduled_recording.device;
		current.recurring_type = scheduled_recording.recurring_type;
		current.action_after = scheduled_recording.action_after;
		current.description = scheduled_recording.description;
		current.channel_id = scheduled_recording.channel_id;
		current.start_time = scheduled_recording.start_time;
		current.duration = scheduled_recording.duration;
		dirty = true;
	}
	// If the scheduled recording is new then add it
	if (scheduled_recording.scheduled_recording_id == 0) {
		g_debug("Adding scheduled recording");
		scheduled_recordings.push_back(scheduled_recording);
		dirty = true;
	}
	// Have to save to update the scheduled recording ID
	Application & application = get_application();
	Data::Connection connection(application.get_database_filename());
	save(connection);
	application.check_scheduled_recordings();
}

void ScheduledRecordingManager::remove_scheduled_recording(guint scheduled_recording_id) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	g_debug("Deleting scheduled recording %d", scheduled_recording_id);
	gboolean found = false;
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end() && !found; i++) {
		ScheduledRecording& scheduled_recording = *i;
		if (scheduled_recording_id == scheduled_recording.scheduled_recording_id) {
			g_debug("Deleting scheduled recording '%s' (%d)",
				scheduled_recording.description.c_str(),
				scheduled_recording.scheduled_recording_id);
			scheduled_recordings.erase(i);
			Data::Connection connection(get_application().get_database_filename());
			Data::Table table = get_application().get_schema().tables["scheduled_recording"];
			Data::TableAdapter adapter(connection, table);
			adapter.delete_row(scheduled_recording_id);
			found = true;
			dirty = true;
		}
	}
	g_debug("Scheduled recording deleted");
	get_application().check_scheduled_recordings();
}

void ScheduledRecordingManager::remove_scheduled_recording(EpgEvent & epg_event) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	for (auto const & scheduled_recording: scheduled_recordings) {
		if (scheduled_recording.channel_id == epg_event.channel_id &&
			scheduled_recording.is_in(
			    convert_to_utc_time(epg_event.start_time),
			    convert_to_utc_time(epg_event.get_end_time()))) {
			remove_scheduled_recording(scheduled_recording.scheduled_recording_id);
			return;
		}
	}
}

ScheduledRecordingList ScheduledRecordingManager::check_scheduled_recordings() {
	ScheduledRecordingList results;
	time_t now = time(NULL);
	g_debug("Checking scheduled recordings");
	Glib::Threads::RecMutex::Lock lock(mutex);
	g_debug("Now: %u", (guint)now);
	g_debug("Removing scheduled recordings older than %u", (guint)now);
	// TODO: Could do better than this…
	ScheduledRecordingList::iterator i = scheduled_recordings.begin();
	while (i != scheduled_recordings.end()) {
		if ((*i).is_old(now)) {
			guint action = (*i).action_after;
			remove_scheduled_recording((*i).scheduled_recording_id);
			action_after(action);
			i = scheduled_recordings.begin();
		}
		else {
			i++;
		}
	}
	if (!scheduled_recordings.empty()) {
		g_debug("=============================================================================================");
		g_debug("#ID | Start Time | Duration | Record | Channel    | Device                      | Description");
		g_debug("=============================================================================================");
		for (auto const & scheduled_recording: scheduled_recordings) {
			gboolean record = scheduled_recording.is_in(now);
			g_debug("%3d | %u | %8d | %s | %10s | %27s | %s",
				scheduled_recording.scheduled_recording_id,
				(guint)scheduled_recording.start_time,
				scheduled_recording.duration,
				record ? "true  " : "false ",
				channel_manager.get_channel_by_id(scheduled_recording.channel_id).name.c_str(),
				scheduled_recording.device.c_str(),
				scheduled_recording.description.c_str());
			if (record) {
				results.push_back(scheduled_recording);
			}
			if (scheduled_recording.get_end_time() < now) {
				dirty = true;
			}
		}
	}
	return results;
}

ScheduledRecording ScheduledRecordingManager::get_scheduled_recording(guint scheduled_recording_id) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	ScheduledRecording* result = NULL;
	for (auto & scheduled_recording: scheduled_recordings) {
		if (scheduled_recording.scheduled_recording_id == scheduled_recording_id) {
			result = &scheduled_recording;
			break;
		}
	}
	if (result == NULL) {
		Glib::ustring message = Glib::ustring::compose(
			_("Scheduled recording '%1' not found"), scheduled_recording_id);
		throw Exception(message);
	}
	return *result;
}

guint ScheduledRecordingManager::is_recording(Channel const & channel) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	guint now = time(NULL);
	for (auto const & scheduled_recording: scheduled_recordings) {
		if (scheduled_recording.is_in(now) && scheduled_recording.channel_id == channel.channel_id) {
			return scheduled_recording.scheduled_recording_id;
		}
	}
	return 0;
}

gboolean ScheduledRecordingManager::is_recording(EpgEvent const & epg_event) {
	Glib::Threads::RecMutex::Lock lock(mutex);
	for (auto const & scheduled_recording: scheduled_recordings) {
		if (scheduled_recording.channel_id == epg_event.channel_id &&
			scheduled_recording.is_in(
			    convert_to_utc_time(epg_event.start_time),
			    convert_to_utc_time(epg_event.get_end_time()))) {
			return true;
		}
	}
	return false;
}

void ScheduledRecordingManager::action_after(guint action) {
	if (action == SCHEDULED_RECORDING_ACTION_AFTER_CLOSE) {
		g_message("Me TV closed by Scheduled Recording");
		action_quit->activate();
	}
	else if (action == SCHEDULED_RECORDING_ACTION_AFTER_SHUTDOWN) {
		DBusGConnection* dbus_connection = get_application().get_dbus_connection();
		if (dbus_connection == NULL) {
			throw Exception(_("DBus connection not available"));
		}
		g_message("Computer shutdown by scheduled recording");
		DBusGProxy* proxy = dbus_g_proxy_new_for_name(dbus_connection,
			"org.gnome.SessionManager",
			"/org/gnome/SessionManager",
			"org.gnome.SessionManager");
		if (proxy == NULL) {
			throw Exception(_("Failed to get org.gnome.SessionManager proxy"));
		}
		GError* error = NULL;
		if (!dbus_g_proxy_call(proxy, "Shutdown", &error, G_TYPE_INVALID, G_TYPE_INVALID)) {
			throw Exception(_("Failed to call Shutdown method"));
		}
		g_message("Shutdown requested");
	}
}
