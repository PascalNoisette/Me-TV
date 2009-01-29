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

#include "lib_xine_engine.h"

#ifdef ENABLE_XINE_LIB_ENGINE

#include "application.h"
#include "exception.h"

#include <gdk/gdkx.h>
#include <libgnome/libgnome.h>
#include <xine/xineutils.h>
#include <math.h>
#include <string.h>

class LibXineException : public Exception
{
private:
	Glib::ustring create_message(xine_stream_t* stream, const Glib::ustring& message)
	{
		Glib::ustring result;
		
		int error_code = xine_get_error(stream);
		switch (error_code)
		{
		case XINE_ERROR_NONE:				result = _("No error"); break;
		case XINE_ERROR_NO_INPUT_PLUGIN:	result = _("No input plugin"); break;
		case XINE_ERROR_NO_DEMUX_PLUGIN:	result = _("No demux plugin"); break;
		case XINE_ERROR_DEMUX_FAILED:		result = _("Demux failed"); break;
		case XINE_ERROR_MALFORMED_MRL:		result = _("Malformed URL"); break;
		case XINE_ERROR_INPUT_FAILED:		result = _("Input failed"); break;
		default:							result = _("Unknown xine error"); break;
		}
		
		Glib::ustring m = message;
		m += ": ";
		m += result;

		g_message("%s", m.c_str());
		return m;
	}
	
public:
	LibXineException(xine_stream_t* stream, const Glib::ustring& message) :
		Exception(create_message(stream, message)) {}
};

LibXineEngine::LibXineEngine (int window_id) : window_id(window_id), moduleLibXine("libxine.so")
{
	xine				= NULL;
	stream				= NULL;
	video_port			= NULL;
	audio_port			= NULL;
	mute_state			= false;
	video_thread		= NULL;
	dual_language_state	= ENGINE_DUAL_LANGUAGE_STATE_DISABLED;

	if (!moduleLibXine)
	{
		throw Exception(_("Failed to load xine library"));
	}
	
	g_static_rec_mutex_init(mutex.gobj());
	
	create();
}

LibXineEngine::~LibXineEngine()
{
	destroy();
}

void LibXineEngine::dest_size_cb ( void *data,
	int video_width, int video_height, double video_pixel_aspect,
	int *dest_width, int *dest_height, double *dest_pixel_aspect )
{
	LibXineEngine* engine = (LibXineEngine*)data;
	*dest_pixel_aspect = engine->pixel_aspect;

	Glib::RecMutex::Lock lock(engine->mutex);
	*dest_width        = engine->width;
	*dest_height       = engine->height;
}

void LibXineEngine::frame_output_cb ( void *data,
	int video_width, int video_height,
	double video_pixel_aspect, int *dest_x, int *dest_y,
	int *dest_width, int *dest_height,
	double *dest_pixel_aspect, int *win_x, int *win_y )
{
	LibXineEngine* engine = (LibXineEngine*)data;
	*dest_pixel_aspect = engine->pixel_aspect;

	Glib::RecMutex::Lock lock(engine->mutex);
	*dest_x            = 0;
	*dest_y            = 0;
	*win_x             = 0;
	*win_y             = 0;
	*dest_width        = engine->width;
	*dest_height       = engine->height;
}

void LibXineEngine::mute(gboolean state)
{
	mute_state = state;
	if (stream != NULL)
	{
		xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, state ? 0 : 100);
	}
}

void LibXineEngine::write(const gchar* buffer, gsize length)
{	
	if (fifo_output_stream)
	{
		gsize bytes_written = 0;
		fifo_output_stream->write(buffer, length, bytes_written);
	}
}

