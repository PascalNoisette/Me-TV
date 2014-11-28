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

#ifndef __FRONTEND_THREAD_H__
#define __FRONTEND_THREAD_H__

#include "thread.h"
#include "epg_thread.h"
#include "channel.h"
#include "channel_stream.h"
#include "dvb_frontend.h"

class FrontendThread: public Thread {
private:
	ChannelStreamList streams;
	EpgThread * epg_thread;
	int fd;
	void write(Glib::RefPtr<Glib::IOChannel> channel, guchar * buffer, gsize length);
	void run();
	void setup_dvb(ChannelStream & stream);
	void start_epg_thread();
	void stop_epg_thread();
public:
	FrontendThread(Dvb::Frontend & frontend);
	~FrontendThread();
	Dvb::Frontend & frontend;
	gboolean is_display();
	gboolean is_recording();
	gboolean is_recording(Channel const & channel);
	void start_recording(Channel & channel, Glib::ustring const & description, gboolean scheduled);
	void stop_recording(const Channel& channel);
	guint get_last_epg_update_time();
	void start();
	void stop();
	void start_display(Channel & channel);
	void stop_display();
	ChannelStreamList & get_streams() { return streams; }
};

typedef std::list<FrontendThread *> FrontendThreadList;

#endif
