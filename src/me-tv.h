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

template<class T>
class StringHashTable
{
private:
	GHashTable* hash_table;
		
	gpointer get_data(const Glib::ustring& key)
	{
		return g_hash_table_lookup(hash_table, key.c_str());
	}
		
public:
	StringHashTable()
	{
		hash_table = g_hash_table_new(g_str_hash, g_str_equal);
		if (hash_table == NULL)
		{
			throw Exception("Failed to create hash table");
		}
	}
		
	~StringHashTable()
	{
		g_hash_table_destroy(hash_table);
	}
	
	gboolean contains_key(const Glib::ustring& key)
	{
		return get_data(key) != NULL;
	}
	
	void set(Glib::ustring& key, T t)
	{
		gsize bytes = key.bytes() + 1;
		gchar buffer[bytes];
		key.copy(buffer, bytes);
		g_hash_table_insert(hash_table, buffer, &t);
	}
		
	T get(const Glib::ustring& key)
	{
		gpointer data = get_data(key);
		if (data == NULL)
		{
			throw Exception(_("Key not found"));
		}
		return (T)data;
	}
};

#endif
