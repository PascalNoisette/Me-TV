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

#include "me-tv.h"
#include <gtkmm.h>
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
	tree_view_displayed_channels->append_column(_("Frequency (Hz)"), columns.column_frequency);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_displayed_channels->get_selection();
	selection->set_mode(Gtk::SELECTION_MULTIPLE);
}

gboolean ChannelsDialog::import_channel(const Channel& channel)
{
	gboolean abort_import = false;
	gboolean add = true;

	const Gtk::TreeModel::Children& children = list_store->children();
	for (Gtk::TreeModel::Children::const_iterator iterator = children.begin(); iterator != children.end(); iterator++)
	{		
		Gtk::TreeModel::Row row	= *iterator;
		if (row[columns.column_name] == channel.name)
		{
			g_debug("Channel import: conflict with '%s'", channel.name.c_str());
			
			add = false;

			Glib::ustring message = Glib::ustring::compose(
				_("A channel named '%1' already exists.  Do you want to overwrite it?"),
				channel.name);
			
			Gtk::MessageDialog dialog(*this, message, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
			dialog.add_button(_("Overwrite existing channel"), Gtk::RESPONSE_ACCEPT);
			dialog.add_button(_("Keep existing channel"), Gtk::RESPONSE_REJECT);
			dialog.add_button(_("Cancel scan/import"), Gtk::RESPONSE_CANCEL);

			Glib::ustring title = PACKAGE_NAME " - ";
			title +=  _("Channel conflict");
			dialog.set_title(title);
			dialog.set_icon_from_file(PACKAGE_DATA_DIR"/me-tv/glade/me-tv.xpm");
			int response = dialog.run();

			switch (response)
			{
				case Gtk::RESPONSE_ACCEPT: // Overwrite
					g_debug("Channel accepted");
					row[columns.column_name]	= channel.name;
					row[columns.column_channel]	= channel;
					break;
				case Gtk::RESPONSE_REJECT:
					g_debug("Channel rejected");
					break;
				default: // Cancel
					g_debug("Import cancelled");
					abort_import = true;
					break;
			}
		}		
	}

	if (add && !abort_import)
	{
		Gtk::TreeModel::iterator row_iterator = list_store->append();
		Gtk::TreeModel::Row row			= *row_iterator;
		row[columns.column_name]		= channel.name;
		row[columns.column_frequency]	= channel.transponder.frontend_parameters.frequency;
		row[columns.column_channel]		= channel;
	}

	return !abort_import;
}

void ChannelsDialog::show_scan_dialog()
{
	FullscreenBugWorkaround fullscreen_bug_workaround;

	ScanDialog& scan_dialog = ScanDialog::create(builder);
	scan_dialog.show();
	Gtk::Main::run(scan_dialog);
	
	ChannelArray channels = scan_dialog.get_selected_channels();	
	for (ChannelArray::const_iterator iterator = channels.begin(); iterator != channels.end(); iterator++)
	{
		if (!import_channel(*iterator))
		{
			break;
		}
	}
}

void ChannelsDialog::on_button_scan_clicked()
{
	show_scan_dialog();
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
			Gtk::TreeModel::Row row			= *row_iterator;
			row[columns.column_name]		= channel.name;
			row[columns.column_frequency]	= channel.transponder.frontend_parameters.frequency;
			row[columns.column_channel]		= channel;
		}
	}
	Gtk::Dialog::on_show();
}
