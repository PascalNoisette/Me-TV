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

#include "profile_manager.h"
#include "me-tv.h"

bool channel_sort_by_index(const Channel& a, const Channel& b)
{
	return a.index < b.index;
}

bool channel_sort_by_name(const Channel& a, const Channel& b)
{
	return a.name < b.name;
}

ProfileManager::ProfileManager()
{
	current_profile = NULL;
	load();
}

ProfileManager::~ProfileManager()
{
	save();
}

void ProfileManager::load()
{
	current_profile = new Profile();
	current_profile->name = "Default";
}

Profile* ProfileManager::find_profile(const Glib::ustring& profile_name)
{
	Profile* result = NULL;
	ProfileList::iterator iterator = profiles.begin();	
	while (iterator != profiles.end() && result == NULL)
	{
		Profile& profile = *iterator;
		if (profile_name == profile.name)
		{
			result = &profile;
		}
		iterator++;
	}	
	return result;
}

Profile& ProfileManager::get_profile(const Glib::ustring& profile_name)
{
	Profile* profile = find_profile(profile_name);
	if (profile == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find profile '%1'"), profile_name));
	}
	return *profile;
}

void ProfileManager::save()
{
}

Profile& ProfileManager::get_current_profile()
{
	if (current_profile == NULL)
	{
		throw Exception(_("There is no current profile"));
	}
	return *current_profile;
}

Channel::Channel()
{
	flags = 0;
	service_id = 0;
	memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
}

Channel* Profile::find_channel(const Glib::ustring& name)
{
	Channel* channel = NULL;

	ChannelList::iterator iterator = channels.begin();
	while (iterator != channels.end() && channel == NULL)
	{
		Channel* current_channel = &(*iterator);
		if (current_channel->name == name)
		{
			channel = current_channel;
		}
		
		iterator++;
	}

	return channel;
}

Channel& Profile::get_channel(const Glib::ustring& name)
{
	Channel* channel = find_channel(name);

	if (channel == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Channel '%1' not found"), name));
	}

	return *channel;
}

void Profile::set_display_channel(const Glib::ustring& channel_name)
{
	set_display_channel(get_channel(channel_name));
}

void Profile::set_display_channel(Channel& channel)
{
	display_channel = channel;
	signal_display_channel_changed(display_channel);
}

void Profile::add_channels(ChannelList& c)
{
	ChannelList::iterator iterator = c.begin();
	while (iterator != c.end())
	{
		Channel& channel = *iterator;
		add_channel(channel);
		iterator++;
	}
}

void Profile::add_channel(Channel& channel)
{
	Channel* c = find_channel(channel.name);

	if (c != NULL)
	{
		g_debug("Channel '%s' already exists", channel.name.c_str());
	}
	else
	{
		g_debug("Channel '%s' added", channel.name.c_str());
		channels.push_back(channel);
	}
}
	
const ChannelList& Profile::get_channels() const
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

Channel* Profile::get_channel(guint frequency, guint service_id)
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
