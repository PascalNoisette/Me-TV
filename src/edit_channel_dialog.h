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

#ifndef __EDIT_CHANNEL_DIALOG_H__
#define __EDIT_CHANNEL_DIALOG_H__

#include <gtkmm.h>

class EditChannelDialog: public Gtk::Dialog {
private:
	Glib::RefPtr<Gtk::Builder> const builder;
	void on_button_edit_channel_save_clicked();
	void on_button_edit_channel_cancel_clicked();
public:
	EditChannelDialog(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder);
	static EditChannelDialog & create(Glib::RefPtr<Gtk::Builder> builder);
	gint run(Gtk::Window * transient_for);
};

#endif
