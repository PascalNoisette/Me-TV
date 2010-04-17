/*
 * Copyright (C) 2010 Michael Lamothe
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

#include <gtkmm.h>
#include <gdk/gdk.h>
#include "application.h"
#include "me-tv-ui.h"

Glib::RefPtr<Gtk::UIManager> ui_manager;

Glib::ustring make_recording_filename(Channel& channel, const Glib::ustring& description)
{
	Glib::ustring start_time = get_local_time_text("%c");
	Glib::ustring filename;
	Glib::ustring title = description;
	
	if (title.empty())
	{
		EpgEvent epg_event;
		if (channel.epg_events.get_current(epg_event))
		{
			title = epg_event.get_title();
		}
	}
	
	if (title.empty())
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2.mpeg",
			channel.name,
			start_time
		);
	}
	else
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2 - %3.mpeg",
			title,
			channel.name,
			start_time
		);
	}

	// Clean filename
	Glib::ustring::size_type position = Glib::ustring::npos;
	while ((position = filename.find('/')) != Glib::ustring::npos)
	{
		filename.replace(position, 1, "_");
	}

	if (get_application().get_boolean_configuration_value("remove_colon"))
	{
		while ((position = filename.find(':')) != Glib::ustring::npos )
		{
			filename.replace(position, 1, "_");
		}
	}

	Glib::ustring fixed_filename = Glib::filename_from_utf8(filename);
	
	return Glib::build_filename(
	    get_application().get_string_configuration_value("recording_directory"),
	    fixed_filename);
}

ComboBoxText::ComboBoxText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_text);
	set_active(0);
}

void ComboBoxText::append_text(const Glib::ustring& text, const Glib::ustring& value)
{
	Gtk::TreeModel::Row row = *list_store->append();
	row[columns.column_text] = text;
	row[columns.column_value] = value;
}

void ComboBoxText::set_active_text(const Glib::ustring& text)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_text] == text)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::set_active_value(const Glib::ustring& value)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_value] == value)
		{
			set_active(i);
		}
	}
}

void ComboBoxText::clear_items()
{
	list_store->clear();
}

Glib::ustring ComboBoxText::get_active_text()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_text];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

Glib::ustring ComboBoxText::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_value];
		}
	}
	
	throw Exception(_("Failed to get active text value"));
}

ComboBoxEntryText::ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBoxEntryText(cobject)
{
}

ChannelComboBox::ChannelComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
}

void ChannelComboBox::load(const ChannelArray& channels)
{
	clear();
	set_model(list_store);
	pack_start(columns.column_name);
	list_store->clear();
	for (ChannelArray::const_iterator i = channels.begin(); i != channels.end(); i++)
	{
		const Channel& channel = *i;
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_id] = channel.channel_id;
		row[columns.column_name] = channel.name;
	}
	set_active(0);
}

void ChannelComboBox::set_selected_channel_id(guint channel_id)
{
	Gtk::TreeNodeChildren children = get_model()->children();
	for (Gtk::TreeNodeChildren::iterator i = children.begin(); i != children.end(); i++)
	{
		Gtk::TreeModel::Row row = *i;
		if (row[columns.column_id] == channel_id)
		{
			set_active(i);
		}
	}
}

guint ChannelComboBox::get_selected_channel_id()
{
	Gtk::TreeModel::Row row = *get_active();
	return row[columns.column_id];
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

IntComboBox::IntComboBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
	: Gtk::ComboBox(cobject)
{
	list_store = Gtk::ListStore::create(columns);
	clear();
	set_model(list_store);
	pack_start(columns.column_int);
	set_size(0);
	set_active(0);
}

void IntComboBox::set_size(guint size)
{	
	g_debug("Setting integer combo box size to %d", size);

	list_store->clear();
	for (guint i = 0; i < size; i++)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = (i+1);
		
		if (list_store->children().size() == 1)
		{
			set_active(0);
		}
	}
	
	if (size == 0)
	{
		Gtk::TreeModel::Row row = *list_store->append();
		row[columns.column_int] = 1;
	}
	
	set_sensitive(size > 1);
}

guint IntComboBox::get_size()
{
	return list_store->children().size();
}

guint IntComboBox::get_active_value()
{
	Gtk::TreeModel::iterator i = get_active();
	if (i)
	{
		Gtk::TreeModel::Row row = *i;
		if (row)
		{
			return row[columns.column_int];
		}
	}
	
	throw Exception(_("Failed to get active integer value"));
}

FullscreenBugWorkaround::FullscreenBugWorkaround()
{
	apply = false;

	if (get_application().get_boolean_configuration_value("fullscreen_bug_workaround"))
	{
		MainWindow& main_window = get_application().get_main_window();
		apply = main_window.is_fullscreen();
		if (apply)
		{
			main_window.unfullscreen(false);
		}
	}
}

FullscreenBugWorkaround::~FullscreenBugWorkaround()
{
	if (apply)
	{
		MainWindow& main_window = get_application().get_main_window();
		main_window.fullscreen(false);
	}
}
