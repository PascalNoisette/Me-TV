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

#include "sink.h"
#include "me-tv.h"
#include "pipeline_manager.h"
#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <alsa/asoundlib.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XShm.h>

#define GUID_YUV12_PLANAR 0x32315659

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

void AlsaAudioThread::run()
{
	snd_pcm_t* handle = NULL;
	
	if (audio_stream == NULL)
	{
		throw Exception("ASSERT: audio_stream is NULL");
	}

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
		AVPacket* packet = audio_packet_queue.pop();
		
		if (packet == NULL)
		{
			terminate();
		}
		else
		{
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
	
AlsaAudioThread::AlsaAudioThread(Glib::Timer& timer, PacketQueue& audio_packet_queue, AVStream* audio_stream) :
	Thread("Alsa Audio"), audio_packet_queue(audio_packet_queue), timer(timer), audio_stream(audio_stream)
{
}

class GtkVideoOutput : public VideoOutput
{
private:
	Glib::RefPtr<Gdk::GC> gc;
public:
	GtkVideoOutput(Glib::RefPtr<Gdk::Window>& window) : VideoOutput(window), gc(Gdk::GC::create(window))
	{
	}
	
	void clear(guint width, guint height)
	{
		window->draw_rectangle(gc, true, 0, 0, width, height);
	}

	void draw(gint x, gint y, guint width, guint height, guchar* buffer, gsize stride)
	{
		window->draw_rgb_image(gc, x, y, width, height, Gdk::RGB_DITHER_NONE, buffer, stride);
	}
};

class XvVideoOutput : public VideoOutput
{
private:
	gint			xv_port;
	XvImage*		image;
	Display*		display;
	GC				gc;

public:
	XvVideoOutput(Glib::RefPtr<Gdk::Window>& window) : VideoOutput(window)
	{
		xv_port = -1;
		image = NULL;
		display = NULL;
		
		guint number_of_adapters = 0;
		XvAdaptorInfo* adapter_info = NULL;
		gc = XCreateGC(display, GDK_WINDOW_XID(window->gobj()), 0, 0);
		
		display = GDK_DISPLAY();
		guint result = XvQueryAdaptors(display, GDK_ROOT_WINDOW(), &number_of_adapters, &adapter_info);

		for (int i = 0; i < number_of_adapters; i++)
		{
			g_debug("name: '%s', type: %s%s%s%s%s, ports: %ld, first port: %ld",
				adapter_info[i].name,
				(adapter_info[i].type & XvInputMask)	? "input | "	: "",
				(adapter_info[i].type & XvOutputMask)	? "output | "	: "",
				(adapter_info[i].type & XvVideoMask)	? "video | "	: "",
				(adapter_info[i].type & XvStillMask)	? "still | "	: "",
				(adapter_info[i].type & XvImageMask)	? "image | "	: "",
				adapter_info[i].num_ports,
				adapter_info[i].base_id);
			
			for (int j = 0; j < adapter_info[i].num_formats; j++)
			{
				g_debug("* depth = %d, visual = %ld",
					adapter_info[i].formats[j].depth,
					adapter_info[i].formats[j].visual_id);
			}
			
			xv_port = adapter_info[i].base_id;
		}
		
		if (xv_port == -1)
		{
			throw Exception(_("Failed to find suitable Xv port"));
		}
		
		g_debug("Selected port %d", xv_port);
		
		gint width, height;
		get_size(width, height);
		image = XvCreateImage(display, xv_port, 0x32595559, NULL, width, height);
	}
	
	void clear(guint width, guint height)
	{
	}

	void draw(gint x, gint y, guint width, guint height, guchar* buffer, gsize stride)
	{
		g_debug("Picture");
		XvPutImage(display, xv_port, window, gc, image,
		    0, 0, image->width, image->height,
		    x, y, width, height);
	}
};

void VideoThread::draw()
{
	gint width = 0;
	gint height = 0;
	
	AVCodecContext* video_codec_context = video_stream->codec;
	
	video_output->get_size(width, height);
		
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

		video_output->clear(video_width, video_height);
	}
		
	sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,
		video_codec_context->height, picture.data, picture.linesize);
	
	video_output->draw(startx, starty,
		video_width, video_height,
		video_buffer,
		video_width * 3);
}

