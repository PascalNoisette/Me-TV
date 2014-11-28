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

#ifndef __ME_TV_UI_H__
#define __ME_TV_UI_H__

#include <gtkmm.h>
#include "me-tv.h"
#include "dvb_frontend.h"
#include "channel.h"

extern Glib::RefPtr<Gtk::UIManager>	ui_manager;

// This class exists because I can't get Gtk::ComboBoxText to work properly
// it seems to have 2 columns
class ComboBoxText : public Gtk::ComboBox {
private:
	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(column_text);
			add(column_value);
		}
		Gtk::TreeModelColumn<Glib::ustring> column_text;
		Gtk::TreeModelColumn<Glib::ustring> column_value;
	};
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
public:
	ComboBoxText(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & xml);
	void clear_items();
	void append_text(Glib::ustring const & text, Glib::ustring const & value = "");
	void set_active_text(Glib::ustring const & text);
	void set_active_value(Glib::ustring const & value);
	Glib::ustring get_active_text();
	Glib::ustring get_active_value();
};

class IntComboBox: public Gtk::ComboBox {
private:
	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() { add(column_int); }
		Gtk::TreeModelColumn<guint> column_int;
	};
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
public:
	IntComboBox(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml);
	void set_size(guint size);
	guint get_size();
	guint get_active_value();
};

class ComboBoxEntryText: public Gtk::ComboBoxEntryText {
public:
	ComboBoxEntryText(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & xml);
};

class ChannelComboBox: public Gtk::ComboBox {
private:
	class ModelColumns: public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(column_id);
			add(column_name);
		}
		Gtk::TreeModelColumn<guint> column_id;
		Gtk::TreeModelColumn<Glib::ustring> column_name;
	};
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
public:
	ChannelComboBox(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml);
	void load(ChannelArray const & channels);
	guint get_selected_channel_id();
	void set_selected_channel_id(guint channel_id);
};

class GdkLock {
public:
	GdkLock() { gdk_threads_enter(); }
	~GdkLock() { gdk_threads_leave(); }
};

class GdkUnlock {
public:
	GdkUnlock() { gdk_threads_leave(); }
	~GdkUnlock() { gdk_threads_enter(); }
};

#endif
