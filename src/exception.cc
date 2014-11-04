/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014  Russel Winder
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
#include "me-tv-i18n.h"

Exception::Exception(Glib::ustring const & exception_message) : message(exception_message) {
	g_debug("Exception: %s", message.c_str());
}

Glib::ustring SystemException::create_message(gint error_number, Glib::ustring const & exception_message) {
	Glib::ustring detail = _("Failed to get error message");
	char* system_error_message = strerror(error_number);
	if (system_error_message != NULL) {
		detail = system_error_message;
	}
	return Glib::ustring::compose("%1: %2", exception_message, detail);
}
