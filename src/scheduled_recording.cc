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

#include "scheduled_recording.h"
#include "me-tv.h"
#include "me-tv-i18n.h"

ScheduledRecording::ScheduledRecording()
{
	scheduled_recording_id	= 0;
	type					= 0;
	channel_id				= 0;
	start_time				= 0;
	duration				= 0;
}

Glib::ustring ScheduledRecording::get_start_time_text() const
{
	return get_local_time_text(start_time, "%c");
}

Glib::ustring ScheduledRecording::get_duration_text() const
{
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0)
	{
		result = Glib::ustring::compose(ngettext("1 hour","%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0)
	{
		result += ", ";
	}
	if (minutes > 0)
	{
		result += Glib::ustring::compose(ngettext("1 minute","%1 minutes", minutes), minutes);
	}
	
	return result;
}

guint ScheduledRecording::get_end_time() const
{
	return start_time + duration;
}

Glib::ustring ScheduledRecording::get_end_time_text() const
{
	return get_local_time_text(get_end_time(), "%c");
}

gboolean ScheduledRecording::is_in(guint at) const
{
	return start_time <= at && start_time+duration > at;
}

gboolean ScheduledRecording::overlaps(const ScheduledRecording& scheduled_recording) const
{
	return is_in(scheduled_recording.start_time) ||
		is_in(scheduled_recording.get_end_time()) ||
		scheduled_recording.is_in(start_time);
}
