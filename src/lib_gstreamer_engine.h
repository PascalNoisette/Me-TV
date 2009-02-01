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
 
#ifndef __LIB_GSTREAMER_ENGINE_H__
#define __LIB_GSTREAMER_ENGINE_H__

#include "config.h"

#ifdef ENABLE_LIBGSTREAMER_ENGINE

#include "engine.h"
#include <gst/gst.h>

class LibGStreamerEngine : public Engine
{
private:
	gint				pid;
	gint				standard_input;
	gboolean			mute_state;
	AudioChannelState	audio_channel_state;
	GstElement*			player;
	GstElement*			sink;

	void play(const Glib::ustring& mrl);
	void stop();
	void set_audio_stream(guint stream) {}
	void set_mute_state(gboolean state) {}
	void set_audio_channel_state(AudioChannelState state) {}
	gboolean is_running();

	GstElement* create_element(const Glib::ustring& factoryname, const gchar *name);
	static gboolean on_bus_message(GstBus *bus, GstMessage *message, gpointer data);
public:
	LibGStreamerEngine();
	~LibGStreamerEngine();
};

#endif

#endif
