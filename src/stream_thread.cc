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

#include "stream_thread.h"
#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
#include <alsa/asoundlib.h>
#include <gdk/gdk.h>
#include "me-tv.h"

class AlsaException : public Exception
{
private:
	static Glib::ustring get_message(const char* strerror, const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		gchar* text = g_strdup_vprintf(fmt, ap);
		Glib::ustring message = text;
		message += ": ";
		message += strerror;
		g_free(text);
		va_end(ap);
		
		return message;
	}
		
public:
	AlsaException(const char* fmt, ...) : Exception(get_message(snd_strerror(errno),fmt)) {}
};
		
void AudioStreamThread::run ()
{
	snd_pcm_t* handle = NULL;
	
	if (audio_stream == NULL)
	{
		throw Exception("ASSERT: audio_stream is NULL");
	}
	
	mutex.lock();

	AVCodec* audio_codec = avcodec_find_decoder(audio_stream->codec->codec_id);
	if (audio_codec == NULL)
	{
		throw Exception("Failed to find audio codec");
	}
	g_debug("AUDIO CODEC: '%s'", audio_codec->name);

	if (avcodec_open(audio_stream->codec, audio_codec) < 0)
	{
		throw Exception("Failed to open audio codec");
	}
	mutex.unlock();

	const gchar* device = "default";
	
	if (snd_pcm_open (&handle, device, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		throw AlsaException("Failed open audio device '%s'", device);
	}
	
	guint result = snd_pcm_set_params(
		handle,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		audio_stream->codec->channels,
		audio_stream->codec->sample_rate,
		1,
		500000);
	if (result < 0)
	{
		throw AlsaException("Failed to set hardware parameters");
	}
	
	if (snd_pcm_prepare(handle) < 0)
	{
		throw AlsaException("Failed to prepare audio interface for use");
	}

	gsize number_audio_buffers = 10;
	gsize buffer_size = ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2) * number_audio_buffers;
	DECLARE_ALIGNED(16, uint8_t, data[buffer_size]);
	gsize count = 0;
	gsize total_audio_buffer_size = 0;
	while (!is_terminated())
	{			
		if (audio_packet_buffers.is_finished())
		{
			terminate();
		}
		else if (audio_packet_buffers.is_empty())
		{
			usleep(10000);
		}
		else
		{
			mutex.lock();
			AVPacket* packet = audio_packet_buffers.pop();

			int audio_buffer_length = sizeof(data) - total_audio_buffer_size;
			gint ffmpeg_result = avcodec_decode_audio2(
				audio_stream->codec,
				(int16_t*)data + total_audio_buffer_size,
				&audio_buffer_length,
				packet->data, packet->size);
			
			total_audio_buffer_size += audio_buffer_length/2;
			
			if (ffmpeg_result < 0)
			{
				throw Exception("Failed to decode audio");
			}
			
			if (audio_buffer_length < 0)
			{
				throw Exception("ASSERT: audio_buffer_size < 0, not implemented");
			}
			av_free_packet(packet);
			delete packet;

			mutex.unlock();
			
			if (count++ > number_audio_buffers)
			{
				snd_pcm_sframes_t frames = snd_pcm_writei (handle, data, total_audio_buffer_size/2);
				if (frames == -EPIPE)
				{
					snd_pcm_prepare(handle);
					snd_pcm_start(handle);
				}
				else if (frames < 0)
				{
					throw AlsaException("Failed to write to ALSA audio interface");
				}
				
				total_audio_buffer_size = 0;
				count = 0;
			}
		}
	}
	g_debug("Audio thread finished");
	
	snd_pcm_close(handle);
	avcodec_close(audio_stream->codec);
}
	
AudioStreamThread::AudioStreamThread(Glib::Timer& timer, Glib::Mutex& mutex, PacketQueue& audio_packet_buffers)
		: audio_packet_buffers(audio_packet_buffers), mutex(mutex), timer(timer)
{
	audio_stream = NULL;
}

void AudioStreamThread::start(AVStream* stream)
{
	audio_stream = stream;		
	Thread::start();
}
		
