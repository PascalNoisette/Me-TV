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

#include "epg_event.h"
#include "me-tv.h"
#include "application.h"

EpgEventText::EpgEventText() {
	epg_event_text_id = 0;
	epg_event_id = 0;
}

EpgEvent::EpgEvent() {
	epg_event_id = 0;
	channel_id = 0;
	event_id = 0;
	start_time = 0;
	duration = 0;
	save = true;
}

Glib::ustring EpgEvent::get_title() const {
	return get_default_text().title;
}

Glib::ustring EpgEvent::get_subtitle() const {
	return get_default_text().subtitle;
}

Glib::ustring EpgEvent::get_description() const {
	return get_default_text().description;
}

EpgEventText EpgEvent::get_default_text() const
{
	EpgEventText result;
	gboolean found = false;
	for (auto const text: texts) {
		if (!preferred_language.empty() && preferred_language == text.language) {
			found = true;
			result = text;
			break;
		}
	}

	if (!found) {
		if (!texts.empty()) {
			result = *(texts.begin());
		}
		else {
			result.title = _("Unknown title");
			result.subtitle = _("Unknown subtitle");
			result.description = _("Unknown description");
		}
	}
	return result;
}

Glib::ustring EpgEvent::get_start_time_text() const {
	return get_local_time_text(convert_to_utc_time(start_time), "%A, %d %B %Y, %H:%M");
}

Glib::ustring EpgEvent::get_duration_text() const {
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0) {
		result = Glib::ustring::compose(ngettext("1 hour", "%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0) {
		result += ", ";
	}
	if (minutes > 0) {
		result += Glib::ustring::compose(ngettext("1 minute", "%1 minutes", minutes), minutes);
	}
	return result;
}
