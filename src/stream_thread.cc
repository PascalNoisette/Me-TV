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

Stream::~Stream()
{
}

void Stream::clear()
{
	video_streams.clear();
	audio_streams.clear();
	subtitle_streams.clear();
	teletext_streams.clear();
}

class Lock : public Glib::RecMutex::Lock
{
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(mutex) {}
	~Lock() {}
};

StreamThread::StreamThread(const Channel& active_channel) :
	Thread("Stream"),
	channel(active_channel),
	frontend(get_application().get_device_manager().get_frontend())
{
	g_debug("Creating StreamThread");
	g_static_rec_mutex_init(mutex.gobj());

	epg_thread = NULL;
	socket = NULL;
	broadcast_failure_message = true;
	
	for (gint i = 0 ; i < 256 ; i++ )
	{
		guint k = 0;
		for (guint j = (i << 24) | 0x800000 ; j != 0x80000000 ; j <<= 1)
		{
			k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
		}
		CRC32[i] = k;
	}

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

	// Fudge the channel open
	int fd = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd == -1)
	{
		throw SystemException(Glib::ustring::compose(_("Failed to open FIFO for reading '%1'"), fifo_path));
	}
	
	output_channel = Glib::IOChannel::create_from_file(fifo_path, "w");
	output_channel->set_encoding("");
	output_channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
	output_channel->set_buffer_size(TS_PACKET_SIZE * 100);

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
	setup_dvb();
	g_debug("Starting stream thread");
	Thread::start();	
	start_epg_thread();
}

