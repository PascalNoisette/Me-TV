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

#include "exception.h"

#define BUFFER_SIZE 2000

Glib::ustring SystemException::create_message(gint error_number, const Glib::ustring& message)
{
	Glib::ustring detail = _("Failed to get error message");
	
	detail = strerror(error_number);
	/*
	char buffer[BUFFER_SIZE];
	if (strerror_r(error_number, buffer, BUFFER_SIZE) == 0)
	{
		detail = Glib::ustring(buffer);
	}
	else
	{
		detail = Glib::ustring::compose("Code %1", errno);
	}
	return Glib::ustring::compose("%1: %2", message, detail);
	*/
	
	return detail;
}
