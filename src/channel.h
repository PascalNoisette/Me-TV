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

#define CHANNEL_FLAG_NONE		0
#define CHANNEL_FLAG_DVB_T		0x01
#define CHANNEL_FLAG_DVB_C		0x02
#define CHANNEL_FLAG_DVB_S		0x08
#define CHANNEL_FLAG_ATSC		0x04

#include <linux/dvb/frontend.h>
#include "epg_events.h"
#include "dvb_transponder.h"

class Channel
{
public:
	Channel();

	guint			channel_id;
	guint			profile_id;
	Glib::ustring	name;
	guint			flags;
	guint			sort_order;
	Glib::ustring	mrl;
	EpgEvents		epg_events;

	// DVB Specific
	guint				service_id;
	Dvb::Transponder	transponder;
	guint get_transponder_frequency();

	Glib::ustring get_text();
};

typedef std::list<Channel> ChannelList;

#endif