void LibXineEngine::create()
{
	x11_visual_t	vis;
	double			res_h, res_v;

	Application& application = Application::get_current();

	Glib::ustring video_driver = application.get_string_configuration_value("xine.video_driver");
	Glib::ustring audio_driver = application.get_string_configuration_value("xine.audio_driver");
		
	xine = xine_new();
	
	if (xine == NULL)
	{
		throw Exception(_("Failed to initialise xine library"));
	}
	
	Glib::ustring xine_config_path = Glib::get_home_dir() + "/xine.config";
	
	if (!Gio::File::create_for_path(xine_config_path)->query_exists())
	{
		Glib::RefPtr<Glib::IOChannel> config_file = Glib::IOChannel::create_from_file(xine_config_path, "w");
		
		Glib::ustring a = Glib::ustring::compose("engine.buffers.audio_num_buffers:%1\n", 50);
		Glib::ustring v = Glib::ustring::compose("engine.buffers.video_num_buffers:%1\n", 1000);

		config_file->write(".version\n");
		config_file->write(a);
		config_file->write(v);
	}
	
	xine_init ( xine );
	xine_config_load(xine, xine_config_path.c_str());
	
	Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
	Glib::RefPtr<Gdk::Screen> screen = display->get_default_screen();
	
	res_h = (screen->get_width() * 1000 / screen->get_width_mm());
	res_v = (screen->get_height() * 1000 / screen->get_height_mm());

	pixel_aspect = res_v / res_h;
	if(fabs(pixel_aspect - 1.0) < 0.01)
	{
		pixel_aspect = 1.0;
	}
	
	width	= 320;
	height	= 200;
		
	if (video_driver != "none")
	{
		vis.display           = GDK_DISPLAY();
		vis.screen            = screen->get_number();
		vis.d                 = window_id;
		vis.dest_size_cb      = dest_size_cb;
		vis.frame_output_cb   = frame_output_cb;
		vis.user_data         = this;

		video_port = xine_open_video_driver ( xine, video_driver.c_str(),
										  XINE_VISUAL_TYPE_X11, ( void * ) &vis );
		if ( video_port == NULL )
		{
			throw Exception(_("Failed to initialise video driver"));
		}
	}

	if (audio_driver != "none")
	{
		audio_port = xine_open_audio_driver ( xine , audio_driver.c_str(), NULL );
		if ( audio_port == NULL)
		{
			throw Exception(_("Failed to initialise audio driver"));
		}
	}
}

