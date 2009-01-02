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

#ifndef __XINE_ENGINE_H__
#define __XINE_ENGINE_H__

#include "engine.h"

class XineEngine : public Engine
{
private:
	gint pid;
	gint window_id;
	gint standard_input;
	gboolean mute_state;

	void play(const Glib::ustring& mrl);
	void stop();
	void mute(gboolean state);
	void expose();
	void set_size(gint width, gint height) {};
	void set_audio_channel(guint channel) {};
	void write(const Glib::ustring& text);
	gboolean is_running();

public:
	XineEngine(int window_id);
	~XineEngine();
};

#endif
