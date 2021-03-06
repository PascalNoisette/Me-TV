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

#include "scheduled_recording.h"
#include "me-tv.h"
#include "me-tv-i18n.h"

ScheduledRecording::ScheduledRecording() {
	scheduled_recording_id	= 0;
	recurring_type = SCHEDULED_RECORDING_RECURRING_TYPE_ONCE;
	action_after = SCHEDULED_RECORDING_ACTION_AFTER_NONE;
	channel_id = 0;
	start_time = 0;
	duration = 0;
}

Glib::ustring ScheduledRecording::get_start_time_text() const {
	return get_local_time_text(start_time, "%c");
}

Glib::ustring ScheduledRecording::get_duration_text() const {
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0) {
		result = Glib::ustring::compose(ngettext("1 hour","%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0) {
		result += ", ";
	}
	if (minutes > 0) {
		result += Glib::ustring::compose(ngettext("1 minute","%1 minutes", minutes), minutes);
	}
	return result;
}

time_t ScheduledRecording::get_end_time() const {
	return start_time + duration;
}

Glib::ustring ScheduledRecording::get_end_time_text() const {
	return get_local_time_text(get_end_time(), "%c");
}

gboolean ScheduledRecording::is_old(time_t now) const {
	return recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_ONCE &&
		(time_t)(start_time + duration) < now;
}

gboolean ScheduledRecording::is_in(time_t at) const {
	return start_time <= at && (time_t)(start_time + duration) > at;
}

gboolean ScheduledRecording::is_in(time_t s, time_t e) const {
	return start_time < s && get_end_time() > e;
}

gboolean ScheduledRecording::overlaps(const ScheduledRecording& scheduled_recording) const {
	std::list<guint> ctime;
	std::list<guint> srtime;
	gboolean type_once_c  = false;
	gboolean type_once_sr = false;
	ctime.push_back(start_time);
	srtime.push_back(scheduled_recording.start_time);
	// Creating the list long enough to overlaps (front of one bigger than the back of the other)
	while ((ctime.back()<srtime.front()+604800 && !type_once_c) || (srtime.back()<ctime.front()+604800 && !type_once_sr)) {
		// Create the list for the current scheduled_recording
		if (recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_ONCE) {
			type_once_c = true;
			ctime.push_back(ctime.back());
		}
		else if (recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYDAY) {
			ctime.push_back(ctime.back() + 86400);
		}
		else if (recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEK) {
			ctime.push_back(ctime.back() + 604800);
		}
		else if (recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEKDAY) {
			time_t tim = ctime.back();
			tm * ts {localtime(&tim)};
			char buf[80];
			strftime(buf, sizeof(buf), "%w", ts);
			switch(atoi(buf)) {
				case 5: ctime.push_back(ctime.back() + 259200); break;
				case 6: ctime.push_back(ctime.back() + 172800); break;
				default: ctime.push_back(ctime.back() + 86400); break;
			}
		}
		// Create the list for the tested scheduled_recording
		if (scheduled_recording.recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_ONCE) {
			type_once_sr = true;
			srtime.push_back(srtime.back());
		}
		else if (scheduled_recording.recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYDAY) {
			srtime.push_back(srtime.back() + 86400);
		}
		else if(scheduled_recording.recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEK) {
			srtime.push_back(srtime.back() + 604800);
		}
		else if(scheduled_recording.recurring_type == SCHEDULED_RECORDING_RECURRING_TYPE_EVERYWEEKDAY) {
			time_t tim = srtime.back();
			tm * ts {localtime(&tim)};
			char buf[80];
			strftime(buf, sizeof(buf), "%w", ts);
			switch(atoi(buf)) {
				case 5: srtime.push_back(srtime.back() + 259200); break;
				case 6: srtime.push_back(srtime.back() + 172800); break;
				default: srtime.push_back(srtime.back() + 86400); break;
			}
		}
	}
	// Now check for overlaps
	for (auto const i: ctime) {
		for (auto const j: srtime) {
			if (!(i+duration <= j) && !(i >= j + scheduled_recording.duration)) { return true; }
		}
	}
	return false;
}
