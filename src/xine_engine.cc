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

#include "xine_engine.h"
#include "exception.h"
#include "application.h"
#include <libgnome/libgnome.h>
#include <gdk/gdkx.h>
#include <sys/wait.h>

XineEngine::XineEngine(int display_window_id) : window_id(display_window_id)
{
	pid = -1;
	standard_input = -1;
}

XineEngine::~XineEngine()
{
	stop();
}

void XineEngine::play(const Glib::ustring& mrl)
{
	StringList argv;
	argv.push_back("/usr/bin/xine");
	argv.push_back("--no-splash");
	argv.push_back("--no-logo");
	argv.push_back("-D");
	argv.push_back("-A");
	argv.push_back("oss");
	argv.push_back("-V");
	argv.push_back("xshm");
	argv.push_back("--no-mouse");
	argv.push_back("--stdctl");
	argv.push_back("--wid");
	argv.push_back(Glib::ustring::compose("%1", window_id));
	argv.push_back(Glib::ustring::compose("fifo://%1", mrl));

	Glib::spawn_async_with_pipes("/tmp",
		argv,
		Glib::SpawnFlags(0),
		sigc::slot< void >(),
		&pid,
		&standard_input,
		NULL,
		NULL);
}

void XineEngine::write(const Glib::ustring& text)
{
	::write(standard_input, text.c_str(), text.length());
}

void XineEngine::stop()
{
	if (pid != -1)
	{
		g_debug("Quitting Xine");
		if (standard_input != -1)
		{
			write("q");
			standard_input = -1;
		}
		waitpid(pid, NULL, 0);
		g_spawn_close_pid(pid);
		pid = -1;
		g_debug("Xine has quit");
	}
}

void XineEngine::mute(gboolean state)
{
	g_debug(state ? "Muting" : "Unmuting");
}

void XineEngine::expose()
{
}
