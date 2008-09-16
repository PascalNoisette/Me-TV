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

#include "status_icon.h"
#include "application.h"
#include "channel.h"
#include "me-tv.h"

StatusIcon::StatusIcon(Glib::RefPtr<Gnome::Glade::Xml>& glade) : glade(glade)
{
	status_icon = Gtk::StatusIcon::create("me-tv");
	popup_menu = dynamic_cast<Gtk::Menu*>(glade->get_widget("menu_application_popup"));
	status_icon->signal_activate().connect(sigc::mem_fun(*this, &StatusIcon::on_activate));
	status_icon->signal_popup_menu().connect(sigc::mem_fun(*this, &StatusIcon::on_popup_menu));
	glade->connect_clicked("application_menu_item_me_tv", sigc::mem_fun(*this, &StatusIcon::on_menu_item_me_tv_clicked));
	glade->connect_clicked("menu_item_popup_quit", sigc::mem_fun(*this, &StatusIcon::on_menu_item_popup_quit_clicked));
}

void StatusIcon::on_popup_menu(guint button, guint32 activate_time)
{
	popup_menu->popup(button, activate_time);
}

void StatusIcon::on_menu_item_popup_quit_clicked()
{
	gtk_main_quit();
}

void StatusIcon::on_menu_item_me_tv_clicked()
{
	get_application().get_main_window().toggle_visibility();
}

void StatusIcon::on_activate()
{
	get_application().get_main_window().toggle_visibility();
}

void StatusIcon::update()
{
	const Channel* channel = get_application().get_profile_manager().get_current_profile().get_display_channel();
	Glib::ustring title = UNKNOWN_TEXT;
	
	if (channel != NULL)
	{
		title = channel->get_text();
	}
	
	status_icon->set_tooltip(title);

	if (get_application().is_recording())
	{
		status_icon->set("me-tv-recording");
	}
	else
	{
		status_icon->set("me-tv");
	}
}
