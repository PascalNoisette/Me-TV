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

#include "profile.h"
#include "exception.h"
#include "data.h"

#include <glibmm/i18n.h>

Profile::Profile()
{
	profile_id = 0;
}

Channel* Profile::find_channel(guint channel_id)
{
	Channel* channel = NULL;

	ChannelList::iterator iterator = channels.begin();
	while (iterator != channels.end() && channel == NULL)
	{
		Channel* current_channel = &(*iterator);
		if (current_channel->channel_id == channel_id)
		{
			channel = current_channel;
		}
		
		iterator++;
	}

	return channel;
}

Channel& Profile::get_channel(guint channel_id)
{
	Channel* channel = find_channel(channel_id);

	if (channel == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Channel '%1' not found"), channel_id));
	}

	return *channel;
}

void Profile::set_display_channel(guint channel_id)
{
	set_display_channel(get_channel(channel_id));
}

void Profile::set_display_channel(const Channel& channel)
{
	g_debug("Setting display channel to '%s'", channel.name.c_str());
	display_channel = channel;
	signal_display_channel_changed(display_channel);
}

void Profile::add_channels(ChannelList& c)
{
	ChannelList::iterator iterator = c.begin();
	while (iterator != c.end())
	{
		Channel& channel = *iterator;
		channels.push_back(channel);
		iterator++;
	}
}

void Profile::add_channel(Channel& channel)
{
	channels.push_back(channel);
}
	
const ChannelList& Profile::get_channels() const
{
	return channels;
}

ChannelList& Profile::get_channels()
{
	return channels;
}

const Channel& Profile::get_display_channel() const
{
	return display_channel;
}

void Profile::clear()
{
	channels.clear();
}

Channel* Profile::find_channel(guint frequency, guint service_id)
{
	Channel* result = NULL;
	
	for (ChannelList::iterator iterator = channels.begin(); iterator != channels.end() && result == NULL; iterator++)
	{
		Channel& channel = *iterator;
		if (channel.frontend_parameters.frequency == frequency && channel.service_id == service_id)
		{
			result = &channel;
		}
	}
	
	return result;
}
