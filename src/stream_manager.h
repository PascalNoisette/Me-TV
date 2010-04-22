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
	std::list<FrontendThread> frontend_threads;
		
public:
	void load();
		
	guint get_last_epg_update_time();
	void set_display_stream(const Channel& channel);
	const ChannelStream& get_display_stream();
	std::list<ChannelStream> get_streams();
	FrontendThread& get_display_frontend_thread();
	const std::list<FrontendThread>& get_frontend_threads() const { return frontend_threads; };

	gboolean is_recording();
	gboolean is_recording(const Channel& channel);
	void start_recording(Channel& channel);
	void start_recording(Channel& channel, const ScheduledRecording& scheduled_recording);
	void stop_recording(const Channel& channel);

	void start();
	void stop();
};

#endif
