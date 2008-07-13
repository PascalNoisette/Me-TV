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
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XShm.h>

#define XV_IMAGE_FORMAT_YUY2	0x32595559

#define VIDEO_IMAGE_BUFFER_SIZE		10
#define AUDIO_BUFFER_SIZE			10
#define VIDEO_FRAME_TOLERANCE		1
#define AUDIO_SAMPLE_TOLERANCE		0

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

AlsaAudioThread::AlsaAudioThread(Pipeline& pipeline, AudioChunkQueue& audio_chunk_queue, guint channels, guint sample_rate) :
	Thread("ALSA Audio"), audio_chunk_queue(audio_chunk_queue), pipeline(pipeline), sample_rate(sample_rate)
{
	handle = NULL;
	const gchar* device = "default";
	
	if (snd_pcm_open (&handle, device, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		throw AlsaException("Failed open audio device '%s'", device);
	}
	
	guint result = snd_pcm_set_params(
		handle,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		channels,
		sample_rate,
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
}

AlsaAudioThread::~AlsaAudioThread()
{
	if (handle != NULL)
	{
		snd_pcm_close(handle);
		handle = NULL;
	}
}

void AlsaAudioThread::run()
{	
	guint start_time = 0;

	while (!is_terminated())
	{
		if (audio_chunk_queue.get_size() == 0)
		{
			usleep(1000);
		}
		else
		{
			AudioChunk* audio_chunk = audio_chunk_queue.pop();
			const Buffer& buffer = audio_chunk->get_buffer();

			guint pts = audio_chunk->get_pts();
			if (start_time <= 0)
			{
				start_time = pts;
			}
			
			gboolean drop = false;
			guint elapsed = pipeline.get_elapsed();
			guint buffer_length = buffer.get_length()/4;
			guint want = elapsed;
			guint got = pts - start_time;
			
			g_debug("AUDIO: pts = %d", pts);
			g_debug("AUDIO: start_time = %d", start_time);
			g_debug("AUDIO: delta = %d", pts - start_time);
			g_debug("AUDIO: want = %d, got = %d", want, got);

			if (start_time > pts)
			{
				g_debug("Start time was greater than PTS, resetting start time to PTS");
				start_time = pts;
			}
			
			if (want < got)
			{
				guint delay = (guint)((got - want) / AV_TIME_BASE);
				g_debug("Delaying audio for %d microseconds", delay);
				usleep(delay);
			}
			else if (want > got)
			{
				g_debug("Dropping audio sample");
				drop = true;
			}
			
			if (!drop)
			{
				snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer.get_data(), buffer_length);
				if (frames == -EPIPE)
				{
					snd_pcm_prepare(handle);
					snd_pcm_start(handle);
				}
				else if (frames < 0)
				{
					throw AlsaException("Failed to write to ALSA audio interface");
				}
			}
			
			delete audio_chunk;
		}
	}
	g_debug("Audio thread finished");
}

Rectangle VideoOutput::calculate_drawing_rectangle(VideoImage* video_image)
{
	Rectangle rectangle;
	int window_width = 0, window_height = 0;
	window->get_size(window_width, window_height);
	const Size& image_size = video_image->get_size();

	gdouble image_aspect_ratio = image_size.width / (gdouble)image_size.height;
	if ((image_aspect_ratio*window_height) > window_width)
	{
		rectangle.width = window_width;
		rectangle.height = window_width / image_aspect_ratio;
	}
	else
	{
		rectangle.width = image_aspect_ratio * window_height;
		rectangle.height = window_height;
	}
	
	if (image_size.width < rectangle.width)
	{
		gdouble ratio = image_size.width/(gdouble)rectangle.width;
		rectangle.height *= ratio;
		rectangle.width *= ratio;
	}
	
	if (image_size.height < rectangle.height)
	{
		gdouble ratio = image_size.height/(gdouble)rectangle.height;
		rectangle.height *= ratio;
		rectangle.width *= ratio;
	}

	if ((image_aspect_ratio*window_height) > window_width)
	{
		rectangle.y = (window_height-rectangle.height)/2;
	}
	else
	{
		rectangle.x = (window_width-rectangle.width)/2;
	}

	return rectangle;
}


class GtkVideoOutput : public VideoOutput
{
private:
	Glib::RefPtr<Gdk::GC> gc;
public:
	GtkVideoOutput(Glib::RefPtr<Gdk::Window> window) : VideoOutput(window), gc(Gdk::GC::create(window))
	{
	}
	
	void draw(VideoImage* video_image)
	{
		static gint previous_width = 0, previous_height = 0;
		gint width = 0, height = 0;

		window->get_size(width, height);
		if (previous_width != width || previous_height != height)
		{
			window->draw_rectangle(gc, true, 0, 0, width, height);

			previous_width = width;
			previous_height = height;
		}

		const Size& size = video_image->get_size();
		Rectangle rectangle = calculate_drawing_rectangle(video_image);
		window->draw_rgb_image(gc, rectangle.x, rectangle.y, rectangle.width, rectangle.height,
			Gdk::RGB_DITHER_MAX, video_image->get_image_data(), size.width*3);
	}
};

class XvVideoOutput : public VideoOutput
{
private:
	gint		xv_port;
	Display*	display;
	GC			gc;
	XvImage*	image;

public:
	XvVideoOutput(Glib::RefPtr<Gdk::Window> window) : VideoOutput(window)
	{
		image = NULL;
		xv_port = -1;
		display = GDK_DISPLAY();

		guint number_of_adapters = 0;
		XvAdaptorInfo* adapter_info = NULL;
		gc = XCreateGC(display, GDK_WINDOW_XID(window->gobj()), 0, 0);
		
		guint result = XvQueryAdaptors(display, GDK_ROOT_WINDOW(), &number_of_adapters, &adapter_info);

		for (int i = 0; i < number_of_adapters; i++)
		{
			g_debug("name: '%s', type: %s%s%s%s%s%s, ports: %ld, first port: %ld",
				adapter_info[i].name,
				(adapter_info[i].type & XvInputMask)	? "input | "	: "",
				(adapter_info[i].type & XvOutputMask)	? "output | "	: "",
				(adapter_info[i].type & XvVideoMask)	? "video | "	: "",
				(adapter_info[i].type & XvStillMask)	? "still | "	: "",
				(adapter_info[i].type & XvImageMask)	? "image | "	: "",
				(adapter_info[i].type & XvRGB)			? "RGB"			: "YUV",
				adapter_info[i].num_ports,
				adapter_info[i].base_id);

			xv_port = adapter_info[i].base_id;
		}
		
		if (xv_port == -1)
		{
			throw Exception(_("Failed to find suitable Xv port"));
		}
		
		int number_of_formats;
		XvImageFormatValues* list = XvListImageFormats( display, xv_port, &number_of_formats);
		for (int i = 0; i < number_of_formats; i++)
		{
			g_debug("0x%08x (%c%c%c%c) %s",
				list[ i ].id,
				( list[ i ].id ) & 0xff,
				( list[ i ].id >> 8 ) & 0xff,
				( list[ i ].id >> 16 ) & 0xff,
				( list[ i ].id >> 24 ) & 0xff,
				( list[ i ].format == XvPacked ) ? "packed" : "planar" );			
		}
		
		g_debug("Selected port %d", xv_port);
	}

	void draw(VideoImage* video_image)
	{
		static gchar* buffer = NULL;
		static gint previous_width = 0, previous_height = 0;

		Rectangle rectangle = calculate_drawing_rectangle(video_image);
		const Size& image_size = video_image->get_size();
		guint image_data_length = image_size.width * image_size.height * 2;
		
		if (previous_width != image_size.width || previous_height != image_size.height)
		{
			if (image != NULL)
			{
				XFree(image);
				image = NULL;
			}

			previous_width = image_size.width;
			previous_height = image_size.height;
		}
		
		if (image == NULL)
		{			
			if (buffer != NULL)
			{
				delete buffer;
				buffer = NULL;
			}
			
			buffer = new gchar[image_data_length];
			
			image = XvCreateImage(display, xv_port, XV_IMAGE_FORMAT_YUY2,
				buffer, rectangle.width, rectangle.height);
		}
				
		memcpy(buffer, video_image->get_image_data(), image_data_length);
		
		XvPutImage(display, xv_port, GDK_WINDOW_XID(window->gobj()), gc, image,
		    0, 0, image->width, image->height,
		    rectangle.x, rectangle.y, rectangle.width, rectangle.height);
	}
};

VideoThread::VideoThread(Pipeline& pipeline, VideoImageQueue& video_image_queue, VideoOutput* video_output, gdouble frame_rate) :
	Thread("GTK Video"), video_image_queue(video_image_queue), pipeline(pipeline), video_output(video_output), frame_rate(frame_rate)
{
}

void VideoThread::run()
{
	gdouble start_time = 0;
	gboolean drop = false;
	
	while (!is_terminated())
	{
		if (video_image_queue.get_size() == 0)
		{
			usleep(1000);
		}
		else
		{
			VideoImage* video_image = video_image_queue.pop();

			gdouble pts = video_image->get_pts();
			
			if (start_time <= 0)
			{
				start_time = pts;
			}

			gdouble elapsed = av_gettime() / AV_TIME_BASE;
			guint wanted_frame = (guint)(elapsed * frame_rate);
			guint got_frame = (pts - start_time) * frame_rate;
			
			/*
			g_debug("VIDEO: pts = %f", video_image->get_pts());
			g_debug("VIDEO: start_time = %f", start_time);
			g_debug("VIDEO: want = %d, got = %d", wanted_frame, got_frame);
			*/
			if ((wanted_frame + VIDEO_FRAME_TOLERANCE) < got_frame)
			{
				guint wait_frames = got_frame - wanted_frame;
				guint delay = wait_frames * frame_rate * 1000;
				if (delay < 10000000)
				{
					//g_debug("Delaying audio for %d microseconds", delay);
					usleep(wait_frames * frame_rate * 1000);
				}
				else
				{
					g_debug("Frame is more than 10 seconds in the future, dropping");
					drop = true;
				}
			}
			else if ((wanted_frame - VIDEO_FRAME_TOLERANCE) > got_frame)
			{
				g_debug("Dropping video frame");
				drop = true;
			}

			if (!drop)
			{
				GdkLock gdk_lock;
				video_output->draw(video_image);
			}
			
			drop = false;
			delete video_image;
		}
	}
	
	g_debug("Video thread finished");
}

Sink::Sink(Pipeline& pipeline, Gtk::DrawingArea& drawing_area) :
	pipeline(pipeline), drawing_area(drawing_area)
{
	g_static_rec_mutex_init(mutex.gobj());

	video_output		= NULL;
	video_thread		= NULL;
	audio_thread		= NULL;
	video_buffer		= NULL;
	video_stream_index	= -1;
	audio_stream_index	= -1;
	img_convert_ctx		= NULL;
	previous_width		= 0;
	previous_width		= 0;
	frame				= NULL;

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
		video_stream = source.get_stream(video_stream_index);
		
		AVCodec* video_codec = avcodec_find_decoder(video_stream->codec->codec_id);
		if (video_codec == NULL)
		{
			throw Exception("Failed to find video codec");
		}
		g_debug("VIDEO CODEC: '%s'", video_codec->name);

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

		Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
		Glib::ustring video_output_name = client->get_string(GCONF_PATH"/video_output");
		if (video_output_name == "GTK")
		{
			GdkLock gdk_lock;
			video_output = new GtkVideoOutput(drawing_area.get_window());
			pixel_format = PIX_FMT_RGB24;
		}
		else if (video_output_name == "Xv")
		{
			GdkLock gdk_lock;
			video_output = new XvVideoOutput(drawing_area.get_window());
			pixel_format = PIX_FMT_YUYV422;
		}
		else
		{
			throw Exception(Glib::ustring::compose(_("Unknown video output '%1'"), video_output_name));
		}

		video_thread = new VideoThread(pipeline, video_image_queue, video_output, frame_rate);
		g_debug("Video thread created");
		video_thread->start();
	}

	if (audio_stream_index >= 0)
	{
		audio_stream = source.get_stream(audio_stream_index);
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

		audio_thread = new AlsaAudioThread(pipeline, audio_chunk_queue, audio_stream->codec->channels, audio_stream->codec->sample_rate);
		g_debug("Audio thread created");
		audio_thread->start();
	}
}

