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
#include <X11/Xlib.h>

#define ME_TV_SUMMARY _("Me TV is a digital television viewer for the GNOME desktop")
#define ME_TV_DESCRIPTION _("Me TV was developed for the modern digital lounge room with a PC for a media centre that is capable "\
	"of normal PC tasks (web surfing, word processing and watching TV). It is not designed to be a "\
	"full-blown media centre such as MythTV but will integrate well with an existing GNOME desktop.\n")

int main (int argc, char *argv[])
{	
	XInitThreads();
	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	if (!Glib::thread_supported())
	{
		Glib::thread_init();
	}
	gdk_threads_init();

	g_printf("Me TV %s\n", VERSION);

	g_log_set_handler(G_LOG_DOMAIN,
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
		log_handler, NULL);
	
	get_signal_error().connect(sigc::ptr_fun(on_error));

	Glib::OptionEntry verbose_option_entry;
	verbose_option_entry.set_long_name("verbose");
	verbose_option_entry.set_short_name('v');
	verbose_option_entry.set_description(_("Enable verbose messages"));

	Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV help options"));
	option_group.add_entry(verbose_option_entry, verbose_logging);

	Glib::OptionContext* option_context = new Glib::OptionContext();
	option_context->set_summary(ME_TV_SUMMARY);
	option_context->set_description(ME_TV_DESCRIPTION);
	option_context->set_main_group(option_group);

	TRY
	Application application(argc, argv, *option_context);
	application.run();
	CATCH

	//delete option_context;

	g_message("Me TV terminated");
	
	return 0;
}
