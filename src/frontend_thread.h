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

#ifndef __FRONTEND_THREAD_H__
#define __FRONTEND_THREAD_H__

#include "thread.h"
#include "epg_thread.h"
#include "channel.h"
#include "channel_stream.h"
#include "dvb_frontend.h"

class FrontendThread : public Thread
{
private:
	Glib::StaticRecMutex		mutex;
	std::list<ChannelStream>	streams;
	EpgThread*					epg_thread;
	gboolean					is_tuned;
	int				stop_crash;
	
	void write(Glib::RefPtr<Glib::IOChannel> channel, guchar* buffer, gsize length);
	void run();
	void setup_dvb(ChannelStream& stream);
	void start_epg_thread();
	void stop_epg_thread();

public:
	FrontendThread(Dvb::Frontend& frontend);
	~FrontendThread();

	Dvb::Frontend& frontend;
	
	gboolean is_recording();
	gboolean is_recording(const Channel& channel);
	void start_recording(const Channel& channel, const Glib::ustring& filename, gboolean scheduled);
	void stop_recording(const Channel& channel);
	guint get_last_epg_update_time();
	void start();
	void stop();
	void start_display(const Channel& channel);
	void stop_display();
	std::list<ChannelStream>& get_streams() { return streams; }
};

#endif
