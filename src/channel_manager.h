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

#ifndef __CHANNEL_MANGER_H__
#define __CHANNEL_MANGER_H__

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <dvb_frontend.h>
#include "exception.h"

#define CHANNEL_FLAG_DVB		1

class Channel
{
public:
	guint flags;
	Glib::ustring name;
	Glib::ustring pre_command;
	Glib::ustring post_command;
	Glib::ustring mrl;
	guint service_id;
	struct dvb_frontend_parameters frontend_parameters;

	Channel()
	{
		flags = 0;
		service_id = 0;
		memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
	}
};

typedef std::list<Channel> ChannelList;

class ChannelManager
{
private:
	ChannelList channels;
	Channel active_channel;

	Channel* find_channel(const Glib::ustring& name)
	{
		gboolean found = false;
		Channel* channel = NULL;

		ChannelList::iterator iterator = channels.begin();
		while (iterator != channels.end() && !found)
		{
			channel = &(*iterator);
			if (channel->name == name)
			{
				found = true;
			}
		}

		return channel;
	}

public:
	Channel& get_channel(const Glib::ustring& name)
	{
		Channel* channel = find_channel(name);

		if (channel == NULL)
		{
			throw Exception(Glib::ustring::format(_("Channel '%s' not found"), name));
		}

		return *channel;
	}

	void set_active(Channel& channel)
	{
		active_channel = channel;
		signal_active_channel_changed(channel);
	}

	void add_channel(Channel& channel)
	{
		Channel* c = find_channel(channel.name);
		
		if (c != NULL)
		{
			throw Exception(_("Cannot add channel '%s', it already exists"));
		}

		channels.push_back(channel);
	}

	sigc::signal<void, Channel&> signal_active_channel_changed;
};


#endif
