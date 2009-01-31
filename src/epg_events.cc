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

#include "epg_events.h"

EpgEvents::EpgEvents()
{
	g_static_rec_mutex_init(mutex.gobj());
}

EpgEvents::~EpgEvents()
{
}

gboolean EpgEvents::exists(const EpgEvent& epg_event)
{
	gboolean result = false;
	
	for (EpgEventList::const_iterator i = list.begin(); i != list.end() && result == false; i++)
	{
		if (epg_event.event_id == (*i).event_id)
		{
			result = true;
		}
	}
	
	return result;
}

gboolean EpgEvents::insert(const EpgEvent& epg_event)
{
	Glib::RecMutex::Lock lock(mutex);
	gboolean event_exists = exists(epg_event);
	if (!event_exists)
	{
		list.push_back(epg_event);
		g_debug("EPG Event %d (%s) added", epg_event.event_id, epg_event.get_title().c_str());
	}
	return !event_exists;
}

void EpgEvents::insert(const EpgEventList& epg_event_list)
{
	Glib::RecMutex::Lock lock(mutex);
	list = epg_event_list;
}

gboolean EpgEvents::get_current(EpgEvent& epg_event)
{
	EpgEvent result;
	gboolean found = false;
	guint now = time(NULL);
	
	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::iterator i = list.begin(); i != list.end() && found == false; i++)
	{
		EpgEvent& current = *i;
		if (epg_event.start_time <= now && epg_event.get_end_time() >= now)
		{
			found = true;
			epg_event = current;
		}
	}
	
	return found;
}

EpgEventList EpgEvents::get_list()
{
	EpgEventList result;
	Glib::RecMutex::Lock lock(mutex);

	for (EpgEventList::iterator i = list.begin(); i != list.end(); i++)
	{
		result.push_back(*i);
	}
	
	return result;
}
