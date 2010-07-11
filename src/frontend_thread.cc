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

#define READ_TIMEOUT 10 /* In milliseconds */

FrontendThread::FrontendThread(Dvb::Frontend& f) : Thread("Frontend"), frontend(f)
{
	g_debug("Creating FrontendThread (%s)", frontend.get_path().c_str());
	
	epg_thread = NULL;

	Glib::ustring input_path = frontend.get_adapter().get_dvr_path();

	g_debug("Opening frontend device '%s' for reading ...", input_path.c_str());
	if ( (fd = ::open(input_path.c_str(), O_RDONLY | O_NONBLOCK) ) < 0 )
	{
		throw SystemException("Failed to open frontend device");
	}
	
	g_debug("FrontendThread created (%s)", frontend.get_path().c_str());
}

FrontendThread::~FrontendThread()
{
	g_debug("Destroying FrontendThread (%s)", frontend.get_path().c_str());
	
	g_debug("About to close input channel ...");
	::close(fd);
	
	stop();
	stop_epg_thread();
	
	g_debug("FrontendThread destroyed (%s)", frontend.get_path().c_str());
}

void FrontendThread::start()
{
	if (is_terminated())
	{
		g_debug("Starting frontend thread (%s)", frontend.get_path().c_str());
		Thread::start();
	}
}

void FrontendThread::stop()
{
	g_debug("Stopping frontend thread (%s)", frontend.get_path().c_str());
	join(true);
	g_debug("Frontend thread stopped and joined (%s)", frontend.get_path().c_str());
}

void FrontendThread::run()
{
	g_debug("Frontend thread running (%s)", frontend.get_path().c_str());

	struct pollfd pfds[1];
	pfds[0].fd = fd;
	pfds[0].events = POLLIN;
	
	guchar buffer[TS_PACKET_SIZE * PACKET_BUFFER_SIZE];
	guchar pat[TS_PACKET_SIZE];
	guchar pmt[TS_PACKET_SIZE];

	guint last_insert_time = 0;
	
	g_debug("Entering FrontendThread loop (%s)", frontend.get_path().c_str());
	while (!is_terminated())
	{
		if (streams.empty())
		{
			usleep(100000);
			continue;
		}
	
		try
		{
			if (::poll(pfds, 1, READ_TIMEOUT) < 0)
			{
				throw SystemException("Frontend poll failed");
			}

			gint bytes_read = ::read(fd, buffer, TS_PACKET_SIZE * PACKET_BUFFER_SIZE);

			if (bytes_read < 0)
			{
				if (errno == EAGAIN)
				{
					continue;
				}

				Glib::ustring message = Glib::ustring::compose("Frontend read failed (%1)", frontend.get_path().c_str());
				throw SystemException(message);
			}

			// Insert PAT/PMT every second second
			time_t now = time(NULL);
			if (now - last_insert_time > 2)
			{
				g_debug("Writing PAT/PMT header");

				for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
				{
					ChannelStream& channel_stream = **i;

					channel_stream.stream.build_pat(pat);
					channel_stream.stream.build_pmt(pmt);

					channel_stream.write(pat, TS_PACKET_SIZE);
					channel_stream.write(pmt, TS_PACKET_SIZE);
				}
				last_insert_time = now;
			}

			for (guint offset = 0; offset < (guint)bytes_read; offset += TS_PACKET_SIZE)
			{
				guint pid = ((buffer[offset+1] & 0x1f) << 8) + buffer[offset+2];

				for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
				{
					ChannelStream& channel_stream = **i;
					if (channel_stream.stream.contains_pid(pid))
					{
						channel_stream.write(buffer+offset, TS_PACKET_SIZE);
					}
				}
			}
		}
		catch(...)
		{
			// The show must go on!
		}
	}
		
	g_debug("FrontendThread loop exited (%s)", frontend.get_path().c_str());
}

void FrontendThread::setup_dvb(ChannelStream& channel_stream)
{
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();

	Buffer buffer;
	const Channel& channel = channel_stream.channel;
	
	channel_stream.clear_demuxers();
	if (channel.transponder != frontend.get_frontend_parameters())
	{
		stop_epg_thread();
		frontend.tune_to(channel.transponder);
	}
	start_epg_thread();
	
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

	g_debug("Finished setting up DVB (%s)", frontend.get_path().c_str());
}

void FrontendThread::start_epg_thread()
{
	if (!disable_epg_thread)
	{
		if (epg_thread == NULL)
		{
			epg_thread = new EpgThread(frontend);
			epg_thread->start();
			g_debug("EPG thread started");
		}
	}
}

void FrontendThread::stop_epg_thread()
{
	if (!disable_epg_thread)
	{
		if (epg_thread != NULL)
		{
			g_debug("Stopping EPG thread (%s)", frontend.get_path().c_str());
			delete epg_thread;
			epg_thread = NULL;
			g_debug("EPG thread stopped (%s)", frontend.get_path().c_str());
		}
	}
}

