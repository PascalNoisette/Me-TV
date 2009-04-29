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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "me-tv.h"
#include <libgnomeuimm.h>

class Engine
{
private:
	int					window_id;
	Gtk::DrawingArea*	drawing_area_video;
public:
	typedef enum
	{
		AUDIO_CHANNEL_STATE_BOTH = 0,
		AUDIO_CHANNEL_STATE_LEFT = 1,
		AUDIO_CHANNEL_STATE_RIGHT = 2
	} AudioChannelState;

	Engine();
	virtual ~Engine() {};
	
	virtual void play(const Glib::ustring& mrl) = 0;
	virtual void stop() = 0;
	virtual void set_mute_state(gboolean state) = 0;
	virtual void set_audio_stream(guint stream) = 0;
	virtual void set_audio_channel_state(AudioChannelState state) = 0;
	virtual void set_subtitle_stream(guint stream) = 0;
	virtual gboolean is_running() = 0;

	gint get_window_id();
	Gtk::DrawingArea* get_drawing_area_video();
};

#endif
