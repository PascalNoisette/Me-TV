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

#include "thread.h"
#include "buffer_queue.h"
#include <list>
#include <gtkmm.h>
#include <avformat.h>
#include <alsa/asoundlib.h>

class Pipeline;

class AlsaAudioThread : public Thread
{
private:
	BufferQueue&	audio_buffer_queue;
	Glib::Timer&	timer;
	snd_pcm_t*		handle;
		
	void run ();
		
public:	
	AlsaAudioThread(Glib::Timer& timer, BufferQueue& audio_buffer_queue, guint channels, guint sample_rate);
	~AlsaAudioThread();
};

class Rectangle
{
public:
	Rectangle() : x(0), y(0), width(0), height(0) {}
	gint x, y;
	guint width, height;
};

class VideoOutput
{
protected:
	Glib::RefPtr<Gdk::Window> window;
public:
	VideoOutput(Glib::RefPtr<Gdk::Window> window) : window(window) {}
	virtual void draw(VideoImage* video_image) = 0;
	void get_size(gint& width, gint& height) { window->get_size(width, height); }
	virtual void on_size(guint width, guint height) = 0;

	Rectangle calculate_drawing_rectangle(VideoImage* video_image)
	{
		Rectangle rectangle;
		int window_width = 0, window_height = 0;
		window->get_size(window_width, window_height);
		const Size& size = video_image->get_size();
		rectangle.width = size.width;
		rectangle.height = size.height;
		return rectangle;
	}

};

class VideoThread : public Thread
{
private:
	VideoImageQueue&	video_image_queue;	
	Glib::Timer&		timer;
	VideoOutput*		video_output;
	gdouble				frame_rate;
		
	void run();

public:
	VideoThread(Glib::Timer& timer, VideoImageQueue& video_image_queue, VideoOutput* video_output, gdouble frame_rate);
};

class Sink
{
private:
	Pipeline&					pipeline;
	VideoImageQueue				video_image_queue;
	BufferQueue					audio_buffer_queue;
	gint						video_stream_index;
	gint						audio_stream_index;
	VideoThread*				video_thread;
	AlsaAudioThread*			audio_thread;
	Glib::Timer					timer;
	Glib::StaticRecMutex		mutex;
	Gtk::DrawingArea&			drawing_area;
	struct SwsContext*			img_convert_ctx;
	AVPicture					picture;
	AVStream*					video_stream;
	AVFrame*					frame;
	gint						startx;
	gint						starty;
	gint						previous_width;
	gint						previous_height;
	gint						video_width;
	gint						video_height;
	guchar*						video_buffer;
	guint						video_buffer_size;
	gdouble						frame_rate;
	AVRational					sample_aspect_ratio;
	AVStream*					audio_stream;
	VideoOutput*				video_output;
	guint						pixel_format;
		
	void destroy();
	void push_video_packet(AVPacket* packet);
	void push_audio_packet(AVPacket* packet);
		
public:
	Sink(Pipeline& pipeline, Gtk::DrawingArea& drawing_area);
	~Sink();
	
	void reset_timer();
	void write(AVPacket* packet);
};

typedef std::list<Sink*> SinkList;

#endif
