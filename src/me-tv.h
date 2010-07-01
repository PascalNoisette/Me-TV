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

#ifndef __ME_TV_H__
#define __ME_TV_H__

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#include <list>
#include <vector>
#include <gtkmm.h>

extern bool				verbose_logging;
extern bool				safe_mode;
extern bool				minimised_mode;
extern bool				disable_epg_thread;
extern bool				disable_epg;
extern bool				no_screensaver_inhibit;
extern Glib::ustring	devices;
extern gint				read_timeout;

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
Glib::ustring encode_xml(const Glib::ustring& s);
Glib::ustring trim_string(const Glib::ustring& s);

guint get_local_time();
Glib::ustring get_local_time_text(const gchar* format);
Glib::ustring get_local_time_text(time_t t, const gchar* format);
guint convert_to_local_time(guint utc);
guint convert_to_utc_time(guint local_time);

void on_error();
void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

void split_string(std::vector<Glib::ustring>& parts, const Glib::ustring& text, const char* delimiter, gboolean remove_empty, gsize max_length);

extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_record;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

extern Glib::RefPtr<Gtk::Action> action_about;
extern Glib::RefPtr<Gtk::Action> action_auto_record;
extern Glib::RefPtr<Gtk::Action> action_channels;
extern Glib::RefPtr<Gtk::Action> action_change_view_mode;
extern Glib::RefPtr<Gtk::Action> action_decrease_volume;
extern Glib::RefPtr<Gtk::Action> action_epg_event_search;
extern Glib::RefPtr<Gtk::Action> action_increase_volume;
extern Glib::RefPtr<Gtk::Action> action_preferences;
extern Glib::RefPtr<Gtk::Action> action_present;
extern Glib::RefPtr<Gtk::Action> action_quit;
extern Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

extern sigc::signal<void, guint> signal_channel_change;
extern sigc::signal<void> signal_update;

#endif
