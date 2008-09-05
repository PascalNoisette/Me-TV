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
 
#include "xine_engine.h"
#include "application.h"
#include "exception.h"

#include <libgnome/libgnome.h>
#include <xine/xineutils.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

class XineException : public Exception
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

		result = Glib::ustring::compose("%1: %2", message, result);
		g_message(result.c_str());
		return result;
	}
	
public:
	XineException(xine_stream_t* stream, const Glib::ustring& message) :
		Exception(create_message(stream, message)) {}
};

void XineEngine::dest_size_cb ( void *data,
	int video_width, int video_height, double video_pixel_aspect,
	int *dest_width, int *dest_height, double *dest_pixel_aspect )
{
	XineEngine* engine = (XineEngine*)data;
	*dest_pixel_aspect = engine->pixel_aspect;

	Glib::RecMutex::Lock lock(engine->mutex);
	*dest_width        = engine->width;
	*dest_height       = engine->height;
}

void XineEngine::frame_output_cb ( void *data,
	int video_width, int video_height,
	double video_pixel_aspect, int *dest_x, int *dest_y,
	int *dest_width, int *dest_height,
	double *dest_pixel_aspect, int *win_x, int *win_y )
{
	XineEngine* engine = (XineEngine*)data;
	*dest_pixel_aspect = engine->pixel_aspect;

	Glib::RecMutex::Lock lock(engine->mutex);
	*dest_x            = 0;
	*dest_y            = 0;
	*win_x             = 0;
	*win_y             = 0;
	*dest_width        = engine->width;
	*dest_height       = engine->height;
}

XineEngine::XineEngine(int window_id) : Engine(), window_id(window_id)
{
	g_static_rec_mutex_init(mutex.gobj());
	xine				= NULL;
	stream				= NULL;
	video_port			= NULL;
	audio_port			= NULL;
	mute_state			= false;

	x11_visual_t	vis;
	int				screen;
	double			res_h, res_v;
		
	Application& application = Application::get_current();

	Glib::ustring video_driver = application.get_string_configuration_value("xine.video_driver");
	Glib::ustring audio_driver = application.get_string_configuration_value("xine.audio_driver");

	xine = xine_new();
	if (xine == NULL)
	{
		throw Exception(_("Failed to initialise xine library"));
	}
	
	Glib::ustring xine_config_path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	xine_config_path = Glib::build_filename(xine_config_path, "xine.config");
	
	if (!Glib::file_test(xine_config_path, Glib::FILE_TEST_EXISTS))
	{
		Glib::RefPtr<Glib::IOChannel> config_file = Glib::IOChannel::create_from_file(xine_config_path, "w");

		config_file->write(".version:2\n");
		config_file->write(Glib::ustring::compose("engine.buffers.audio_num_buffers:%1\n", 50));
		config_file->write(Glib::ustring::compose("engine.buffers.video_num_buffers:%1\n", 1000));
		
		config_file->close();
	}
	
	xine_init ( xine );
	xine_config_load(xine, xine_config_path.c_str());
		
	Display* display = XOpenDisplay(NULL);
	screen = XDefaultScreen(display);
	
	res_h = (DisplayWidth(display, screen) * 1000 / DisplayWidthMM(display, screen));
	res_v = (DisplayHeight(display, screen) * 1000 / DisplayHeightMM(display, screen));

	pixel_aspect = res_v / res_h;
	if(fabs(pixel_aspect - 1.0) < 0.01)
	{
		pixel_aspect = 1.0;
	}
	
	width	= 320;
	height	= 200;
		
	if (video_driver != "none")
	{
		vis.display           = display;
		vis.screen            = screen;
		vis.d                 = window_id;
		vis.dest_size_cb      = dest_size_cb;
		vis.frame_output_cb   = frame_output_cb;
		vis.user_data         = this;

		video_port = xine_open_video_driver ( xine, video_driver.c_str(),
										  XINE_VISUAL_TYPE_X11, (void *) &vis );
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
	stream	= xine_stream_new ( xine, audio_port, video_port );
	
	if (stream == NULL)
	{
		throw XineException(stream, "Failed to create stream");
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
			throw Exception(Glib::ustring::compose(_("Unknown deinterlace_type: '%1'"), deinterlace_type));
		}
	}
}

XineEngine::~XineEngine()
{
	stop();
	
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

void XineEngine::mute(gboolean state)
{
	mute_state = state;
	if (stream != NULL)
	{
		xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, state ? 0 : 100);
	}
}

void XineEngine::play(const Glib::ustring& mrl)
{	
	Glib::ustring path = "fifo://" + mrl;
	
	g_debug("About to open MRL '%s' ...", path.c_str());
	if ( !xine_open ( stream, path.c_str() ) )
	{
		throw XineException (stream, _("Failed to open video stream"));
	}
	
	g_debug("About to play ...");
	if ( !xine_play ( stream, 0, 0 ) )
	{
		throw XineException (stream, _("Failed to play video stream."));
	}
	g_debug("Xine engine playing");
}

void XineEngine::stop()
{
	if (stream != NULL)
	{
		xine_stop(stream);
		xine_close(stream);
		xine_dispose(stream);
		stream = NULL;
	}
}

void XineEngine::expose()
{
	if (video_port != NULL)
	{
		XExposeEvent expose_event;
		memset(&expose_event, 0, sizeof(XExposeEvent));
		expose_event.x = 0;
		expose_event.y = 0;
		expose_event.width = width;
		expose_event.height = height;
		expose_event.display = GDK_DISPLAY();
		expose_event.window = window_id;
		xine_port_send_gui_data (video_port, XINE_GUI_SEND_EXPOSE_EVENT, &expose_event);
	}
}

void XineEngine::set_size(gint w, gint h)
{
	Glib::RecMutex::Lock lock(mutex);
	width = w;
	height = h;
}
