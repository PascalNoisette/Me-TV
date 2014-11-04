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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "me-tv.h"
#include <glibmm.h>
#include <errno.h>
#include <string.h>

class Exception: public Glib::Exception {
protected:
	Glib::ustring message;

public:
	Exception(Glib::ustring const & exception_message);
	~Exception() throw() {}
	Glib::ustring what() const { return message; }
};

class SystemException: public Exception {
private:
	Glib::ustring create_message(gint error_number, Glib::ustring const & message);

public:
	SystemException(Glib::ustring const & m) : Exception(create_message(errno, m)) {}
};

class TimeoutException: public Exception {
public:
	TimeoutException(Glib::ustring const & m) : Exception(m) {}
};

#endif
