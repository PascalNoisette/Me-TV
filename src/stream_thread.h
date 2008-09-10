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
	void clear()
	{
		video_streams.clear();
		audio_streams.clear();
		subtitle_streams.clear();
		teletext_streams.clear();
	}
	
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
	gint					CRC32[256];
	Glib::StaticRecMutex	mutex;
	Dvb::Frontend&			frontend;
	Engine*					engine;
	EpgThread*				epg_thread;
	Stream					stream;
	guint					pmt_pid;
	Glib::ustring			fifo_path;
	GUdpSocket*				socket;
	GInetAddr*				inet_address;
	gboolean				manual_recording;
	gboolean				broadcast_failure_message;
	gint					output_fd;
	gint					recording_fd;
	guint					timeout_source;
	sigc::connection		show_connection;
	sigc::connection		hide_connection;
	sigc::connection		connection_configure;

	bool on_drawing_area_configure_event(GdkEventConfigure* event);
	void on_record_state_changed(gboolean record_state, const Glib::ustring& filename, gboolean manual);
	void on_mute_state_changed(gboolean mute_state);
	void on_broadcast_state_changed(gboolean broadcast_state);
	void on_main_window_show();
	void on_main_window_hide();

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
	void setup_dvb(Dvb::Frontend& frontend, const Channel& channel);

	void start_epg_thread();
	void stop_epg_thread();
		
public:
	StreamThread(const Channel& channel);
	~StreamThread();

	void start();
	void start_engine();
	void stop_engine();
	Engine& get_engine();

	gboolean is_manual_recording();
	gboolean is_recording();
	gboolean is_broadcasting();
};

#endif
