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

#ifndef __DATA_H__
#define __DATA_H__

#include <sqlite3.h>
#include "me-tv.h"
#include "profile.h"
#include "epg_event.h"
#include "scheduled_recording.h"

class Statement
{
private:
	Glib::RecMutex::Lock	lock;
	sqlite3_stmt*			statement;
	sqlite3*				database;
	Glib::ustring			command;

public:
	Statement(sqlite3* database, const Glib::ustring& command);
	~Statement();

	gint step();
	gint get_int(guint column);
	const Glib::ustring get_text(guint column);
};

class Data
{
private:
	sqlite3*	database;
		
	guint execute_non_query(const Glib::ustring& command);
	guint execute_query(const Glib::ustring& command);
	sqlite3_stmt* prepare(const Glib::ustring& command);
	void step(sqlite3_stmt* statement);

	void load_epg_event(Statement& statement, EpgEvent& epg_event);
	void load_scheduled_recording(Statement& statement, ScheduledRecording& scheduled_recording);
	
public:
	Data(gboolean initialise = false);
	~Data();

	gboolean get_current_epg_event(const Channel& channel, EpgEvent& epg_event);
	EpgEventList get_epg_events(const Channel& channel, guint start_time, guint end_time);
	void replace_epg_event(EpgEvent& epg_event);
	void replace_epg_event_text(EpgEventText& epg_event_text);
	gint get_last_epg_update_time();
	
	ProfileList get_all_profiles();
	void replace_profile(Profile& profile);
	void replace_channel(Channel& channel);
	void delete_channel(guint channel_id);
		
	void replace_scheduled_recording(ScheduledRecording& scheduled_recording);
	ScheduledRecordingList get_scheduled_recordings();
	void delete_scheduled_recording(guint scheduled_recording_id);
	void delete_old_sceduled_recordings();
	gboolean get_scheduled_recording(guint scheduled_recording_id, ScheduledRecording& scheduled_recording);
};

#endif
