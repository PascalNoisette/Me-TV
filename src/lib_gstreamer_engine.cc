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
#include <gst/gstelement.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>

LibGStreamerEngine::LibGStreamerEngine()
{
	static gboolean initialised = false;
	volume = 1.3;
	pipeline = NULL;
	
	if (!initialised)
	{
		gst_init(0, NULL);
		initialised = true;
	}
	
	gchar* version = gst_version_string();
	g_debug("%s", version);
	g_free(version);
}

LibGStreamerEngine::~LibGStreamerEngine()
{
}

gboolean LibGStreamerEngine::on_bus_message(GstBus *bus, GstMessage *message, gpointer data)
{
	LibGStreamerEngine* engine = (LibGStreamerEngine*)data;
	gboolean show = true;
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
			show = false;
			break;
	}
	
	if (show)
	{
		g_message("GStreamer %s: %s", type.c_str(), text.c_str());
	}
	
	return TRUE;
}

void LibGStreamerEngine::play(const Glib::ustring& mrl)
{
	Glib::ustring command_line = get_application().get_string_configuration_value("gstreamer_command_line");
	Glib::ustring spec = Glib::ustring::compose(command_line, mrl);
	g_debug("GStreamer command line: %s", command_line.c_str());

	GError* error = NULL;
	pipeline = gst_parse_launch(spec.c_str(), &error);
	if (error != NULL)
	{
		Glib::ustring message = Glib::ustring::compose("Failed to launch GStreamer command: %1", error->message);
		throw Exception(message);
	}
	
	GstElement* audiosink = gst_bin_get_by_name(GST_BIN(pipeline), "videosink");
	if (audiosink == NULL)
	{
		throw Exception("Failed to get videosink element");
	}

	GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, on_bus_message, (gpointer)this);
	gst_object_unref (bus);

	gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (audiosink), get_window_id());

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

void LibGStreamerEngine::stop()
{
	g_debug("Stopping GStreamer pipeline");
	gst_element_set_state (pipeline, GST_STATE_NULL);

	if (pipeline != NULL)
	{
		gst_object_unref (GST_OBJECT (pipeline));
		pipeline = NULL;
	}
}

gboolean LibGStreamerEngine::is_running()
{
	GstState state;
	gst_element_get_state(pipeline, &state, NULL, NULL);
	return state != GST_STATE_NULL;
}

void LibGStreamerEngine::set_mute_state(gboolean state)
{
	gdouble mute_volume = state ? 0.0 : volume;
	set_volume(mute_volume);
}

void LibGStreamerEngine::set_volume(gdouble value)
{
	if (GST_IS_ELEMENT(pipeline))
	{
		GstElement* volume = gst_bin_get_by_name(GST_BIN(pipeline), "volume");
		if (volume == NULL)
		{
			throw Exception("Failed to get volume element");
		}
		
		g_object_set(G_OBJECT(volume), "volume", value, (gchar *)NULL);
	}
}

#endif
