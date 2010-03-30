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

#include "frontend_thread.h"
#include "application.h"
#include "dvb_si.h"

class Lock : public Glib::RecMutex::Lock
{
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(mutex) {}
	~Lock() {}
};

FrontendThread::FrontendThread(Dvb::Frontend& f) : Thread("Frontend"), frontend(f)
{
	g_debug("Creating FrontendThread");
	g_static_rec_mutex_init(mutex.gobj());
	
	epg_thread = NULL;

	g_debug("FrontendThread created");
}

FrontendThread::~FrontendThread()
{
	g_debug("Destroying FrontendThread");
	stop();
}

void FrontendThread::start()
{
	g_debug("Starting frontend thread");
	Thread::start();
}

void FrontendThread::stop()
{
	g_debug("Stopping frontend thread");
	stop_epg_thread();
	join(true);
	g_debug("Frontend thread stopped");
}

void FrontendThread::run()
{
	g_debug("Frontend thread running");

	guchar buffer[TS_PACKET_SIZE * PACKET_BUFFER_SIZE];
	guchar pat[TS_PACKET_SIZE];
	guchar pmt[TS_PACKET_SIZE];

	Glib::ustring input_path = frontend.get_adapter().get_dvr_path();

	int fd = 0;
	if ( (fd = ::open( input_path.c_str(), O_RDONLY | O_NONBLOCK) ) < 0 )
	{
		throw SystemException("Failed to open frontend");
	}
		
	g_debug("Opening frontend device '%s' for reading ...", input_path.c_str());
	Glib::RefPtr<Glib::IOChannel> input_channel = Glib::IOChannel::create_from_fd(fd);
	input_channel->set_encoding("");
	g_debug("Frontend device opened");
	
	guint last_insert_time = 0;
	gsize bytes_read;
	
	while (!is_terminated())
	{
		try
		{
			// Insert PAT/PMT every second second
			time_t now = time(NULL);
			if (now - last_insert_time > 2)
			{
				g_debug("Writing PAT/PMT header");
			
				for (std::list<ChannelStream>::iterator i = streams.begin(); i != streams.end(); i++)
				{
					ChannelStream& channel_stream = *i;

					channel_stream.stream.build_pat(pat);
					channel_stream.stream.build_pmt(pmt);

					channel_stream.write(pat, TS_PACKET_SIZE);
					channel_stream.write(pmt, TS_PACKET_SIZE);
				}
				last_insert_time = now;
			}

			if (input_channel->read((gchar*)buffer, TS_PACKET_SIZE * PACKET_BUFFER_SIZE, bytes_read) != Glib::IO_STATUS_NORMAL)
			{
				g_debug("Input channel read failed");
				usleep(1000000);
			}
			else
			{
				for (guint offset = 0; offset < bytes_read; offset += TS_PACKET_SIZE)
				{
					guint pid = ((buffer[offset+1] & 0x1f) << 8) + buffer[offset+2];

					for (std::list<ChannelStream>::iterator i = streams.begin(); i != streams.end(); i++)
					{
						ChannelStream& channel_stream = *i;
						if (channel_stream.stream.contains_pid(pid))
						{
							channel_stream.write(buffer+offset, TS_PACKET_SIZE);
						}
					}
				}
			}
		}
		catch(...)
		{
		}
	}
		
	g_debug("FrontendThread loop exited");
	
	Lock lock(mutex, __PRETTY_FUNCTION__);

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

void FrontendThread::setup_dvb(ChannelStream& channel_stream)
{
	Lock lock(mutex, __PRETTY_FUNCTION__);
	
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	Buffer buffer;
	const Channel& channel = channel_stream.channel;
	
	channel_stream.clear_demuxers();
	if (channel.transponder.frontend_parameters.frequency != frontend.get_frontend_parameters().frequency)
	{
		stop_epg_thread();
		frontend.tune_to(channel.transponder);
		start_epg_thread();
	}
	
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	demuxer_pat.read_section(buffer);
	demuxer_pat.stop();

	Mpeg::Stream& stream = channel_stream.stream;
	stream.clear();
	
	stream.set_pmt_pid(buffer, channel.service_id);
	
	Dvb::Demuxer demuxer_pmt(demux_path);
	demuxer_pmt.set_filter(stream.get_pmt_pid(), PMT_ID);
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

void FrontendThread::start_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, __PRETTY_FUNCTION__);

		stop_epg_thread();
		epg_thread = new EpgThread();
		epg_thread->start();
		g_debug("EPG thread started");
	}
}

void FrontendThread::stop_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, __PRETTY_FUNCTION__);

		if (epg_thread != NULL)
		{
			g_debug("Stopping EPG thread");
			delete epg_thread;
			epg_thread = NULL;
			g_debug("EPG thread stopped");
		}
	}
}

const ChannelStream& FrontendThread::get_display_stream()
{
	Lock lock(mutex, __PRETTY_FUNCTION__);

	if (streams.size() == 0)
	{
		throw Exception(_("Display stream has not been created"));
	}
	
	return (*(streams.begin()));
}

void FrontendThread::set_display_stream(const Channel& channel)
{
	g_debug("FrontendThread::set_display_stream(%s)", channel.name.c_str());
	Lock lock(mutex, __PRETTY_FUNCTION__);
	
	if (streams.size() == 0)
	{
		g_debug("Creating new stream output");
		
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
		g_debug("Display stream exists");
	}

	ChannelStream& stream = *(streams.begin());
	stream.channel = channel;
	try
	{
		setup_dvb(stream);
	}
	catch(...)
	{
		g_debug("DVB setup failed, terminating frontend thread");
		terminate();
		throw;
	}
}

bool is_recording_stream(ChannelStream& channel_stream)
{
	return channel_stream.type == CHANNEL_STREAM_TYPE_RECORDING;
}

void FrontendThread::start_recording(const Channel& channel, const Glib::ustring& filename, gboolean scheduled)
{
	Lock lock(mutex, __PRETTY_FUNCTION__);
	
	ChannelStreamType requested_type = scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING :
		CHANNEL_STREAM_TYPE_RECORDING;
	ChannelStreamType current_type = CHANNEL_STREAM_TYPE_NONE;

	std::list<ChannelStream>::iterator iterator = streams.begin();
		
	if (iterator != streams.end())
	{
		iterator++;
	}
	
	while (iterator != streams.end())
	{
		ChannelStream& channel_stream = *iterator;
		if (
		    channel_stream.channel == channel &&
		    (
		        channel_stream.type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING ||
				channel_stream.type == CHANNEL_STREAM_TYPE_RECORDING)
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
			setup_dvb(channel_stream);
			streams.push_back(channel_stream);
		}
	}
		
	
	g_debug("New recording channel created");
}

void FrontendThread::stop_recording(const Channel& channel)
{
	Lock lock(mutex, __PRETTY_FUNCTION__);

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

guint FrontendThread::get_last_epg_update_time()
{
	guint result = 0;

	Lock lock(mutex, __PRETTY_FUNCTION__);
	if (epg_thread != NULL)
	{
		result = epg_thread->get_last_epg_update_time();
	}
	
	return result;
}

gboolean FrontendThread::is_recording()
{
	Lock lock(mutex, __PRETTY_FUNCTION__);
	return streams.size() > 1;
}

gboolean FrontendThread::is_recording(const Channel& channel)
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
