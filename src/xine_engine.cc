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

#ifdef ENABLE_XINE_ENGINE

#define KILL_SLEEP_TIME		100000
#define KILL_SLEEP_TIMEOUT  2000000

XineEngine::XineEngine()
{
	pid = -1;
	standard_input = -1;
	requested_mute_state = false;
	actual_mute_state = false;
	audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;
	audio_stream = 0;
	subtitle_stream = 0;
}

XineEngine::~XineEngine()
{
	stop();
}

void XineEngine::play(const Glib::ustring& mrl)
{
	Application& application = get_application();
	this->mrl = mrl;
	
	g_debug("XineEngine::play(\"%s\")", mrl.c_str());

	StringList argv;
	argv.push_back("xine");
	argv.push_back("--config");
	argv.push_back(Glib::build_filename(application.get_application_dir(), "/xine.config"));
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
	if (audio_channel_state != AUDIO_CHANNEL_STATE_BOTH)
	{
		argv.push_back("--post");
		
		gint channel = -1;
		switch (audio_channel_state)
		{
			case AUDIO_CHANNEL_STATE_LEFT: channel = 0; break;
			case AUDIO_CHANNEL_STATE_RIGHT: channel = 1; break;
			default: channel = -1; break;
		}
		
		Glib::ustring dual_audio_parameter = Glib::ustring::compose("upmix_mono:channel=%1", channel);
		argv.push_back(dual_audio_parameter);
	}
	if (audio_stream != 0)
	{
		argv.push_back("-a");
		argv.push_back(Glib::ustring::compose("%1", audio_stream));
	}

	// Initial window size hack
	gint width, height;
	get_drawing_area_video()->get_window()->get_size(width, height);
	argv.push_back("--geometry");
	argv.push_back(Glib::ustring::compose("%1x%2", width, height));

	argv.push_back("--wid");
	argv.push_back(Glib::ustring::compose("%1", get_window_id()));
	argv.push_back(Glib::ustring::compose("fifo://%1#gui.audio_mixer_method:Software", mrl));

	g_debug("=================================================");
	for (StringList::iterator i = argv.begin(); i != argv.end(); i++)
	{
		g_debug("> %s", (*i).c_str());
	}
	g_debug("=================================================");
										  
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
			
		set_mute_state(requested_mute_state);
		g_debug("Spawned xine on pid %d", pid);
	}
	catch (const Exception& exception)
	{
		g_debug("Failed to spawn xine: %s", exception.what().c_str());
		stop();
	}
}

void XineEngine::write(const Glib::ustring& text)
{
	g_debug("Writing '%s' (%zu)", text.c_str(), text.length());
	if (::write(standard_input, text.c_str(), text.length()) <= 0)
	{
		g_debug("Failed to write command '%s'", text.c_str());
	}
}

void XineEngine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting Xine");
		if (standard_input != -1)
		{
			write("pause\n");
			write("quit\n");
		}
		kill(pid, SIGHUP);
		
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
	if (standard_input != -1)
	{
		close(standard_input);
		standard_input = -1;
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

void XineEngine::set_mute_state(gboolean state)
{
	if (pid != -1)
	{
		if (state != actual_mute_state)
		{
			g_debug(state ? "Muting" : "Unmuting");
			write("mute\n");
			actual_mute_state = state;
		}
	}
	requested_mute_state = state;
}

void XineEngine::restart()
{
	stop();
	play(mrl);
}

void XineEngine::set_audio_channel_state(AudioChannelState state)
{
	if (audio_channel_state != state)
	{
		audio_channel_state = state;
		if (pid != -1)
		{
			restart();
		}
	}
}

void XineEngine::set_audio_stream(guint stream)
{
	if (audio_stream != stream)
	{
		audio_stream = stream;
		if (pid != -1)
		{
			restart();
		}
	}
}

void XineEngine::set_subtitle_stream(guint stream)
{
	if (subtitle_stream != stream)
	{
		subtitle_stream = stream;
		if (pid != -1)
		{
			restart();
		}
	}
}

#endif
