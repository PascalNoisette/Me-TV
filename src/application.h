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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include <giomm.h>
#include "config.h"
#include "device_manager.h"
#include "profile_manager.h"
#include "channel_manager.h"
#include "dvb_demuxer.h"
#include "ffmpeg_renderer.h"
#include "dvb_si.h"

typedef std::list<Dvb::Demuxer*> DemuxerList;

class DvbThread : public Thread
{
private:
	DemuxerList		demuxers;
	FFMpegRenderer&	renderer;
	Dvb::Frontend&	frontend;
	Channel&		channel;

	void remove_all_demuxers()
	{
		while (demuxers.size() > 0)
		{
			Dvb::Demuxer* demuxer = demuxers.front();
			demuxers.pop_front();
			delete demuxer;
		}
	}

	Dvb::Demuxer& add_pes_demuxer(const Glib::ustring& demux_path,
		guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
	{	
		Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
		demuxers.push_back(demuxer);
		g_debug(_("Setting %s PID filter to %d (0x%X)"), type_text, pid, pid);
		demuxer->set_pes_filter(pid, pid_type);
		return *demuxer;
	}
		
	void run()
	{
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		
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

		// Only take the first video stream
		if (pms.video_streams.size() > 0)
		{
			Dvb::SI::VideoStream video_stream = pms.video_streams[0];
			add_pes_demuxer(demux_path, video_stream.pid, DMX_PES_OTHER, "video");
		}

		gsize audio_streams_size = pms.audio_streams.size();
		for (guint i = 0; i < audio_streams_size; i++)
		{
			Dvb::SI::AudioStream audio_stream = pms.audio_streams[i];
			add_pes_demuxer(demux_path, audio_stream.pid,
							DMX_PES_OTHER,
							audio_stream.is_ac3 ? "AC3" : "audio");
		}
					
		gsize subtitle_streams_size = pms.subtitle_streams.size();
		for (guint i = 0; i < subtitle_streams_size; i++)
		{
			Dvb::SI::SubtitleStream subtitle_stream = pms.subtitle_streams[i];
			add_pes_demuxer(demux_path, subtitle_stream.pid, DMX_PES_OTHER, "subtitle");
		}

		if (pms.teletext_streams.size() > 0)
		{
			Dvb::SI::TeletextStream first_teletext_stream = pms.teletext_streams[0];
			add_pes_demuxer(demux_path, first_teletext_stream.pid, DMX_PES_OTHER, "teletext");
		}
		
		g_debug(_("PIDs read successfully"));

		renderer.open("/dev/adapter0/dvr0");
		sleep(1000000000);
		
		remove_all_demuxers ();
	}

public:
	DvbThread(Dvb::Frontend& frontend, Channel& channel, FFMpegRenderer& renderer) :
		frontend(frontend), channel(channel), renderer(renderer) {}
};

class Application : public Gnome::Main
{
private:
	static Application* current;
	Glib::RefPtr<Gnome::Glade::Xml> glade;
	ProfileManager		profile_manager;
	Dvb::DeviceManager	device_manager;
	ChannelManager		channel_manager;
	FFMpegRenderer		renderer;
	DvbThread*			dvb_thread;

	void on_active_channel_changed(Channel& channel);

public:
	Application(int argc, char *argv[]);
	void run();
	static Application& get_current();
	
	ProfileManager& get_profile_manager()		{ return profile_manager; }
	Dvb::DeviceManager& get_device_manager()	{ return device_manager; }
	ChannelManager& get_channel_manager()		{ return channel_manager; }
	FFMpegRenderer& get_renderer()				{ return renderer; }
};

#endif
