/*
 * Copyright (C) 2008 Michael Lamothe
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

#include "epg_event.h"
#include "me-tv.h"
#include "application.h"

Glib::ustring EpgEvent::get_title() const
{
	Glib::ustring result;
	const Glib::ustring& preferred_language = get_application().get_preferred_language();
	
	for (EpgEventTextList::const_iterator i = texts.begin(); i != texts.end(); i++)
	{
		EpgEventText text = *i;
		if (result.size() == 0 || (preferred_language.size() > 0 && preferred_language == text.language))
		{
			result = text.title;
		}
	}
	
	if (result.size() == 0)
	{
		result = UNKNOWN_TEXT;
	}
	
	return result;		
}
	
Glib::ustring EpgEvent::get_description() const
{
	Glib::ustring result;
	const Glib::ustring& preferred_language = get_application().get_preferred_language();
	
	for (EpgEventTextList::const_iterator i = texts.begin(); i != texts.end(); i++)
	{
		EpgEventText text = *i;
		if (result.size() == 0 || (preferred_language.size() > 0 && preferred_language == text.language))
		{
			result = text.description;
		}
	}
	
	if (result.size() == 0)
	{
		result = UNKNOWN_TEXT;
	}
	
	return result;
}

Glib::ustring EpgEvent::get_start_time_text() const
{
	return get_local_time_text(
		convert_to_utc_time(start_time), "%c");
}

Glib::ustring EpgEvent::get_duration_text() const
{
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0)
	{
		result = Glib::ustring::compose(ngettext("1 hour", "%1 hours", hours), hours);
	}
	if (hours > 0 && minutes > 0)
	{
		result += ", ";
	}
	if (minutes > 0)
	{
		result += Glib::ustring::compose(ngettext("1 minute", "%1 minutes", minutes), minutes);
	}
	
	return result;
}
