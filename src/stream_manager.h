/*
 * Copyright (C) 2010 Michael Lamothe
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

#include "frontend_thread.h"
#include "scheduled_recording.h"

class StreamManager
{
private:
	FrontendThreadList frontend_threads;
		
public:
	~StreamManager();
	
	void load();
		
	guint get_last_epg_update_time();
	FrontendThreadList& get_frontend_threads() { return frontend_threads; };

	void start_display(const Channel& channel);
	void stop_display();
	gboolean has_display_stream();
	ChannelStream& get_display_stream();
	FrontendThread& get_display_frontend_thread();

	gboolean is_recording();
	gboolean is_recording(const Channel& channel);
	void start_recording(Channel& channel);
	void start_recording(Channel& channel, const ScheduledRecording& scheduled_recording);
	void stop_recording(const Channel& channel);

	void start();
	void stop();
};

#endif
