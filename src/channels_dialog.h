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

#ifndef __CHANNELS_DIALOG_H__
#define __CHANNELS_DIALOG_H__

#include <gtkmm.h>

class ChannelsDialog: public Gtk::Dialog {
private:
	typedef enum {
		CHANNEL_CONFLICT_ACTION_NONE,
		CHANNEL_CONFLICT_OVERWRITE,
		CHANNEL_CONFLICT_KEEP,
		CHANNEL_CONFLICT_RENAME,
	} ChannelConflictAction;

	class ModelColumns: public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(column_name);
			add(column_frequency);
			add(column_channel);
		}
		Gtk::TreeModelColumn<Glib::ustring> column_name;
		Gtk::TreeModelColumn<guint> column_frequency;
		Gtk::TreeModelColumn<Channel> column_channel;
	};

	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	const Glib::RefPtr<Gtk::Builder> builder;
	Gtk::TreeView * tree_view_displayed_channels;
	ChannelConflictAction channel_conflict_action;
	void show_scan_dialog();
	gboolean import_channel(Channel const & channel);
	gboolean does_channel_exist(Glib::ustring const & channel_name);
	void on_show();
	void on_button_scan_clicked();
	void on_button_edit_selected_channel_clicked();
	void on_button_remove_selected_channels_clicked();

public:
	ChannelsDialog(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder);
	ChannelArray get_channels();
	static ChannelsDialog& create(Glib::RefPtr<Gtk::Builder> builder);
};

#endif
