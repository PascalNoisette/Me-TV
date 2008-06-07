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

#include "ffmpeg_renderer.h"
#include <gdk/gdk.h>

FFMpegRenderer::FFMpegRenderer()
{
}

FFMpegRenderer::~FFMpegRenderer()
{
	close();
}

void FFMpegRenderer::mute(gboolean state)
{
}

void FFMpegRenderer::open(const Glib::ustring& mrl)
{
	this->mrl = mrl;
	stream_thread.start(mrl);
}

void FFMpegRenderer::close()
{
	stream_thread.terminate();

	gdk_threads_leave();
	stream_thread.join(true);
	gdk_threads_enter();		
}

void FFMpegRenderer::set_drawing_area(Gtk::DrawingArea* d)
{
	stream_thread.set_drawing_area(d);
}