void LibXineEngine::play(const Glib::ustring& mrl)
{
	Application& application = Application::get_current();

	stream	= xine_stream_new ( xine, audio_port, video_port );
	
	if (stream == NULL)
	{
		throw LibXineException(stream, "Failed to create stream");
	}

	mute(mute_state);

	if (video_port != NULL)
	{
		xine_gui_send_vo_data ( stream, XINE_GUI_SEND_DRAWABLE_CHANGED, ( void * ) window_id );
		xine_gui_send_vo_data ( stream, XINE_GUI_SEND_VIDEOWIN_VISIBLE, ( void * ) 1 );

		// Set up deinterlacing
		Glib::ustring deinterlace_type = application.get_string_configuration_value("xine.deinterlace_type");
		if (deinterlace_type == "default")
		{
			xine_set_param( stream, XINE_PARAM_VO_DEINTERLACE, true );
		}
		else if (deinterlace_type == "tvtime")
		{
			xine_post_wire_video_port (xine_get_video_source (stream), video_port);
			
			xine_post_t* plugin = xine_post_init (xine, "tvtime", 0, &audio_port, &video_port);	
			if (plugin == NULL)
			{
				throw Exception(_("Failed to create tvtime plugin"));
			}
			
			xine_post_out_t* plugin_output = xine_post_output (plugin, "video out")
				? : xine_post_output (plugin, "video")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
			if (plugin_output == NULL)
			{
				throw Exception(_("Failed to get xine plugin output for deinterlacing"));
			}
			
			xine_post_in_t* plugin_input = xine_post_input (plugin, "video")
				? : xine_post_input (plugin, "video in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);
			
			if (plugin_input == NULL)
			{
				throw Exception(_("Failed to get xine plugin input for deinterlacing"));
			}

			xine_post_wire (xine_get_video_source (stream), plugin_input);
			xine_post_wire_video_port (plugin_output, video_port);

			xine_set_param( stream, XINE_PARAM_VO_DEINTERLACE, true );
		}
		else if (deinterlace_type == "none")
		{
			// Ignore
		}
		else
		{
			Glib::ustring message = Glib::ustring::compose(_("Unknown deinterlace_type: '%1'"), deinterlace_type);
			throw Exception(message);
		}
	}
	
	if (video_thread != NULL)
	{
		throw Exception(_("Video thread was not NULL"));
	}

	fifo_path = mrl;
	video_thread = g_thread_create((GThreadFunc)video_thread_function, this, TRUE, NULL);

	if (fifo_output_stream == NULL)
	{
		fifo_output_stream = Glib::IOChannel::create_from_file(mrl, "w");
		fifo_output_stream->set_encoding("");
	}
}

gpointer LibXineEngine::video_thread_function(LibXineEngine* engine)
{
	TRY;
	
	g_debug("Video thread created");

	Glib::ustring path = "fifo:/" + engine->fifo_path;

	if (engine->stream == NULL)
	{
		throw Exception(_("Stream is NULL"));
	}
	
	g_debug(_("About to open FIFO '%s' ..."), path.c_str());
	if ( !xine_open ( engine->stream, path.c_str() ) )
	{
		if (engine->fifo_output_stream != NULL)
		{
			throw LibXineException (engine->stream, _("Failed to open video stream"));
		}
	}
	
	if (engine->fifo_output_stream != NULL)
	{
		g_debug("About to play from FIFO ...");
		if ( !xine_play ( engine->stream, 0, 0 ) )
		{
			throw LibXineException (engine->stream, _("Failed to play video stream."));
		}
		g_debug("Playing from FIFO");
	}
	
	THREAD_CATCH;
	
	return NULL;
}

void LibXineEngine::destroy()
{
	close();
	
	if (xine != NULL)
	{
		if ( audio_port != NULL )
		{
			xine_close_audio_driver ( xine, audio_port );
		}
		audio_port = NULL;
		
		if ( video_port != NULL )
		{
			xine_close_video_driver ( xine, video_port );
		}
		video_port = NULL;

		xine_exit ( xine );
	
		xine = NULL;
	}
}

void LibXineEngine::close()
{
	if (fifo_output_stream != NULL)
	{
		fifo_output_stream.reset();
	}

	if (video_thread != NULL)
	{
		g_thread_join(video_thread);
		video_thread = NULL;
	}

	if (stream != NULL)
	{
		xine_stop(stream);
		xine_close(stream);
		xine_dispose(stream);
		stream = NULL;
	}
}

void LibXineEngine::set_subtitle_channel(gint channel)
{
	if (stream == NULL)
	{
		throw Exception("Failed to set subtitle channel: stream has not been created");
	}
	
	g_debug("set_subtitle_channel(%d)", channel);
	xine_set_param(stream, XINE_PARAM_SPU_CHANNEL, channel);
}

void LibXineEngine::set_audio_channel(gint channel)
{
	if (stream == NULL)
	{
		throw Exception("Failed to set subtitle channel: stream has not been created");
	}

	g_debug("set_audio_channel(%d)", channel);
	xine_set_param(stream, XINE_PARAM_AUDIO_CHANNEL_LOGICAL, channel);
}

void LibXineEngine::set_dual_language_state(gint state)
{
	static xine_post_t* plugin = NULL;
	
	if (dual_language_state != state)
	{
		if (state == ENGINE_DUAL_LANGUAGE_STATE_DISABLED)
		{
			if (plugin != NULL)
			{
				g_debug("Disabling dual language");
				xine_post_wire_audio_port (xine_get_audio_source (stream), audio_port);
			}
		}
		else
		{
			switch (state)
			{
			case ENGINE_DUAL_LANGUAGE_STATE_LEFT: g_debug("Enabling dual language for left channel");  break;
			case ENGINE_DUAL_LANGUAGE_STATE_RIGHT: g_debug("Enabling dual language for right channel");  break;
			default:
				throw Exception(_("Unknown dual language state"));
			}
			
			if (plugin == NULL)
			{
				g_debug("Creating upmix_mono plugin");
				xine_post_wire_audio_port(xine_get_audio_source (stream), audio_port);

				plugin = xine_post_init (xine, "upmix_mono", 0, &audio_port, &video_port);
				if (plugin == NULL)
				{
					throw Exception(_("Failed to create upmix_mono plugin"));
				}
				
				g_debug("upmix_mono plugin created");
			}
			else
			{
				g_debug("upmix_mono plugin already created, using existing");
			}
			
			xine_post_out_t* plugin_output = xine_post_output (plugin, "audio out")
				? : xine_post_output (plugin, "audio")
				? : xine_post_output (plugin, xine_post_list_outputs (plugin)[0]);
			if (plugin_output == NULL)
			{
				throw Exception(_("Failed to get xine plugin output for upmix_mono"));
			}
			
			xine_post_in_t* plugin_input = xine_post_input (plugin, "audio")
				? : xine_post_input (plugin, "audio in")
				? : xine_post_input (plugin, xine_post_list_inputs (plugin)[0]);
			
			if (plugin_input == NULL)
			{
				throw Exception(_("Failed to get xine plugin input for upmix_mono"));
			}
			
			xine_post_wire (xine_get_audio_source (stream), plugin_input);
			xine_post_wire_audio_port (plugin_output, audio_port);
			
			g_debug("upmix_mono plugin wired");
			int parameter = -1;
			switch (state)
			{
			case ENGINE_DUAL_LANGUAGE_STATE_LEFT: parameter = 0; break;
			case ENGINE_DUAL_LANGUAGE_STATE_RIGHT: parameter = 1; break;
			default: break;
			}

			g_debug("Setting channel on upmix_mono plugin to %d", parameter);

			const xine_post_in_t *in = xine_post_input (plugin, "parameters");
			const xine_post_api_t* api = (const xine_post_api_t*)in->data;
			const xine_post_api_descr_t* param_desc = api->get_param_descr();
			
			if (param_desc->struct_size != 4)
			{
				throw Exception("ASSERT: parameter size != 4");
			}

			api->set_parameters (plugin, (void*)&parameter);
		}
		
		dual_language_state = state;
	}
}

#endif
