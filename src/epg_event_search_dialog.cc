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

#include "epg_event_search_dialog.h"
#include "scheduled_recording_dialog.h"
#include "application.h"
#include "epg_event_dialog.h"

EpgEventSearchDialog& EpgEventSearchDialog::get(Glib::RefPtr<Gtk::Builder> builder)
{
	EpgEventSearchDialog* epg_event_search_dialog = NULL;
	builder->get_widget_derived("dialog_epg_event_search", epg_event_search_dialog);
	return *epg_event_search_dialog;
}

EpgEventSearchDialog::EpgEventSearchDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder), image_record(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_MENU)
{
	Gtk::Button* button = NULL;
	builder->get_widget("button_epg_event_search", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &EpgEventSearchDialog::search));

	Gtk::Entry* entry = NULL;
	builder->get_widget("entry_epg_event_search", entry);
	entry->signal_activate().connect(sigc::mem_fun(*this, &EpgEventSearchDialog::search));

	builder->get_widget("tree_view_epg_event_search", tree_view_epg_event_search);

	tree_view_epg_event_search->signal_row_activated().connect(
	    sigc::mem_fun(*this, &EpgEventSearchDialog::on_row_activated));
	list_store = Gtk::ListStore::create(columns);
	tree_view_epg_event_search->set_model(list_store);
 	tree_view_epg_event_search->append_column(_("Title"), columns.column_title);
	tree_view_epg_event_search->append_column(_("Channel"), columns.column_channel_name);
 	tree_view_epg_event_search->append_column(_("Record"), columns.column_image);
	tree_view_epg_event_search->append_column(_("Start Time"), columns.column_start_time);
	tree_view_epg_event_search->append_column(_("Duration"), columns.column_duration);

	list_store->set_sort_column(columns.column_start_time, Gtk::SORT_ASCENDING);
}

void EpgEventSearchDialog::search()
{	
	Gtk::Entry* entry = NULL;
	builder->get_widget("entry_epg_event_search", entry);

	Gtk::CheckButton* check = NULL;
	builder->get_widget("check_button_search_description", check);

	Glib::ustring text = entry->get_text().uppercase();

	if (text.size() == 0)
	{
		throw Exception(_("No search text specified"));
	}
	
	list_store->clear();

	Application& application = get_application();
	bool search_description = check->get_active();
	ChannelArray channels = application.channel_manager.get_channels();
	for (ChannelArray::iterator i = channels.begin(); i != channels.end(); i++)
	{
		Channel& channel = *i;

		EpgEventList list = channel.epg_events.search(text, search_description);
		
		for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
		{
			EpgEvent& epg_event = *j;
			
			Gtk::TreeModel::Row row = *(list_store->append());

			gboolean record = application.scheduled_recording_manager.is_recording(epg_event);
			row[columns.column_image]			= record ? "Yes" : "No";
			row[columns.column_id]				= epg_event.epg_event_id;
			row[columns.column_title]			= epg_event.get_title();
			row[columns.column_start_time]		= epg_event.get_start_time_text();
			row[columns.column_duration]		= epg_event.get_duration_text();
			row[columns.column_channel]			= epg_event.channel_id;
			row[columns.column_channel_name]	= application.channel_manager.get_channel_by_id(epg_event.channel_id).name;
			row[columns.column_epg_event]		= epg_event;
		}
	}
}

void EpgEventSearchDialog::on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column)
{
	TRY
		
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_epg_event_search->get_selection();	
	if (selection->count_selected_rows() == 0)
	{
		throw Exception(_("No EPG event selected"));
	}
	
	Gtk::TreeModel::Row row = *(selection->get_selected());

	EpgEvent epg_event = row[columns.column_epg_event];
	EpgEventDialog& epg_event_dialog = EpgEventDialog::create(builder);
	epg_event_dialog.show_epg_event(epg_event);

	search();

	CATCH
}