void FrontendThread::start_display(Channel& channel)
{
	g_debug("FrontendThread::start_display(%s)", channel.name.c_str());
	stop();
	
	g_debug("Creating new stream output");
	
	Glib::ustring fifo_path = Glib::build_filename(get_application().get_application_dir(), "me-tv.fifo");

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

	Glib::ustring description;

	EpgEvent epg_event;
	if (channel.epg_events.get_current(epg_event))
	{
		description = epg_event.get_title();
	}

	ChannelStream* channel_stream = new ChannelStream(CHANNEL_STREAM_TYPE_DISPLAY, channel, fifo_path, description);
	close(fd);
	setup_dvb(*channel_stream);
	streams.push_back(channel_stream);

	start();
}

void FrontendThread::stop_display()
{
	stop();

	ChannelStreamList::iterator iterator = streams.begin();

	while (iterator != streams.end())
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->type == CHANNEL_STREAM_TYPE_DISPLAY)
		{
			delete channel_stream;
			iterator = streams.erase(iterator);
			g_debug("Stopped display stream");
		}
		else
		{
			iterator++;
		}
	}

	start();
}

Glib::ustring make_recording_filename(Channel& channel, const Glib::ustring& description)
{
	Glib::ustring start_time = get_local_time_text("%c");
	Glib::ustring filename;
	Glib::ustring title = description;
	
	if (title.empty())
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2.mpeg",
			channel.name,
			start_time
		);
	}
	else
	{
		filename = Glib::ustring::compose
		(
			"%1 - %2 - %3.mpeg",
			title,
			channel.name,
			start_time
		);
	}

	// Clean filename
	Glib::ustring::size_type position = Glib::ustring::npos;
	while ((position = filename.find('/')) != Glib::ustring::npos)
	{
		filename.replace(position, 1, "_");
	}

	if (configuration_manager.get_boolean_value("remove_colon"))
	{
		while ((position = filename.find(':')) != Glib::ustring::npos )
		{
			filename.replace(position, 1, "_");
		}
	}

	Glib::ustring fixed_filename = Glib::filename_from_utf8(filename);
	
	return Glib::build_filename(
	    configuration_manager.get_string_value("recording_directory"),
	    fixed_filename);
}

bool is_recording_stream(ChannelStream* channel_stream)
{
	return channel_stream->type == CHANNEL_STREAM_TYPE_RECORDING || channel_stream->type == CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING;
}

void FrontendThread::start_recording(Channel& channel, const Glib::ustring& description, gboolean scheduled)
{
	stop();	

	ChannelStreamType requested_type = scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING :
		CHANNEL_STREAM_TYPE_RECORDING;
	ChannelStreamType current_type = CHANNEL_STREAM_TYPE_NONE;

	ChannelStreamList::iterator iterator = streams.begin();
	
	while (iterator != streams.end())
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			current_type = channel_stream->type;
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

			if (channel.transponder != frontend.get_frontend_parameters())
			{
				g_debug("Need to change transponders to record this channel");

				gboolean was_display = is_display();

				if (was_display)
				{
					g_debug("This frontend is being used for the display so we need to switch display channels also");
				}
				
				// Need to kill all current streams
				ChannelStreamList::iterator iterator = streams.begin();
				while (iterator != streams.end())
				{
					ChannelStream* channel_stream = *iterator;
					delete channel_stream;
					iterator = streams.erase(iterator);
				}

				if (was_display)
				{
					start_display(channel);
				}
			}

			ChannelStream* channel_stream = new ChannelStream(
				scheduled ? CHANNEL_STREAM_TYPE_SCHEDULED_RECORDING : CHANNEL_STREAM_TYPE_RECORDING,
				channel, make_recording_filename(channel, description), description);
			setup_dvb(*channel_stream);
			streams.push_back(channel_stream);
		}
	}
	
	g_debug("New recording channel created (%s)", frontend.get_path().c_str());

	start();
}

void FrontendThread::stop_recording(const Channel& channel)
{
	stop();

	ChannelStreamList::iterator iterator = streams.begin();

	while (iterator != streams.end())
	{
		ChannelStream* channel_stream = *iterator;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			delete channel_stream;
			iterator = streams.erase(iterator);
		}
		else
		{
			iterator++;
		}
	}

	start();
}

guint FrontendThread::get_last_epg_update_time()
{
	guint result = 0;

	if (epg_thread != NULL)
	{
		result = epg_thread->get_last_epg_update_time();
	}
	
	return result;
}

gboolean FrontendThread::is_recording()
{
	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		if (is_recording_stream(*i))
		{
			return true;
		}
	} 

	return false;
}

gboolean FrontendThread::is_display()
{
	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		if ((*i)->type == CHANNEL_STREAM_TYPE_DISPLAY)
		{
			return true;
		}
	} 

	return false;
}

gboolean FrontendThread::is_recording(const Channel& channel)
{
	for (ChannelStreamList::iterator i = streams.begin(); i != streams.end(); i++)
	{
		ChannelStream* channel_stream = *i;
		if (channel_stream->channel == channel && is_recording_stream(channel_stream))
		{
			return true;
		}
	}

	return false;
}
