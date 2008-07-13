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

#include "source.h"
#include "dvb_frontend.h"
#include "dvb_si.h"
#include "scheduler.h"
#include "application.h"

#define TS_PACKET_SIZE			188
#define BUFFER_SIZE				TS_PACKET_SIZE * 100
#define TIME_SHIFT_BUFFER_SIZE	1000000

Source::Source(const Channel& channel) : buffer(BUFFER_SIZE)
{	
	g_message(_("Creating source for '%s'"), channel.name.c_str());

	mrl = channel.mrl;
	
	if (!channel.pre_command.empty())
	{
		execute_command(channel.pre_command);
	}

	post_command = channel.post_command;
	
	if (channel.flags & CHANNEL_FLAG_DVB)
	{
		source_type = SOURCE_TYPE_STREAM;
		Dvb::Frontend& frontend = get_frontend();
		if (mrl.empty())
		{
			mrl = frontend.get_adapter().get_dvr_path();
		}

		setup_dvb(frontend, channel);
	}
	
	create();
}

Source::Source(const Glib::ustring& mrl) : buffer(BUFFER_SIZE)
{
	this->mrl = mrl;
	source_type = SOURCE_TYPE_FILE;
	create();
}

Source::~Source()
{
	g_debug("Destroying source");

	remove_all_demuxers();
	
	if (format_context != NULL)
	{
		av_close_input_file(format_context);
		format_context = NULL;
	}

	if (!post_command.empty())
	{
		execute_command(post_command);
	}
	
	if (format_context != NULL)
	{
		av_close_input_file(format_context);
		format_context = NULL;
	}
	g_debug("Source destroyed");
}

int Source::read_data(void* data, guchar* buffer, int size)
{
	Source* source = (Source*)data;
	return source->read_data(buffer, size);
}

Glib::RefPtr<Glib::IOChannel> writer = Glib::IOChannel::create_from_file("/home/michael/.me-tv/video.fifo", "w");

int Source::read_data(guchar* destination_buffer, int size)
{
	gsize bytes_read = 0;

	/*if (!opened)
	{
		memcpy(destination_buffer, buffer.get_data(), buffer_bytes_read);
		bytes_read = size;
	}
	else*/
	{
		input_channel->read((gchar*)destination_buffer, size, bytes_read);
	}
	
	gsize bytes_written;
	writer->write((gchar*)destination_buffer, bytes_read, bytes_written);

	return bytes_read;
}

void Source::create()	
{	
	format_context = NULL;
	writer->set_encoding("");

	av_register_all();
	
	if (source_type == SOURCE_TYPE_STREAM)
	{		
		input_channel = Glib::IOChannel::create_from_file(mrl, "r");
		input_channel->set_encoding("");

		AVInputFormat* input_format = av_find_input_format("mpegts");
		if(input_format == NULL)
		{
			throw Exception(_("Failed to find input format"));
		}
		input_format->flags |= AVFMT_NOFILE; 

		g_debug("Reading sample packets");
		gboolean got_pat = false;
		while (!got_pat)
		{
			input_channel->read((gchar*)buffer.get_data(), buffer.get_length(), buffer_bytes_read);
			g_debug("Read %d sample packets", buffer_bytes_read/TS_PACKET_SIZE);

			guchar* b = buffer.get_data();
			for (int i = 0; i < buffer_bytes_read; i += TS_PACKET_SIZE)
			{
				guint sync = b[i];
				guint pid = ((b[i+1] & 0x1F)<<8) + b[i+2];
				if (sync == 0x47 && pid == 0)
				{
					g_debug("PAT read");
					got_pat = true;
				}
			}
		}

		opened = false;
		ByteIOContext io_context;
		if (init_put_byte(&io_context, buffer.get_data(), buffer.get_length(), 0, this, Source::read_data, NULL, NULL) < 0)
		{
			throw Exception("Failed to initialise byte IO context");
		}
		
		g_debug("Opening '%s' stream", mrl.c_str());
		if (av_open_input_stream(&format_context, &io_context, "", input_format, NULL) < 0) 
		{
			throw Exception("Failed to open input stream: " + mrl);
		}
		opened = true;
	}
	else if (source_type == SOURCE_TYPE_FILE)
	{
		g_debug("Opening '%s' file", mrl.c_str());
		if (av_open_input_file(&format_context, mrl.c_str(), NULL, 0, NULL) < 0) 
		{
			throw Exception("Failed to open input file: " + mrl);
		}
	}
	
	g_debug("'%s' opened", mrl.c_str());

	if (av_find_stream_info(format_context)<0)
	{
		throw Exception(_("Couldn't find stream information"));
	}

	dump_format(format_context, 0, mrl.c_str(), false);
}

