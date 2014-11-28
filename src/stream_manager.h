/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __STREAM_THREAD_H__
#define __STREAM_THREAD_H__

#include "frontend_thread.h"
#include "scheduled_recording.h"

class StreamManager {
private:
	FrontendThreadList frontend_threads;
	void update_record_action();
public:
	void initialise();
	~StreamManager();
	guint get_last_epg_update_time();
	FrontendThreadList & get_frontend_threads() { return frontend_threads; };
	void start_display(Channel & channel);
	void stop_display();
	gboolean has_display_stream();
	Channel & get_display_channel();
	ChannelStream & get_display_stream();
	FrontendThread & get_display_frontend_thread();
	gboolean is_recording();
	gboolean is_recording(Channel const & channel);
	void start_recording(Channel & channel);
	void start_recording(Channel & channel, ScheduledRecording const & scheduled_recording);
	void stop_recording(Channel const & channel);
	void start();
	void stop();
};

#endif
