/*
 * Copyright (C) 2009 Michael Lamothe
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
#include "stream_thread.h"
#include "application.h"
#include "device_manager.h"
#include "dvb_transponder.h"

#define TS_PACKET_SIZE 188

class Lock : public Glib::RecMutex::Lock
{
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(mutex) {}
	~Lock() {}
};

StreamThread::StreamThread(const Channel& active_channel) :
	Thread("Stream"),
	display_channel(active_channel),
	frontend(get_application().device_manager.get_frontend())
{
	g_debug("Creating StreamThread");
	g_static_rec_mutex_init(mutex.gobj());

	epg_thread = NULL;
	socket = NULL;
	broadcast_failure_message = true;

	Glib::ustring filename = Glib::ustring::compose("me-tv-%1.fifo", frontend.get_adapter().get_index());
	fifo_path = Glib::build_filename(get_application().get_application_dir(), filename);

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

	StreamOutput stream_output(active_channel, fifo_path);
	outputs.push_back(stream_output);

	close(fd);

	g_debug("StreamThread created");
}

StreamThread::~StreamThread()
{
	g_debug("Destroying StreamThread");
	stop_epg_thread();
	join(true);
	remove_all_demuxers();
	g_debug("StreamThread destroyed");
}

void StreamThread::start()
{
	g_debug("Starting stream thread");
	Thread::start();
}

const Glib::ustring& StreamThread::get_fifo_path() const
{
	return fifo_path;
}

void StreamThread::run()
{
	guchar buffer[TS_PACKET_SIZE * 10];
	guchar pat[TS_PACKET_SIZE];
	guchar pmt[TS_PACKET_SIZE];

	setup_dvb(display_channel, (*(outputs.begin())).stream);
	start_epg_thread();

	Glib::ustring input_path = frontend.get_adapter().get_dvr_path();
	Glib::RefPtr<Glib::IOChannel> input_channel = Glib::IOChannel::create_from_file(input_path, "r");
	input_channel->set_encoding("");
		
	guint last_insert_time = 0;
	gsize bytes_read;
	
	TRY
	while (!is_terminated())
	{
		// Insert PAT/PMT every second
		time_t now = time(NULL);
		if (now - last_insert_time > 2)
		{
			g_debug("Writing PAT/PMT header");
			
			for (std::list<StreamOutput>::iterator iterator = outputs.begin(); iterator != outputs.end(); iterator++)
			{
				StreamOutput& stream_output = *iterator;

				stream_output.stream.build_pat(pat);
				stream_output.stream.build_pmt(pmt);

				stream_output.write(pat, TS_PACKET_SIZE);
				stream_output.write(pmt, TS_PACKET_SIZE);
			}
			last_insert_time = now;
		}
				
		input_channel->read((gchar*)buffer, TS_PACKET_SIZE * 10, bytes_read);

		guint pid = buffer[2];

		for (std::list<StreamOutput>::iterator iterator = outputs.begin(); iterator != outputs.end(); iterator++)
		{
			StreamOutput& stream_output = *iterator;
			if (stream_output.stream.contains_pid(pid))
			{
				stream_output.write(buffer, bytes_read);
			}
		}

		if (socket != NULL)
		{
			if (gnet_udp_socket_send(socket, (const gchar*)buffer, bytes_read, inet_address) != 0)
			{
				if (broadcast_failure_message)
				{
					g_message("Failed to send to UDP socket");
				}
				broadcast_failure_message = false;
			}
		}
	}
	THREAD_CATCH
		
	g_debug("StreamThread loop exited");
	
	Lock lock(mutex, "StreamThread::run() - exit");

	g_debug("About to close input channel ...");
	input_channel->close(true);
	input_channel.reset();
	g_debug("Input channel reset");
	
	for (std::list<StreamOutput>::iterator iterator = outputs.begin(); iterator != outputs.end(); iterator++)
	{
		(*iterator).output_channel.reset();
	}
	
	g_debug("Stream Output channels reset");
}

void StreamThread::remove_all_demuxers()
{
	Lock lock(mutex, "StreamThread::remove_all_demuxers()");
	g_debug("Removing demuxers");
	while (demuxers.size() > 0)
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer& StreamThread::add_pes_demuxer(const Glib::ustring& demux_path,
	guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
{	
	Lock lock(mutex, "StreamThread::add_pes_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug("Setting %s PID filter to %d (0x%X)", type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer& StreamThread::add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id)
{	
	Lock lock(mutex, "StreamThread::add_section_demuxer()");
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void StreamThread::setup_dvb(const Channel& channel, Mpeg::Stream& stream)
{
	Lock lock(mutex, "StreamThread::setup_dvb()");
	
	stop_epg_thread();
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	
	remove_all_demuxers();
	
	frontend.tune_to(channel.transponder);

	Buffer buffer;	
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	demuxer_pat.read_section(buffer);
	demuxer_pat.stop();

	stream.set_program_map_pid(buffer, channel.service_id);
	
	Dvb::Demuxer demuxer_pmt(demux_path);
	demuxer_pmt.set_filter(stream.get_program_map_pid(), PMT_ID);
	demuxer_pmt.read_section(buffer);
	demuxer_pmt.stop();

	stream.parse_pms(buffer);
			
	gsize video_streams_size = stream.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		add_pes_demuxer(demux_path, stream.video_streams[i].pid, DMX_PES_OTHER, "video");
	}

	gsize audio_streams_size = stream.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		add_pes_demuxer(demux_path, stream.audio_streams[i].pid, DMX_PES_OTHER,
			stream.audio_streams[i].is_ac3 ? "AC3" : "audio");
	}
				
	gsize subtitle_streams_size = stream.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		add_pes_demuxer(demux_path, stream.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
	}

	gsize teletext_streams_size = stream.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		add_pes_demuxer(demux_path, stream.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
	}

	g_debug("Finished setting up DVB");
}

void StreamThread::start_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, "StreamThread::start_epg_thread()");

		stop_epg_thread();
		epg_thread = new EpgThread();
		epg_thread->start();
		g_debug("EPG thread started");
	}
}

void StreamThread::stop_epg_thread()
{
	if (!disable_epg_thread)
	{
		Lock lock(mutex, "StreamThread::stop_epg_thread()");

		if (epg_thread != NULL)
		{
			g_debug("Stopping EPG thread");
			delete epg_thread;
			epg_thread = NULL;
			g_debug("EPG thread stopped");
		}
	}
}

const Mpeg::Stream& StreamThread::get_stream() const
{
	return (*(outputs.begin())).stream;
}

void StreamThread::start_recording(const Channel& channel, const Glib::ustring& filename)
{
	Lock lock(mutex, "StreamThread::start_recording()");
	
	StreamOutput stream_output(channel, filename);
	outputs.push_back(stream_output);
	
	g_debug("New recording channel created");
}

void StreamThread::stop_recording(const Channel& channel)
{
	Lock lock(mutex, "StreamThread::stop_recording()");

	std::list<StreamOutput>::iterator iterator = outputs.begin();

	// Skip the first output because it's the display one
	if (iterator != outputs.end())
	{
		iterator++;
	}
	
	while (iterator != outputs.end())
	{
		StreamOutput& stream_output = *iterator;
		if (stream_output.channel == channel)
		{
			stream_output.output_channel.reset();
		}
		iterator++;
	}
}

void StreamThread::start_broadcasting()
{
	Lock lock(mutex, "StreamThread::start_broadcasting()");
	if (socket == NULL)
	{
		Application& application = get_application();
		Glib::ustring address = application.get_string_configuration_value("broadcast_address");
		guint port = application.get_int_configuration_value("broadcast_port");
		
		g_debug("Creating internet address for '%s:%d'", address.c_str(), port);
		
		inet_address = gnet_inetaddr_new(address.c_str(), port);
		if (inet_address == NULL)
		{
			throw Exception(_("Failed to create internet address"));
		}

		socket = gnet_udp_socket_new_full(inet_address, port);
		if (socket == NULL)
		{
			throw Exception(_("Failed to create socket"));
		}
		g_debug("Broadcasting started");
	}
}

void StreamThread::stop_broadcasting()
{
	Lock lock(mutex, "StreamThread::stop_broadcasting()");
	if (socket != NULL)
	{
		gnet_udp_socket_delete(socket);
		socket = NULL;
		gnet_inetaddr_delete(inet_address);
		inet_address = NULL;
		g_debug("Broadcasting stopped");
	}
}

guint StreamThread::get_last_epg_update_time()
{
	guint result = 0;

	Lock lock(mutex, "StreamThread::get_epg_last_update_time()");
	if (epg_thread != NULL)
	{
		result = epg_thread->get_last_epg_update_time();
	}
	
	return result;
}

StreamThread::StreamOutput::StreamOutput(const Channel& c, const Glib::ustring& filename)
{
	channel = c;
	output_channel = Glib::IOChannel::create_from_file(filename, "w");
	output_channel->set_encoding("");
	output_channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
	output_channel->set_buffer_size(TS_PACKET_SIZE * 100);

	g_debug("Creating new recording output '%s' -> '%s'", channel.name.c_str(), filename.c_str());
}

void StreamThread::StreamOutput::write(guchar* buffer, gsize length)
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
