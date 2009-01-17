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

#ifndef __VLC_ENGINE_H__
#define __VLC_ENGINE_H__

#include "engine.h"

#ifdef USE_VLC_0_8_6_API
#include <vlc/libvlc.h>
#else
#include <vlc/vlc.h>
#endif

class VlcEngine : public Engine
{
private:
	libvlc_instance_t*		instance;
	libvlc_exception_t		exception;
	gint				window_id;
	gboolean mute_state;
	gint volume;


#ifndef USE_VLC_0_8_6_API
	libvlc_media_player_t*	media_player;
#endif

	void check_exception();
	void play(const Glib::ustring& mrl);
	void stop();
	void mute(gboolean state);
	void set_volume(gint newlevel);
	void set_size(gint width, gint height) {};
	void set_audio_channel(guint channel) {};
	gboolean is_running(); 

public:
	VlcEngine(int window_id);
	~VlcEngine();
};

#endif
