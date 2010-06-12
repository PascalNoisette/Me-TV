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
#include "edit_channel_dialog.h"
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
	channel_conflict_action = CHANNEL_CONFLICT_ACTION_NONE;

	builder->get_widget("button_scan", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_scan_clicked));

	builder->get_widget("button_edit_selected_channel", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ChannelsDialog::on_button_edit_selected_channel_clicked));
	
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

gboolean ChannelsDialog::does_channel_exist(const Glib::ustring& channel_name)
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

gboolean ChannelsDialog::import_channel(const Channel& channel)
{
	gboolean abort_import = false;
	gboolean add_channel = true;
	Glib::ustring channel_name = channel.name;
	ChannelConflictAction action = channel_conflict_action;
	
	const Gtk::TreeModel::Children& children = list_store->children();
	for (Gtk::TreeModel::Children::const_iterator iterator = children.begin(); iterator != children.end(); iterator++)
	{		
		Gtk::TreeModel::Row row	= *iterator;
		if (row[columns.column_name] == channel.name)
		{
			g_debug("Channel add: conflict with '%s'", channel.name.c_str());

			add_channel = false;

			if (channel_conflict_action != CHANNEL_CONFLICT_ACTION_NONE)
			{
				g_debug("Repeating channel conflict action");
			}
			else
			{
				Glib::ustring message = Glib::ustring::compose(
					_("A channel named <b>'%1'</b> already exists."),
					encode_xml(channel.name));

				Gtk::Label* label_channel_conflict_message = NULL;
				builder->get_widget("label_channel_conflict_message", label_channel_conflict_message);
				label_channel_conflict_message->set_label(message);

				Gtk::Dialog* dialog_channel_conflict = NULL;
				builder->get_widget("dialog_channel_conflict", dialog_channel_conflict);
				int response = dialog_channel_conflict->run();
				dialog_channel_conflict->hide();

				switch (response)
				{
					case 0: // OK
					{
						Gtk::RadioButton* radio_button_channel_conflict_overwrite = NULL;
						Gtk::RadioButton* radio_button_channel_conflict_keep = NULL;
						Gtk::RadioButton* radio_button_channel_conflict_rename = NULL;
						Gtk::CheckButton* check_button_channel_conflict_repeat = NULL;

						builder->get_widget("radio_button_channel_conflict_overwrite", radio_button_channel_conflict_overwrite);
						builder->get_widget("radio_button_channel_conflict_keep", radio_button_channel_conflict_keep);
						builder->get_widget("radio_button_channel_conflict_rename", radio_button_channel_conflict_rename);
						builder->get_widget("check_button_channel_conflict_repeat", check_button_channel_conflict_repeat);
					
						if (radio_button_channel_conflict_overwrite->get_active())
						{
							action = CHANNEL_CONFLICT_OVERWRITE;
						}
						else if (radio_button_channel_conflict_keep->get_active())
						{
							action = CHANNEL_CONFLICT_KEEP;
						}
						else if (radio_button_channel_conflict_rename->get_active())
						{
							action = CHANNEL_CONFLICT_RENAME;
						}

						if (check_button_channel_conflict_repeat->get_active())
						{
							g_debug("Setting repeating channel conflict action");
							channel_conflict_action = action;						
						}
					}
						break;
				
					default: // Cancel channel import
						g_debug("Import cancelled");
						abort_import = true;
						break;
				}
			}

			if (!abort_import && action != CHANNEL_CONFLICT_ACTION_NONE)
			{
				if (action == CHANNEL_CONFLICT_OVERWRITE)
				{
					g_debug("Overwriting channel");

					row[columns.column_name] = channel.name;
					row[columns.column_channel] = channel;
				}
				else if (action == CHANNEL_CONFLICT_KEEP)
				{
					g_debug("Keeping channel");
				}
				else if (action == CHANNEL_CONFLICT_RENAME)
				{
					g_debug("Renaming new channel");
					guint index = 2;

					Glib::ustring channel_name = Glib::ustring::compose("%1 (%2)", channel.name, index);
					while (does_channel_exist(channel_name))
					{
						index++;
						channel_name = Glib::ustring::compose("%1 (%2)", channel.name, index);
					}
					add_channel = true;
				}
			}
		}		
	}

	if (add_channel && !abort_import)
	{
		Gtk::TreeModel::iterator row_iterator = list_store->append();
		Gtk::TreeModel::Row row			= *row_iterator;
		row[columns.column_name]		= channel_name;
		row[columns.column_frequency]	= channel.transponder.frontend_parameters.frequency;
		row[columns.column_channel]		= channel;
	}

	return !abort_import;
}

void ChannelsDialog::show_scan_dialog()
{
	// Check that there is a device
	get_application().device_manager.check_frontend();

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

void ChannelsDialog::on_button_edit_selected_channel_clicked()
{
	get_window()->freeze_updates();
	Glib::RefPtr<Gtk::TreeSelection> tree_selection = tree_view_displayed_channels->get_selection();
	std::list<Gtk::TreeModel::Path> selected_channels = tree_selection->get_selected_rows();
	if (selected_channels.size() == 0)
	{
		get_window()->thaw_updates();
		throw Exception(_("No channel selected"));
	}
	else if (selected_channels.size() > 1)
	{
		get_window()->thaw_updates();
		throw Exception(_("Select only one channel"));
	}
	else
	{
		EditChannelDialog& edit_channel_dialog = EditChannelDialog::create(builder);
		edit_channel_dialog.run(this);
		edit_channel_dialog.hide();
		get_window()->thaw_updates();
	}
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
