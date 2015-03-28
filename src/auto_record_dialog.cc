/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "auto_record_dialog.h"
#include "application.h"

AutoRecordDialog & AutoRecordDialog::create(Glib::RefPtr<Gtk::Builder> builder) {
	AutoRecordDialog* dialog_auto_record = NULL;
	builder->get_widget_derived("dialog_auto_record", dialog_auto_record);
	return *dialog_auto_record;
}

AutoRecordDialog::AutoRecordDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Dialog(cobject), builder(builder) {
	builder->get_widget("tree_view_auto_record", tree_view_auto_record);
	list_store = Gtk::ListStore::create(columns);
	tree_view_auto_record->set_model(list_store);
	tree_view_auto_record->append_column_editable(_("Title"), columns.column_title);
	Gtk::Button* button_auto_record_add = NULL;
	builder->get_widget("button_auto_record_add", button_auto_record_add);
	button_auto_record_add->signal_clicked().connect(sigc::mem_fun(*this, &AutoRecordDialog::on_add));
	Gtk::Button* button_auto_record_delete = NULL;
	builder->get_widget("button_auto_record_delete", button_auto_record_delete);
	button_auto_record_delete->signal_clicked().connect(sigc::mem_fun(*this, &AutoRecordDialog::on_delete));
}

void AutoRecordDialog::run() {
	StringList auto_record_list = configuration_manager.get_string_list_value("auto_record");
	list_store->clear();
	for (auto const item: auto_record_list) {
		(*(list_store->append()))[columns.column_title] = item;
	}
	if (Gtk::Dialog::run() == 0) {
		auto_record_list.clear();
		Gtk::TreeModel::Children children = list_store->children();
		for (auto const item: children) {
			Glib::ustring title = trim_string(item[columns.column_title]);
			if (!title.empty()) {
				auto_record_list.push_back(title);
			}
		}
		configuration_manager.set_string_list_value("auto_record", auto_record_list);
		get_application().check_auto_record();
	}
	hide();
}

void AutoRecordDialog::on_add() {
	Gtk::TreeIter iterator = list_store->append();
	(*iterator)[columns.column_title] = "";
	tree_view_auto_record->get_selection()->select(iterator);
}

void AutoRecordDialog::on_delete() {
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_auto_record->get_selection();
	if (selection->count_selected_rows() == 0) {
		throw Exception(_("No row selected"));
	}
	list_store->erase(selection->get_selected());
}
