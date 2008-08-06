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

#ifndef __PROFILE_H__
#define __PROFILE_H__

#include <glibmm.h>
#include "channel.h"

class Profile
{
private:
	ChannelList channels;
	Channel* display_channel;

public:
	Profile();
		
	void set_display_channel(const Channel& channel);
	void set_display_channel(guint channel_id);
	void add_channel(Channel& channel);
	void add_channels(ChannelList& channels);
	void clear();
	ChannelList& get_channels();
	const ChannelList& get_channels() const;
	const Channel* get_display_channel() const;
	sigc::signal<void, const Channel&> signal_display_channel_changed;
	Channel& get_channel(guint channel_id);
	Channel* find_channel(guint frequency, guint service_id);
	Channel* find_channel(guint channel_id);

	guint			profile_id;
	Glib::ustring	name;
};

typedef std::list<Profile> ProfileList;

#endif
