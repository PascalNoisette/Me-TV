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

#include "channel_stream.h"
#include "dvb_si.h"

class Lock: public Glib::Threads::RecMutex::Lock {
public:
	Lock(Glib::Threads::RecMutex & mutex, Glib::ustring const & name) :
		Glib::Threads::RecMutex::Lock(mutex) {}
	~Lock() { }
};

ChannelStream::ChannelStream(ChannelStreamType t, Channel & c, Glib::ustring const & f, Glib::ustring const & d)
	: channel(c) {
	type = t;
	filename = f;
	description = d;
	last_insert_time = 0;
	g_debug("Added new channel stream '%s' -> '%s'", channel.name.c_str(), filename.c_str());
}

ChannelStream::~ChannelStream() {
	g_debug("Destroying channel stream '%s' -> '%s'", channel.name.c_str(), filename.c_str());
	clear_demuxers();
	if (output_channel) { output_channel.reset(); }
}

void ChannelStream::open() {
	output_channel = Glib::IOChannel::create_from_file(filename, "w");
	output_channel->set_encoding("");
	output_channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
	output_channel->set_buffer_size(TS_PACKET_SIZE * PACKET_BUFFER_SIZE);
}

void ChannelStream::clear_demuxers() {
	Lock lock(mutex, "ChannelStream::clear_demuxers()");
	g_debug("Removing demuxers");
	while (!demuxers.empty()) {
		Dvb::Demuxer * demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer & ChannelStream::add_pes_demuxer(Glib::ustring const & demux_path,
																						 guint pid, dmx_pes_type_t pid_type, gchar const * type_text) {
	Lock lock(mutex, "ChannelStream::add_pes_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug("Setting %s PID filter to %d (0x%X)", type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer & ChannelStream::add_section_demuxer(Glib::ustring const & demux_path, guint pid, guint id) {
	Lock lock(mutex, "FrontendThread::add_section_demuxer()");
	Dvb::Demuxer * demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void ChannelStream::write(guchar * buffer, gsize length) {
	if (!output_channel) { open(); }
	try {
		time_t now = time(NULL);
		if (now - last_insert_time > 2) {
			last_insert_time = now;
			guchar data[TS_PACKET_SIZE];
			stream.build_pat(data);
			write(data, TS_PACKET_SIZE);
			stream.build_pmt(data);
			write(data, TS_PACKET_SIZE);
		}
		gsize bytes_written = 0;
		output_channel->write((const gchar*)buffer, length, bytes_written);
	}
	catch (...) { g_debug("Failed to write to output channel"); }
}