void VideoStreamThread::draw(Glib::RefPtr<Gdk::Window>& window, Glib::RefPtr<Gdk::GC>& gc)
{
	gint width = 0;
	gint height = 0;
	
	AVCodecContext* video_codec_context = video_stream->codec;
	
	window->get_size(width, height);
		
	if (previous_width != width || previous_height != height)
	{
		if (img_convert_ctx != NULL)
		{
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
		}
		
		previous_width = width;
		previous_height = height;
	}

	if (img_convert_ctx == NULL)
	{
		gdouble aspect_ratio = video_codec_context->width / (gdouble)video_codec_context->height;
		if (sample_aspect_ratio.num != 0 && sample_aspect_ratio.den != 0)
		{
			aspect_ratio *= sample_aspect_ratio.num / (gdouble)sample_aspect_ratio.den;
		}
		
		if (width > height * aspect_ratio)
		{
			video_width = height * aspect_ratio;
			video_height = height;
			startx = (width - video_width)/2;
			starty = 0;
		}
		else
		{
			video_width = width;
			video_height = width / aspect_ratio; 
			startx = 0;
			starty = (height - video_height)/2;
		}

		img_convert_ctx = sws_getContext(
			video_codec_context->width, video_codec_context->height, video_codec_context->pix_fmt,
			video_width, video_height, PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
		if (img_convert_ctx == NULL)
		{
			throw Exception("Cannot initialise the conversion context");
		}
		
		delete [] video_buffer;
		video_buffer = NULL;
		
		gsize video_buffer_size = avpicture_get_size(PIX_FMT_RGB24, video_width, video_height);
		video_buffer = new guchar[video_buffer_size];

		avpicture_fill(&picture, video_buffer, PIX_FMT_RGB24, video_width, video_height);

		window->draw_rectangle(gc, true, 0, 0, width, height);
	}

	sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,
		video_codec_context->height, picture.data, picture.linesize);
	
	window->draw_rgb_image(gc,
		startx, starty,
		video_width, video_height,
		Gdk::RGB_DITHER_NONE,
		video_buffer,
		video_width * 3);
}

void VideoStreamThread::run()
{
	gdouble		frame_rate = 0;
	gboolean	video_frame_finished = false;

	Glib::RefPtr<Gdk::Window> window = drawing_area.get_window();
	Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(window);
	
	mutex.lock();		
	
	AVCodec* video_codec = avcodec_find_decoder(video_stream->codec->codec_id);
	if (video_codec == NULL)
	{
		throw Exception("Failed to find video codec");
	}
	g_debug("VIDEO CODEC: '%s'", video_codec->name);

	if (video_codec->capabilities & CODEC_CAP_TRUNCATED)
	{
		video_stream->codec->flags |= CODEC_FLAG_TRUNCATED;
	}

	if (video_stream->r_frame_rate.den != 0 && video_stream->r_frame_rate.num != 0)
	{
		frame_rate = av_q2d(video_stream->r_frame_rate);
	}
	else
	{
		frame_rate = 1/av_q2d(video_stream->codec->time_base);
	}
	g_debug("FRAME RATE: %5.2f", frame_rate);
	
	if (avcodec_open(video_stream->codec, video_codec) < 0)
	{
		throw Exception("Failed to open video codec");
	}

	frame = avcodec_alloc_frame();
	if (frame == NULL)
	{
		throw Exception("Failed to allocate frame");
	}

	sample_aspect_ratio = video_stream->codec->sample_aspect_ratio;
	mutex.unlock();

	guint frame_count = 0;
	guint interval = 1000000 / frame_rate;

	while (!is_terminated())
	{
		video_frame_finished = false;
		
		if (video_packet_buffer.is_finished())
		{
			terminate();
		}
		else if (video_packet_buffer.is_empty())
		{
			usleep(1000);
		}
		else
		{
			gboolean drop_frame = false;
			gdouble elapsed = timer.elapsed();
			guint frame_wanted = elapsed * frame_rate;
			gint frame_diff = frame_count - frame_wanted;
			
			if (frame_diff > 2)
			{
				guint delay = (frame_diff-1) / frame_rate * 1000000;
				usleep(delay);
			}
			else if (frame_diff < -2)
			{
				drop_frame = true;
			}

			frame_count++;
			
			AVPacket* packet = video_packet_buffer.pop();
			
			mutex.lock();
			
			avcodec_decode_video(video_stream->codec,
				frame, &video_frame_finished, packet->data, packet->size);
			if (video_frame_finished && !drop_frame)
			{
				GdkLock gdk_lock;
				draw(window, gc);
			}
			av_free_packet(packet);
			delete packet;
			
			mutex.unlock();
		}
	}
	g_debug("Video thread finished");
	
	mutex.lock();
	av_free(frame);
	avcodec_close(video_stream->codec);
	mutex.unlock();
}

