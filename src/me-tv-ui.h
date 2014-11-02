/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014  Russel Winder
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

#ifndef __ME_TV_UI_H__
#define __ME_TV_UI_H__

#include <gtkmm.h>
#include "me-tv.h"
#include "dvb_frontend.h"
#include "channel.h"

extern Glib::RefPtr<Gtk::UIManager>	ui_manager;

class IntComboBox : public Gtk::ComboBox {
private:
	class ModelColumns : public Gtk::TreeModel::ColumnRecord {
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

class ChannelComboBox : public Gtk::ComboBox {
private:
	class ModelColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() {
			add(column_id);
			add(column_name);
		}
		Gtk::TreeModelColumn<guint>			column_id;
		Gtk::TreeModelColumn<Glib::ustring>	column_name;
	};
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;

public:
	ChannelComboBox(BaseObjectType *const cobject, Glib::RefPtr<Gtk::Builder> const & xml);
	void load(ChannelArray const & channels);
	guint get_selected_channel_id();
	void set_selected_channel_id(guint channel_id);
};

#endif
