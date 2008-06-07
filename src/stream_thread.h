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

#ifndef __STREAM_THREAD_H__
#define __STREAM_THREAD_H__

#include <ffmpeg/avformat.h>
#include <glibmm.h>
#include <gtkmm.h>
#include <gdkmm.h>
#include <queue>
#include "thread.h"

class PacketQueue
{
private:
	gboolean				finished;
	std::queue<AVPacket*>	queue;
	Glib::Mutex				mutex;
		
public:
	PacketQueue()
	{
		finished = false;
	}
		
	void push(AVPacket* packet)
	{
		Glib::Mutex::Lock lock(mutex);
		
		if (av_dup_packet(packet) < 0)
		{
			throw Exception("Failed to dup packet");
		}
		AVPacket* new_packet = new AVPacket();
		*new_packet = *packet;
		queue.push(new_packet);
	}

	AVPacket* pop()
	{
		Glib::Mutex::Lock lock(mutex);
		AVPacket* front = queue.front();
		queue.pop();
		return front;
	}

	gsize get_size()
	{
		Glib::Mutex::Lock lock(mutex);
		return queue.size();
	}
		
	gboolean is_empty()
	{
		Glib::Mutex::Lock lock(mutex);
		return queue.empty();
	}
		
	gboolean set_finished()
	{
		finished = true;
	}

	gboolean is_finished()
	{
		return finished && is_empty();
	}
};

class AudioStreamThread : public Thread
{
private:
	PacketQueue&	audio_packet_buffers;
	AVStream*		audio_stream;
	Glib::Mutex&	mutex;
	Glib::Timer&	timer;
		
	void run ();
		
public:	
	AudioStreamThread(Glib::Timer& timer, Glib::Mutex& mutex, PacketQueue& audio_packet_buffers);
	void start(AVStream* stream);
};

class VideoStreamThread : public Thread
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
	Glib::Mutex&			mutex;
	Glib::Timer&			timer;
		
	void draw(Glib::RefPtr<Gdk::Window>& window, Glib::RefPtr<Gdk::GC>& gc);
	void run();

public:
	VideoStreamThread(Glib::Timer& timer, Gtk::DrawingArea& drawing_area,
					  Glib::Mutex& mutex, PacketQueue& video_packet_buffer);
	~VideoStreamThread();
	void start(AVStream* stream);
};

class StreamThread : public Thread
{
private:
	Glib::ustring		mrl;
	Glib::Mutex			mutex;
	Gtk::DrawingArea*	drawing_area;
		
	void run();
		
public:
	StreamThread();
	void set_drawing_area(Gtk::DrawingArea* d);
	void start(const Glib::ustring& mrl);
};

#endif
