/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014 Russel Winder
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

extern bool verbose_logging;
extern bool safe_mode;
extern bool minimised_mode;
extern bool disable_epg_thread;
extern bool disable_epg;
extern bool no_screensaver_inhibit;
extern Glib::ustring	devices;
extern gint read_timeout;
extern Glib::ustring engine;

extern Glib::ustring	preferred_language;

typedef std::vector<Glib::ustring> StringArray;
typedef std::list<Glib::ustring> StringList;

// TODO: change this to use std::string ?
struct StringTable {
	char const * text;
	guint value;
};

guint convert_string_to_value(StringTable const * table, Glib::ustring const & text);
Glib::ustring convert_value_to_string(StringTable const * table, guint value);
void replace_text(Glib::ustring & text, Glib::ustring const & from, Glib::ustring const & to);
Glib::ustring encode_xml(Glib::ustring const & s);
Glib::ustring trim_string(Glib::ustring const & s);
time_t get_local_time();
Glib::ustring get_local_time_text(gchar const * format);
Glib::ustring get_local_time_text(time_t t, gchar const * format);
guint convert_to_local_time(guint utc);
guint convert_to_utc_time(guint local_time);
void on_error();
void log_handler(gchar const * log_domain, GLogLevelFlags log_level, gchar const * message, gpointer user_data);
void split_string(std::vector<Glib::ustring> & parts, Glib::ustring const & text, char const * delimiter, gboolean remove_empty, gsize max_length);

extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
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

extern sigc::signal<void, guint> signal_start_display;
extern sigc::signal<void> signal_stop_display;
extern sigc::signal<void> signal_update;
extern sigc::signal<void, Glib::ustring> signal_error;

#endif
