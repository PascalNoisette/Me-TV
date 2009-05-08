/*
 * Copyright (C) 2009 Michael Lamothe
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
#include "me-tv.h"
#include <glibmm.h>
#include "me-tv-i18n.h"
#include <glib/gprintf.h>
#include <X11/Xlib.h>

#define ME_TV_SUMMARY _("Me TV is a digital television viewer for the GNOME desktop")
#define ME_TV_DESCRIPTION _("Me TV was developed for the modern digital lounge room with a PC for a media centre that is capable "\
	"of normal PC tasks (web surfing, word processing and watching TV). It is not designed to be a "\
	"full-blown media centre such as MythTV but will integrate well with an existing GNOME desktop.\n")

int main (int argc, char *argv[])
{	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	g_printf("Me TV %s\n", VERSION);

	if (!Glib::thread_supported())
	{
		Glib::thread_init();
	}
	gdk_threads_init();

	g_log_set_handler(G_LOG_DOMAIN,
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
		log_handler, NULL);
	
	get_signal_error().connect(sigc::ptr_fun(on_error));

	Glib::OptionEntry verbose_option_entry;
	verbose_option_entry.set_long_name("verbose");
	verbose_option_entry.set_short_name('v');
	verbose_option_entry.set_description(_("Enable verbose messages"));

	Glib::OptionEntry safe_mode_option_entry;
	safe_mode_option_entry.set_long_name("safe-mode");
	safe_mode_option_entry.set_short_name('s');
	safe_mode_option_entry.set_description(_("Start in safe mode"));

	Glib::OptionEntry minimised_option_entry;
	minimised_option_entry.set_long_name("minimised");
	minimised_option_entry.set_short_name('m');
	minimised_option_entry.set_description(_("Start minimised in notification area"));

	Glib::OptionEntry device_option_entry;
	device_option_entry.set_long_name("device");
	device_option_entry.set_short_name('d');
	device_option_entry.set_description(_("Set the default DVB frontend device (e.g. /dev/dvb/adapter0/frontend0)"));

	Glib::OptionEntry disable_epg_thread_option_entry;
	disable_epg_thread_option_entry.set_long_name("disable-epg-thread");
	disable_epg_thread_option_entry.set_description(_("Disable the EPG thread.  Me TV will stop collecting EPG events."));

	Glib::OptionEntry disable_epg_option_entry;
	disable_epg_option_entry.set_long_name("disable-epg");
	disable_epg_option_entry.set_description(_("Stops the rendering of the EPG event buttons on the UI."));

	Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV help options"));
	option_group.add_entry(verbose_option_entry, verbose_logging);
	option_group.add_entry(safe_mode_option_entry, safe_mode);
	option_group.add_entry(minimised_option_entry, minimised_mode);
	option_group.add_entry(device_option_entry, default_device);
	option_group.add_entry(disable_epg_thread_option_entry, disable_epg_thread);
	option_group.add_entry(disable_epg_option_entry, disable_epg);

	Glib::OptionContext* option_context = new Glib::OptionContext();
	option_context->set_summary(ME_TV_SUMMARY);
	option_context->set_description(ME_TV_DESCRIPTION);
	option_context->set_main_group(option_group);
		
	try
	{
		Application application(argc, argv, *option_context);
		application.run();
	}
	catch (const Glib::Exception& exception)
	{
		Gtk::MessageDialog dialog(exception.what(),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.set_title(PACKAGE_NAME);
		dialog.run();
	}
	catch (...)
	{
		Gtk::MessageDialog dialog(_("An unhandled error occurred"),
			false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		dialog.set_title(PACKAGE_NAME);
		dialog.run();
	}

	// Seems to crash if I delete option_context.
	//delete option_context;

	g_message("Me TV terminated");
	
	return 0;
}
