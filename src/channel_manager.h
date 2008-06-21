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

#define CHANNEL_FLAG_NONE		0
#define CHANNEL_FLAG_DVB		1

class Channel
{
public:
	int index;
	guint flags;
	Glib::ustring name;
	Glib::ustring pre_command;
	Glib::ustring post_command;
	Glib::ustring mrl;
	guint service_id;
	struct dvb_frontend_parameters frontend_parameters;

	Channel();
};

typedef std::list<Channel> ChannelList;

bool channel_sort_by_index(const Channel& a, const Channel& b);
bool channel_sort_by_name(const Channel& a, const Channel& b);

class ChannelManager
{
private:
	ChannelList channels;
	Channel display_channel;
	Channel* find_channel(const Glib::ustring& channel_name);

public:
	Channel& get_channel(const Glib::ustring& name);
	void set_display_channel(const Glib::ustring& channel_name);
	void set_display_channel(Channel& channel);
	void add_channel(Channel& channel);
	void add_channels(ChannelList& channels);
	void clear();
	const ChannelList& get_channels() const;
	const Channel& get_display_channel() const;
	sigc::signal<void, Channel&> signal_display_channel_changed;
};


#endif
