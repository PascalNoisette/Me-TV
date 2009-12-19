/*
 * Copyright (C) 2009 Michael Lamothe
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

#include "thread.h"
#include "epg_thread.h"
#include "channel.h"
#include "dvb_demuxer.h"
#include "me-tv-ui.h"
#include "mpeg_stream.h"
#include <gnet.h>

class StreamManager : public Thread
{
public:
	class ChannelStream
	{
	public:
		ChannelStream(const Channel& channel, const Glib::ustring& filename);
	
		Channel							channel;			
		Mpeg::Stream					stream;
		Glib::RefPtr<Glib::IOChannel>	output_channel;
		Glib::ustring					filename;
		Dvb::DemuxerList				demuxers;
		Glib::StaticRecMutex			mutex;

		Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
			guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
		Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);

		void clear_demuxers();
		void write(guchar* buffer, gsize length);
	};

private:
	Glib::StaticRecMutex		mutex;
	EpgThread*					epg_thread;
	GUdpSocket*					socket;
	GInetAddr*					inet_address;
	gboolean					broadcast_failure_message;
	std::list<ChannelStream>	streams;

	void run();
	void write(Glib::RefPtr<Glib::IOChannel> channel, guchar* buffer, gsize length);

	static gboolean on_timeout(gpointer data);
	void on_timeout();

	void setup_dvb(const Channel& channel, ChannelStream& stream);

	void start_epg_thread();
	void stop_epg_thread();
		
public:
	StreamManager();
	~StreamManager();

	void set_display_stream(const Channel& channel);
	const ChannelStream& get_display_stream();
	void start();
	void stop();
	guint get_last_epg_update_time();

	void start_recording(const Channel& channel, const Glib::ustring& filename);
	void stop_recording(const Channel& channel);
	gboolean is_recording();
	gboolean is_recording(const Channel& channel);
	
	void start_broadcasting();
	void stop_broadcasting();
};

#endif
