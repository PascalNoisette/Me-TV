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

#include "engine.h"
#include "application.h"
#include "me-tv-i18n.h"
#include <gdk/gdkx.h>

Engine::Engine()
{
	drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(get_application().get_glade()->get_widget("drawing_area_video"));

	window_id = GDK_WINDOW_XID(drawing_area_video->get_window()->gobj());
	if (window_id == 0)
	{
		throw Exception(_("Window ID was 0"));
	}
}

gint Engine::get_window_id()
{
	return window_id;
}

Gtk::DrawingArea* Engine::get_drawing_area_video()
{
	return drawing_area_video;
}
