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

#include "auto_record_dialog.h"
#include "application.h"

AutoRecordDialog& AutoRecordDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	AutoRecordDialog* dialog_auto_record = NULL;
	builder->get_widget_derived("dialog_auto_record", dialog_auto_record);
	return *dialog_auto_record;
}

AutoRecordDialog::AutoRecordDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Dialog(cobject), builder(builder)
{
	builder->get_widget("tree_view_auto_record", tree_view_auto_record);

	list_store = Gtk::ListStore::create(columns);
	tree_view_auto_record->set_model(list_store);
	tree_view_auto_record->append_column_editable(_("Title"), columns.column_title);
}

void AutoRecordDialog::run()
{
	Application& application = get_application();

	StringList auto_record_list = application.get_string_list_configuration_value("auto_record");
	
	Gtk::Dialog::run();
}
