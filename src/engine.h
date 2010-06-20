/*
 * Copyright (C) 2010 Michael Lamothe
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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "me-tv.h"
#include <gtkmm.h>
#include <X11/X.h>

class Engine
{
public:
	typedef enum
	{
		AUDIO_CHANNEL_STATE_BOTH = 0,
		AUDIO_CHANNEL_STATE_LEFT = 1,
		AUDIO_CHANNEL_STATE_RIGHT = 2
	} AudioChannelState;

private:
	gint				pid;
	gboolean			mute_state;
	gboolean			deinterlacer_state;
	AudioChannelState	audio_channel_state;
	gint				audio_stream;
	gint				subtitle_stream;
	Glib::ustring		mrl;
	Window				window;

	void sendKeyEvent(int keycode, int modifiers);

public:
	void play(const Glib::ustring& mrl);
	void stop();
	void pause(gboolean state);
	void set_mute_state(gboolean state);
	void set_volume(float value);
	void set_subtitle_stream(gint stream);
	void set_audio_stream(gint stream);
	void set_audio_channel_state(AudioChannelState state);
	gboolean is_running();

	Engine();
	~Engine();
};

#endif