Sink::~Sink()
{
	destroy();

	if (audio_stream != NULL)
	{
		avcodec_close(audio_stream->codec);
	}

	if (video_output != NULL)
	{
		delete video_output;
		video_output = NULL;
	}

	if (video_stream != NULL)
	{
		avcodec_close(video_stream->codec);
	}

	if (frame != NULL)
	{
		av_free(frame);
		frame = NULL;
	}
	
	if (video_buffer != NULL)
	{
		delete [] video_buffer;
		video_buffer = NULL;
	}
}

void flush_video_image_queue(VideoImageQueue& video_image_queue)
{
	while (video_image_queue.get_size() > 0)
	{
		delete video_image_queue.pop();
	}
}

void Sink::push_video_packet(AVPacket* packet)
{
	gboolean video_frame_finished = false;
	
	avcodec_decode_video(video_stream->codec,
		frame, &video_frame_finished, packet->data, packet->size);

	if (video_frame_finished)
	{
		AVCodecContext* video_codec_context = video_stream->codec;

		double pts = 0;
		if(packet->dts == AV_NOPTS_VALUE && frame->opaque && *(uint64_t*)frame->opaque != AV_NOPTS_VALUE)
		{
			pts = *(uint64_t *)frame->opaque;
		}
		else if (packet->dts != AV_NOPTS_VALUE)
		{
			pts = packet->dts;
		}
		pts *= av_q2d(video_stream->time_base);
		
		gint width = 0;
		gint height = 0;

		{
			GdkLock gdk_lock;
			drawing_area.get_window()->get_size(width, height);
		}
		
		if (previous_width != width || previous_height != height)
		{
			g_debug("Size changed (%d, %d)", width, height);
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
			}
			else
			{
				video_width = width;
				video_height = width / aspect_ratio; 
			}
			
			video_width = (video_width/2)*2;

			img_convert_ctx = sws_getContext(
				video_codec_context->width, video_codec_context->height, video_codec_context->pix_fmt,
				video_width, video_height, pixel_format, SWS_BILINEAR, NULL, NULL, NULL);
			if (img_convert_ctx == NULL)
			{
				throw Exception("Cannot initialise the conversion context");
			}
			
			if (video_buffer != NULL)
			{
				delete [] video_buffer;
				video_buffer = NULL;
			}
			
			video_buffer_size = avpicture_get_size(pixel_format, video_width, video_height);
			video_buffer = new guchar[video_buffer_size];

			avpicture_fill(&picture, video_buffer, pixel_format, video_width, video_height);
		}
		
		sws_scale(img_convert_ctx, frame->data, frame->linesize, 0,
			video_codec_context->height, picture.data, picture.linesize);
		
		GdkLock gdk_lock;
		VideoImage* video_image = new VideoImage(video_width, video_height, video_buffer_size/(video_width*video_height),pts);
		memcpy(video_image->get_image_data(), video_buffer, video_buffer_size);
		video_image_queue.push(video_image);
	}
}

