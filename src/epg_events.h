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

#ifndef __EPG_EVENTS_H__
#define __EPG_EVENTS_H__

#include "epg_event.h"
#include "data.h"

typedef std::list<EpgEvent> EpgEventList;

class EpgEvents
{
private:
	EpgEventList			list;
	Glib::StaticRecMutex	mutex;
	
	void set_saved(guint epg_event_id);
public:
	EpgEvents();
	~EpgEvents();
		
	gboolean		add_epg_event(const EpgEvent& epg_event);
	gboolean		get_current(EpgEvent& epg_event);
	EpgEvent		get_epg_event(guint epg_event_id);
	EpgEventList	get_list(guint start_time, guint end_time);
	void			prune();
	void			load(Data::Connection& connection, guint channel_id);
	void			save(Data::Connection& connection, guint channel_id);
	EpgEventList	search(const Glib::ustring& text, gboolean search_description);
};

#endif
