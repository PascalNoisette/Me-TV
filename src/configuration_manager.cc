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

#include "configuration_manager.h"

#define GCONF_PATH "/apps/me-tv"

void ConfigurationManager::initialise() {
	gconf_client = Gnome::Conf::Client::get_default_client();
	if (get_int_value("epg_span_hours") == 0) { set_int_value("epg_span_hours", 3); }
	if (get_int_value("epg_page_size") == 0) { set_int_value("epg_page_size", 20); }
}

Glib::ustring ConfigurationManager::get_path(Glib::ustring const & key) {
	return Glib::ustring::compose(GCONF_PATH"/%1", key);
}

void ConfigurationManager::set_string_default(Glib::ustring const & key, Glib::ustring const & value) {
	Glib::ustring path = get_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID) {
		g_debug("Setting string configuration value '%s' = '%s'", key.c_str(), value.c_str());
		gconf_client->set(path, value);
	}
}

void ConfigurationManager::set_int_default(Glib::ustring const & key, gint value) {
	Glib::ustring path = get_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID) {
		g_debug("Setting int configuration value '%s' = '%d'", path.c_str(), value);
		gconf_client->set(path, value);
	}
}

void ConfigurationManager::set_boolean_default(Glib::ustring const & key, gboolean value) {
	Glib::ustring path = get_path(key);
	Gnome::Conf::Value v = gconf_client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID) {
		g_debug("Setting int configuration value '%s' = '%s'", path.c_str(), value ? "true" : "false");
		gconf_client->set(path, (bool)value);
	}
}

StringList ConfigurationManager::get_string_list_value(Glib::ustring const & key) {
	return gconf_client->get_string_list(get_path(key));
}

Glib::ustring ConfigurationManager::get_string_value(Glib::ustring const & key) {
	return gconf_client->get_string(get_path(key));
}

gint ConfigurationManager::get_int_value(Glib::ustring const & key) {
	return gconf_client->get_int(get_path(key));
}

gint ConfigurationManager::get_boolean_value(Glib::ustring const & key) {
	return gconf_client->get_bool(get_path(key));
}

void ConfigurationManager::set_string_list_value(Glib::ustring const & key, StringList const & value) {
	gconf_client->set_string_list(get_path(key), value);
}

void ConfigurationManager::set_string_value(Glib::ustring const & key, Glib::ustring const & value) {
	gconf_client->set(get_path(key), value);
}

void ConfigurationManager::set_int_value(Glib::ustring const & key, gint value) {
	gconf_client->set(get_path(key), (gint)value);
}

void ConfigurationManager::set_boolean_value(Glib::ustring const & key, gboolean value) {
	gconf_client->set(get_path(key), (bool)value);
}
