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

#ifndef __DATA_H__
#define __DATA_H__

#include "me-tv.h"
#include <sqlite3.h>

class EpgEvent
{
public:
	guint frequency;
	guint service_id;
	guint event_id;
	guint start_time;
	guint duration;
	Glib::ustring title;
	Glib::ustring description;
};

typedef std::list<EpgEvent> EpgEventList;

class Data
{
private:
	sqlite3* database;
		
	guint execute_non_query(const Glib::ustring& command);
	guint execute_query(const Glib::ustring& command);
	sqlite3_stmt* prepare(const Glib::ustring& command);
	void step(sqlite3_stmt* statement);

public:
	Data();
	~Data();
		
	EpgEventList get_epg_events(guint frequency, guint service_id, guint start_time, guint end_time);
	void insert_or_ignore_epg_event(guint frequency, guint service_id, guint event_id, guint start_time,
		guint duration, const Glib::ustring& title, const Glib::ustring& description);
};

#endif
