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

#include "stream_thread.h"
#include "application.h"
#include "device_manager.h"
#include "xine_engine.h"
#include "mplayer_engine.h"
#include <glibmm.h>
#include <gdk/gdkx.h>

#define TS_PACKET_SIZE 188

class Lock : public Glib::RecMutex::Lock
{
private:
	Glib::ustring name;
		
	Glib::StaticRecMutex& log(Glib::StaticRecMutex& mutex, const Glib::ustring& name)
	{
//		g_debug("'%s' trying to lock", name.c_str());
		return mutex;
	}
public:
	Lock(Glib::StaticRecMutex& mutex, const Glib::ustring& name) :
		Glib::RecMutex::Lock(log(mutex, name)), name(name)
	{
//		g_debug("'%s' acquired lock", name.c_str());
	}
	
	~Lock()
	{
//		g_debug("'%s' released lock", name.c_str());
	}
};

class EngineStartThread : public Thread
{
private:
	const Glib::ustring&	fifo_path;
	Engine*					engine;
		
	void run()
	{
		g_debug("Telling engine to start playing");
		engine->play(fifo_path);
		g_debug("Engine playing");
	}

public:
	EngineStartThread(Engine* engine, const Glib::ustring& fifo_path)
		: Thread("Engine start thread"), engine(engine), fifo_path(fifo_path) {}
};

class StopEngineThread : public Thread
{
private:
	Engine*	engine;
	
	void run()
	{
		g_debug("Stopping engine in thread");
		engine->stop();
		g_debug("Engine stopped in thread");
	}
public:
	StopEngineThread(Engine* engine) : Thread("Stop engine"), engine(engine) {}
};

StreamThread::StreamThread(const Channel& channel) :
	Thread("Stream"),
	frontend(get_application().get_device_manager().get_frontend()),
	channel(channel)
{
	g_debug("Creating StreamThread");
	g_static_rec_mutex_init(mutex.gobj());

	engine = NULL;
	epg_thread = NULL;
	socket = NULL;
	manual_recording = false;
	broadcast_failure_message = true;
	output_fd = -1;
	recording_fd = -1;
	timeout_source = -1;
	
	for(gint i = 0 ; i < 256 ; i++ )
	{
		gint k = 0;
		for (gint j = (i << 24) | 0x800000 ; j != 0x80000000 ; j <<= 1)
		{
			k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
		}
		CRC32[i] = k;
	}
	
	Application& application = get_application();
	application.signal_record_state_changed.connect(sigc::mem_fun(*this, &StreamThread::on_record_state_changed));
	application.signal_mute_state_changed.connect(sigc::mem_fun(*this, &StreamThread::on_mute_state_changed));
	application.signal_broadcast_state_changed.connect(sigc::mem_fun(*this, &StreamThread::on_broadcast_state_changed));
	
	MainWindow& main_window = application.get_main_window();
	show_connection = main_window.signal_show().connect(sigc::mem_fun(*this, &StreamThread::on_main_window_show));
	hide_connection = main_window.signal_hide().connect(sigc::mem_fun(*this, &StreamThread::on_main_window_hide));

	g_debug("StreamThread created");
}

StreamThread::~StreamThread()
{
	g_debug("Destroying StreamThread");
	show_connection.disconnect();
	hide_connection.disconnect();
	stop_epg_thread();
	join(true);
	stop_engine();
	remove_all_demuxers();
	g_debug("StreamThread destroyed");
}

gboolean StreamThread::on_timeout(gpointer data)
{
	StreamThread* stream_thread = (StreamThread*)data;
	stream_thread->on_timeout();
	return TRUE;
}

void StreamThread::on_timeout()
{
	Lock lock(mutex, "Engine expose");
	if (engine != NULL)
	{
		engine->expose();
	}
}

void StreamThread::start()
{
	setup_dvb(frontend, channel);

	g_debug("Starting stream thread");
	Thread::start();
	
	Lock lock(mutex, "StreamThread::start()");
	Application& application = get_application();
	MainWindow& main_window = application.get_main_window();
	if (main_window.is_muted())
	{
		on_mute_state_changed(true);
	}
	
	if (application.get_main_window().is_broadcasting())
	{
		on_broadcast_state_changed(true);
	}
	
	if (main_window.property_visible())
	{
		g_debug("Main window visible, starting engine");
		start_engine();
	}
	else
	{
		g_debug("Main window not visible, not starting engine");
	}

	start_epg_thread();
}

