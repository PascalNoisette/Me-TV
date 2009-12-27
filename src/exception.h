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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "me-tv.h"
#include <glibmm.h>
#include <errno.h>
#include <string.h>

#define TRY		try {
#define CATCH	} \
	catch(const Glib::Exception& exception) { \
		get_signal_error().emit(exception.what().c_str()); } \
	catch(...) { get_signal_error().emit("An unhandled error occurred"); }
#define THREAD_CATCH	} catch(const Glib::Exception& exception) { g_message("Error in thread: %s", exception.what().c_str()); } \
						catch(...) { g_message("An unhandled error occurred"); }

class Exception : public Glib::Exception
{
protected:
	Glib::ustring message;
public:
	Exception(const Glib::ustring& exception_message);
	~Exception() throw() {}
	Glib::ustring what() const { return message; }
};

class SystemException : public Exception
{
private:
	Glib::ustring create_message(gint error_number, const Glib::ustring& message);
public:
	SystemException(const Glib::ustring& m) : Exception(create_message(errno, m)) {}
};

class TimeoutException : public Exception
{
public:
	TimeoutException(const Glib::ustring& m) : Exception(m) {}
};

#endif
