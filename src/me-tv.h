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

#ifndef __ME_TV_H__
#define __ME_TV_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include <gdk/gdk.h>
#include <glibmm/i18n.h>
#include <list>

typedef sigc::signal<void, const Glib::ustring&> StringSignal;

StringSignal& get_signal_error();

class ComboBoxText : public Gtk::ComboBoxText
{
public:
	ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
			: Gtk::ComboBoxText(cobject)
	{
	}
};

class ComboBoxEntryText : public Gtk::ComboBoxEntryText
{
public:
	ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
			: Gtk::ComboBoxEntryText(cobject)
	{
	}
};

class GdkLock
{
public:
	GdkLock() { gdk_threads_enter(); }
	~GdkLock() { gdk_threads_leave(); }
};

typedef std::list<Glib::ustring> StringList;

#endif
