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

#include "me-tv.h"
#include <libgnomeuimm.h>
#include "scan_dialog.h"
#include "application.h"
#include "channels_dialog.h"

ChannelsDialog& ChannelsDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ChannelsDialog* channels_dialog = NULL;
	builder->get_widget_derived("dialog_channels", channels_dialog);
	return *channels_dialog;
}

ChannelsDialog::ChannelsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	Gtk::Button* button = NULL;

	builder->get_widget("button_scan", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));
	
	builder->get_widget("button_remove_selected_channels", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_remove_selected_channels_clicked));

	tree_view_displayed_channels = NULL;
	
	builder->get_widget("tree_view_displayed_channels", tree_view_displayed_channels);
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_displayed_channels->set_model(list_store);
	tree_view_displayed_channels->append_column(_("Channel Name"), columns.column_name);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_displayed_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
}

gboolean ChannelsDialog::channel_exists(const Glib::ustring& channel_name)
{
	const Gtk::TreeModel::Children& children = list_store->children();
	for (Gtk::TreeModel::Children::const_iterator iterator = children.begin(); iterator != children.end(); iterator++)
	{
		Gtk::TreeModel::Row row	= *iterator;
		if (row[columns.column_name] == channel_name)
		{
			return true;
		}
	}

	return false;
}

void ChannelsDialog::show_scan_dialog()
{
	// Check for a valid frontend device
	get_application().device_manager.get_frontend();
	
	FullscreenBugWorkaround fullscreen_bug_workaround;

	ScanDialog& scan_dialog = ScanDialog::create(builder);
	scan_dialog.show();
	Gtk::Main::run(scan_dialog);

	gboolean abort = false;
	
	ChannelArray channels = scan_dialog.get_channels();	
	for (ChannelArray::const_iterator iterator = channels.begin(); iterator != channels.end() && !abort; iterator++)
	{
		const Channel& channel = *iterator;

		gboolean add = true;
		
		if (channel_exists(channel.name))
		{
			Glib::ustring message = Glib::ustring::compose(
				_("A channel named '%1' already exists.  Do you want to overwrite it?"),
				channel.name);
			
			Gtk::MessageDialog dialog(*this, message, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
			dialog.add_button("Overwrite existing channel", Gtk::RESPONSE_ACCEPT);
			dialog.add_button("Keep existing channel", Gtk::RESPONSE_REJECT);
			dialog.add_button("Cancel scan/import", Gtk::RESPONSE_CANCEL);
			dialog.set_title(PACKAGE_NAME " - Channel conflict");
			dialog.set_icon_from_file(PACKAGE_DATA_DIR"/me-tv/glade/me-tv.xpm");
			int response = dialog.run();

			switch (response)
			{
				case Gtk::RESPONSE_ACCEPT: break;
				case Gtk::RESPONSE_REJECT: add = false; break;
				case Gtk::RESPONSE_CANCEL: add = false; abort = true; break;
				default: throw Exception("Invalid response");
			}
		}

		if (add)
		{
			Gtk::TreeModel::iterator row_iterator = list_store->append();
			Gtk::TreeModel::Row row		= *row_iterator;
			row[columns.column_name]	= channel.name;
			row[columns.column_channel]	= channel;
		}
	}
}

void ChannelsDialog::on_button_scan_clicked()
{
	TRY
	show_scan_dialog();
	CATCH
}

void ChannelsDialog::on_button_remove_selected_channels_clicked()
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

ChannelArray ChannelsDialog::get_channels()
{
	ChannelArray result;
	Glib::RefPtr<Gtk::TreeModel> model = tree_view_displayed_channels->get_model();
	Gtk::TreeModel::Children children = model->children();
	Gtk::TreeIter iterator = children.begin();
	guint sort_order = 0;
	while (iterator != children.end())
	{
		Gtk::TreeModel::Row row(*iterator);
		Channel channel = row.get_value(columns.column_channel);
		channel.sort_order = sort_order++;
		result.push_back(channel);
		iterator++;
	}
	return result;
}

void ChannelsDialog::on_show()
{
	TRY
	list_store->clear();
	
	Application& application = get_application();

	ChannelArray& channels = application.channel_manager.get_channels();
	if (channels.empty() && !application.device_manager.get_frontends().empty())
	{
		show_scan_dialog();
	}
	else
	{
		for (ChannelArray::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
		{
			const Channel& channel = *iterator;

			Gtk::TreeModel::iterator row_iterator = list_store->append();
			Gtk::TreeModel::Row row		= *row_iterator;
			row[columns.column_name]	= channel.name;
			row[columns.column_channel]	= channel;
		}
	}
	Gtk::Dialog::on_show();

	CATCH
}
