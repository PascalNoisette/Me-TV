/*
 * Copyright (C) 2008 Michael Lamothe
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

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "scan_dialog.h"
#include "application.h"
#include "channels_dialog.h"

ChannelsDialog* ChannelsDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	ChannelsDialog* channels_dialog = NULL;
	glade->get_widget_derived("dialog_channels", channels_dialog);
	return channels_dialog;
}

ChannelsDialog::ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::Dialog(cobject), glade(glade)
{
	glade->connect_clicked("button_scan",							sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));
	glade->connect_clicked("button_remove_selected_channels",		sigc::mem_fun(*this, &ChannelsDialog::on_button_button_remove_selected_channels_clicked));
	glade->connect_clicked("button_select_all_displayed_channels",	sigc::mem_fun(*this, &ChannelsDialog::on_button_select_all_displayed_channels_clicked));
	
	tree_view_displayed_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_displayed_channels"));
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_displayed_channels->set_model(list_store);
	tree_view_displayed_channels->append_column("Channel Name", columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_displayed_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
}

void ChannelsDialog::on_button_select_all_displayed_channels_clicked()
{
	tree_view_displayed_channels->get_selection()->select_all();
}

void ChannelsDialog::on_button_scan_clicked()
{
	ScanDialog* scan_dialog = ScanDialog::create(glade);
	gint scan_dialog_result = scan_dialog->run();
	scan_dialog->hide();
	if (scan_dialog_result == Gtk::RESPONSE_OK)
	{
		ScannedServiceList scanned_services = scan_dialog->get_scanned_services();
		
		std::list<ScannedService>::iterator iterator = scanned_services.begin();
		while (iterator != scanned_services.end())
		{
			ScannedService& scanned_service = *iterator;

			Channel channel;
			channel.service_id			= scanned_service.id;
			channel.name				= scanned_service.name;
			channel.frontend_parameters	= scanned_service.frontend_parameters;
			channel.flags				= CHANNEL_FLAG_DVB_T;

			Gtk::TreeModel::iterator row_iterator = list_store->append();
			Gtk::TreeModel::Row row		= *row_iterator;
			row[columns.column_name]	= channel.name;
			row[columns.column_channel]	= channel;
			
			iterator++;			
		}
	}
}

void ChannelsDialog::on_button_button_remove_selected_channels_clicked()
{
	get_window()->freeze_updates();
	Glib::RefPtr<Gtk::TreeSelection> tree_selection = tree_view_displayed_channels->get_selection();
	std::list<Gtk::TreeModel::Path> selected_channels = tree_selection->get_selected_rows();
	while (selected_channels.size() > 0)
	{		
		list_store->erase(list_store->get_iter(*selected_channels.begin()));
		selected_channels = tree_selection->get_selected_rows();
	}
	get_window()->thaw_updates();
}

ChannelList ChannelsDialog::get_channels()
{
	ChannelList result;
	Glib::RefPtr<Gtk::TreeModel> model = tree_view_displayed_channels->get_model();
	Gtk::TreeModel::Children children = model->children();
	Gtk::TreeIter iterator = children.begin();
	while (iterator != children.end())
	{
		Gtk::TreeModel::Row row(*iterator);
		result.push_back(row.get_value(columns.column_channel));
		iterator++;
	}
	return result;
}

void ChannelsDialog::set_channels(const ChannelList& channels)
{
	list_store->clear();
	
	ChannelList::const_iterator iterator = channels.begin();
	while (iterator != channels.end())
	{
		const Channel& channel = *iterator;

		Gtk::TreeModel::iterator row_iterator = list_store->append();
		Gtk::TreeModel::Row row		= *row_iterator;
		row[columns.column_name]	= channel.name;
		row[columns.column_channel]	= channel;
		
		iterator++;			
	}
}