void StreamThread::start_engine()
{
	Application& application = get_application();
	Gtk::DrawingArea& drawing_area_video = application.get_main_window().get_drawing_area();
	{
		Lock lock(mutex, __PRETTY_FUNCTION__);
		if (engine != NULL)
		{
			throw Exception("Failed to start engine: Engine has already been started");
		}
			
		int window_id = GDK_WINDOW_XID(drawing_area_video.get_window()->gobj());

		g_debug("Creating engine");
		Glib::ustring engine_type = application.get_string_configuration_value("engine_type");
		if (engine_type == "xine")
		{
			engine = new XineEngine(window_id);
		}
		else if (engine_type == "mplayer")
		{
			engine = new MplayerEngine(window_id);
		}
		else
		{
			throw Exception(_("Unknown engine type"));
		}
		g_debug("%s engine created", engine_type.c_str());
	}
	
	Glib::ustring filename = Glib::ustring::compose("me-tv-%1.fifo", frontend.get_adapter().get_index());
	fifo_path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	fifo_path = Glib::build_filename(fifo_path, filename);
	
	if (!Glib::file_test(fifo_path, Glib::FILE_TEST_EXISTS))
	{
		if (mkfifo(fifo_path.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)
		{
			throw Exception(Glib::ustring::compose(_("Failed to create FIFO '%1'"), fifo_path));
		}
	}

	if (engine != NULL)
	{
		gint width, height;
		drawing_area_video.get_window()->get_size(width, height);
		engine->set_size(width, height);

		connection_configure = drawing_area_video.signal_configure_event().connect(
			sigc::mem_fun(*this, &StreamThread::on_drawing_area_configure_event));

		EngineStartThread engine_start_thread(engine, fifo_path);
		engine_start_thread.start();

		Lock lock(mutex, "Output FD open");
		g_debug("Opening '%s'", fifo_path.c_str());
		output_fd = open(fifo_path.c_str(), O_WRONLY, 0);
		
		if (output_fd == -1)
		{
			throw SystemException("Failed to create FIFO output");
		}
		g_debug("Output FD created");
	}

	timeout_source = gdk_threads_add_timeout(1000, &StreamThread::on_timeout, this);
}

void StreamThread::stop_engine()
{
	Lock lock(mutex, "Output FD close");
	if (engine != NULL)
	{
		connection_configure.disconnect();
		
		StopEngineThread stop_engine_thread(engine);
		stop_engine_thread.start();

		if (output_fd != -1)
		{
			close(output_fd);
			output_fd = -1;
		}

		stop_engine_thread.join(true);
		
		delete engine;
		engine = NULL;
	}

	if (timeout_source != -1)
	{
		g_source_remove(timeout_source);
	}
}

void StreamThread::on_main_window_show()
{
	TRY
	g_debug("Starting on_main_window_show()");
	start_engine();
	g_debug("Exiting on_main_window_show()");
	CATCH
}

void StreamThread::on_main_window_hide()
{
	TRY
	g_debug("Starting on_main_window_hide()");
	stop_engine();
	g_debug("Exiting on_main_window_hide()");
	CATCH
}

void StreamThread::on_mute_state_changed(gboolean mute_state)
{
	if (engine != NULL)
	{
		engine->mute(mute_state);
	}
}

Engine& StreamThread::get_engine()
{
	if (engine == NULL)
	{
		throw Exception(_("Engine has not been created"));
	}
	return *engine;
}

void StreamThread::on_record_state_changed(gboolean record_state, const Glib::ustring& filename, gboolean manual)
{
	Lock lock(mutex, "StreamThread::on_record_state_changed()");
	if (record_state && recording_fd == -1)
	{
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		recording_fd = open(filename.c_str(), O_CREAT | O_WRONLY, mode);
		if (recording_fd == -1)
		{
			throw SystemException("Failed to open recording file");
		}
		g_debug("Recording file opened");

		manual_recording = manual;
	}
	else if (!record_state && recording_fd != -1)
	{
		manual_recording = false;
		
		close(recording_fd);
		recording_fd = -1;
		g_debug("Recording file cleared");
	}
}

void StreamThread::on_broadcast_state_changed(gboolean broadcast_state)
{
	Lock lock(mutex, "StreamThread::on_broadcast_state_changed()");
	if (broadcast_state && socket == NULL)
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
	else if (!broadcast_state && socket != NULL)
	{
		gnet_udp_socket_delete(socket);
		socket = NULL;
		gnet_inetaddr_delete(inet_address);
		inet_address = NULL;
		g_debug("Broadcasting stopped");
	}
}

void StreamThread::write(gchar* buffer, gsize length)
{
	if (output_fd != -1)
	{
		::write(output_fd, buffer, length);
	}
	
	if (recording_fd != -1)
	{
		::write(recording_fd, buffer, length);
	}
	
	if (socket != NULL)
	{
		if (gnet_udp_socket_send(socket, buffer, length, inet_address) != 0)
		{
			if (broadcast_failure_message)
			{
				g_message(_("Failed to send to UDP socket"));
			}
			broadcast_failure_message = false;
		}
	}
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
	gsize bytes_written;
	
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
	g_debug("Input channel closed");
}

gboolean StreamThread::is_recording()
{
	Lock lock(mutex, "StreamThread::is_recording()");
	return recording_fd != -1;
}

gboolean StreamThread::is_manual_recording()
{
	Lock lock(mutex, "StreamThread::is_manual_recording()");
	return recording_fd != -1 && manual_recording;
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
	for (gint index = 0; index < stream.audio_streams.size(); index++)
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
	for (gint index = 0; index < stream.subtitle_streams.size(); index++)
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
	for (gint index = 0; index < stream.teletext_streams.size(); index++)
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

void StreamThread::setup_dvb(Dvb::Frontend& frontend, const Channel& channel)
{
	Lock lock(mutex, "StreamThread::setup_dvb()");
	
	stop_epg_thread();
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
	pmt_pid = 0;
	
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

gboolean StreamThread::is_broadcasting()
{
	Lock lock(mutex, "StreamThread::is_broadcasting()");
	return socket != NULL;
}

bool StreamThread::on_drawing_area_configure_event(GdkEventConfigure* event)
{
	Lock lock(mutex, "StreamThread::on_drawing_area_configure_event()");
	if (engine != NULL)
	{
		engine->set_size(event->width, event->height);
	}
}

gboolean StreamThread::is_engine_running()
{
	Lock lock(mutex, "StreamThread::is_engine_running()");
	return (engine != NULL);
}

const Stream& StreamThread::get_stream() const
{
	return stream;
}
