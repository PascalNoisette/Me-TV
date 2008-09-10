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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#define CHANNEL_FLAG_NONE		0
#define CHANNEL_FLAG_DVB_T		0x01
#define CHANNEL_FLAG_DVB_C		0x10

#include <linux/dvb/frontend.h>
#include "epg_event.h"

class Channel
{
public:
	Channel();

	guint channel_id;
	guint profile_id;
	Glib::ustring name;
	guint flags;
	guint sort_order;
	Glib::ustring mrl;

	// DVB Specific
	guint service_id;
	struct dvb_frontend_parameters frontend_parameters;
		
	gboolean get_current_epg_event(EpgEvent& epg_event) const;
	Glib::ustring get_text() const;
};

typedef std::list<Channel> ChannelList;

#endif
