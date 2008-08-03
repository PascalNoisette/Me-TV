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

#ifndef __MPLAYER_ENGINE_H__
#define __MPLAYER_ENGINE_H__

#include "engine.h"

class MplayerEngine : public Engine
{
private:
	gint pid;
	gint standard_input;
public:
	MplayerEngine();
	~MplayerEngine();

	void play(Glib::RefPtr<Gdk::Window> window, const Glib::ustring& mrl);
	void record(const Glib::ustring& filename);
	void mute(gboolean state);
};

#endif