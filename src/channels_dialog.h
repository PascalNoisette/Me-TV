/*
 * Copyright (C) 2009 Michael Lamothe
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

#ifndef __CHANNELS_DIALOG_H__
#define __CHANNELS_DIALOG_H__

#include <libgnomeuimm.h>

class ChannelsDialog : public Gtk::Dialog
{
private:
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_name);
			add(column_channel);
		}

		Gtk::TreeModelColumn<Glib::ustring>		column_name;
		Gtk::TreeModelColumn<Channel>			column_channel;
	};
		
	ModelColumns						columns;
	Glib::RefPtr<Gtk::ListStore>		list_store;
	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::TreeView*						tree_view_displayed_channels;
	
	void show_scan_dialog();
	
	void on_show();
	void on_button_scan_clicked();
	void on_button_button_remove_selected_channels_clicked();

public:
	ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
		
	ChannelList get_channels();

	static ChannelsDialog& create(Glib::RefPtr<Gtk::Builder> builder);
};

#endif
