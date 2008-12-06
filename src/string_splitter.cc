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
 
#include "string_splitter.h"
#include "exception.h"

StringSplitter::StringSplitter(const Glib::ustring& text, const char* deliminator, gsize max_length)
{
	parts = g_strsplit(text.c_str(), deliminator, max_length);
	count = 0;
	gchar** iterator = parts;
	while (*iterator++ != NULL && count < max_length)
	{
		count++;
	}
}
	
StringSplitter::~StringSplitter()
{
	g_strfreev(parts);
}

const gchar* StringSplitter::get_value(guint index)
{
	if (index >= count)
	{
		throw Exception(_("Index out of bounds"));
	}
	return parts[index];
}

gint StringSplitter::get_int_value(guint index)
{
	return atoi(get_value(index));
}

gsize StringSplitter::get_count() const { return count; }
