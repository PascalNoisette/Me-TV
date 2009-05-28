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
#include "application.h"
#include "devices_dialog.h"

DevicesDialog& DevicesDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	DevicesDialog* devices_dialog = NULL;
	builder->get_widget_derived("dialog_devices", devices_dialog);
	return *devices_dialog;
}

DevicesDialog::DevicesDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	builder->get_widget("tree_view_devices", tree_view_devices);
	
	list_store = Gtk::ListStore::create(columns);
	tree_view_devices->set_model(list_store);
	tree_view_devices->append_column(_("Device Name"), columns.column_name);
	tree_view_devices->append_column(_("Frontend Path"), columns.column_path);
	
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_devices->get_selection();
	selection->set_mode(Gtk::SELECTION_SINGLE);
}

void DevicesDialog::on_show()
{
	TRY
	Dvb::Frontend& current_frontend = get_application().device_manager.get_frontend();
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_devices->get_selection();

	list_store->clear();

	const FrontendList& frontends = get_application().device_manager.get_frontends();
	for (FrontendList::const_iterator i = frontends.begin(); i != frontends.end(); i++)
	{
		Dvb::Frontend* frontend = *i;
		
		Gtk::TreeModel::iterator row_iterator = list_store->append();
		Gtk::TreeModel::Row row			= *row_iterator;
		row[columns.column_name]		= frontend->get_frontend_info().name;
		row[columns.column_path]		= frontend->get_path();
		row[columns.column_frontend]	= frontend;
		
		if (frontend == &current_frontend)
		{
			selection->select(row);
		}
	}
	CATCH

	Gtk::Dialog::on_show();
}

void DevicesDialog::on_response(int response_id)
{
	TRY
	if (response_id == 0)
	{
		Glib::RefPtr<Gtk::TreeSelection> tree_selection = tree_view_devices->get_selection();
		Gtk::TreeModel::iterator iterator = tree_selection->get_selected();
		
		if (iterator)
		{
			Gtk::TreeModel::Row row = *iterator;
			Dvb::Frontend* frontend = row[columns.column_frontend];
			Dvb::Frontend& current_frontend = get_application().device_manager.get_frontend();
			
			if (frontend != &current_frontend)
			{
				Application& application = get_application();
				application.set_string_configuration_value("default_device", frontend->get_path());
				application.device_manager.set_frontend(*frontend);
				application.restart_stream();
			}
		}
	}
	CATCH
}
