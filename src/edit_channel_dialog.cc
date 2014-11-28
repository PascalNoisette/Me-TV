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

#include "edit_channel_dialog.h"
#include <gtkmm.h>

EditChannelDialog & EditChannelDialog::create(Glib::RefPtr<Gtk::Builder> builder) {
	EditChannelDialog * edit_channel_dialog = NULL;
	builder->get_widget_derived("dialog_edit_channel", edit_channel_dialog);
	return *edit_channel_dialog;
}

EditChannelDialog::EditChannelDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder) {
}

gint EditChannelDialog::run(Gtk::Window* transient_for) {
	gint dialog_response = Gtk::RESPONSE_CANCEL;
	if (transient_for != NULL) { set_transient_for(*transient_for); }
	dialog_response = Gtk::Dialog::run();
	return dialog_response;
}

void EditChannelDialog::on_button_edit_channel_save_clicked() { }

void EditChannelDialog::on_button_edit_channel_cancel_clicked() { }
