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

#ifndef __ME_TV_H__
#define __ME_TV_H__

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#include <list>
#include <vector>
#include <glibmm.h>

extern bool verbose_logging;
extern bool safe_mode;
extern bool minimised_mode;

typedef sigc::signal<void, const Glib::ustring&> StringSignal;

StringSignal& get_signal_error();

typedef std::vector<Glib::ustring> StringArray;
typedef std::list<Glib::ustring> StringList;

struct StringTable
{
	const char*	text;
	guint		value;
};

guint convert_string_to_value(const StringTable* table, const Glib::ustring& text);
Glib::ustring convert_value_to_string(const StringTable* table, guint value);

void replace_text(Glib::ustring& text, const Glib::ustring& from, const Glib::ustring& to);
Glib::ustring get_local_time_text(const gchar* format);
Glib::ustring get_local_time_text(time_t t, const gchar* format);
Glib::ustring encode_xml(const Glib::ustring& s);

guint convert_to_local_time(guint utc);
guint convert_to_utc_time(guint local_time);

void on_error(const Glib::ustring& message);
void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

#endif
