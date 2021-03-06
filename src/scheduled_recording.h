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

#ifndef __SCHEDULED_RECORDING_H__
#define __SCHEDULED_RECORDING_H__

#include <glibmm.h>
#include <list>

// TODO: replace these constants with an enum?

constexpr auto SCHEDULED_RECORDING_ACTION_AFTER_NONE = 0;
constexpr auto SCHEDULED_RECORDING_ACTION_AFTER_CLOSE = 1;
constexpr auto SCHEDULED_RECORDING_ACTION_AFTER_SHUTDOWN = 2;

constexpr auto SCHEDULED_RECORDING_RECURRING_TYPE_ONCE = 0;
constexpr auto SCHEDULED_RECORDING_RECURRING_TYPE_EVERYDAY = 1;
constexpr auto SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEK = 2;
constexpr auto SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEKDAY = 3;

class ScheduledRecording {
public:
	guint scheduled_recording_id;
	Glib::ustring	description;
	guint recurring_type;
	guint action_after;
	guint channel_id;
	time_t start_time;
	guint duration;
	Glib::ustring	device;
	ScheduledRecording();
	Glib::ustring get_start_time_text() const;
	Glib::ustring get_duration_text() const;
	time_t get_end_time() const;
	Glib::ustring get_end_time_text() const;
	gboolean is_old() const { return is_old(time_t(NULL)); }
	gboolean is_old(time_t now) const;
	gboolean is_in(time_t at) const;
	gboolean is_in(time_t start_time, time_t end_time) const;
	gboolean overlaps(ScheduledRecording const & scheduled_recording) const;
};

#endif
