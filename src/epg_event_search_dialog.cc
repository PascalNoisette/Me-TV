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
#include "application.h"

EpgEventSearchDialog& EpgEventSearchDialog::get(Glib::RefPtr<Gtk::Builder> builder)
{
	EpgEventSearchDialog* epg_event_search_dialog = NULL;
	builder->get_widget_derived("dialog_epg_event_search", epg_event_search_dialog);
	return *epg_event_search_dialog;
}

EpgEventSearchDialog::EpgEventSearchDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	Gtk::Button* button = NULL;
	builder->get_widget("button_epg_event_search", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &EpgEventSearchDialog::search));

	builder->get_widget("tree_view_epg_event_search", tree_view_epg_event_search);

	tree_view_epg_event_search->signal_row_activated().connect(
	    sigc::mem_fun(*this, &EpgEventSearchDialog::on_row_activated));
	list_store = Gtk::ListStore::create(columns);
	tree_view_epg_event_search->set_model(list_store);
 	tree_view_epg_event_search->append_column(_("ID"), columns.column_id);
 	tree_view_epg_event_search->append_column(_("Title"), columns.column_title);
	tree_view_epg_event_search->append_column(_("Channel"), columns.column_channel_name);
	tree_view_epg_event_search->append_column(_("Start Time"), columns.column_start_time);
	tree_view_epg_event_search->append_column(_("Duration"), columns.column_duration);

	list_store->set_sort_column(columns.column_start_time, Gtk::SORT_ASCENDING);
}

void EpgEventSearchDialog::search()
{
	list_store->clear();
	
	Gtk::Entry* entry = NULL;
	builder->get_widget("entry_epg_event_search", entry);

	Gtk::CheckButton* check = NULL;
	builder->get_widget("check_button_search_description", check);

	Application& application = get_application();
	Data::Table table_epg_event_text = application.get_schema().tables["epg_event_text"];
	Data::TableAdapter adapter_epg_event_text(application.connection, table_epg_event_text);
	Glib::ustring clause = Glib::ustring::compose("TITLE LIKE '%%%1%%'", entry->get_text());
	if (check->get_active())
	{
		clause += Glib::ustring::compose(" OR DESCRIPTION LIKE '%%%1%%'", entry->get_text());
	}
	Data::DataTable data_table_epg_event_text = adapter_epg_event_text.select_rows(clause);
	for (Data::Rows::iterator i = data_table_epg_event_text.rows.begin(); i != data_table_epg_event_text.rows.end(); i++)	
	{
		Data::Row data_row = *i;

		Gtk::TreeModel::Row row = *(list_store->append());
		row[columns.column_id]		= data_row["epg_event_id"].int_value;
		row[columns.column_title]	= data_row["title"].string_value;
	}
}

void EpgEventSearchDialog::on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column)
{
	g_debug("Row activated");
}