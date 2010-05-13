/*
 * Copyright (C) 2010 Michael Lamothe
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

#ifndef __SCHEDULED_RECORDING_H__
#define __SCHEDULED_RECORDING_H__

#include <glibmm.h>
#include <list>

#define SCHEDULED_RECORDING_ACTION_AFTER_NONE		0
#define SCHEDULED_RECORDING_ACTION_AFTER_CLOSE		1
#define SCHEDULED_RECORDING_ACTION_AFTER_SHUTDOWN	2

#define SCHEDULED_RECORDING_RECURRING_TYPE_ONCE			0
#define SCHEDULED_RECORDING_RECURRING_TYPE_EVERYDAY		1
#define SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEK	2
#define SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEKDAY	3

class ScheduledRecording
{
public:
	guint			scheduled_recording_id;
	Glib::ustring	description;
	guint			recurring_type;
	guint			action_after;
	guint			channel_id;
	guint			start_time;
	guint			duration;
	Glib::ustring	device;
		
	ScheduledRecording();
		
	Glib::ustring get_start_time_text() const;
	Glib::ustring get_duration_text() const;
	guint get_end_time() const;
	Glib::ustring get_end_time_text() const;
		
	gboolean is_in(guint at) const;
	gboolean is_in(guint start_time, guint end_time) const;
	gboolean overlaps(const ScheduledRecording& scheduled_recording) const;
};

#endif
