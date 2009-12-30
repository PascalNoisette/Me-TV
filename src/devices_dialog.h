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

#ifndef __DEVICES_DIALOG_H__
#define __DEVICES_DIALOG_H__

#include <libgnomeuimm.h>

class DevicesDialog : public Gtk::Dialog
{
private:
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_name);
			add(column_path);
			add(column_frontend);
		}

		Gtk::TreeModelColumn<Glib::ustring>		column_name;
		Gtk::TreeModelColumn<Glib::ustring>		column_path;
		Gtk::TreeModelColumn<Dvb::Frontend*>	column_frontend;
	};
		
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	const Glib::RefPtr<Gtk::Builder> builder;
	Gtk::TreeView* tree_view_devices;

	void on_show();
	void on_response(int response_id);

public:
	DevicesDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

	static DevicesDialog& create(Glib::RefPtr<Gtk::Builder> builder);
};

#endif
