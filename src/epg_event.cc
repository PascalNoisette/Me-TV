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

Glib::ustring EpgEvent::get_title(const Glib::ustring& language) const
{
	Glib::ustring result;

	for (EpgEventTextList::const_iterator i = texts.begin(); i != texts.end(); i++)
	{
		EpgEventText text = *i;
		if (result.size() == 0 || (language.size() > 0 && language == text.language))
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
	
Glib::ustring EpgEvent::get_description(const Glib::ustring& language) const
{
	Glib::ustring result;
	
	for (EpgEventTextList::const_iterator i = texts.begin(); i != texts.end(); i++)
	{
		EpgEventText text = *i;
		if (result.size() == 0 || (language.size() > 0 && language == text.language))
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
	return get_time_text(convert_to_local_time(convert_to_local_time(start_time)), "%c");
}

Glib::ustring EpgEvent::get_duration_text() const
{
	Glib::ustring result;
	guint hours = duration / (60*60);
	guint minutes = (duration % (60*60)) / 60;
	if (hours > 0)
	{
		result = Glib::ustring::compose("%1 hours", hours);
	}
	if (hours > 0 && minutes > 0)
	{
		result += ", ";
	}
	if (minutes > 0)
	{
		result += Glib::ustring::compose("%1 minutes", minutes);
	}
	
	return result;
}