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
#include "exception.h"
#include <glib/gprintf.h>

gboolean verbose_logging;
StringSignal signal_error;

StringSignal& get_signal_error()
{
	return signal_error;
}

void replace_text(Glib::ustring& text, const Glib::ustring& from, const Glib::ustring& to)
{
	Glib::ustring::size_type position = 0;
	while ((position = text.find(from, position)) != Glib::ustring::npos)
	{
		text.replace(position, from.size(), to);
		position += to.size();
	}
}

Glib::ustring get_time_text(time_t t, const gchar* format)
{
	struct tm tp;
	char buffer[100];

	if (localtime_r(&t, &tp) == NULL)
	{
		throw Exception("Failed to get time");
	}
	strftime(buffer, 100, format, &tp);
	
	return buffer;
}

Glib::ustring encode_xml(const Glib::ustring& s)
{
	Glib::ustring result = s;
	
	replace_text(result, "&", "&amp;");
	replace_text(result, "<", "&lt;");
	replace_text(result, ">", "&gt;");
	replace_text(result, "'", "&quot;");

	return result;
}

guint get_local_time()
{
	return convert_to_local_time(time(NULL));
}

guint convert_to_local_time(guint gmt)
{
	return gmt - timezone;
}

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	if (log_level != G_LOG_LEVEL_DEBUG || verbose_logging)
	{
		Glib::ustring time_text = get_time_text(get_local_time(), "%x %T");
		g_printf("%s: %s\n", time_text.c_str(), message);
	}
}

void on_error(const Glib::ustring& message)
{
	g_message(message.c_str());
}
