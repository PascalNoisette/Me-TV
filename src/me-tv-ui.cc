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

#include <gtkmm.h>
#include <gdk/gdk.h>
#include "application.h"
#include "me-tv-ui.h"

Glib::RefPtr<Gtk::UIManager> ui_manager;

ComboBoxText::ComboBoxText(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & xml)
	: Gtk::ComboBox(cobject) {
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_text);
	set_active(0);
}

void ComboBoxText::append_text(Glib::ustring const & text, Glib::ustring const & value) {
	Gtk::TreeModel::Row row = *list_store->append();
	row[columns.column_text] = text;
	row[columns.column_value] = value;
}

void ComboBoxText::set_active_text(Glib::ustring const & text) {
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); ++i) {
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_text] == text) { set_active(i); }
	}
}

void ComboBoxText::set_active_value(Glib::ustring const & value) {
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); ++i) {
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_value] == value) { set_active(i); }
	}
}

void ComboBoxText::clear_items() { list_store->clear(); }

Glib::ustring ComboBoxText::get_active_text() {
	Gtk::TreeModel::iterator i = get_active();
	if (i) {
		Gtk::TreeModel::Row row = *i;
		if (row) { return row[columns.column_text]; }
	}
	throw Exception(_("Failed to get active text value"));
}

Glib::ustring ComboBoxText::get_active_value() {
	Gtk::TreeModel::iterator i = get_active();
	if (i) {
		Gtk::TreeModel::Row row = *i;
		if (row) { return row[columns.column_value]; }
	}
	throw Exception(_("Failed to get active text value"));
}

ComboBoxEntryText::ComboBoxEntryText(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml)
	: Gtk::ComboBoxEntryText(cobject) {
}

ChannelComboBox::ChannelComboBox(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml)
	: Gtk::ComboBox(cobject) {
	list_store = Gtk::ListStore::create(columns);
}

void ChannelComboBox::load(ChannelArray const & channels) {
	clear();
	set_model(list_store);
	pack_start(columns.column_name);
	list_store->clear();
	for (auto && channel: channels) {
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_id] = channel.channel_id;
		row[columns.column_name] = channel.name;
	}
	set_active(0);
}

void ChannelComboBox::set_selected_channel_id(guint channel_id) {
	Gtk::TreeNodeChildren children = get_model()->children();
	for (auto i = children.begin(); i != children.end(); ++i) {
		auto const row = *i;
		if (row[columns.column_id] == channel_id) { set_active(i); }
	}
}

guint ChannelComboBox::get_selected_channel_id() {
	Gtk::TreeModel::Row row = *get_active();
	return row[columns.column_id];
}

IntComboBox::IntComboBox(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml)
	: Gtk::ComboBox(cobject) {
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_int);
	set_size(0);
	set_active(0);
}

void IntComboBox::set_size(guint size) {
	g_debug("Setting integer combo box size to %d", size);
	list_store->clear();
	for (guint i = 0; i < size; i++) {
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = (i+1);
		if (list_store->children().size() == 1) { set_active(0); }
	}
	if (size == 0) {
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = 1;
	}
	set_sensitive(size > 1);
}

guint IntComboBox::get_size() { return list_store->children().size(); }

guint IntComboBox::get_active_value() {
	Gtk::TreeModel::iterator i = get_active();
	if (i) {
		Gtk::TreeModel::Row row = *i;
		if (row) { return row[columns.column_int]; }
	}
	throw Exception(_("Failed to get active integer value"));
}
