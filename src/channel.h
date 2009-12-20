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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "epg_events.h"
#include "dvb_transponder.h"
#include <linux/dvb/frontend.h>

class Channel
{
public:
	Channel();

	guint				channel_id;
	Glib::ustring		name;
	guint				type;
	guint				sort_order;
	Glib::ustring		mrl;
	EpgEvents			epg_events;
	
	guint				service_id;
	Dvb::Transponder	transponder;
	
	guint				get_transponder_frequency();
	Glib::ustring		get_text();

	bool operator==(const Channel& channel) const;
	bool operator!=(const Channel& channel) const;
};

class ChannelArray : public std::vector<Channel>
{
public:
	gboolean contains(guint channel_id);
};

#endif
