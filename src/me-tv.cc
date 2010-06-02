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

#include "me-tv.h"
#include "me-tv-i18n.h"
#include "exception.h"
#include <glib/gprintf.h>
#include "application.h"

bool			verbose_logging		= false;
bool			safe_mode			= false;
bool			minimised_mode		= false;
bool			disable_epg_thread	= false;
bool			disable_epg			= false;
gint			read_timeout		= 5;

Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_record;

Glib::RefPtr<Gtk::Action> action_about;
Glib::RefPtr<Gtk::Action> action_channels;
Glib::RefPtr<Gtk::Action> action_change_view_mode;
Glib::RefPtr<Gtk::Action> action_epg_event_search;
Glib::RefPtr<Gtk::Action> action_preferences;
Glib::RefPtr<Gtk::Action> action_quit;
Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

void replace_text(Glib::ustring& text, const Glib::ustring& from, const Glib::ustring& to)
{
	Glib::ustring::size_type position = 0;
	while ((position = text.find(from, position)) != Glib::ustring::npos)
	{
		text.replace(position, from.size(), to);
		position += to.size();
	}
}

guint get_local_time()
{
	return convert_to_local_time(time(NULL));
}

Glib::ustring get_local_time_text(const gchar* format)
{
	return get_local_time_text(time(NULL), format);
}

Glib::ustring get_local_time_text(time_t t, const gchar* format)
{
	struct tm tp;
	char buffer[100];

	if (localtime_r(&t, &tp) == NULL)
	{
		throw Exception(_("Failed to get time"));
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
	replace_text(result, "'", "&apos;");
	replace_text(result, "\"", "&quot;");

	return result;
}

guint convert_to_local_time(guint utc)
{
	return utc + timezone;
}

guint convert_to_utc_time(guint local_time)
{
	return local_time - timezone;
}

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	if (log_level != G_LOG_LEVEL_DEBUG || verbose_logging)
	{
		Glib::ustring time_text = get_local_time_text("%x %T");
		g_printf("%s: %s\n", time_text.c_str(), message);
	}
}

void on_error()
{
	try
	{
		throw;
	}
	catch (const Exception& exception)
	{
		g_message("Exception: %s", exception.what().c_str());
	}
	catch (const Glib::Error& exception)
	{
		g_message("Exception: %s", exception.what().c_str());
	}
	catch (...)
	{
		g_message("Unhandled exception");
	}
}

guint convert_string_to_value(const StringTable* table, const Glib::ustring& text)
{
	gboolean found = false;
	const StringTable*	current = table;

	while (current->text != NULL && !found)
	{
		if (text == current->text)
		{
			found = true;
		}
		else
		{
			current++;
		}
	}
	
	if (!found)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find a value for '%1'"), text));
	}
	
	return (guint)current->value;
}

Glib::ustring convert_value_to_string(const StringTable* table, guint value)
{
	gboolean found = false;
	const StringTable*	current = table;

	while (current->text != NULL && !found)
	{
		if (value == current->value)
		{
			found = true;
		}
		else
		{
			current++;
		}
	}
	
	if (!found)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find a text value for '%1'"), value));
	}
	
	return current->text;
}

void split_string(std::vector<Glib::ustring>& parts, const Glib::ustring& text, const char* delimiter, gboolean remove_empty, gsize max_length)
{
	gchar** temp_parts = g_strsplit_set(text.c_str(), delimiter, max_length);	
	gchar** iterator = temp_parts;
	guint count = 0;
	while (*iterator != NULL)
	{
		gchar* part = *iterator++;
		if (part[0] != 0)
		{
			count++;
			parts.push_back(part);
		}
	}
	g_strfreev(temp_parts);
}
