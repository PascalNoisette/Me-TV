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

#include "engine.h"
#include "exception.h"
#include "application.h"
#include <libgnome/libgnome.h>
#include <gdk/gdkx.h>
#include <sys/wait.h>

#define KILL_SLEEP_TIME		100000
#define KILL_SLEEP_TIMEOUT  2000000

Engine::Engine(const Glib::ustring& engine_type)
{
	pid = -1;
	mute_state = false;
	audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;
	audio_stream = 0;
	subtitle_stream = -1;
	type = engine_type;

	Gtk::DrawingArea* drawing_area_video = NULL;
	get_application().get_builder()->get_widget("drawing_area_video", drawing_area_video);

	window = GDK_WINDOW_XID(drawing_area_video->get_window()->gobj());
	if (window == 0)
	{
		throw Exception(_("Window ID was 0"));
	}
}

Engine::~Engine()
{
	stop();
}

void Engine::play(const Glib::ustring& mrl)
{
	Application& application = get_application();
	this->mrl = mrl;
	
	g_debug("Engine::play(\"%s\")", mrl.c_str());

	StringList argv;
	argv.push_back("me-tv-" + type);
	argv.push_back(Glib::ustring::compose("fifo://%1", mrl));
	argv.push_back(Glib::ustring::compose("%1", window));
	argv.push_back(application.get_string_configuration_value("xine.video_driver"));
	argv.push_back(application.get_string_configuration_value("xine.audio_driver"));
	argv.push_back(mute_state ? "mute" : "unmute");

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
			Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
			sigc::slot< void >(),
			&pid,
			NULL,
			NULL,
			NULL);

		set_mute_state(mute_state);
		g_debug("Spawned engine on pid %d", pid);
	}
	catch (const Exception& exception)
	{
		g_debug("Failed to spawn engine: %s", exception.what().c_str());
		stop();
	}
}

void Engine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting Engine");
		kill(pid, SIGHUP);
		
		gboolean done = false;
		gint elapsed_time = 0;
		while (!done)
		{
			pid_t pid_result = ::waitpid(pid, NULL, WNOHANG);
			
			if (pid_result < 0)
			{
				done = true;
				g_debug("Failed to wait for engine to exit: waitpid returned %d", pid_result);
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
					g_debug("Engine is still running, waiting for %d usec", KILL_SLEEP_TIME);
					usleep(KILL_SLEEP_TIME);
					elapsed_time += KILL_SLEEP_TIME;
				}
			}
			else // pid_result > 0
			{
				done = true;
				g_debug("Engine has terminated normally");
				g_spawn_close_pid(pid);
				pid = -1;
			}
		}
		if (pid != -1)
		{
			g_debug("Killing engine");
			kill(pid, SIGKILL);
			usleep(KILL_SLEEP_TIME);
			g_spawn_close_pid(pid);
			pid = -1;
		}
		g_debug("Engine stop complete");
	}
}

gboolean Engine::is_running()
{
	int result = -1;
	
	if (pid != -1)
	{
		result = ::waitpid(pid, NULL, WNOHANG | __WALL);
	}
	
	return result == 0;
}

void Engine::sendKeyEvent(int keycode, int modifiers)
{
	XKeyEvent event;
	Display* display = GDK_DISPLAY();

	event.display     = display;
	event.window      = window;
	event.root        = XDefaultRootWindow(display);
	event.subwindow   = None;
	event.time        = CurrentTime;
	event.x           = 1;
	event.y           = 1;
	event.x_root      = 1;
	event.y_root      = 1;
	event.same_screen = True;
	event.keycode     = XKeysymToKeycode(display, keycode);
	event.state       = modifiers;

	event.type = KeyPress;
	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

	event.type = KeyRelease;
	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
}

void Engine::set_mute_state(gboolean state)
{
	g_debug("Engine::set_mute_state(%s)", state ? "true" : "false");
	if (state != mute_state)
	{
		mute_state = state;
		if (pid != -1)
		{
			g_debug(state ? "Sending mute" : "Sending unmuting");
			sendKeyEvent(
				state ? XK_m : XK_m,
				state ? 0 : XK_Shift_L);
		}
	}
}

void Engine::set_audio_channel_state(AudioChannelState state)
{
	if (audio_channel_state != state)
	{
		audio_channel_state = state;
		if (pid != -1)
		{
			int modifiers = 0;
			switch(state)
			{					
			case AUDIO_CHANNEL_STATE_LEFT:
				modifiers &= XK_Shift_L;
				break;
				
			case AUDIO_CHANNEL_STATE_RIGHT:
				modifiers &= XK_Shift_R;
				break;

			default:
				modifiers &= XK_Shift_L & XK_Shift_R;
				break;
			}
			sendKeyEvent(XK_a, modifiers);
		}
	}
}

void Engine::set_audio_stream(guint stream)
{
	if (audio_stream != stream)
	{
		audio_stream = stream;
		if (pid != -1)
		{
			sendKeyEvent(XK_1 + stream, XK_Shift_L);
		}
	}
}

void Engine::set_subtitle_stream(gint stream)
{
	if (subtitle_stream != stream)
	{
		subtitle_stream = stream;
		if (pid != -1)
		{
			sendKeyEvent(XK_1 + stream, XK_Shift_R);
		}
	}
}

