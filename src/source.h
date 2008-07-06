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

#ifndef __SOURCE_H__
#define __SOURCE_H__

#include "channels_dialog.h"
#include "dvb_frontend.h"
#include "dvb_demuxer.h"
#include "thread.h"
#include "sink.h"

typedef enum
{
	SOURCE_TYPE_STREAM,
	SOURCE_TYPE_FILE
} SourceType;

class Source
{
private:
	Glib::ustring					mrl;
	DemuxerList						demuxers;
	AVFormatContext*				format_context;
	Glib::ustring					post_command;
	Glib::RefPtr<Glib::IOChannel>	input_channel;
	Buffer							buffer;
	gboolean						opened;
	gsize							buffer_bytes_read;
	SourceType						source_type;
		
	void execute_command(const Glib::ustring& command);
	void remove_all_demuxers();
	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text);
	Dvb::Demuxer& add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id);
	void setup_dvb(Dvb::Frontend& frontend, const Channel& channel);
	Dvb::Frontend& get_frontend();
	void create();

	static int read_data(void* data, guchar* buffer, int size);
	int read_data(guchar* buffer, int size);

public:
	Source(const Channel& channel);
	Source(const Glib::ustring& mrl);
	~Source();
	
	void seek(gint64 position);
	gint64 get_position();

	gboolean read(AVPacket* packet);

	AVFormatContext* get_format_context() const { return format_context; }
	AVStream* get_stream(guint index) const;
	gsize get_stream_count() const { return format_context->nb_streams; }
};

#endif
