/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "engine.h"
#include "exception.h"
#include "application.h"
#include "me-tv.h"
#include <gtkmm.h>
#include <gdk/gdkx.h>
#include <sys/wait.h>

constexpr auto KILL_SLEEP_TIME = 100000;
constexpr auto KILL_SLEEP_TIMEOUT = 2000000;

Engine::Engine() {
	pid = -1;
	mute_state = false;
	deinterlacer_state = false;
	audio_channel_state = AUDIO_CHANNEL_STATE_BOTH;
	audio_stream = 0;
	subtitle_stream = -1;
	Gtk::DrawingArea * drawing_area_video = NULL;
	get_application().get_builder()->get_widget("drawing_area_video", drawing_area_video);
	window = GDK_WINDOW_XID(drawing_area_video->get_window()->gobj());
	if (window == 0) { throw Exception(_("Window ID was 0")); }
}

Engine::~Engine() { stop(); }

void Engine::play(Glib::ustring const & mrl) {
	Application & application = get_application();
	this->mrl = mrl;
	g_debug("Engine::play(\"%s\")", mrl.c_str());
	// TODO: Put some protections in place to ensure no fails here. Especially the engine search.
	StringList argv;
	argv.push_back(Glib::ustring::compose("me-tv-player-%1", engine));
	argv.push_back(Glib::ustring::compose("fifo://%1", mrl));
	argv.push_back(Glib::ustring::compose("%1", window));
	argv.push_back(configuration_manager.get_string_value("video_driver"));
	argv.push_back(configuration_manager.get_string_value("audio_driver"));
	argv.push_back(configuration_manager.get_string_value("deinterlace_type"));
	argv.push_back(mute_state ? "true" : "false");
	argv.push_back(Glib::ustring::compose("%1", audio_stream));
	argv.push_back(Glib::ustring::compose("%1", subtitle_stream));
	argv.push_back(Glib::ustring::compose("%1", (int)(volume * 100)));
	g_debug("=================================================");
	for (auto && arg: argv) { g_debug("> %s", arg.c_str()); }
	g_debug("=================================================");
	try {
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
	catch (Exception const & exception) {
		g_debug("Failed to spawn engine: %s", exception.what().c_str());
		stop();
	}
}

void Engine::stop() {
	if (pid != -1) {
		g_debug("Quitting Engine");
		kill(pid, SIGHUP);
		gboolean done = false;
		gint elapsed_time = 0;
		while (!done) {
			pid_t pid_result = ::waitpid(pid, NULL, WNOHANG);
			if (pid_result < 0) {
				done = true;
				g_debug("Failed to wait for engine to exit: waitpid returned %d", pid_result);
			}
			else if (pid_result == 0) {
				if (elapsed_time >= KILL_SLEEP_TIMEOUT) {
					g_debug("Timeout (%d usec) elapsed, exiting wait loop", KILL_SLEEP_TIMEOUT);
					done = true;
				}
				else {
					g_debug("Engine is still running, waiting for %d usec", KILL_SLEEP_TIME);
					usleep(KILL_SLEEP_TIME);
					elapsed_time += KILL_SLEEP_TIME;
				}
			}
			else { // pid_result > 0
				done = true;
				g_debug("Engine has terminated normally");
				g_spawn_close_pid(pid);
				pid = -1;
			}
		}
		if (pid != -1) {
			g_debug("Killing engine");
			kill(pid, SIGKILL);
			usleep(KILL_SLEEP_TIME);
			g_spawn_close_pid(pid);
			pid = -1;
		}
		g_debug("Engine stop complete");
	}
}

gboolean Engine::is_running() {
	int result = -1;
	if (pid != -1) {
		result = ::waitpid(pid, NULL, WNOHANG | __WALL);
	}
	return result == 0;
}

void Engine::sendKeyEvent(int keycode, int modifiers) {
	XKeyEvent event;
	Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	event.display = display;
	event.window = window;
	event.root = XDefaultRootWindow(display);
	event.subwindow = None;
	event.time = CurrentTime;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.same_screen = True;
	event.keycode = keycode;
	event.state = modifiers;
	event.type = KeyPress;
	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*)(void*)&event);
	event.type = KeyRelease;
	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*)(void*)&event);
}

void Engine::pause(gboolean state) {
	g_debug("Engine::pause(%s)", state ? "true" : "false");
	if (pid != -1) {
		g_debug(state ? "Sending pause" : "Sending unpause");
		sendKeyEvent(
			XK_space,
			state ? XK_Control_L : 0);
	}
}

void Engine::set_mute_state(gboolean state) {
	g_debug("Engine::set_mute_state(%s)", state ? "true" : "false");
	if (state != mute_state) {
		mute_state = state;
		if (pid != -1) {
			g_debug(state ? "Sending mute" : "Sending unmute");
			sendKeyEvent(
				XK_m,
				state ? 0 : XK_Control_L);
		}
	}
}

void Engine::set_audio_channel_state(AudioChannelState state) {
	if (audio_channel_state != state) {
		audio_channel_state = state;
		if (pid != -1) {
			int modifiers = 0;
			switch(state) {
			case AUDIO_CHANNEL_STATE_LEFT:
				modifiers = XK_Control_L;
				break;
			case AUDIO_CHANNEL_STATE_RIGHT:
				modifiers = XK_Control_R;
				break;
			default:
				modifiers = XK_Control_L & XK_Control_R;
				break;
			}
			sendKeyEvent(XK_a, modifiers);
		}
	}
}

void Engine::set_volume(float value) {
	g_debug("Setting volume: %f", value);
	volume = value;
	if (pid != -1) {
		// At value = 1.0 the key will be a colon (XK_colon)
		sendKeyEvent(XK_0 + (int)(value * 10), XK_Control_L & XK_Control_R);
	}
}

void Engine::set_audio_stream(gint stream) {
	if (audio_stream != stream) {
		audio_stream = stream;
		if (pid != -1) {
			sendKeyEvent(XK_0 + stream, XK_Control_L);
		}
	}
}

void Engine::set_subtitle_stream(gint stream) {
	if (subtitle_stream != stream) {
		subtitle_stream = stream;
		if (pid != -1) {
			sendKeyEvent(XK_0 + stream, XK_Control_R);
		}
	}
}
