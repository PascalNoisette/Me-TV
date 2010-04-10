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

gboolean ScheduledRecording::is_in(guint s, guint e) const
{
	return start_time < s && get_end_time() > e;
}

gboolean ScheduledRecording::overlaps(const ScheduledRecording& scheduled_recording) const
{
	// Current recording is before the "scheduled_recording"
	if (start_time+duration < scheduled_recording.start_time)
		return false;
	// Current recording is overlaping the "scheduled_recording"
	else if (is_in(scheduled_recording.start_time) || is_in(scheduled_recording.get_end_time()))
		return true;
	// Current recording is after the "scheduled_recording" but "scheduled_recording" have no recurring
	else if (scheduled_recording.get_end_time() < start_time && scheduled_recording.type == 0)
		return false;
	// Current recording is after the "scheduled_recording" and there is a chance of conflict because of recurring
	else
	{
		guint tstartt 	= scheduled_recording.start_time;
		while(tstartt < start_time + duration)
		{
			if(scheduled_recording.type == 1)
				tstartt += 86400;
			else if(scheduled_recording.type == 2)
				tstartt += 604800;
			else if(scheduled_recording.type == 3)
			{
				time_t tim = tstartt;
				struct tm *ts;
				char buf[80];
				ts = localtime(&tim);
				strftime(buf, sizeof(buf), "%w", ts);
				switch(atoi(buf))
				{
					case 5 : tstartt += 259200;break;
					case 6 : tstartt += 172800;break;
					default: tstartt += 86400;break;
				}
			}  
			if (is_in(tstartt) || is_in(tstartt+duration))
				return true;
		}
		return false;
	}
}
