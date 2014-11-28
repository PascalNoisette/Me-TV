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

#ifndef __STATUS_ICON_H__
#define __STATUS_ICON_H__

#include <gtkmm.h>

class StatusIcon {
private:
	Glib::RefPtr<Gtk::StatusIcon> status_icon;
	Gtk::Menu * menu_status;
	void on_popup_menu(guint button, guint32 activate_time);
	void on_menu_item_status_quit_clicked();
	void on_menu_item_status_me_tv_clicked();
	void on_activate();
public:
	StatusIcon();
	void update();
};

#endif
