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

#ifndef __GSTREAMER_ENGINE_H__
#define __GSTREAMER_ENGINE_H__

#include "engine.h"
#include <gst/gstplugin.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>

class GStreamerEngine : public Engine
{
private:
	GstElement*	pipeline;
	GstElement*	source;
	GstElement*	decoder;
	GstElement*	volume;
	GstElement*	deinterlace;
	GstElement*	video_sink;
	GstElement*	audio_sink;

	GstElement* create_element(const Glib::ustring& factoryname, const Glib::ustring& name);
	static void connect_dynamic_pad (GstElement* element, GstPad* pad, GStreamerEngine* engine);
	void stop();
		
	void play(Gtk::Widget& widget, const Glib::ustring& filename);
	void mute(gboolean state);

public:
	GStreamerEngine();
	~GStreamerEngine();
};

#endif
