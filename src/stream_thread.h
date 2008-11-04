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

#ifndef __STREAM_THREAD_H__
#define __STREAM_THREAD_H__

#include "thread.h"
#include "dvb_frontend.h"
#include "dvb_demuxer.h"
#include "engine.h"
#include "epg_thread.h"
#include "channel.h"
#include "dvb_si.h"
#include "me-tv-ui.h"
#include <gnet.h>

class Stream
{
public:
	~Stream();

	void clear();
	
	std::vector<Dvb::SI::VideoStream>		video_streams;
	std::vector<Dvb::SI::AudioStream>		audio_streams;
	std::vector<Dvb::SI::SubtitleStream>	subtitle_streams;
	std::vector<Dvb::SI::TeletextStream>	teletext_streams;
};

class StreamThread : public Thread
{
private:
	const Channel&			channel;
	DemuxerList				demuxers;
	Glib::ustring			fifo_path;
	gint					CRC32[256];
	Glib::StaticRecMutex	mutex;
	Dvb::Frontend&			frontend;
	Engine*					engine;
	EpgThread*				epg_thread;
	Stream					stream;
	guint					pmt_pid;
	GUdpSocket*				socket;
	GInetAddr*				inet_address;
	gboolean				broadcast_failure_message;
	gint					recording_fd;
	Glib::RefPtr<Glib::IOChannel> output_channel;

	void run();
	gboolean is_pid_used(guint pid);
	void build_pat(gchar* buffer);
	void build_pmt(gchar* buffer);
	void calculate_crc(guchar *p_begin, guchar *p_end);
	void write(gchar* buffer, gsize length);

	static gboolean on_timeout(gpointer data);
	void on_timeout();

	void remove_all_demuxers();
	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);
	void setup_dvb();

	void start_epg_thread();
	void stop_epg_thread();
		
public:
	StreamThread(const Channel& channel);
	~StreamThread();

	void start();
	const Stream& get_stream() const;

	void start_recording(const Glib::ustring& filename);
	void stop_recording();
	
	void start_broadcasting();
	void stop_broadcasting();

	const Glib::ustring& get_fifo_path() const;
};

#endif
