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

#ifndef __CHANNELS_DIALOG_H__
#define __CHANNELS_DIALOG_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "scan_dialog.h"
#include "application.h"

typedef enum
{
	CHANNEL_TYPE_NONE,
	CHANNEL_TYPE_DVB,
	CHANNEL_TYPE_CUSTOM
} ChannelType;

class Channel
{
public:
	virtual const Glib::ustring& get_name() const { throw Exception("get_name() not available"); }
	virtual ChannelType get_type() const { return CHANNEL_TYPE_NONE; }
	virtual Glib::ustring get_type_text() const  { throw Exception("get_type_text() not available"); }
};

typedef std::list<Channel> ChannelList;

class DvbChannel : public Channel
{
public:
	struct dvb_frontend_parameters frontend_parameters;
	guint service_id;
	Glib::ustring service_name;
	const Glib::ustring& get_name() const { return service_name; }
	ChannelType get_type() const { return CHANNEL_TYPE_DVB; }
	Glib::ustring get_type_text() const { return "DVB"; }
};

class CustomChannel : public Channel
{
public:
	Glib::ustring name;
	Glib::ustring pre_command;
	Glib::ustring post_command;
	Glib::ustring mrl;
	const Glib::ustring& get_name() const { return name; }
	ChannelType get_type() const { return CHANNEL_TYPE_CUSTOM; }
	Glib::ustring get_type_text() const { return "Custom"; }
};

class ChannelsDialog : public Gtk::Dialog
{
private:
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_name);
			add(column_type);
			add(column_channel);
		}

		Gtk::TreeModelColumn<Glib::ustring>		column_name;
		Gtk::TreeModelColumn<Glib::ustring>		column_type;
		Gtk::TreeModelColumn<Channel>			column_channel;
	};
		
	ModelColumns columns;
	Glib::RefPtr<Gtk::ListStore> list_store;
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
	Gtk::TreeView* tree_view_displayed_channels;
	ChannelList channels;
public:
	ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
		Gtk::Dialog(cobject), glade(glade)
	{
		glade->connect_clicked("button_scan", sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));
		
		ProfileManager& profile_manager = Application::get_current().get_profile_manager();
		Profile profile = profile_manager.get_current();
		
		ComboBoxEntryText* combo_box_entry_text_profile = NULL;
		glade->get_widget_derived("combo_box_entry_profile", combo_box_entry_text_profile);
		combo_box_entry_text_profile->append_text(profile.get_name());
		combo_box_entry_text_profile->set_active_text(profile.get_name());

		tree_view_displayed_channels = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_displayed_channels"));
		
		list_store = Gtk::ListStore::create(columns);
		tree_view_displayed_channels->set_model(list_store);
		tree_view_displayed_channels->append_column("Channel Name", columns.column_name);
		tree_view_displayed_channels->append_column("Type", columns.column_type);
	}

	void on_button_scan_clicked()
	{
		gsize frontend_count = Application::get_current().get_device_manager().get_frontends().size();
		if (frontend_count == 0)
		{
			Gtk::MessageDialog dialog(*this, "There are no tuners to scan", false, Gtk::MESSAGE_ERROR);
			dialog.run();
		}
		else
		{
			channels.clear();
			
			ScanDialog* scan_dialog = NULL;
			glade->get_widget_derived("dialog_scan", scan_dialog);
			scan_dialog->run();
			scan_dialog->hide();
			
			std::list<ScannedService> selected_services = scan_dialog->get_scanned_services();
			std::list<ScannedService>::iterator iterator = selected_services.begin();
			while (iterator != selected_services.end())
			{
				ScannedService& scanned_service = *iterator;

				DvbChannel channel;
				channel.service_id = scanned_service.id;
				channel.service_name = scanned_service.name;
				channel.frontend_parameters = scanned_service.frontend_parameters;
				channels.push_back(channel);

				Gtk::TreeModel::iterator row_iterator = list_store->append();
				Gtk::TreeModel::Row row = *row_iterator;
				row[columns.column_name] = channel.service_name;
				row[columns.column_type] = "DVB";
				row[columns.column_channel] = channel;
				
				g_debug("Added channel '%s'", channel.service_name.c_str());
				
				iterator++;
			}		
		}
	}
};

#endif