void Source::remove_all_demuxers()
{
	while (demuxers.size() > 0)
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer& Source::add_pes_demuxer(const Glib::ustring& demux_path,
	guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
{	
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug(_("Setting %s PID filter to %d (0x%X)"), type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer& Source::add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id)
{	
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void Source::setup_dvb(Dvb::Frontend& frontend, const Channel& channel)
{
	g_debug("Setting up DVB");
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	
	remove_all_demuxers();
	
	Dvb::Transponder transponder;
	transponder.frontend_parameters = channel.frontend_parameters;
	frontend.tune_to(transponder);
	
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	Dvb::SI::SectionParser parser;
	
	Dvb::SI::ProgramAssociationSection pas;
	parser.parse_pas(demuxer_pat, pas);
	demuxer_pat.stop();

	guint length = pas.program_associations.size();
	guint pmt_pid = 0;
	
	g_debug("Found %d associations", length);
	for (guint i = 0; i < length; i++)
	{
		Dvb::SI::ProgramAssociation program_association = pas.program_associations[i];
		
		if (program_association.program_number == channel.service_id)
		{
			pmt_pid = program_association.program_map_pid;
		}
	}
	
	if (pmt_pid == 0)
	{
		throw Exception(_("Failed to find PMT ID for service"));
	}
	
	Dvb::Demuxer demuxer_pmt(demux_path);
	demuxer_pmt.set_filter(pmt_pid, PMT_ID);

	Dvb::SI::ProgramMapSection pms;
	parser.parse_pms(demuxer_pmt, pms);
	demuxer_pmt.stop();
		
	add_pes_demuxer(demux_path, PAT_PID, DMX_PES_OTHER, "PAT");
	add_pes_demuxer(demux_path, pmt_pid, DMX_PES_OTHER, "PMT");
	
	gsize video_streams_size = pms.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.video_streams[i].pid, DMX_PES_OTHER, "video");
	}

	gsize audio_streams_size = pms.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		if (pms.audio_streams[i].is_ac3)
		{
			g_debug("Ignoring AC3 stream");
		}
		else
		{
			add_pes_demuxer(demux_path, pms.audio_streams[i].pid, DMX_PES_OTHER, "audio");
		}
	}
				
	gsize subtitle_streams_size = pms.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
	}

	gsize teletext_streams_size = pms.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		g_debug("Ignoring TT stream");
		//add_pes_demuxer(demux_path, pms.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
	}
	g_debug("Finished setting up DVB");
}

Dvb::Frontend& Source::get_frontend()
{
	Event event(0, 0);
	Dvb::Frontend* frontend = get_application().get_device_manager().request_frontend(event);
	if (frontend == NULL)
	{
		throw Exception(_("No frontend available"));
	}
	return *frontend;
}

void Source::execute_command(const Glib::ustring& command)
{
	std::string standard_output;
	std::string standard_error;
	
	g_debug("Command: '%s'", command.c_str());
	Glib::spawn_command_line_sync(command, &standard_output, &standard_error, NULL);
	if (!standard_output.empty())
	{
		g_message("standard_output: '%s'", standard_output.c_str());
	}
	if (!standard_error.empty())
	{
		throw Exception(Glib::ustring::compose("standard_error: '%1'", standard_error));
	}
	g_debug("Command complete");
}

AVStream* Source::get_stream(guint index) const
{
	if (index >= format_context->nb_streams)
	{
		throw Exception(_("Stream index out of bounds"));
	}
	return format_context->streams[index];
}

gboolean Source::read(AVPacket* packet)
{
	return av_read_frame(format_context, packet) >= 0;
}

gdouble Source::get_duration()
{
	if (format_context == NULL)
	{
		throw Exception("format_context is NULL");
	}
	
	gdouble duration = format_context->duration;
	return duration == AV_NOPTS_VALUE ? 0 : duration;
}

gdouble Source::get_start_time()
{
	if (format_context == NULL)
	{
		throw Exception("format_context is NULL");
	}
	
	gdouble start_time = format_context->start_time;
	return start_time == AV_NOPTS_VALUE ? 0 : start_time / AV_TIME_BASE;
}
