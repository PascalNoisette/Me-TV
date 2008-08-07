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
 
#ifndef __XINE_ENGINE_H__
#define __XINE_ENGINE_H__

#include <xine.h>
#include "engine.h"

class XineEngine : public Engine
{
private:
	xine_t*						xine;
	xine_stream_t*				stream;
	xine_video_port_t*			video_port;
	xine_audio_port_t*			audio_port;
	gint						width, height;
	double						pixel_aspect;
	gboolean					mute_state;
	Glib::StaticRecMutex		mutex;
	
	static void dest_size_cb ( void *data,
		int video_width, int video_height, double video_pixel_aspect,
		int *dest_width, int *dest_height, double *dest_pixel_aspect );
	static void frame_output_cb ( void *data,
		int video_width, int video_height, double video_pixel_aspect, int *dest_x, int *dest_y,
		int *dest_width, int *dest_height, double *dest_pixel_aspect, int *win_x, int *win_y );
	
	void play(Gtk::Widget& widget, const Glib::ustring& mrl);
	void mute(gboolean state);
	bool on_drawing_area_configure_event(GdkEventConfigure* event);
	
public:
	XineEngine();
	~XineEngine();
};

#endif
