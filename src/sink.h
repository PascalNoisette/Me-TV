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

#include <avformat.h>
#include "thread.h"
#include <alsa/asoundlib.h>
#include <list>
#include <gtkmm.h>

class Pipeline;
#include <queue>

class Buffer
{
private:
	gsize	length;
	guchar*	data;
public:
	Buffer(gulong length) : length(length) { data = new guchar[length]; }
	~Buffer() { delete [] data; }
		
	gsize get_length() const { return length; }
	guchar* get_data() const { return data; }
};

class Size
{
public:
	Size() : width(0), height(0) {}
	Size(guint width, guint height) : width(width), height(height) {}
	guint width;
	guint height;
};

class VideoImage
{
private:
	Buffer image_data;
	Size size;
	guint bits_per_pixel;
public:
	VideoImage(guint width, guint height, guint bits_per_pixel) :
		size(width, height), image_data(width * height * bits_per_pixel) {}
	guchar* get_image_data() const { return image_data.get_data(); }
	const Size& get_size() const { return size; }
};

template<class T>
class Queue
{
private:
	Glib::StaticRecMutex    mutex;
	std::queue<T>			queue;
public:
	Queue<T>()
	{
		g_static_rec_mutex_init(mutex.gobj());
	}

	void push(T& data)
	{
		Glib::RecMutex::Lock lock(mutex);
		queue.push(data);
	}

	T& front()
	{
		Glib::RecMutex::Lock lock(mutex);
		return queue.front();
	}
		
	T& pop()
	{
		Glib::RecMutex::Lock lock(mutex);
		if (queue.size() == 0)
		{
			throw Exception("Buffer queue underrun");
		}
		T& buffer = queue.front();
		queue.pop();
		return buffer;
	}

	gsize get_size()
	{
		Glib::RecMutex::Lock lock(mutex);
		return queue.size();
	}
};

typedef Queue<Buffer*> BufferQueue;
typedef Queue<VideoImage*> VideoImageQueue;

class AlsaAudioThread : public Thread
{
private:
	BufferQueue&	audio_buffer_queue;
	Glib::Timer&	timer;
	snd_pcm_t*		handle;
	guint			sample_rate;
		
	void run();
		
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

	Rectangle calculate_drawing_rectangle(VideoImage* video_image);
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
