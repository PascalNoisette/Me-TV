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

#ifndef __CHANNEL_MANAGER_H__
#define __CHANNEL_MANAGER_H__

#include "channel.h"
#include "data.h"

class ChannelManager {
private:
	Glib::Threads::RecMutex mutex;
	ChannelArray channels;
	gboolean dirty;

public:
	void initialise();
	void load(Data::Connection & connection);
	void save(Data::Connection & connection);
	void prune_epg();
	Glib::Threads::RecMutex & get_mutex() { return mutex; }
	ChannelArray const & get_channels() const;
	ChannelArray & get_channels();
	void add_channel(const Channel& channel);
	void add_channels(const ChannelArray& channels);
	void set_channels(const ChannelArray& channels);
	void clear();
	Channel & get_channel_by_id(guint channel_id);
	Channel & get_channel_by_index(guint number);
	Channel * find_channel(guint frequency, guint service_id);
	Channel * find_channel(guint channel_id);
	gboolean is_dirty();
};

#endif
