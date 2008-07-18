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

#ifndef __ME_TV_UI_H__
#define __ME_TV_UI_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "dvb_frontend.h"
#include "device_manager.h"

class ComboBoxText : public Gtk::ComboBoxText
{
public:
	ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
};

class ComboBoxEntryText : public Gtk::ComboBoxEntryText
{
public:
	ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
};

class ComboBoxFrontend : public Gtk::ComboBoxText
{
private:
	DeviceManager& device_manager;
	StringArray paths;
public:
	ComboBoxFrontend(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml);
	Dvb::Frontend& get_selected_frontend();
};

class GdkLock
{
public:
	GdkLock();
	~GdkLock();
};

class GdkUnlock
{
public:
	GdkUnlock();
	~GdkUnlock();
};

#endif
