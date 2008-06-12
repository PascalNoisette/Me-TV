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

#ifndef __SINK_H__
#define __SINK_H__

#include "packet_queue.h"
#include "thread.h"
#include <list>
#include <gtkmm.h>

class Pipeline;

class Sink : public Thread
{
private:
	Pipeline& pipeline;
public:
	Sink(Pipeline& pipeline) : Thread("Sink"), pipeline(pipeline) {}
	Pipeline& get_pipeline() { return pipeline; }
};

typedef std::list<Sink*> SinkList;

class AlsaAudioThread : public Thread
{
private:
	PacketQueue&	audio_packet_buffers;
	AVStream*		audio_stream;
	Glib::Timer&	timer;
		
	void run ();
		
public:	
	AlsaAudioThread(Glib::Timer& timer, PacketQueue& audio_packet_buffers);
	void start(AVStream* stream);
};

class GtkVideoThread : public Thread
{
private:
	Gtk::DrawingArea&		drawing_area;
	struct SwsContext*		img_convert_ctx;
	guint					aspect_ratio_type;
	AVRational				sample_aspect_ratio;
	gint					previous_width;
	gint					previous_height;
	gint					video_width;
	gint					video_height;
	gint					startx;
	gint					starty;
	guchar*					video_buffer;
	PacketQueue&			video_packet_buffer;	
	AVPicture				picture;
	AVStream*				video_stream;
	AVFrame*				frame;
	gdouble					frame_rate;
	Glib::Timer&			timer;
		
	void draw(Glib::RefPtr<Gdk::Window>& window, Glib::RefPtr<Gdk::GC>& gc);
	void run();

public:
	GtkVideoThread(Glib::Timer& timer, PacketQueue& video_packet_buffer, Gtk::DrawingArea& drawing_area);
	~GtkVideoThread();
	void start(AVStream* stream);
};

class GtkAlsaSink : public Sink
{
private:
	PacketQueue&		packet_queue;
	PacketQueue			video_packet_queue;
	PacketQueue			audio_packet_queue;
	gint				video_stream_index;
	gint				audio_stream_index;
	GtkVideoThread*		video_thread;
	AlsaAudioThread*	audio_thread;
	Glib::Timer			timer;

	void run();
		
public:
	GtkAlsaSink(Pipeline& pipeline, Gtk::DrawingArea& drawing_area);
	~GtkAlsaSink();
};

#endif
