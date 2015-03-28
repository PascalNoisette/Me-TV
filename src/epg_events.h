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

#ifndef __EPG_EVENTS_H__
#define __EPG_EVENTS_H__

#include "data.h"
#include "epg_event.h"

typedef std::list<EpgEvent> EpgEventList;

class EpgEvents {
private:
	EpgEventList list;
	Glib::Threads::RecMutex mutex;
	gboolean dirty;
	void set_saved(guint epg_event_id);
	void swap(EpgEvents &);
public:
	EpgEvents();
	~EpgEvents();
	EpgEvents(EpgEvents const &);
	EpgEvents & operator=(EpgEvents const &);
	gboolean add_epg_event(EpgEvent const & epg_event);
	gboolean get_current(EpgEvent & epg_event);
	EpgEventList	get_list(time_t start_time, time_t end_time);
	void prune();
	void load(Data::Connection & connection, guint channel_id);
	void save(Data::Connection & connection, guint channel_id);
	EpgEventList	search(Glib::ustring const & text, gboolean search_description);
	gboolean is_dirty() const { return dirty; }
};

#endif
