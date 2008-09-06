/*
 * Copyright (C) 2008 Michael Lamothe
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

#ifndef __STATUS_ICON_H__
#define __STATUS_ICON_H__

#include <libgnomeuimm.h>
#include <libglademm.h>

class StatusIcon
{
private:
	Glib::RefPtr<Gnome::Glade::Xml> glade;
	Glib::RefPtr<Gtk::StatusIcon>	status_icon;
	Gtk::Menu*						popup_menu;
		
	void on_popup_menu(guint button, guint32 activate_time);
	void on_menu_item_me_tv_clicked();
	void on_menu_item_popup_quit_clicked();
	void on_activate();
	void on_record_state_changed(gboolean record_state, const Glib::ustring& filename, gboolean manual);

public:
	StatusIcon(Glib::RefPtr<Gnome::Glade::Xml>& glade);
	void update();
};

#endif
