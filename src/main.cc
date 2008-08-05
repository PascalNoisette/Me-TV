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

#include "application.h"
#include "config.h"
#include "me-tv.h"
#include <glibmm.h>
#include <glib/gprintf.h>
#include <gst/gst.h>

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	Glib::ustring time_text = get_time_text(get_local_time(), "%x %T");
	g_printf("%s: %s\n", time_text.c_str(), message);
}

void on_error(const Glib::ustring& message)
{
	g_message(message.c_str());
}

int main (int argc, char *argv[])
{	
	try
	{
		tzset();
		
		if (!Glib::thread_supported())
		{
			Glib::thread_init();
		}
		gdk_threads_init();

		g_log_set_handler(G_LOG_DOMAIN,
			(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			log_handler, NULL);
		
		gst_init(&argc, &argv);
		
		g_message("Me TV %s", VERSION);
		g_debug(gst_version_string());

		get_signal_error().connect(sigc::ptr_fun(on_error));
		
		TRY
		Application application(argc, argv);
		application.run();
		CATCH
	}
	catch(const Glib::Error& error)
	{
		g_error(error.what().c_str());
	}
	g_debug("Me TV terminated normally");
	
	return 0;
}