VideoStreamThread::VideoStreamThread(Glib::Timer& timer, Gtk::DrawingArea& drawing_area, Glib::Mutex& mutex, PacketQueue& video_packet_buffer)
		: video_packet_buffer(video_packet_buffer), mutex(mutex), timer(timer), drawing_area(drawing_area)
{
	img_convert_ctx	= NULL;
	video_buffer	= NULL;
	video_stream	= NULL;
	previous_width	= 0;
	previous_width	= 0;
	video_width		= 0;
	video_height	= 0;
	startx			= 0;
	starty			= 0;
	frame			= NULL;
}

VideoStreamThread::~VideoStreamThread()
{
	if (video_buffer != NULL)
	{
		delete [] video_buffer;
		video_buffer = NULL;
	}
}
	
void VideoStreamThread::start(AVStream* stream)
{
	video_stream = stream;
	Thread::start();
}
		
void StreamThread::run()
{
	Glib::Timer			timer;
	AVFormatContext*	format_context;
	PacketQueue			video_packet_buffer;
	VideoStreamThread*	video_stream_thread = NULL;
	PacketQueue			audio_packet_buffer;
	AudioStreamThread*	audio_stream_thread = NULL;
	gint				video_stream_index;
	gint				audio_stream_index;

	const gchar* filename = mrl.c_str();
	
	video_stream_index = -1;
	audio_stream_index = -1;
	
	mutex.lock();
	av_register_all();
	
	if (av_open_input_file(&format_context, filename, NULL, 0, NULL) != 0)
	{
		Glib::ustring message = "Failed to open input file: ";
		message += filename;
		throw Exception(message);
	}

	if (av_find_stream_info(format_context)<0)
	{
		throw Exception("Couldn't find stream information");
	}

	dump_format(format_context, 0, filename, false);
	
	for (guint index = 0; index < format_context->nb_streams; index++)
	{
		AVStream* stream = format_context->streams[index];
		switch (stream->codec->codec_type)
		{
		case CODEC_TYPE_VIDEO:
			video_stream_index = index;
			break;
		case CODEC_TYPE_AUDIO:
			audio_stream_index = index;
			g_debug("Stream language: '%s'", stream->language);
			break;
		}
	}
	
	if (video_stream_index != -1 && drawing_area != NULL)
	{
		g_debug("Creating video stream thread");
		video_stream_thread = new VideoStreamThread(timer, *drawing_area, mutex, video_packet_buffer);
		video_stream_thread->start(format_context->streams[video_stream_index]);
	}
	
	if (audio_stream_index != -1)
	{
		g_debug("Creating audio stream thread");
		audio_stream_thread = new AudioStreamThread(timer, mutex, audio_packet_buffer);
		audio_stream_thread->start(format_context->streams[audio_stream_index]);
	}
	mutex.unlock();
	
	timer.start();
	
	g_debug("Entering stream loop");
	while (!is_terminated())
	{
		AVPacket packet;
		
		gint result = 0;

		mutex.lock();
		result = av_read_frame(format_context, &packet);
		mutex.unlock();
		
		if (result < 0)
		{
			terminate();
		}
		else
		{
			// Make sure that we don't get too far ahead
			while ((audio_packet_buffer.get_size() > 10 || video_packet_buffer.get_size() > 10) && !is_terminated())
			{
				usleep(1000);
			}
			
			if (packet.stream_index == video_stream_index)
			{
				video_packet_buffer.push(&packet);
			}
			else if (packet.stream_index == audio_stream_index)
			{
				audio_packet_buffer.push(&packet);
			}
			else
			{
				av_free_packet(&packet);
			}
		}
	}
	g_debug("Stream thread finished");

	video_packet_buffer.set_finished();
	audio_packet_buffer.set_finished();
	
	if (video_stream_thread != NULL)
	{
		video_stream_thread->join();
		delete video_stream_thread;
	}
	
	if (audio_stream_thread != NULL)
	{
		audio_stream_thread->join();
		delete audio_stream_thread;
	}
	
	av_close_input_file(format_context);
	format_context = NULL;
}

StreamThread::StreamThread()
{
	drawing_area = NULL;
}
	
void StreamThread::start(const Glib::ustring& mrl)
{
	this->mrl = mrl;
	g_debug("Starting stream thread");
	Thread::start();
}

void StreamThread::set_drawing_area(Gtk::DrawingArea* d)
{
	drawing_area = d;
}