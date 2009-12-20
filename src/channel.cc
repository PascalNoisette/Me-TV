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

#include "me-tv.h"
#include "channel.h"
#include "data.h"
#include <string.h>

Channel::Channel()
{
	channel_id		= 0;
	type			= 0;
	service_id		= 0;
}

Glib::ustring Channel::get_text()
{
	Glib::ustring result = encode_xml(name);
	EpgEvent epg_event;
	
	if (epg_events.get_current(epg_event))
	{
		result += " - ";
		result += epg_event.get_title();
	}
	
	return result;
}

guint Channel::get_transponder_frequency()
{
	return transponder.frontend_parameters.frequency;
}

gboolean ChannelArray::contains(guint channel_id)
{
	for (const_iterator i = begin(); i != end(); i++)
	{
		if ((*i).channel_id == channel_id)
		{
			return true;
		}
	}
	
	return false;
}

bool Channel::operator==(const Channel& channel) const
{
	return channel.service_id == service_id &&
		channel.transponder.frontend_parameters.frequency == transponder.frontend_parameters.frequency;
}

bool Channel::operator!=(const Channel& channel) const
{
	return !(*this == channel);
}
