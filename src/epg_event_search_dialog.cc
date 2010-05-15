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

	list_store_search = Gtk::ListStore::create(search_columns);
	
	combo_box_entry_search = NULL;
	builder->get_widget("combo_box_entry_search", combo_box_entry_search);
	combo_box_entry_search->set_model(list_store_search);
	combo_box_entry_search->set_text_column(0);
	combo_box_entry_search->get_entry()->signal_activate().connect(sigc::mem_fun(*this, &EpgEventSearchDialog::search));
	list_store_search->set_sort_column(search_columns.column_text, Gtk::SORT_ASCENDING);

	builder->get_widget("tree_view_epg_event_search", tree_view_epg_event_search);

	pixbuf_record = Gtk::Widget::render_icon(Gtk::Stock::MEDIA_RECORD, Gtk::ICON_SIZE_MENU);

	tree_view_epg_event_search->signal_button_press_event().connect(
	    sigc::mem_fun(*this, &EpgEventSearchDialog::on_button_press_event));
	tree_view_epg_event_search->signal_row_activated().connect(
	    sigc::mem_fun(*this, &EpgEventSearchDialog::on_row_activated));
	list_store_results = Gtk::ListStore::create(results_columns);
	tree_view_epg_event_search->set_model(list_store_results);
 	tree_view_epg_event_search->append_column(_(" "),			results_columns.column_image);
 	tree_view_epg_event_search->append_column(_("Title"),		results_columns.column_title);
	tree_view_epg_event_search->append_column(_("Channel"),		results_columns.column_channel_name);
	tree_view_epg_event_search->append_column(_("Start Time"),	results_columns.column_start_time_text);
	tree_view_epg_event_search->append_column(_("Duration"),	results_columns.column_duration);

	list_store_results->set_sort_column(results_columns.column_start_time, Gtk::SORT_ASCENDING);
}

void EpgEventSearchDialog::search()
{
	Gtk::CheckButton* check = NULL;
	builder->get_widget("check_button_search_description", check);

	Glib::ustring text = combo_box_entry_search->get_active_text();
	Glib::ustring text_uppercase = text.uppercase();

	if (text.empty())
	{
		throw Exception(_("No search text specified"));
	}
	
	list_store_results->clear();

	Application& application = get_application();
	bool found = false;

	StringList recent_searches = application.get_string_list_configuration_value("recent_searches");
	list_store_search->clear();

    for (StringList::iterator i = recent_searches.begin(); i != recent_searches.end(); i++)
    {
		Glib::ustring recent_search = *i;
        (*list_store_search->append())[search_columns.column_text] = recent_search;
		if (recent_search.uppercase() == text_uppercase)
		{
			found = true;
		}
	}

	if (!found)
	{
		(*list_store_search->append())[search_columns.column_text] = text;
		recent_searches.push_back(text);
		application.set_string_list_configuration_value("recent_searches", recent_searches);
	}

	int got_results = false;

	bool search_description = check->get_active();
	ChannelArray channels = application.channel_manager.get_channels();
	for (ChannelArray::iterator i = channels.begin(); i != channels.end(); i++)
	{
		Channel& channel = *i;

		EpgEventList list = channel.epg_events.search(text_uppercase, search_description);
		
		for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
		{
			EpgEvent& epg_event = *j;
			
			Gtk::TreeModel::Row row = *(list_store_results->append());

			gboolean record = application.scheduled_recording_manager.is_recording(epg_event);
			if (record)
			{
				row[results_columns.column_image] = pixbuf_record;
			}
			
			row[results_columns.column_id]				= epg_event.epg_event_id;
			row[results_columns.column_title]			= epg_event.get_title();
			row[results_columns.column_start_time]		= epg_event.start_time;
			row[results_columns.column_start_time_text]	= epg_event.get_start_time_text();
			row[results_columns.column_duration]		= epg_event.get_duration_text();
			row[results_columns.column_channel]			= epg_event.channel_id;
			row[results_columns.column_channel_name]	= application.channel_manager.get_channel_by_id(epg_event.channel_id).name;
			row[results_columns.column_epg_event]		= epg_event;			
		}

		got_results = got_results || !list.empty();
	}

	if (!got_results)
	{
		Gtk::MessageDialog dialog(*this, _("No results"));
		dialog.set_modal(true);
		dialog.set_title(PACKAGE_NAME);
		dialog.run();
	}
}

void EpgEventSearchDialog::on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column)
{
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_epg_event_search->get_selection();	
	if (selection->count_selected_rows() == 0)
	{
		throw Exception(_("No EPG event selected"));
	}
	
	Gtk::TreeModel::Row row = *(selection->get_selected());

	EpgEvent epg_event = row[results_columns.column_epg_event];
	EpgEventDialog::create(builder).show_epg_event(epg_event);
	
	search();
}

bool EpgEventSearchDialog::on_button_press_event(GdkEventButton* button)
{
	if (button->button == 3)
	{
		Gtk::TreeModel::Path path;

		if (tree_view_epg_event_search->get_path_at_pos(button->x, button->y, path))
		{
			Gtk::TreeModel::iterator i = list_store_results->get_iter(path);
			EpgEvent epg_event = (*i)[results_columns.column_epg_event];

			ScheduledRecordingManager& scheduled_recording_manager = get_application().scheduled_recording_manager;

			if (scheduled_recording_manager.is_recording(epg_event))
			{
				scheduled_recording_manager.remove_scheduled_recording(epg_event);
			}
			else
			{
				scheduled_recording_manager.set_scheduled_recording(epg_event);
			}

			search();
		}
	}

	return false;
}

void EpgEventSearchDialog::on_show()
{
	list_store_search->clear();

	Application& application = get_application();
	StringList recent_searches = application.get_string_list_configuration_value("recent_searches");

	list_store_search->clear();
	for (StringList::iterator i = recent_searches.begin(); i != recent_searches.end(); i++)
	{
		(*list_store_search->append())[search_columns.column_text] = *i;
	}

	Gtk::Dialog::on_show();
}