void VideoThread::run()
{
	Glib::RefPtr<Gdk::Window> window = drawing_area.get_window();

	gboolean	video_frame_finished = false;
	guint		frame_count = 0;
	guint		interval = 1000000 / frame_rate;
	gboolean	first = true;

	Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
	Glib::ustring video_output_name = client->get_string(GCONF_PATH"/video_output");
	if (video_output_name == "GTK")
	{
		video_output = new GtkVideoOutput(window);
	}
	else if (video_output_name == "Xv")
	{
		video_output = new XvVideoOutput(window);
	}
	else
	{
		throw Exception(Glib::ustring::compose(_("Unknown video output '%1'"), video_output_name));
	}
	
	while (!is_terminated())
	{
		video_frame_finished = false;
		
		AVPacket* packet = video_packet_queue.pop();
		
		if (packet == NULL)
		{
			terminate();
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
						
			avcodec_decode_video(video_stream->codec,
				frame, &video_frame_finished, packet->data, packet->size);
			if (first && !video_frame_finished)
			{
				g_debug("Flushing");
				avcodec_flush_buffers(video_stream->codec);
				avcodec_decode_video(video_stream->codec,
					frame, &video_frame_finished, packet->data, packet->size);
			}
			if (video_frame_finished && !drop_frame)
			{
				first = false;
				GdkLock gdk_locks;
				draw();
			}
			av_free_packet(packet);
			delete packet;
		}
	}
	g_debug("Video thread finished");
	
	if (video_output != NULL)
	{
		delete video_output;
		video_output = NULL;
	}
	
	avcodec_close(video_stream->codec);
}

VideoThread::VideoThread(Glib::Timer& timer, PacketQueue& video_packet_queue, AVStream* video_stream, Gtk::DrawingArea& drawing_area) :
	Thread("GTK Video"), video_packet_queue(video_packet_queue), timer(timer), drawing_area(drawing_area), video_stream(video_stream)
{
	img_convert_ctx	= NULL;
	video_buffer	= NULL;
	previous_width	= 0;
	previous_width	= 0;
	video_width		= 0;
	video_height	= 0;
	startx			= 0;
	starty			= 0;
	frame			= NULL;
	video_output	= NULL;

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
}

VideoThread::~VideoThread()
{
	if (frame != NULL)
	{
		av_free(frame);
	}
	
	if (video_buffer != NULL)
	{
		delete [] video_buffer;
		video_buffer = NULL;
	}
}

Sink::Sink(Pipeline& pipeline, Gtk::DrawingArea& drawing_area) :
	Thread("Sink"), pipeline(pipeline), packet_queue(pipeline.get_packet_queue())
{
	g_static_rec_mutex_init(mutex.gobj());

	video_thread = NULL;
	audio_thread = NULL;
	video_stream_index = -1;
	audio_stream_index = -1;

	Source& source = pipeline.get_source();
	gsize stream_count = source.get_stream_count();
	
	for (guint index = 0; index < stream_count; index++)
	{
		AVStream* stream = source.get_stream(index);
		switch (stream->codec->codec_type)
		{
		case CODEC_TYPE_VIDEO:
			if (video_stream_index == -1)
			{
				video_stream_index = index;
			}
			break;
		case CODEC_TYPE_AUDIO:
			if (audio_stream_index == -1)
			{
				audio_stream_index = index;
			}
			g_debug("Stream language: '%s'", stream->language);
			break;
		}
	}
	
	if (video_stream_index >= 0)
	{
		video_thread = new VideoThread(timer, video_packet_queue, source.get_stream(video_stream_index), drawing_area);
		g_debug("Video thread created");
	}

	if (audio_stream_index >= 0)
	{
		audio_thread = new AlsaAudioThread(timer, audio_packet_queue, source.get_stream(audio_stream_index));
		g_debug("Audio thread created");
	}
}

Sink::~Sink()
{
	stop();
}

void Sink::run()
{
	if (video_thread != NULL)
	{
		video_thread->start();
	}
	
	if (audio_thread != NULL)
	{
		audio_thread->start();
	}

	g_debug("Starting Sink loop");
	while (!is_terminated())
	{
		AVPacket* packet = packet_queue.pop();
		
		if (packet == NULL)
		{
			terminate();
		}
		else
		{
			if (packet->stream_index == video_stream_index)
			{
				video_packet_queue.push(packet);
			}
			else if (packet->stream_index == audio_stream_index)
			{
				audio_packet_queue.push(packet);
			}
			else
			{
				av_free_packet(packet);
			}
			
			delete packet;
		}
	}
	g_debug("Finished Sink loop");

	destroy();
	
	g_debug("Sink thread finished");
}

void Sink::destroy()
{
	g_debug(__PRETTY_FUNCTION__);
	Glib::RecMutex::Lock lock(mutex);
	
	g_debug("Finishing queues");
	video_packet_queue.finish();
	audio_packet_queue.finish();	

	if (video_thread != NULL)
	{
		g_debug("Waiting for video thread to terminate");
		gdk_threads_leave();
		video_thread->join(true);
		gdk_threads_enter();
		video_thread = NULL;
	}
	
	if (audio_thread != NULL)
	{
		g_debug("Waiting for audio thread to terminate");
		audio_thread->join(true);
		audio_thread = NULL;
	}
	
	g_debug("GtkAlsaSink destroyed");
}

void Sink::stop()
{
	destroy();
	g_debug("Waiting for Sink to terminate");
	join(true);
}
