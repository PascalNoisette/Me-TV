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

#include <glibmm.h>
#include "stream_manager.h"
#include "application.h"
#include "device_manager.h"
#include "dvb_transponder.h"
#include "dvb_si.h"

#define TS_PACKET_SIZE		188
#define PACKET_BUFFER_SIZE	100

class Lock : public Glib::RecMutex::Lock
{
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(mutex) {}
	~Lock() {}
};

StreamManager::StreamManager() :
	Thread("Stream")
{
	g_debug("Creating StreamManager");
	g_static_rec_mutex_init(mutex.gobj());
	
	epg_thread = NULL;

	g_debug("StreamManager created");
}

StreamManager::~StreamManager()
{
	g_debug("Destroying StreamManager");
	stop_epg_thread();
	join(true);
	g_debug("StreamManager destroyed");
}

void StreamManager::start()
{
	g_debug("Starting stream thread");
	Thread::start();
}

void StreamManager::run()
{
	guchar buffer[TS_PACKET_SIZE * PACKET_BUFFER_SIZE];
	guchar pat[TS_PACKET_SIZE];
	guchar pmt[TS_PACKET_SIZE];

	Dvb::Frontend& frontend = get_application().device_manager.get_frontend();
	Glib::ustring input_path = frontend.get_adapter().get_dvr_path();

	Glib::RefPtr<Glib::IOChannel> input_channel = Glib::IOChannel::create_from_file(input_path, "r");
	input_channel->set_flags(input_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
	input_channel->set_encoding("");
	
	guint last_insert_time = 0;
	gsize bytes_read;
	
	while (!is_terminated())
	{
		TRY

		// Insert PAT/PMT every second second
		time_t now = time(NULL);
		if (now - last_insert_time > 2)
		{
			g_debug("Writing PAT/PMT header");
			
			for (std::list<ChannelStream>::iterator iterator = streams.begin(); iterator != streams.end(); iterator++)
			{
				ChannelStream& channel_stream = *iterator;

				channel_stream.stream.build_pat(pat);
				channel_stream.stream.build_pmt(pmt);

				channel_stream.write(pat, TS_PACKET_SIZE);
				channel_stream.write(pmt, TS_PACKET_SIZE);
			}
			last_insert_time = now;
		}

		if (input_channel->read((gchar*)buffer, TS_PACKET_SIZE * PACKET_BUFFER_SIZE, bytes_read) != Glib::IO_STATUS_NORMAL)
		{
			usleep(10000);
		}
		else
		{
			for (guint i = 0; i < bytes_read; i += TS_PACKET_SIZE)
			{
				guint pid = ((buffer[i+1] & 0x1f) << 8) + buffer[i+2];
			
				for (std::list<ChannelStream>::iterator iterator = streams.begin(); iterator != streams.end(); iterator++)
				{
					ChannelStream& channel_stream = *iterator;
					if (channel_stream.stream.contains_pid(pid))
					{
						channel_stream.write(buffer+i, TS_PACKET_SIZE);
					}
				}
			}
		}
		
		THREAD_CATCH
	}
		
	g_debug("StreamManager loop exited");
	
	Lock lock(mutex, "StreamManager::run() - exit");

	g_debug("Removing streams ...");
	std::list<ChannelStream>::iterator iterator = streams.begin();
	while (iterator != streams.end())
	{
		(*iterator).output_channel.reset();
		streams.pop_back();
		iterator = streams.begin();
	}
	g_debug("Streams removed");

	g_debug("About to close input channel ...");
	input_channel->close(true);
	input_channel.reset();
	g_debug("Input channel reset");
}

void StreamManager::ChannelStream::clear_demuxers()
{
	Lock lock(mutex, "ChannelStream::clear_demuxers()");
	
	g_debug("Removing demuxers");
	while (demuxers.size() > 0)
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer& StreamManager::ChannelStream::add_pes_demuxer(const Glib::ustring& demux_path,
	guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
{	
	Lock lock(mutex, "ChannelStream::add_pes_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug("Setting %s PID filter to %d (0x%X)", type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer& StreamManager::ChannelStream::add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id)
{	
	Lock lock(mutex, "StreamManager::add_section_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void StreamManager::setup_dvb(const Channel& channel, StreamManager::ChannelStream& channel_stream)
{
	Lock lock(mutex, "StreamManager::setup_dvb()");
	
	Dvb::Frontend& frontend = get_application().device_manager.get_frontend();
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();

	Buffer buffer;	

	channel_stream.clear_demuxers();
	if (channel.transponder.frontend_parameters.frequency != frontend.get_frontend_parameters().frequency)
	{
		stop_epg_thread();		
		frontend.tune_to(channel.transponder);

		Dvb::SI::SectionParser parser;
		Dvb::SI::ServiceDescriptionSection sds;

		g_debug("Getting Service Description Section");
		Dvb::Demuxer demuxer_sds(demux_path);
		demuxer_sds.set_filter(SDT_PID, SDT_ID);
		parser.parse_sds(demuxer_sds, sds);
		demuxer_sds.stop();

		if (sds.epg_events_available)
		{
			g_debug("EPG events are available on this transponder, starting EPG thread");
			start_epg_thread();
		}
		else
		{
			g_debug("EPG events are not available on this transponder");
		}
	}
	
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	demuxer_pat.read_section(buffer);
	demuxer_pat.stop();

	Mpeg::Stream& stream = channel_stream.stream;
	stream.clear();
	
	stream.set_pmt_pid(buffer, channel.service_id);
	
	Dvb::Demuxer demuxer_pmt(demux_path);
	demuxer_pmt.set_filter(stream.get_pmt_pid(), PMT_ID, 0xFF);
	demuxer_pmt.read_section(buffer);
	demuxer_pmt.stop();

	stream.parse_pms(buffer);

	guint pcr_pid = stream.get_pcr_pid();
	if (pcr_pid != 0x1FFF)
	{
		channel_stream.add_pes_demuxer(demux_path, pcr_pid, DMX_PES_OTHER, "PCR");
	}
	
	gsize video_streams_size = stream.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.video_streams[i].pid, DMX_PES_OTHER, "video");
	}

	gsize audio_streams_size = stream.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.audio_streams[i].pid, DMX_PES_OTHER, "audio");
	}
				
	gsize subtitle_streams_size = stream.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
	}

	gsize teletext_streams_size = stream.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		channel_stream.add_pes_demuxer(demux_path, stream.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
	}

	g_debug("Finished setting up DVB");
}

void StreamManager::start_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, "StreamManager::start_epg_thread()");

		stop_epg_thread();
		epg_thread = new EpgThread();
		epg_thread->start();
		g_debug("EPG thread started");
	}
}

