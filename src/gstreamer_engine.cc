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

#include "gstreamer_engine.h"
#include "exception.h"
#include <gdk/gdkx.h>

static gboolean bus_call (GstBus* bus, GstMessage* message, gpointer data)
{
	GstMessageType message_type = GST_MESSAGE_TYPE (message);
	const gchar* message_type_name = gst_message_type_get_name(message_type);
	Glib::ustring text;
	
	switch (message_type)
	{
	case GST_MESSAGE_EOS:
		text = "End-of-stream";
		break;
	case GST_MESSAGE_ERROR:
	{
		gchar *debug;
		GError *err;

		gst_message_parse_error (message, &err, &debug);
		g_free (debug);

		text = err->message;
		g_error_free (err);
		break;
	}
	default:
		break;
	}
	
	if (!text.empty())
	{
		g_debug("GStreamer '%s' Message: %s", message_type_name, text.c_str());
	}
	
	return TRUE;
}

GstElement* GStreamerEngine::create_element(const Glib::ustring& factoryname, const Glib::ustring& name)
{
	GstElement* element = gst_element_factory_make(factoryname.c_str(), name.c_str());
	
	if (element == NULL)
	{
		throw Exception(Glib::ustring::compose(N_("Failed to create GStreamer element '%1'"), name));
	}
	
	gst_bin_add(GST_BIN(pipeline), element);
	
	return element;
}

GStreamerEngine::GStreamerEngine()
{
	pipeline		= gst_pipeline_new("pipeline");
	source			= create_element("filesrc", "source");
	GstElement* q1	= create_element("queue", "q1");
	decoder			= create_element("decodebin2", "decoder");
	volume			= create_element("volume", "volume");
	deinterlace		= create_element("ffdeinterlace", "deinterlace");
	GstElement* q2	= create_element("queue", "q2");
	video_sink		= create_element("xvimagesink", "video_sink");
	GstElement* q3	= create_element("queue", "q3");
	audio_sink		= create_element("gconfaudiosink", "audio_sink");

	GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE(pipeline));
	gst_bus_add_watch (bus, bus_call, this);
	gst_object_unref (bus);
	
	g_signal_connect(G_OBJECT(decoder), "pad-added", G_CALLBACK(connect_dynamic_pad), this);
	g_object_set (G_OBJECT (video_sink), "force-aspect-ratio", true, NULL);
	
	gst_element_link_many(source, q1, decoder, NULL);
	gst_element_link_many(deinterlace, q2, video_sink, NULL);
	gst_element_link_many(volume, q3, audio_sink, NULL);
}

GStreamerEngine::~GStreamerEngine()
{
	stop();
	gst_object_unref(GST_OBJECT(pipeline));
}

void GStreamerEngine::connect_dynamic_pad (GstElement* element, GstPad* pad, GStreamerEngine* engine)
{
	// Video
	GstPad* video_sink_pad = gst_element_get_pad (engine->deinterlace, "sink");
	if (video_sink_pad == NULL)
	{
		throw Exception("Failed to get video sink pad");
	}
	gst_pad_link (pad, video_sink_pad);
	gst_object_unref (video_sink_pad);

	// Audio
	GstPad* audio_sink_pad = gst_element_get_pad (engine->volume, "sink");
	if (audio_sink_pad == NULL)
	{
		throw Exception("Failed to get audio sink pad");
	}
	gst_pad_link (pad, audio_sink_pad);
	gst_object_unref (audio_sink_pad);
}

void GStreamerEngine::play(Gtk::Widget& widget, const Glib::ustring& filename)
{
	stop();

	if (!filename.empty())
	{
		int window_id = GDK_WINDOW_XID(widget.get_window()->gobj());
		gst_x_overlay_set_xwindow_id (GST_X_OVERLAY(video_sink), window_id);

		g_debug("GStreamer file source: %s", filename.c_str());
		g_object_set (G_OBJECT(source), "location", filename.c_str(), NULL);
		g_debug("Starting pipeline");
		gst_element_set_state (pipeline, GST_STATE_PLAYING);
	}
}
	
void GStreamerEngine::stop()
{
	g_debug("Stopping pipeline");
	gst_element_set_state (pipeline, GST_STATE_NULL);
}

void GStreamerEngine::mute(gboolean state)
{
	g_object_set(G_OBJECT(volume), "mute", state, NULL);
}
