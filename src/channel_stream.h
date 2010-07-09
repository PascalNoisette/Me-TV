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
 
#ifndef __CHANNEL_STREAM_H__
#define __CHANNEL_STREAM_H__

#include "mpeg_stream.h"
#include "dvb_demuxer.h"
#include "channel.h"

typedef enum
{
	CHANNEL_STREAM_TYPE_NONE = -1,
	CHANNEL_STREAM_TYPE_DISPLAY = 0,
	CHANNEL_STREAM_TYPE_RECORDING = 1,
	CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING = 2
} ChannelStreamType;

class ChannelStream
{
private:
	Dvb::DemuxerList				demuxers;
	Glib::StaticRecMutex			mutex;
		
public:
	ChannelStream(ChannelStreamType type, Channel& channel, const Glib::ustring& filename,
		const Glib::ustring& description);
	~ChannelStream();

	Glib::RefPtr<Glib::IOChannel>	output_channel;
	Mpeg::Stream					stream;
	Glib::ustring					filename;
	Glib::ustring					description;
	ChannelStreamType				type;
	Channel&						channel;

	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);

	void write(guchar* buffer, gsize length);
	void clear_demuxers();
};

typedef std::list<ChannelStream*> ChannelStreamList;

#endif
