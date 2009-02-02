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

#ifdef ENABLE_MPLAYER_ENGINE

#include "exception.h"
#include "application.h"
#include <gdk/gdkx.h>
#include <sys/wait.h>
#include <iomanip>

#define KILL_SLEEP_TIME		100000
#define KILL_SLEEP_TIMEOUT  2000000

#define MPLAYER_CACHE_KB   4096

MplayerEngine::MplayerEngine()
{
	pid = -1;
	standard_input = -1;
	mute_state = false;
	monitoraspect = Gdk::screen_width()/(double)Gdk::screen_height();
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
	argv.push_back("-stop-xscreensaver");
	argv.push_back("-use-filedir-conf");
	argv.push_back("-cache");
	argv.push_back(Glib::ustring::format(std::fixed, MPLAYER_CACHE_KB));
	argv.push_back("-ao");
	argv.push_back(application.get_string_configuration_value("mplayer.audio_driver"));
	argv.push_back("-softvol");
	argv.push_back("-vo");
	argv.push_back(application.get_string_configuration_value("mplayer.video_driver"));
	argv.push_back("-framedrop");
	argv.push_back("-hardframedrop");
	argv.push_back("-tskeepbroken");
	argv.push_back("-zoom");
	argv.push_back("-monitoraspect");
	argv.push_back(Glib::ustring::format(std::fixed, std::setprecision(6), monitoraspect));
	argv.push_back("-vf");
	argv.push_back("pp=fd");
	argv.push_back("-wid");
	argv.push_back(Glib::ustring::compose("%1", get_window_id()));
	argv.push_back(Glib::ustring::compose("%1", mrl));

	try
	{
		Glib::spawn_async_with_pipes("/tmp",
			argv,
			Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD | 
			Glib::SPAWN_STDOUT_TO_DEV_NULL | Glib::SPAWN_STDERR_TO_DEV_NULL,
			sigc::slot< void >(),
			&pid,
			&standard_input,
			NULL,
			NULL);

		mute_state = false;
		g_debug("Spawned mplayer on pid %d", pid);
	}
	catch (const Exception& exception)
	{
		g_debug("Failed to spawn mplayer!");
		stop();
	}
}

void MplayerEngine::write(const Glib::ustring& text)
{
	g_debug("Writing '%s' (%d)", text.c_str(), text.length());
	if (::write(standard_input, text.c_str(), text.length()) <= 0)
	{
		g_debug("Failed to write command '%s'", text.c_str());
	}
}

void MplayerEngine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting MPlayer");
		if (standard_input != -1)
		{
			write("pause\n");
			write("quit\n");
		}
		else
			kill(pid, SIGHUP);

		gboolean done = false;
		gint elapsed_time = 0;
		while (!done)
		{
			pid_t pid_result = ::waitpid(pid, NULL, WNOHANG);
			
			if (pid_result < 0)
			{
				done = true;
				g_debug("Failed to wait for MPlayer to exit: waitpid returned %d", pid_result);
			}
			else if (pid_result == 0)
			{
				if (elapsed_time >= KILL_SLEEP_TIMEOUT)
				{
					g_debug("Timeout (%d usec) elapsed, exiting wait loop", KILL_SLEEP_TIMEOUT);
					done = true;
				}
				else
				{
					g_debug("MPlayer is still running, waiting for %d usec", KILL_SLEEP_TIME);
					usleep(KILL_SLEEP_TIME);
					elapsed_time += KILL_SLEEP_TIME;
				}
			}
			else // pid_result > 0
			{
				done = true;
				g_debug("MPlayer has terminated normally");
				g_spawn_close_pid(pid);
				pid = -1;
			}
		}
		if (pid != -1)
		{
			g_debug("Killing MPlayer");
			kill(pid, SIGKILL);
			usleep(KILL_SLEEP_TIME);
			g_spawn_close_pid(pid);
			pid = -1;
		}
		g_debug("MPlayer stop complete");
	}
	if (standard_input != -1)
	{
		close(standard_input);
		standard_input = -1;
	}
}

gboolean MplayerEngine::is_running()
{
	int result = -1;
	
	if (pid != -1)
	{
		result = ::waitpid(pid, NULL, WNOHANG | __WALL);
	}
	
	return result == 0;
}

void MplayerEngine::set_mute_state(gboolean state)
{
	if (state != mute_state)
	{
		g_debug(state ? "Muting" : "Unmuting");
		write("mute\n");
		mute_state = state;
	}
}

#endif