void StreamManager::stop_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, "StreamManager::stop_epg_thread()");

		if (epg_thread != NULL)
		{
			g_debug("Stopping EPG thread");
			delete epg_thread;
			epg_thread = NULL;
			g_debug("EPG thread stopped");
		}
	}
}

const StreamManager::ChannelStream& StreamManager::get_display_stream()
{
	Lock lock(mutex, "StreamManager::get_display_stream()");

	if (streams.size() == 0)
	{
		throw Exception(_("Display stream has not been created"));
	}
	
	return (*(streams.begin()));
}

void StreamManager::set_display_stream(const Channel& channel)
{
	g_debug("StreamManager::set_display(%s)", channel.name.c_str());
	Lock lock(mutex, "StreamManager::set_display_stream()");
	
	if (streams.size() == 0)
	{
		g_debug("Creating new stream output");
		
		Dvb::Frontend& frontend = get_application().device_manager.get_frontend();
		
		Glib::ustring filename = Glib::ustring::compose("me-tv-%1.fifo", frontend.get_adapter().get_index());
		Glib::ustring fifo_path = Glib::build_filename(get_application().get_application_dir(), filename);
 
		if (Glib::file_test(fifo_path, Glib::FILE_TEST_EXISTS))
		{
			unlink(fifo_path.c_str());
		}

		if (mkfifo(fifo_path.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
		{
			throw Exception(Glib::ustring::compose(_("Failed to create FIFO '%1'"), fifo_path));
		}

		// Fudge the channel open.  Allows Glib::IO_FLAG_NONBLOCK
		int fd = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
		if (fd == -1)
		{
			throw SystemException(Glib::ustring::compose(_("Failed to open FIFO for reading '%1'"), fifo_path));
		}

		ChannelStream channel_stream(CHANNEL_STREAM_TYPE_DISPLAY, channel, fifo_path);
		streams.push_back(channel_stream);

		close(fd);
	}
	else
	{
		g_debug("Stream output exists");
	}

	ChannelStream& stream = *(streams.begin());
	stream.channel = channel;
	setup_dvb(channel, stream);
}

bool is_recording_stream(StreamManager::ChannelStream& channel_stream)
{
	return channel_stream.type == StreamManager::CHANNEL_STREAM_TYPE_RECORDING;
}

void StreamManager::start_recording(const Channel& channel, const Glib::ustring& filename, gboolean scheduled)
{
	Lock lock(mutex, "StreamManager::start_recording()");
	
	ChannelStreamType requested_type = scheduled ? StreamManager::CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING :
		StreamManager::CHANNEL_STREAM_TYPE_RECORDING;
	ChannelStreamType current_type = StreamManager::CHANNEL_STREAM_TYPE_NONE;

	std::list<StreamManager::ChannelStream>::iterator iterator = streams.begin();
		
	if (iterator != streams.end())
	{
		iterator++;
	}
	
	while (iterator != streams.end())
	{
		StreamManager::ChannelStream& channel_stream = *iterator;
		if (
		    channel_stream.channel == channel &&
		    (
		        channel_stream.type == StreamManager::CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING ||
				channel_stream.type == StreamManager::CHANNEL_STREAM_TYPE_RECORDING)
		    )
		{
			current_type = channel_stream.type;
			break;
		}
		iterator++;
	}

	// No change required
	if (current_type == requested_type)
	{
		g_debug("Channel '%s' is currently being recorded (%s)",
		    channel.name.c_str(), scheduled ? "scheduled" : "manual");
	}
	else
	{
		// If SR requested but recording is currently manual then stop the current manual one
		if (requested_type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING && current_type == CHANNEL_STREAM_TYPE_RECORDING)
		{
			stop_recording(channel);
		}

		if (requested_type == CHANNEL_STREAM_TYPE_RECORDING && current_type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING)
		{
			g_debug("Ignoring request to manually record a channel that is currently scheduled for recording");
		}
		else
		{
			g_debug("Channel '%s' is currently not being recorded", channel.name.c_str());

			if (channel.transponder != get_application().channel_manager.get_display_channel().transponder)
			{
				g_debug("Need to change transponders to record this channel");

				if (scheduled)
				{
					// Need to kill all current recordings
					streams.remove_if(is_recording_stream);
				}
			
				get_application().set_display_channel(channel);
			}

			ChannelStream channel_stream(
			scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING : CHANNEL_STREAM_TYPE_RECORDING,
				channel, filename);
			setup_dvb(channel, channel_stream);
			streams.push_back(channel_stream);
		}
	}
		
	
	g_debug("New recording channel created");
}

void StreamManager::stop_recording(const Channel& channel)
{
	Lock lock(mutex, "StreamManager::stop_recording()");

	std::list<ChannelStream>::iterator iterator = streams.begin();

	if (iterator != streams.end())
	{
		iterator++;
	}

	while (iterator != streams.end())
	{
		ChannelStream& channel_stream = *iterator;
		if (channel_stream.channel == channel)
		{
			channel_stream.output_channel.reset();
			iterator = streams.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}
}

guint StreamManager::get_last_epg_update_time()
{
	guint result = 0;

	Lock lock(mutex, "StreamManager::get_epg_last_update_time()");
	if (epg_thread != NULL)
	{
		result = epg_thread->get_last_epg_update_time();
	}
	
	return result;
}

StreamManager::ChannelStream::ChannelStream(ChannelStreamType t, const Channel& c, const Glib::ustring& f)
{
	g_static_rec_mutex_init(mutex.gobj());

	type = t;
	channel = c;
	filename = f;
	output_channel = Glib::IOChannel::create_from_file(filename, "w");
	output_channel->set_encoding("");
	output_channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
	output_channel->set_buffer_size(TS_PACKET_SIZE * PACKET_BUFFER_SIZE);
	
	g_debug("Added new output stream '%s' -> '%s'", channel.name.c_str(), filename.c_str());
}

void StreamManager::ChannelStream::write(guchar* buffer, gsize length)
{
	if (output_channel)
	{
		try
		{
			gsize bytes_written = 0;
			output_channel->write((const gchar*)buffer, length, bytes_written);
		}
		catch(...)
		{
			static time_t previous = 0;
			time_t now = time(NULL);
			if (now != previous)
			{
				g_debug("No output connected");
				previous = now;
			}
		}
	}
}

gboolean StreamManager::is_recording()
{
	return streams.size() > 1;
}

gboolean StreamManager::is_recording(const Channel& channel)
{
	std::list<ChannelStream>::iterator iterator = streams.begin();

	// Skip over display stream
	if (iterator != streams.end())
	{
		iterator++;
	}

	while (iterator != streams.end())
	{
		ChannelStream& stream = *iterator;
		if (stream.channel == channel)
		{
			return true;
		}
		iterator++;
	}
	return false;
}
