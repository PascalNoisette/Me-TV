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

#include "me-tv.h"
#include "channel.h"
#include "data.h"
#include <string.h>

Channel::Channel() {
	channel_id	= 0;
	service_id	= 0;
}

Glib::ustring Channel::get_text() {
	Glib::ustring result = encode_xml(name);
	EpgEvent epg_event;
	if (epg_events.get_current(epg_event)) {
		result += " - ";
		result += epg_event.get_title();
	}
	return result;
}

guint Channel::get_transponder_frequency() {
	return transponder.frontend_parameters.frequency;
}

gboolean ChannelArray::contains(guint channel_id) {
	for (const_iterator i = begin(); i != end(); i++) {
		if ((*i).channel_id == channel_id) { return true; }
	}
	return false;
}
