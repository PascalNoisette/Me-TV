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

#include "channel.h"
#include "data.h"
#include "me-tv.h"
#include <string.h>

Channel::Channel()
{
	channel_id = 0;
	profile_id = 0;
	flags = 0;
	service_id = 0;
	memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
}

gboolean Channel::get_current_epg_event(EpgEvent& epg_event) const
{
	gboolean found = false;
	guint now = convert_to_local_time(time(NULL));
	
	for (EpgEventList::const_iterator i = epg_events.begin(); i != epg_events.end() && found != true; i++)
	{
		const EpgEvent& e = *i;
		if (e.start_time <= now && e.get_end_time() >= now)
		{
			epg_event = e;
			found = true;
		}
	}

	return found;
}

Glib::ustring Channel::get_text() const
{
	Glib::ustring result = encode_xml(name);
	EpgEvent epg_event;
	
	if (get_current_epg_event(epg_event))
	{
		result += " - ";
		result += epg_event.get_title();
	}
	
	return result;
}

guint Channel::get_transponder_frequency()
{
	return frontend_parameters.frequency;
}

gboolean Channel::add_epg_event(EpgEvent& epg_event)
{
	gboolean found = false;
	
	EpgEventList::iterator i = epg_events.begin();
	EpgEventList::iterator at = i;
	for (; i != epg_events.end() && found != true; i++)
	{
		EpgEvent& e = *i;
		if (e.event_id >= epg_event.event_id)
		{
			found = true;
		}
		else
		{
			at = i;
		}
	}
	
	if (!found)
	{
		epg_events.insert(at, epg_event);
		g_debug("EPG Event %d (%s) added", epg_event.event_id, epg_event.get_title().c_str());
	}
	
	return !found;
}
