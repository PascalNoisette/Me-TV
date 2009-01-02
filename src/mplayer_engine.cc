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

#include "mplayer_engine.h"
#include "exception.h"
#include "application.h"
#include <gdk/gdkx.h>
#include <sys/wait.h>

MplayerEngine::MplayerEngine(int window_id) : window_id(window_id)
{
}

MplayerEngine::~MplayerEngine()
{
	stop();
}

void MplayerEngine::play(const Glib::ustring& mrl)
{
        Application& application = get_application();

        StringList argv;
	argv.push_back("mplayer");
	argv.push_back("-really-quiet");
	argv.push_back("-slave");
	argv.push_back("-softvol");
	argv.push_back("-stop-xscreensaver");
	argv.push_back("-vo");
	argv.push_back(application.get_string_configuration_value("mplayer.video_driver"));
	argv.push_back("-ao");
	argv.push_back(application.get_string_configuration_value("mplayer.audio_driver"));
	argv.push_back("-zoom");
	argv.push_back("-vf");
	argv.push_back("pp=fd");
        argv.push_back("-wid");
        argv.push_back(Glib::ustring::compose("%1", window_id));
        argv.push_back(Glib::ustring::compose("%1", mrl));

        Glib::spawn_async_with_pipes("/tmp",
                argv,
                Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
                sigc::slot< void >(),
                &pid,
                &standard_input,
                NULL,
                NULL);
}

void MplayerEngine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting MPlayer");
		::write(standard_input, "q\n", 2);
		
		waitpid(pid, NULL, 0);
		pid = -1;
		g_debug("MPlayer has quit");
	}
}

void MplayerEngine::mute(gboolean state)
{
	if (standard_input != -1)
	{
		g_debug(state ? "Muting" : "Unmuting");
		::write(standard_input, "m\n", 2);
	}
}

void MplayerEngine::expose()
{
}

gboolean MplayerEngine::is_running()
{
        return pid != -1;
}

