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

#ifndef __EPG_EVENT_SEARCH_DIALOG_H__
#define __EPG_EVENT_SEARCH_DIALOG_H__

#include "me-tv-ui.h"

class EpgEventSearchDialog : public Gtk::Dialog
{
private:
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_id);
			add(column_title);
			add(column_channel);
			add(column_channel_name);
			add(column_start_time);
			add(column_duration);
			add(column_epg_event);
			add(column_is_scheduled);
		}

		Gtk::TreeModelColumn<guint>			column_id;
		Gtk::TreeModelColumn<Glib::ustring>	column_title;
		Gtk::TreeModelColumn<guint>			column_channel;
		Gtk::TreeModelColumn<Glib::ustring>	column_channel_name;
		Gtk::TreeModelColumn<Glib::ustring>	column_start_time;
		Gtk::TreeModelColumn<Glib::ustring>	column_duration;
		Gtk::TreeModelColumn<EpgEvent>		column_epg_event;
		Gtk::TreeModelColumn<gboolean>		column_is_scheduled;
	};

	ModelColumns						columns;
	Glib::RefPtr<Gtk::ListStore>		list_store;
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::TreeView*						tree_view_epg_event_search;

	void on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column);
	void search();
public:
	EpgEventSearchDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

	static EpgEventSearchDialog& get(Glib::RefPtr<Gtk::Builder> builder);
};

#endif
