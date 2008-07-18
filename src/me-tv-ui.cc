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

#include <libgnomeuimm.h>
#include <libglademm.h>
#include <gdk/gdk.h>
#include "application.h"
#include "me-tv-ui.h"

ComboBoxText::ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::ComboBoxText(cobject)
{
}

ComboBoxEntryText::ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::ComboBoxEntryText(cobject)
{
}

ComboBoxFrontend::ComboBoxFrontend(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::ComboBoxText(cobject), device_manager(get_application().get_device_manager())
{
	const FrontendList& frontends = device_manager.get_frontends();
	paths.resize(frontends.size());
	gint index = 0;
	for (FrontendList::const_iterator iterator = frontends.begin(); iterator != frontends.end(); iterator++)
	{
		Dvb::Frontend* frontend = *iterator;
		append_text(frontend->get_frontend_info().name);
		paths[index++] = frontend->get_path();
	}
		
	set_active(0);
}

Dvb::Frontend& ComboBoxFrontend::get_selected_frontend()
{
	Glib::ustring path = paths[get_active()];
	return device_manager.get_frontend_by_path(path);
}

GdkLock::GdkLock()
{
	gdk_threads_enter();
}

GdkLock::~GdkLock()
{
	gdk_threads_leave();
}

GdkUnlock::GdkUnlock()
{
	gdk_threads_leave();
}

GdkUnlock::~GdkUnlock()
{
	gdk_threads_enter();
}
