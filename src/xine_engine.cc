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

#include "xine_engine.h"
#include "exception.h"
#include "application.h"
#include <libgnome/libgnome.h>
#include <gdk/gdkx.h>
#include <sys/wait.h>

#define KILL_SLEEP_TIME		100000
#define KILL_SLEEP_TIMEOUT  2000000

XineEngine::XineEngine(int display_window_id) : window_id(display_window_id)
{
	pid = -1;
	standard_input = -1;
	mute_state = false;
}

XineEngine::~XineEngine()
{
	stop();
}

void XineEngine::play(const Glib::ustring& mrl)
{
	Application& application = get_application();

	StringList argv;
	argv.push_back("xine");
	argv.push_back("--no-splash");
	argv.push_back("--no-logo");
	argv.push_back("--no-gui");
	argv.push_back("--no-mouse");
	argv.push_back("--stdctl");
	argv.push_back("-D");
	argv.push_back("-A");
	argv.push_back(application.get_string_configuration_value("xine.audio_driver"));
	argv.push_back("-V");
	argv.push_back(application.get_string_configuration_value("xine.video_driver"));

	// Initial window size hack
	gint width, height;
	Gtk::DrawingArea* drawing_area_video = dynamic_cast<Gtk::DrawingArea*>(application.get_glade()->get_widget("drawing_area_video"));
	drawing_area_video->get_window()->get_size(width, height);
	argv.push_back("--geometry");
	argv.push_back(Glib::ustring::compose("%1x%2", width, height));

	argv.push_back("--wid");
	argv.push_back(Glib::ustring::compose("%1", window_id));
	argv.push_back(Glib::ustring::compose("fifo://%1", mrl));

	Glib::spawn_async_with_pipes("/tmp",
		argv,
		Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
		sigc::slot< void >(),
		&pid,
		&standard_input,
		NULL,
		NULL);

	mute_state = false;
	mute(mute_state);
										  
	g_debug("Spawned xine on pid %d", pid);
}

void XineEngine::write(const Glib::ustring& text)
{
	g_debug("Writing '%s' (%d)", text.c_str(), text.length());
	::write(standard_input, text.c_str(), text.length());
}

void XineEngine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting Xine");
		if (standard_input != -1)
		{
			write("stop playback\n");
			write("quit\n");
			standard_input = -1;
		}
		
		gboolean done = false;
		gint elapsed_time = 0;
		while (!done)
		{
			pid_t pid_result = ::waitpid(pid, NULL, WNOHANG);
			
			if (pid_result < 0)
			{
				done = true;
				g_debug("Failed to wait for xine to exit: waitpid returned %d", pid_result);
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
					g_debug("xine is still running, waiting for %d usec", KILL_SLEEP_TIME);
					usleep(KILL_SLEEP_TIME);
					elapsed_time += KILL_SLEEP_TIME;
				}
			}
			else // pid_result > 0
			{
				done = true;
				g_debug("xine has terminated normally");
				g_spawn_close_pid(pid);
				pid = -1;
			}
		}
		if (pid != -1)
		{
			g_debug("Killing Xine");
			kill(pid, SIGKILL);
			usleep(KILL_SLEEP_TIME);
			g_spawn_close_pid(pid);
			pid = -1;
		}
		g_debug("Xine stop complete");
	}
}

gboolean XineEngine::is_running()
{
	int result = -1;
	
	if (pid != -1)
	{
		result = ::waitpid(pid, NULL, WNOHANG | __WALL);
	}
	
	return result == 0;
}

void XineEngine::mute(gboolean state)
{
	if (state != mute_state)
	{
		g_debug(state ? "Muting" : "Unmuting");
		write("mute\n");
		mute_state = state;
	}
}

