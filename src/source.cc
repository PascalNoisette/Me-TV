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

Source::Source(PacketQueue& packet_queue, const Channel& channel) : Thread("Source"), packet_queue(packet_queue)
{
	Dvb::Frontend& frontend = get_frontend();
	
	this->mrl = frontend.get_adapter().get_dvr_path();

	if (!channel.pre_command.empty())
	{
		execute_command(channel.pre_command);
	}
	
	if (channel.flags | CHANNEL_FLAG_DVB)
	{
		setup_dvb(frontend, channel);
	}
	create();
}

Source::Source(PacketQueue& packet_queue, const Glib::ustring& mrl) : Thread("Source"), packet_queue(packet_queue)
{
	this->mrl = mrl;
	create();
}

Source::~Source()
{
	if (format_context != NULL)
	{
		av_close_input_file(format_context);
		format_context = NULL;
	}
}

void Source::create()
{
	format_context = NULL;

	const gchar* filename = mrl.c_str();
	av_register_all();
	
	g_debug("Opening '%s'", filename);
	if (av_open_input_file(&format_context, filename, NULL, 0, NULL) != 0)
	{
		throw Exception("Failed to open input file: " + mrl);
	}
	g_debug("'%s' opened", filename);

	if (av_find_stream_info(format_context)<0)
	{
		throw Exception(_("Couldn't find stream information"));
	}

	dump_format(format_context, 0, filename, false);
}

void Source::remove_all_demuxers()
{
	while (demuxers.size() > 0)
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
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
	
void Source::setup_dvb(Dvb::Frontend& frontend, const Channel& channel)
{
	g_debug("Setting up DVB");
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	
	remove_all_demuxers();
	
	Dvb::Transponder transponder;
	transponder.frontend_parameters = channel.frontend_parameters;
	frontend.tune_to(transponder);
	
	Dvb::Demuxer demuxer_pat(demux_path);
	Dvb::Demuxer demuxer_pmt(demux_path);
	Dvb::SI::SectionParser parser;
	
	Dvb::SI::ProgramAssociationSection pas;
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	parser.parse_pas(demuxer_pat, pas);
	guint length = pas.program_associations.size();
	guint pmt_pid = 0;
	
	g_debug(_("Found %d associations"), length);
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
	demuxer_pat.stop();
	
	demuxer_pmt.set_filter(pmt_pid, PMT_ID);
	Dvb::SI::ProgramMapSection pms;
	parser.parse_pms(demuxer_pmt, pms);
	demuxer_pmt.stop();

	gsize video_streams_size = pms.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.video_streams[i].pid, DMX_PES_OTHER, "video");
	}

	gsize audio_streams_size = pms.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.audio_streams[i].pid, DMX_PES_OTHER,
			pms.audio_streams[i].is_ac3 ? "AC3" : "audio");
	}
				
	gsize subtitle_streams_size = pms.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
	}

	gsize teletext_streams_size = pms.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
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
		throw Exception(Glib::ustring::format("standard_error: '%s'", standard_error.c_str()));
	}
	g_debug("Command complete");
}

void Source::run()
{	
	g_debug("Entering source thread loop");
	while (!is_terminated())
	{
		AVPacket packet;

		gint result = av_read_frame(format_context, &packet);

		if (result < 0)
		{
			terminate();
		}
		else
		{
			packet_queue.push(&packet);
		}
	}
	g_debug("Source thread loop finished");
}

AVStream* Source::get_stream(guint index) const
{
	if (index >= format_context->nb_streams)
	{
		throw Exception(_("Stream index out of bounds"));
	}
	return format_context->streams[index];
}

