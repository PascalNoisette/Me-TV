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

#define CHANNEL_FLAG_NONE		0
#define CHANNEL_FLAG_DVB_T		1

#include <linux/dvb/frontend.h>

class Channel
{
public:
	Channel()
	{
		channel_id = 0;
		profile_id = 0;
		flags = 0;
		service_id = 0;
		memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
	}

	guint channel_id;
	guint profile_id;
	Glib::ustring name;
	guint flags;
	guint sort_order;
	Glib::ustring mrl;

	// DVB Specific
	guint service_id;
	struct dvb_frontend_parameters frontend_parameters;
};

typedef std::list<Channel> ChannelList;
