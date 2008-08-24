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

#include "mplayer_engine.h"
#include "exception.h"
#include <gdk/gdkx.h>
#include <sys/wait.h>

class Parameters
{
private:
	GSList* parameters;
	guint length;

public:
	Parameters()
	{
		length = 0;
		parameters = NULL;
	}
	
	~Parameters()
	{
		clear();
	}
	
	void clear()
	{
		GSList* iterator = parameters;
		while (iterator != NULL)
		{
			g_free(iterator->data);
			iterator = g_slist_next(iterator);
		}
		g_slist_free(parameters);
		
		length = 0;
		parameters = NULL;
	}
	
		void add(const Glib::ustring& value)
	{
		parameters = g_slist_append(parameters, g_strdup(value.c_str()));
		length++;
	}
	
	void add(gint value)
	{
		parameters = g_slist_append(parameters, g_strdup_printf("%d", value));
		length++;
	}
	
	Glib::ustring join(const gchar* separator = " ")
	{
		gchar** arguments = get_arguments();
		gchar* joined = g_strjoinv(separator, arguments);
		Glib::ustring result = joined;
		g_free(joined);
		delete [] arguments;
		
		return result;
	}
	
	gchar** get_arguments()
	{
		gchar** arguments = new gchar*[length + 1];
		guint index = 0;

		GSList* iterator = parameters;
		while (iterator != NULL)
		{
			arguments[index++] = (gchar*)iterator->data;
			iterator = g_slist_next(iterator);
		}
		arguments[index++] = NULL;
				
		return arguments;
	}
};

MplayerEngine::MplayerEngine(int window_id) : window_id(window_id)
{
}

MplayerEngine::~MplayerEngine()
{
	stop();
}

void MplayerEngine::play(const Glib::ustring& mrl)
{
	Parameters parameters;

	parameters.add("mplayer");
	parameters.add("-really-quiet");
	parameters.add("-slave");
	parameters.add("-softvol");
	parameters.add("-stop-xscreensaver");
	parameters.add("-vo");
	parameters.add("x11");
	parameters.add("-zoom");
	parameters.add("-vf");
	parameters.add("pp=fd");
	parameters.add("-wid");
	parameters.add(window_id);
	parameters.add(mrl);

	Glib::ustring command_line = parameters.join();
	g_debug("Command: %s", command_line.c_str());
	GError* error = NULL;
	gchar** arguments = parameters.get_arguments();
	
	g_spawn_async_with_pipes(
							 NULL,
							 arguments,
							 NULL,
							 G_SPAWN_SEARCH_PATH,
							 NULL,
							 NULL,
							 &pid,
							 &standard_input,
							 NULL,
							 NULL,
							 &error
							 );
	
	delete [] arguments;
	
	if (error != NULL)
	{
		throw Exception(error->message);
	}
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