void Sink::push_audio_packet(AVPacket* packet)
{
	gsize data_size = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
	DECLARE_ALIGNED(16, uint8_t, data[data_size]);
	
	int audio_buffer_length = data_size;
	gint ffmpeg_result = avcodec_decode_audio2(
		audio_stream->codec,
		(int16_t*)data,
		&audio_buffer_length,
		packet->data, packet->size);

	if (ffmpeg_result < 0)
	{
		throw Exception("Failed to decode audio");
	}

	while (audio_chunk_queue.get_size() > AUDIO_BUFFER_SIZE)
	{
		usleep(1000);
	}

	gdouble pts = packet->dts;
	AudioChunk* audio_chunk = new AudioChunk(audio_buffer_length, packet->pts);
	memcpy(audio_chunk->get_buffer().get_data(), data, audio_buffer_length);
	audio_chunk_queue.push(audio_chunk);
}

void Sink::write(AVPacket* packet)
{
	if (packet->stream_index == video_stream_index)
	{
		push_video_packet(packet);
	}
	else if (packet->stream_index == audio_stream_index)
	{
		push_audio_packet(packet);
	}
}

void Sink::destroy()
{
	g_debug(__PRETTY_FUNCTION__);
	Glib::RecMutex::Lock lock(mutex);
	
	if (video_thread != NULL)
	{
		g_debug("Waiting for video thread to join ...");
		{
			GdkUnlock gdk_unlock;
			video_thread->join(true);
		}
		video_thread = NULL;
		g_debug("Video thread joined");
	}
	
	if (audio_thread != NULL)
	{
		g_debug("Waiting for audio thread to join ...");
		audio_thread->join(true);
		audio_thread = NULL;
		g_debug("Audio thread joined");
	}
	
	g_debug("Sink destroyed");
}