void StreamThread::write(gchar* buffer, gsize length)
{
	if (output_channel)
	{
		try
		{
			gsize bytes_written = 0;
			output_channel->write(buffer, length, bytes_written);
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
		
	if (recording_channel)
	{
		gsize bytes_written = 0;
		recording_channel->write(buffer, length, bytes_written);
	}
	
	if (socket != NULL)
	{
		if (gnet_udp_socket_send(socket, buffer, length, inet_address) != 0)
		{
			if (broadcast_failure_message)
			{
				g_message("Failed to send to UDP socket");
			}
			broadcast_failure_message = false;
		}
	}
}

const Glib::ustring& StreamThread::get_fifo_path() const
{
	return fifo_path;
}

void StreamThread::run()
{
	gchar buffer[TS_PACKET_SIZE * 10];
	gchar pat[TS_PACKET_SIZE];
	gchar pmt[TS_PACKET_SIZE];

	Glib::ustring input_path = frontend.get_adapter().get_dvr_path();
	Glib::RefPtr<Glib::IOChannel> input_channel = Glib::IOChannel::create_from_file(input_path, "r");
	input_channel->set_encoding("");

	build_pat(pat);
	build_pmt(pmt);
		
	guint last_insert_time = 0;
	gsize bytes_read;
	
	TRY
	while (!is_terminated())
	{
		// Insert PAT/PMT every second
		time_t now = time(NULL);
		if (now - last_insert_time > 1)
		{
			g_debug("Writing PAT/PMT header");
			
			write(pat, TS_PACKET_SIZE);
			write(pmt, TS_PACKET_SIZE);
			last_insert_time = now;
		}
				
		input_channel->read(buffer, TS_PACKET_SIZE * 10, bytes_read);
		write(buffer, bytes_read);
	}
	THREAD_CATCH
	g_debug("StreamThread loop exited");
	
	Lock lock(mutex, "StreamThread::run() - exit");

	g_debug("About to close input channel ...");
	input_channel->close(true);
	input_channel.reset();
	g_debug("Input channel reset");
	
	output_channel.reset();
	g_debug("Output channel reset");
}

void StreamThread::calculate_crc(guchar *p_begin, guchar *p_end)
{
	unsigned int i_crc = 0xffffffff;

	// Calculate the CRC
	while( p_begin < p_end )
	{
		i_crc = (i_crc<<8) ^ CRC32[ (i_crc>>24) ^ ((unsigned int)*p_begin) ];
		p_begin++;
	}

	// Store it after the data
	p_end[0] = (i_crc >> 24) & 0xff;
	p_end[1] = (i_crc >> 16) & 0xff;
	p_end[2] = (i_crc >>  8) & 0xff;
	p_end[3] = (i_crc >>  0) & 0xff;
}

gboolean StreamThread::is_pid_used(guint pid)
{
	gboolean used = false;
	guint index = 0;
	
	if (stream.video_streams.size() > 0)
	{
		used = (pid==stream.video_streams[0].pid);
	}
	
	while (index < stream.audio_streams.size() && !used)
	{
		if (pid==stream.audio_streams[index].pid)
		{
			used = true;
		}
		else
		{
			index++;
		}
	}

	index = 0;
	while (index < stream.subtitle_streams.size() && !used)
	{
		if (pid==stream.subtitle_streams[index].pid)
		{
			used = true;
		}
		else
		{
			index++;
		}
	}
	
	return used;
}

void StreamThread::build_pat(gchar* buffer)
{
	buffer[0x00] = 0x47; // sync_byte
	buffer[0x01] = 0x40;
	buffer[0x02] = 0x00; // PID = 0x0000
	buffer[0x03] = 0x10; // | (ps->pat_counter & 0x0f);
	buffer[0x04] = 0x00; // CRC calculation begins here
	buffer[0x05] = 0x00; // 0x00: Program association section
	buffer[0x06] = 0xb0;
	buffer[0x07] = 0x11; // section_length = 0x011
	buffer[0x08] = 0x00;
	buffer[0x09] = 0xbb; // TS id = 0x00b0 (what the vlc calls "Stream ID")
	buffer[0x0a] = 0xc1;
	// section # and last section #
	buffer[0x0b] = buffer[0x0c] = 0x00;
	// Network PID (useless)
	buffer[0x0d] = buffer[0x0e] = 0x00;
	buffer[0x0f] = 0xe0;
	buffer[0x10] = 0x10;
	
	// Program Map PID
	pmt_pid = 0x64;
	while (is_pid_used(pmt_pid))
	{
		pmt_pid--;
	}
	
	buffer[0x11] = 0x03;
	buffer[0x12] = 0xe8;
	buffer[0x13] = 0xe0;
	buffer[0x14] = pmt_pid;
	
	// Put CRC in buffer[0x15...0x18]
	calculate_crc( (guchar*)buffer + 0x05, (guchar*)buffer + 0x15 );
	
	// needed stuffing bytes
	for (gint i=0x19; i < 188; i++)
	{
		buffer[i]=0xff;
	}
}

void StreamThread::build_pmt(gchar* buffer)
{
	int i, off=0;
	
	Dvb::SI::VideoStream video_stream;
	
	if (stream.video_streams.size() > 0)
	{
		video_stream = stream.video_streams[0];
	}

	buffer[0x00] = 0x47; //sync_byte
	buffer[0x01] = 0x40;
	buffer[0x02] = pmt_pid;
	buffer[0x03] = 0x10;
	buffer[0x04] = 0x00; // CRC calculation begins here
	buffer[0x05] = 0x02; // 0x02: Program map section
	buffer[0x06] = 0xb0;
	buffer[0x07] = 0x20; // section_length
	buffer[0x08] = 0x03;
	buffer[0x09] = 0xe8; // prog number
	buffer[0x0a] = 0xc1;
	// section # and last section #
	buffer[0x0b] = buffer[0x0c] = 0x00;
	// PCR PID
	buffer[0x0d] = video_stream.pid>>8;
	buffer[0x0e] = video_stream.pid&0xff;
	// program_info_length == 0
	buffer[0x0f] = 0xf0;
	buffer[0x10] = 0x00;
	// Program Map / Video PID
	buffer[0x11] = video_stream.type; // video stream type
	buffer[0x12] = video_stream.pid>>8;
	buffer[0x13] = video_stream.pid&0xff;
	buffer[0x14] = 0xf0;
	buffer[0x15] = 0x09; // es info length
	// useless info
	buffer[0x16] = 0x07;
	buffer[0x17] = 0x04;
	buffer[0x18] = 0x08;
	buffer[0x19] = 0x80;
	buffer[0x1a] = 0x24;
	buffer[0x1b] = 0x02;
	buffer[0x1c] = 0x11;
	buffer[0x1d] = 0x01;
	buffer[0x1e] = 0xfe;
	off = 0x1e;
	
	// Audio streams
	for (guint index = 0; index < stream.audio_streams.size(); index++)
	{
		Dvb::SI::AudioStream audio_stream = stream.audio_streams[index];
		
		gchar language_code[4] = { 0 };
		
		strncpy(language_code, audio_stream.language.c_str(), 3);

		if ( audio_stream.is_ac3 )
		{
			buffer[++off] = 0x81; // stream type = xine see this as ac3
			buffer[++off] = audio_stream.pid>>8;
			buffer[++off] = audio_stream.pid&0xff;
			buffer[++off] = 0xf0;
			buffer[++off] = 0x0c; // es info length
			buffer[++off] = 0x05;
			buffer[++off] = 0x04;
			buffer[++off] = 0x41;
			buffer[++off] = 0x43;
			buffer[++off] = 0x2d;
			buffer[++off] = 0x33;
		}
		else
		{
			buffer[++off] = 0x04; // stream type = audio
			buffer[++off] = audio_stream.pid>>8;
			buffer[++off] = audio_stream.pid&0xff;
			buffer[++off] = 0xf0;
			buffer[++off] = 0x06; // es info length
		}
		buffer[++off] = 0x0a; // iso639 descriptor tag
		buffer[++off] = 0x04; // descriptor length
		buffer[++off] = language_code[0];
		buffer[++off] = language_code[1];
		buffer[++off] = language_code[2];
		buffer[++off] = 0x00; // audio type
	}
	
	// Subtitle streams
	for (guint index = 0; index < stream.subtitle_streams.size(); index++)
	{
		Dvb::SI::SubtitleStream subtitle_stream = stream.subtitle_streams[index];
		
		gchar language_code[4] = { 0 };
		
		strncpy(language_code, subtitle_stream.language.c_str(), 3);
		
		buffer[++off] = 0x06; // stream type = ISO_13818_PES_PRIVATE
		buffer[++off] = subtitle_stream.pid>>8;
		buffer[++off] = subtitle_stream.pid&0xff;
		buffer[++off] = 0xf0;
		buffer[++off] = 0x0a; // es info length
		buffer[++off] = 0x59; // DVB sub tag
		buffer[++off] = 0x08; // descriptor length
		buffer[++off] = language_code[0];
		buffer[++off] = language_code[1];
		buffer[++off] = language_code[2];
		buffer[++off] = subtitle_stream.subtitling_type;
		buffer[++off] = subtitle_stream.composition_page_id>>8;
		buffer[++off] = subtitle_stream.composition_page_id&0xff;
		buffer[++off] = subtitle_stream.ancillary_page_id>>8;
		buffer[++off] = subtitle_stream.ancillary_page_id&0xff;
	}
	
	// TeleText streams
	for (guint index = 0; index < stream.teletext_streams.size(); index++)
	{
		Dvb::SI::TeletextStream teletext_stream = stream.teletext_streams[index];
						
		gint language_count = teletext_stream.languages.size();

		buffer[++off] = 0x06; // stream type = ISO_13818_PES_PRIVATE
		buffer[++off] = teletext_stream.pid>>8;
		buffer[++off] = teletext_stream.pid&0xff;
		buffer[++off] = 0xf0;
		buffer[++off] = (language_count * 5) + 4; // es info length
		buffer[++off] = 0x56; // DVB teletext tag
		buffer[++off] = language_count * 5; // descriptor length

		for (gint language_index = 0; language_index < language_count; language_index++)
		{
			Dvb::SI::TeletextLanguageDescriptor descriptor = teletext_stream.languages[language_index];
			gchar language_code[4] = { 0 };
			strncpy(language_code, descriptor.language.c_str(), 3);
			
			buffer[++off] = language_code[0];
			buffer[++off] = language_code[1];
			buffer[++off] = language_code[2];
			//buffer[++off] = (descriptor.type & 0x1F) | ((descriptor.magazine_number << 5) & 0xE0);
			buffer[++off] = (descriptor.magazine_number & 0x06) | ((descriptor.type << 3) & 0xF8);
			buffer[++off] = descriptor.page_number;
		}
	}

	buffer[0x07] = off-3; // update section_length

	// Put CRC in ts[0x29...0x2c]
	calculate_crc( (guchar*)buffer+0x05, (guchar*)buffer+off+1 );
	
	// needed stuffing bytes
	for (i=off+5 ; i < 188 ; i++)
	{
		buffer[i]=0xff;
	}
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

void StreamThread::setup_dvb()
{
	Lock lock(mutex, "StreamThread::setup_dvb()");
	
	stop_epg_thread();
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	
	remove_all_demuxers();
	
	frontend.tune_to(channel.transponder);
	
	Dvb::Demuxer demuxer_pat(demux_path);
	demuxer_pat.set_filter(PAT_PID, PAT_ID);
	Dvb::SI::SectionParser parser;
	
	Dvb::SI::ProgramAssociationSection pas;
	parser.parse_pas(demuxer_pat, pas);
	demuxer_pat.stop();

	guint length = pas.program_associations.size();
	pmt_pid = 0;
	
	g_debug("Found %d associations", length);
	g_debug("Searching for service ID %d", channel.service_id);
	for (guint i = 0; i < length; i++)
	{
		Dvb::SI::ProgramAssociation program_association = pas.program_associations[i];
		
		if (program_association.program_number == channel.service_id)
		{
			pmt_pid = program_association.program_map_pid;
			g_debug("%d: PMT ID: %d", i, pmt_pid);
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
			
	gsize video_streams_size = pms.video_streams.size();
	for (guint i = 0; i < video_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.video_streams[i].pid, DMX_PES_OTHER, "video");
		stream.video_streams.push_back(pms.video_streams[i]);
	}

	gsize audio_streams_size = pms.audio_streams.size();
	for (guint i = 0; i < audio_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.audio_streams[i].pid, DMX_PES_OTHER,
			pms.audio_streams[i].is_ac3 ? "AC3" : "audio");
		stream.audio_streams.push_back(pms.audio_streams[i]);
	}
				
	gsize subtitle_streams_size = pms.subtitle_streams.size();
	for (guint i = 0; i < subtitle_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.subtitle_streams[i].pid, DMX_PES_OTHER, "subtitle");
		stream.subtitle_streams.push_back(pms.subtitle_streams[i]);
	}

	gsize teletext_streams_size = pms.teletext_streams.size();
	for (guint i = 0; i < teletext_streams_size; i++)
	{
		add_pes_demuxer(demux_path, pms.teletext_streams[i].pid, DMX_PES_OTHER, "teletext");
		stream.teletext_streams.push_back(pms.teletext_streams[i]);
	}

	g_debug("Finished setting up DVB");
}

void StreamThread::start_epg_thread()
{
	Lock lock(mutex, "StreamThread::start_epg_thread()");

	stop_epg_thread();
	epg_thread = new EpgThread();
	epg_thread->start();
	g_debug("EPG thread started");
}

void StreamThread::stop_epg_thread()
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

const Stream& StreamThread::get_stream() const
{
	return stream;
}

void StreamThread::start_recording(const Glib::ustring& filename)
{
	Lock lock(mutex, "StreamThread::start_recording()");
	if (recording_channel)
	{
		g_debug("Already recording!");
	}
	else
	{
		g_debug("Creating new recording channel (%s)", filename.c_str());
		Glib::RefPtr<Glib::IOChannel> channel = Glib::IOChannel::create_from_file(filename, "w");
		channel->set_encoding("");
		channel->set_flags(output_channel->get_flags() & Glib::IO_FLAG_NONBLOCK);
		channel->set_buffer_size(TS_PACKET_SIZE * 100);
		recording_channel = channel;
		g_debug("New recording channel created");
	}
}

void StreamThread::stop_recording()
{
	Lock lock(mutex, "StreamThread::stop_recording()");
	if (recording_channel)
	{
		recording_channel.reset();
		g_debug("Recording channel cleared");
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
