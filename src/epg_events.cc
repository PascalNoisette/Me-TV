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
#include "me-tv.h"

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

bool sort_function(const EpgEvent& a, const EpgEvent& b)
{
	return a.epg_event_id < b.epg_event_id;
}

gboolean EpgEvents::insert(const EpgEvent& epg_event)
{
	Glib::RecMutex::Lock lock(mutex);
	gboolean event_exists = exists(epg_event);
	if (!event_exists)
	{
		list.push_back(epg_event);
		list.sort(sort_function);
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
	guint now = convert_to_local_time(time(NULL));
	
	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::iterator i = list.begin(); i != list.end() && found == false; i++)
	{
		EpgEvent& current = *i;
		if (current.start_time <= now && current.get_end_time() >= now)
		{
			found = true;
			epg_event = current;
		}
	}
	
	return found;
}

EpgEventList EpgEvents::get_list(gboolean update_save)
{
	EpgEventList result;
	Glib::RecMutex::Lock lock(mutex);

	for (EpgEventList::iterator i = list.begin(); i != list.end(); i++)
	{
		EpgEvent& epg_event = *i;
		
		result.push_back(epg_event);

		if (update_save)
		{
			epg_event.save = false;
		}
	}
	
	return result;
}

guint now = 0;

bool is_old(EpgEvent& epg_event)
{
	return epg_event.get_end_time() < now;
}

void EpgEvents::prune()
{
	now = convert_to_local_time(time(NULL));
	Glib::RecMutex::Lock lock(mutex);
	list.remove_if(is_old);
}
