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

ChannelComboBox::ChannelComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::ComboBoxText(cobject)
{
}

void ChannelComboBox::load(const ChannelList& channels)
{
	clear();
	for (ChannelList::const_iterator i = channels.begin(); i != channels.end(); i++)
	{
		const Channel& channel = *i;
		append_text(channel.name);
	}
	set_active(0);
}

guint ChannelComboBox::get_selected_channel_id()
{
	return 1;
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
