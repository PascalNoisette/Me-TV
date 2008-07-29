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

#include "me-tv.h"

void replace_text(Glib::ustring& text, const Glib::ustring& from, const Glib::ustring& to)
{
	Glib::ustring::size_type position = 0;
	while ((position = text.find(from, position)) != Glib::ustring::npos)
	{
		text.replace(position, from.size(), to);
		position += to.size();
	}
}

Glib::ustring get_time_string(time_t t, const gchar* format)
{
	struct tm tp;
	char buffer[1000];

	localtime_r(&t, &tp);
	strftime(buffer, 1000, format, &tp);
	
	return buffer;
}

Glib::ustring encode_xml(const Glib::ustring& s)
{
	Glib::ustring result = s;
	
	replace_text(result, "&", "&amp;");
	replace_text(result, "<", "&lt;");
	replace_text(result, ">", "&gt;");

	return result;
}
