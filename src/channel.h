/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "epg_events.h"
#include "dvb_transponder.h"
#include <linux/dvb/frontend.h>

class Channel {
public:
	Channel();
	guint channel_id;
	Glib::ustring name;
	guint sort_order;
	Glib::ustring mrl;
	EpgEvents epg_events;
	guint service_id;
	Dvb::Transponder transponder;
	guint get_transponder_frequency();
	Glib::ustring get_text();
	bool operator==(const Channel& channel) const {
		return channel.service_id == service_id && channel.transponder == transponder;
	}
	bool operator!=(const Channel& channel) const { return !(*this == channel); }
};

class ChannelArray : public std::vector<Channel> {
public:
	gboolean contains(guint channel_id);
};

#endif
