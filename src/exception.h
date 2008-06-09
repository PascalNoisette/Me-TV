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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <glibmm.h>
#include <errno.h>

#define TRY		try {
#define CATCH	} catch(const Glib::Exception& exception) { g_debug(exception.what().c_str()); } \
				catch(...) { g_debug("An unhandled error occurred"); }

class Exception : public Glib::Exception
{
protected:
	Glib::ustring message;
public:
	Exception(const Glib::ustring& message) : message(message) { g_debug("Exception: %s", message.c_str()); }
	~Exception() throw() {}
	Glib::ustring what() const { return message; }
};

class SystemException : public Exception
{
private:
	Glib::ustring create_message(const Glib::ustring& message)
	{
		Glib::ustring m = message;
		m += ": ";
		m += strerror(errno);
		return m;
	}

public:
	SystemException(const Glib::ustring& m) : Exception(create_message(m)) {}
};

class TimeoutException : public Exception
{
public:
	TimeoutException(const Glib::ustring& m) : Exception(m) {}
};

#endif