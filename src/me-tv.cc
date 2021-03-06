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

#include "me-tv.h"
#include "me-tv-i18n.h"
#include "exception.h"
#include <glib/gprintf.h>
#include "application.h"

bool verbose_logging = false;
bool safe_mode = false;
bool minimised_mode = false;
bool disable_epg_thread = false;
bool disable_epg = false;
bool no_screensaver_inhibit = false;
Glib::ustring	devices;
gint read_timeout = 5;
Glib::ustring engine = "xine";

Glib::ustring preferred_language;

ChannelManager channel_manager;
ScheduledRecordingManager scheduled_recording_manager;
DeviceManager device_manager;
StreamManager stream_manager;
ConfigurationManager configuration_manager;
WebManager web_manager;

Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

Glib::RefPtr<Gtk::Action> action_about;
Glib::RefPtr<Gtk::Action> action_auto_record;
Glib::RefPtr<Gtk::Action> action_channels;
Glib::RefPtr<Gtk::Action> action_change_view_mode;
Glib::RefPtr<Gtk::Action> action_decrease_volume;
Glib::RefPtr<Gtk::Action> action_epg_event_search;
Glib::RefPtr<Gtk::Action> action_increase_volume;
Glib::RefPtr<Gtk::Action> action_preferences;
Glib::RefPtr<Gtk::Action> action_present;
Glib::RefPtr<Gtk::Action> action_quit;
Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

sigc::signal<void, guint> signal_start_display;
sigc::signal<void> signal_stop_display;
sigc::signal<void> signal_update;
sigc::signal<void, Glib::ustring> signal_error;

void replace_text(Glib::ustring & text, Glib::ustring const & from, Glib::ustring const & to) {
	Glib::ustring::size_type position = 0;
	while ((position = text.find(from, position)) != Glib::ustring::npos) {
		text.replace(position, from.size(), to);
		position += to.size();
	}
}

time_t get_local_time() {
	return convert_to_local_time(time(NULL));
}

Glib::ustring get_local_time_text(gchar const * format) {
	return get_local_time_text(time(NULL), format);
}

Glib::ustring get_local_time_text(time_t t, gchar const * format) {
	tm tp;
	char buffer[100];
	if (localtime_r(&t, &tp) == NULL) { throw Exception(_("Failed to get time")); }
	strftime(buffer, 100, format, &tp);
	return buffer;
}

Glib::ustring encode_xml(Glib::ustring const & s) {
	Glib::ustring result = s;
	replace_text(result, "&", "&amp;");
	replace_text(result, "<", "&lt;");
	replace_text(result, ">", "&gt;");
	replace_text(result, "'", "&apos;");
	replace_text(result, "\"", "&quot;");
	return result;
}

guint convert_to_local_time(guint utc) {
	return utc + timezone;
}

guint convert_to_utc_time(guint local_time) {
	return local_time - timezone;
}

void log_handler(gchar const * log_domain, GLogLevelFlags log_level, gchar const * message, gpointer user_data) {
	if (log_level != G_LOG_LEVEL_DEBUG || verbose_logging) {
		Glib::ustring time_text = get_local_time_text("%x %T");
		g_printf("%s: %s\n", time_text.c_str(), message);
	}
}

void on_error() {
	try { throw; }
	catch (Exception const & exception) { signal_error(exception.what()); }
	catch (Glib::Error const & exception) { signal_error(exception.what()); }
	catch (...) {
		g_message("Unhandled exception");
		signal_error("Unhandled exception");
	}
}

guint convert_string_to_value(StringTable const * table, Glib::ustring const & text) {
	gboolean found = false;
	StringTable const * current = table;
	while (current->text != NULL && !found) {
		if (text == current->text) { found = true; }
		else { ++current; }
	}
	if (!found) {
		throw Exception(Glib::ustring::compose(_("Failed to find a value for '%1'"), text));
	}
	return (guint)current->value;
}

Glib::ustring convert_value_to_string(StringTable const * table, guint value) {
	gboolean found = false;
	StringTable const * current = table;
	while (current->text != NULL && !found) {
		if (value == current->value) { found = true; }
		else { ++current; }
	}
	if (!found) {
		throw Exception(Glib::ustring::compose(_("Failed to find a text value for '%1'"), value));
	}
	return current->text;
}

void split_string(std::vector<Glib::ustring> & parts, Glib::ustring const & text, char const * delimiter, gboolean remove_empty, gsize max_length) {
	gchar ** temp_parts = g_strsplit_set(text.c_str(), delimiter, max_length);
	gchar ** iterator = temp_parts;
	guint count = 0;
	while (*iterator != NULL) {
		gchar* part = *iterator++;
		if (part[0] != 0) {
			++count;
			parts.push_back(part);
		}
	}
	g_strfreev(temp_parts);
}

Glib::ustring trim_string(Glib::ustring const & s) {
	Glib::ustring result;
	glong length = s.bytes();
	if (length > 0) {
		gchar buffer[length + 1];
		s.copy(buffer, length);
		buffer[length] = 0;
		result = g_strstrip(buffer);
	}
	return result;
}
