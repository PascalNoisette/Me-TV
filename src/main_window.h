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

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <libgnomeuimm.h>
#include <libglademm.h>

class MainWindow : public Gtk::Window
{
private:
	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
	Gtk::DrawingArea* drawing_area_video;
public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) : Gtk::Window(cobject), glade(glade)
	{
		drawing_area_video = (Gtk::DrawingArea*)glade->get_widget("drawing_area_video");
		drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	}
};


#endif
