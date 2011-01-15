/*
 * Copyright (C) 2011 Michael Lamothe
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

#include "me-tv.h"
#include "me-tv-i18n.h"
#include "application.h"
#include <gconfmm.h>
#include <glib/gprintf.h>
#include <X11/Xlib.h>
#include <unique/unique.h>

#define ME_TV_SUMMARY _("Me TV is a digital television viewer for GTK")
#define ME_TV_DESCRIPTION _("Me TV was developed for the modern digital lounge room with a PC for a media centre that is capable "\
	"of normal PC tasks (web surfing, word processing and watching TV).\n")

static UniqueResponse on_message_received (
	UniqueApp*			app,
	UniqueCommand		command,
	UniqueMessageData*  message,
	guint          		time_,
	gpointer       		user_data)
{
	UniqueResponse response = UNIQUE_RESPONSE_FAIL;

	switch (command)
    {
		case 1:
			action_present->activate();
			response = UNIQUE_RESPONSE_OK;
			break;
			
		default:
			break;
	}

	return response;
}

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

	Gnome::Conf::init();
	Gtk::Main main(argc, argv);

	Glib::add_exception_handler(&on_error);

	g_log_set_handler(G_LOG_DOMAIN,
		(GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
		log_handler, NULL);

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

	Glib::OptionEntry disable_epg_thread_option_entry;
	disable_epg_thread_option_entry.set_long_name("disable-epg-thread");
	disable_epg_thread_option_entry.set_description(_("Disable the EPG thread.  Me TV will stop collecting EPG events."));

	Glib::OptionEntry disable_epg_option_entry;
	disable_epg_option_entry.set_long_name("disable-epg");
	disable_epg_option_entry.set_description(_("Stops the rendering of the EPG event buttons on the UI."));

	Glib::OptionEntry no_screensaver_inhibit_option_entry;
	no_screensaver_inhibit_option_entry.set_long_name("no-screensaver-inhibit");
	no_screensaver_inhibit_option_entry.set_description(_("Tells Me TV not to call the screensaver Inhibit/UnInhibit methods for GNOME Screensaver."));

	Glib::OptionEntry devices_option_entry;
	devices_option_entry.set_long_name("devices");
	devices_option_entry.set_description(_("Only use the specified frontend devices (e.g. --devices=/dev/dvb/adapter0/frontend0,/dev/dvb/adapter0/frontend1)"));

	Glib::OptionEntry read_timeout_option_entry;
	read_timeout_option_entry.set_long_name("read-timeout");
	read_timeout_option_entry.set_description(_("How long to wait (in seconds) before timing out while waiting for data from demuxer (default 5)."));
	
	Glib::OptionGroup option_group(PACKAGE_NAME, "", _("Show Me TV help options"));
	option_group.add_entry(verbose_option_entry, verbose_logging);
	option_group.add_entry(safe_mode_option_entry, safe_mode);
	option_group.add_entry(minimised_option_entry, minimised_mode);
	option_group.add_entry(disable_epg_thread_option_entry, disable_epg_thread);
	option_group.add_entry(disable_epg_option_entry, disable_epg);
	option_group.add_entry(no_screensaver_inhibit_option_entry, no_screensaver_inhibit);
	option_group.add_entry(devices_option_entry, devices);
	option_group.add_entry(read_timeout_option_entry, read_timeout);

	Glib::OptionContext option_context;
	option_context.set_summary(ME_TV_SUMMARY);
	option_context.set_description(ME_TV_DESCRIPTION);
	option_context.set_main_group(option_group);
		
	try
	{
		option_context.parse(argc, argv);

		UniqueApp* unique_application = unique_app_new_with_commands(
			"org.lamothe.me-tv", NULL,
		    "run", (UniqueCommand)1,
			(char*)NULL);

		if (unique_app_is_running(unique_application))
		{
			g_debug("Me TV is already running");

			UniqueMessageData* message = unique_message_data_new();
			unique_app_send_message(unique_application, (UniqueCommand)1, message);
		}
		else
		{
			g_signal_connect(unique_application, "message-received", G_CALLBACK (on_message_received), NULL);

			Application application;
			application.run();
		}
	}
	catch (const Glib::Exception& exception)
	{
		g_message("Exception: %s", exception.what().c_str());
	}
	catch (...)
	{
		g_message(_("An unhandled error occurred"));		
	}
	
	return 0;
}
