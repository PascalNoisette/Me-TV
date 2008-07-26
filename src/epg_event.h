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

#ifndef __EPG_EVENT_H__
#define __EPG_EVENT_H__

#include <glibmm.h>
#include <list>

class EpgEventText
{
public:
	guint epg_event_text_id;
	guint epg_event_id;
	Glib::ustring language;
	Glib::ustring title;
	Glib::ustring description;
};

typedef std::list<EpgEventText> EpgEventTextList;

class EpgEvent
{
public:
	guint epg_event_id;
	guint channel_id;
	guint event_id;
	guint start_time;
	guint duration;
	EpgEventTextList texts;
};

typedef std::list<EpgEvent> EpgEventList;

#endif
