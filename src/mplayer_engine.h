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

#ifndef __MPLAYER_ENGINE_H__
#define __MPLAYER_ENGINE_H__

#include "me-tv.h"

#ifdef ENABLE_MPLAYER_ENGINE

#include "engine.h"

class MplayerEngine : public Engine
{
private:
	gint pid;
	gint standard_input;
	gboolean mute_state;
	gdouble monitoraspect;

	void play(const Glib::ustring& mrl);
	void stop();
	void set_mute_state(gboolean state);
	void expose();
	void set_audio_stream(guint stream) {};
	void set_audio_channel_state(AudioChannelState state) {};
	void write(const Glib::ustring& text);
	gboolean is_running();

public:
	MplayerEngine();
	~MplayerEngine();
};

#endif

#endif
