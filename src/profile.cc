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

#include "profile.h"
#include "exception.h"
#include "data.h"
#include "me-tv-i18n.h"

Profile::Profile()
{
	g_debug("Profile constructor");
	profile_id = 0;
	display_channel = NULL;
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
	g_message("Setting display channel to '%s' (%d)", channel.name.c_str(), channel.channel_id);
	display_channel = find_channel(channel.channel_id);
	if (display_channel == NULL)
	{
		throw Exception(_("Failed to set display channel: channel not found"));
	}
	else
	{
		signal_display_channel_changed(*display_channel);
	}
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
	if (channels.size() >= MAX_CHANNELS)
	{
		Glib::ustring message = Glib::ustring::compose(ngettext(
			"Failed to add channel: You cannot have more than %1 channel",
			"Failed to add channel: You cannot have more than %1 channels",
			MAX_CHANNELS), MAX_CHANNELS);
		throw Exception(message);
	}
	channels.push_back(channel);
	g_debug("Channel '%s' added to profile", channel.name.c_str());
}

const ChannelList& Profile::get_channels() const
{
	return channels;
}

ChannelList& Profile::get_channels()
{
	return channels;
}

Channel* Profile::get_display_channel()
{
	return display_channel;
}

void Profile::clear()
{
	channels.clear();
	g_debug("'%s' channels cleared", name.c_str());
}

Channel* Profile::find_channel(guint frequency, guint service_id)
{
	Channel* result = NULL;
	
	for (ChannelList::iterator iterator = channels.begin(); iterator != channels.end() && result == NULL; iterator++)
	{
		Channel& channel = *iterator;
		if (channel.get_transponder_frequency() == frequency && channel.service_id == service_id)
		{
			result = &channel;
		}
	}
	
	return result;
}

void Profile::set_channels(ChannelList& new_channels)
{
	g_debug("Setting channels");
	
	guint display_channel_frequency = 0;
	guint display_channel_service_id = 0;

	if (display_channel != NULL)
	{
		display_channel_frequency = display_channel->get_transponder_frequency();
		display_channel_service_id = display_channel->service_id;
		display_channel = NULL; // Will be destroyed by clear, so flag it
	}
	
	clear();
	add_channels(new_channels);

	if (display_channel_frequency != 0)
	{
		Channel* channel = find_channel(display_channel_frequency, display_channel_service_id);
		if (channel == NULL && channels.size() > 0)
		{
			channel = &(*(channels.begin()));
		}
		
		if (channel != NULL)
		{
			set_display_channel(*channel);
		}
	}
	g_debug("Finished setting channels");
}

void Profile::next_channel()
{
	if (channels.size() == 0)
	{
		throw Exception(_("No channels"));
	}
	else if (display_channel == NULL)
	{
		set_display_channel(*(channels.begin()));
	}
	else
	{
		gboolean done = false;
		
		ChannelList::iterator iterator = channels.begin();
		while (iterator != channels.end() && !done)
		{
			Channel& channel = *iterator;
			if (channel.get_transponder_frequency() == display_channel->get_transponder_frequency() && channel.service_id == display_channel->service_id)
			{
				iterator++;
				if (iterator != channels.end())
				{
					set_display_channel(*iterator);
				}
				done = true;
			}
			else
			{
				iterator++;
			}			
		}
	}
}

void Profile::previous_channel()
{
	if (channels.size() == 0)
	{
		throw Exception(_("No channels"));
	}
	else if (display_channel == NULL)
	{
		ChannelList::iterator iterator = channels.end();
		iterator--;
		set_display_channel(*iterator);
	}
	else
	{
		gboolean done = false;
		
		ChannelList::iterator iterator = channels.begin();
		ChannelList::iterator previous_iterator = iterator;
		while (iterator != channels.end() && !done)
		{
			Channel& channel = *iterator;
			if (channel.get_transponder_frequency() == display_channel->get_transponder_frequency() && channel.service_id == display_channel->service_id)
			{
				if (previous_iterator != iterator)
				{
					set_display_channel(*previous_iterator);
				}
				done = true;
			}
			else
			{
				previous_iterator = iterator;
				iterator++;
			}
		}
	}
}
