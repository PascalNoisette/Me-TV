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

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "me-tv.h"
#include <gdk/gdkx.h>
#include <gst/gstplugin.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>

class Engine
{
public:
	virtual void play(Glib::RefPtr<Gdk::Window> window, const Glib::ustring& mrl) = 0;
	virtual void stop() = 0;
};

class GStreamerEngine : public Engine
{
private:
	GstElement*	player;
	GstElement*	sink;

	GstElement* create_element(const Glib::ustring& factoryname, const Glib::ustring& name)
	{
		GstElement* element = gst_element_factory_make(factoryname.c_str(), name.c_str());
		
		if (element == NULL)
		{
			throw Exception(Glib::ustring::compose(N_("Failed to create GStreamer element '%1'"), name));
		}
		
		return element;
	}
		
public:
	GStreamerEngine(int argc, char* argv[])
	{
		gst_init(&argc, &argv);
		g_debug(gst_version_string());

		player		= create_element("playbin", "player");
		sink		= create_element("xvimagesink", "sink");
		g_object_set (G_OBJECT (player), "video_sink", sink, NULL);
	}
		
	void play(Glib::RefPtr<Gdk::Window> window, const Glib::ustring& mrl)
	{
		stop();

		int window_id = GDK_WINDOW_XID(window->gobj());
		gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (sink), window_id);

		g_object_set (G_OBJECT (player), "uri", mrl.c_str(), NULL);
		gst_element_set_state (player, GST_STATE_PLAYING);
	}
		
	void stop()
	{
		gst_element_set_state (GST_ELEMENT(player), GST_STATE_NULL);
	}
};

#endif
