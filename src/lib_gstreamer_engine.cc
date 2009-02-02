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

#include "lib_gstreamer_engine.h"

#ifdef ENABLE_LIBGSTREAMER_ENGINE

#include "application.h"
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>

LibGStreamerEngine::LibGStreamerEngine()
{
	static gboolean initialised = false;

	if (!initialised)
	{
		gst_init(0, NULL);
		initialised = true;
	}
	
	gchar* version = gst_version_string();
	g_debug("%s", version);
	g_free(version);

	player	= create_element("playbin", "player");
	sink	= create_element("xvimagesink", "sink");
}

LibGStreamerEngine::~LibGStreamerEngine()
{
	if (player != NULL)
	{
		gst_object_unref (GST_OBJECT (player));
		player = NULL;
	}
}

gboolean LibGStreamerEngine::on_bus_message(GstBus *bus, GstMessage *message, gpointer data)
{
	LibGStreamerEngine* engine = (LibGStreamerEngine*)data;
	GError* error = NULL;
	gchar* debug = NULL;
	Glib::ustring type = GST_MESSAGE_TYPE_NAME(message);
	Glib::ustring text;
	
	switch (GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_WARNING:
			gst_message_parse_warning(message, &error, &debug);
			text = debug;
			break;
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(message, &error, &debug);
			text = debug;
			break;
		default:
			break;
	}
	g_message("GStreamer %s: %s", type.c_str(), text.c_str());
	
	return TRUE;
}

void LibGStreamerEngine::play(const Glib::ustring& mrl)
{	
	Glib::ustring uri = "file://" + mrl;
	
	g_object_set (G_OBJECT (player), "video_sink", sink, (gchar*)NULL);
	g_object_set (G_OBJECT (player), "uri", uri.c_str(), (gchar*)NULL);
	g_object_set (G_OBJECT (sink), "force-aspect-ratio", true, (gchar*)NULL);
		
	GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (player));
	gst_bus_add_watch (bus, on_bus_message, (gpointer)this);
	gst_object_unref (bus);

	gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (sink), get_window_id());

	gst_element_set_state (player, GST_STATE_PLAYING);
}

void LibGStreamerEngine::stop()
{
	g_debug("Stopping GStreamer pipeline");
	gst_element_set_state (player, GST_STATE_NULL);
}

gboolean LibGStreamerEngine::is_running()
{
	GstState state;
	gst_element_get_state(player, &state, NULL, NULL);
	return state != GST_STATE_NULL;
}

GstElement* LibGStreamerEngine::create_element(const Glib::ustring& factoryname, const gchar *name)
{
	GstElement* element = gst_element_factory_make(factoryname.c_str(), name);
	
	if (element == NULL)
	{
		Glib::ustring message = Glib::ustring::compose(N_("Failed to create GStreamer element '%1'"), name);
		throw Exception(message);
	}
	
	return element;
}

void LibGStreamerEngine::set_mute_state(gboolean state)
{
	g_object_set (G_OBJECT (player), "volume", state ? 0.0 : 10.0, (gchar*)NULL);
}


#endif