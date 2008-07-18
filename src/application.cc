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

#include "application.h"
#include "main_window.h"
#include "config.h"
#include "device_manager.h"
#include "dvb_si.h"

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
}

void set_default(Glib::RefPtr<Gnome::Conf::Client> client, const Glib::ustring& path, const Glib::ustring& value)
{
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%s'", path.c_str(), value.c_str());
		client->set(path, value);
	}
}

void set_default(Glib::RefPtr<Gnome::Conf::Client> client, const Glib::ustring& path, gint value)
{
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%d'", path.c_str(), value);
		client->set(path, value);
	}
}

Application::Application(int argc, char *argv[]) :
	Gnome::Main("Me TV", VERSION, Gnome::UI::module_info_get(), argc, argv)
{
	if (current != NULL)
	{
		throw Exception("Application has already been initialised");
	}
	
	current = this;
	main_window = NULL;
		
	Glib::RefPtr<Gnome::Conf::Client> client = Gnome::Conf::Client::get_default_client();
	set_default(client, GCONF_PATH"/video_output", "Xv");
	set_default(client, GCONF_PATH"/epg_span_hours", 3);
	
	Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
	Glib::ustring glade_path = current_directory + "/me-tv.glade";

	if (!Gio::File::create_for_path(glade_path)->query_exists())
	{
		glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
	}
	
	g_debug("Using glade file '%s'", glade_path.c_str());
	
	glade = Gnome::Glade::Xml::create(glade_path);
	
	channel_manager.signal_display_channel_changed.connect(
		sigc::mem_fun(*this, &Application::on_display_channel_changed));

	channel_manager.add_channels(profile_manager.get_current_profile().channels);

	engine = new GStreamerEngine(argc, argv);
}

void Application::run()
{
	glade->get_widget_derived("window_main", main_window);
	Gnome::Main::run(*main_window);
}

Application& Application::get_current()
{
	if (current == NULL)
	{
		throw Exception("Application has not been initialised");
	}
	
	return *current;
}

void Application::on_display_channel_changed(Channel& channel)
{
	TRY
	Dvb::Frontend& frontend = device_manager.get_frontend();
	setup_dvb(frontend, channel);
	engine->play(main_window->get_drawing_area().get_window(), "file://" + frontend.get_adapter().get_dvr_path());
	CATCH
}

void Application::remove_all_demuxers()
{
	while (demuxers.size() > 0)
	{
		Dvb::Demuxer* demuxer = demuxers.front();
		demuxers.pop_front();
		delete demuxer;
		g_debug("Demuxer removed");
	}
}

Dvb::Demuxer& Application::add_pes_demuxer(const Glib::ustring& demux_path,
	guint pid, dmx_pes_type_t pid_type, const gchar* type_text)
{	
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	g_debug(_("Setting %s PID filter to %d (0x%X)"), type_text, pid, pid);
	demuxer->set_pes_filter(pid, pid_type);
	return *demuxer;
}

Dvb::Demuxer& Application::add_section_demuxer(const Glib::ustring& demux_path, guint pid, guint id)
{	
	Dvb::Demuxer* demuxer = new Dvb::Demuxer(demux_path);
	demuxers.push_back(demuxer);
	demuxer->set_filter(pid, id);
	return *demuxer;
}

void Application::setup_dvb(Dvb::Frontend& frontend, const Channel& channel)
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

Engine& Application::get_engine()
{
	if (engine == NULL)
	{
		throw Exception("Engine has not been initialised");
	}
	
	return *engine;
}
