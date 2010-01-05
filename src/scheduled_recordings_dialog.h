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

#ifndef __SCHEDULED_RECORDINGS_DIALOG_H__
#define __SCHEDULED_RECORDINGS_DIALOG_H__

#include <gtkmm.h>
#include "data.h"

class ScheduledRecordingsDialog : public Gtk::Dialog
{
private:	
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_sort);
			add(column_scheduled_recording_id);
			add(column_description);
			add(column_channel);
			add(column_start_time);
			add(column_duration);
			add(column_device);
		}

		Gtk::TreeModelColumn<guint>			column_sort;
		Gtk::TreeModelColumn<guint>			column_scheduled_recording_id;
		Gtk::TreeModelColumn<Glib::ustring>	column_description;
		Gtk::TreeModelColumn<Glib::ustring>	column_channel;
		Gtk::TreeModelColumn<Glib::ustring>	column_start_time;
		Gtk::TreeModelColumn<Glib::ustring>	column_duration;
		Gtk::TreeModelColumn<Glib::ustring>	column_device;
	};
	
	ModelColumns						columns;
	Glib::RefPtr<Gtk::ListStore>		list_store;
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::TreeView*						tree_view_scheduled_recordings;
		
	void on_button_scheduled_recordings_add_clicked();
	void on_button_scheduled_recordings_delete_clicked();
	void on_button_scheduled_recordings_edit_clicked();

	void on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);		
	void on_show();

	void show_scheduled_recording(guint scheduled_recording_id);
	guint get_selected_scheduled_recording_id();

public:	
	ScheduledRecordingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	
	static ScheduledRecordingsDialog& create(Glib::RefPtr<Gtk::Builder> builder);

	void update();
};

#endif
