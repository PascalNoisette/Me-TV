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
#include "config.h"
#include "device_manager.h"
#include "dvb_si.h"
#include "data.h"
#include "gstreamer_engine.h"
#include "mplayer_engine.h"

Application* Application::current = NULL;

Application& get_application()
{
	return Application::get_current();
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
	engine = NULL;

	client = Gnome::Conf::Client::get_default_client();
	
	set_int_configuration_default("epg_span_hours", 3);
	set_int_configuration_default("last_channel", -1);
	set_string_configuration_default("recording_directory", Glib::get_home_dir());
	set_string_configuration_default("engine_type", "gstreamer");
	set_boolean_configuration_default("keep_above", true);
	set_int_configuration_default("record_extra_before", 5);
	set_int_configuration_default("record_extra_after", 10);
	
	Glib::ustring path = Glib::build_filename(Glib::get_home_dir(), ".me-tv");
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
	if (!file->query_exists())
	{
		file->make_directory();
	}
	
	// Initialise database
	Data data(true);
	
	Glib::ustring current_directory = Glib::path_get_dirname(argv[0]);
	Glib::ustring glade_path = current_directory + "/me-tv.glade";

	if (!Gio::File::create_for_path(glade_path)->query_exists())
	{
		glade_path = PACKAGE_DATA_DIR"/me-tv/glade/me-tv.glade";
	}
	
	g_debug("Using glade file '%s'", glade_path.c_str());
	
	glade = Gnome::Glade::Xml::create(glade_path);
	
	profile_manager.load();
	Profile& profile = profile_manager.get_current_profile();
	profile.signal_display_channel_changed.connect(
		sigc::mem_fun(*this, &Application::on_display_channel_changed));	
}

Application::~Application()
{
	if (engine != NULL)
	{
		delete engine;
		engine = NULL;
	}

	if (main_window != NULL)
	{
		delete main_window;
		main_window = NULL;
	}
}

Glib::ustring Application::get_configuration_path(const Glib::ustring& key)
{
	return Glib::ustring::compose(GCONF_PATH"/%1", key);
}

void Application::set_string_configuration_default(const Glib::ustring& key, const Glib::ustring& value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting string configuration value '%s' = '%s'", key.c_str(), value.c_str());
		client->set(path, value);
	}
}

void Application::set_int_configuration_default(const Glib::ustring& key, gint value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%d'", path.c_str(), value);
		client->set(path, value);
	}
}

void Application::set_boolean_configuration_default(const Glib::ustring& key, gboolean value)
{
	Glib::ustring path = get_configuration_path(key);
	Gnome::Conf::Value v = client->get(path);
	if (v.get_type() == Gnome::Conf::VALUE_INVALID)
	{
		g_debug("Setting int configuration value '%s' = '%s'", path.c_str(), value ? "true" : "false");
		client->set(path, (bool)value);
	}
}

Glib::ustring Application::get_string_configuration_value(const Glib::ustring& key)
{
	return client->get_string(get_configuration_path(key));
}

gint Application::get_int_configuration_value(const Glib::ustring& key)
{
	return client->get_int(get_configuration_path(key));
}

gint Application::get_boolean_configuration_value(const Glib::ustring& key)
{
	return client->get_bool(get_configuration_path(key));
}

void Application::set_string_configuration_value(const Glib::ustring& key, const Glib::ustring& value)
{
	client->set(get_configuration_path(key), value);
}

void Application::set_int_configuration_value(const Glib::ustring& key, gint value)
{
	client->set(get_configuration_path(key), (gint)value);
}

void Application::set_boolean_configuration_value(const Glib::ustring& key, gboolean value)
{
	client->set(get_configuration_path(key), (bool)value);
}

void Application::run()
{
	status_icon = new StatusIcon(glade);
	main_window = MainWindow::create(glade);
	main_window->show();
	Gnome::Main::run();
}

Application& Application::get_current()
{
	if (current == NULL)
	{
		throw Exception("Application has not been initialised");
	}
	
	return *current;
}

void Application::set_source(const Glib::ustring& source)
{
	if (engine != NULL)
	{
		g_debug("Deleting engine");
		delete engine;
		engine = NULL;
	}
	
	if (!source.empty())
	{
		Glib::ustring engine_type = get_string_configuration_value("engine_type");
		if (engine_type == "gstreamer")
		{
			engine = new GStreamerEngine();
		}
		else if (engine_type == "mplayer")
		{
			engine = new MplayerEngine();
		}
		else
		{
			throw Exception(_("Unknown engine type"));
		}
		
		engine->mute(main_window->get_mute_state());
		engine->play(main_window->get_drawing_area().get_window(), source);

		update_ui();
	}
}

void Application::update_ui()
{
	main_window->update();
	status_icon->update();
}

void Application::record(const Glib::ustring& filename)
{
	if (engine == NULL)
	{
		throw Exception("Nothing to record");
	}
	
	engine->record(filename);
}

void Application::mute(gboolean state)
{
	if (main_window != NULL)
	{
		main_window->mute(state);
	}
	
	if (engine != NULL)
	{
		engine->mute(state);
	}
}

void Application::on_display_channel_changed(const Channel& channel)
{
	TRY
	Dvb::Frontend& frontend = device_manager.get_frontend();
	setup_dvb(frontend, channel);
	set_source(frontend.get_adapter().get_dvr_path());
	set_int_configuration_default("last_channel", channel.channel_id);
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
	epg_thread.join(true);
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	
	remove_all_demuxers();
	
	const Dvb::Transponder* current_transponder = frontend.get_current_transponder();
	if (current_transponder == NULL || current_transponder->frontend_parameters.frequency != channel.frontend_parameters.frequency)
	{
		Dvb::Transponder transponder;
		transponder.frontend_parameters = channel.frontend_parameters;
		frontend.tune_to(transponder);
	}
	else
	{
		g_debug("Frontend already tuned to '%d'", channel.frontend_parameters.frequency);
	}
	
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
	
	epg_thread.start();

	g_debug("Finished setting up DVB");
}

class EITDemuxers
{
private:
	GSList* eit_demuxers;
	guint demuxer_count;
	Glib::ustring demuxer_path;

public:
	EITDemuxers(const Glib::ustring& path)
	{
		demuxer_path = path;
		demuxer_count = 0;
		eit_demuxers = NULL;
	}
	
	~EITDemuxers()
	{
		delete_all();
	}
		
	Dvb::Demuxer* add()
	{
		Dvb::Demuxer* demuxer = new Dvb::Demuxer(demuxer_path);
		eit_demuxers = g_slist_append(eit_demuxers, demuxer);
		demuxer_count++;
		return demuxer;
	}

	void delete_all()
	{
		while (eit_demuxers != NULL)
		{
			delete (Dvb::Demuxer*)eit_demuxers->data;
			eit_demuxers = g_slist_delete_link(eit_demuxers, eit_demuxers);
		}
		demuxer_count = 0;
	}

	void get_next_eit(Dvb::SI::SectionParser& parser, Dvb::SI::EventInformationSection& section, gboolean is_atsc)
	{
		if (eit_demuxers == NULL)
		{
			throw Exception("No demuxers");
		}
		
		Dvb::Demuxer* selected_eit_demuxer = NULL;
		
		struct pollfd fds[demuxer_count];
		guint count = 0;
		
		GSList* eit_demuxer = eit_demuxers;
		while (eit_demuxer != NULL)
		{				
			fds[count].fd = ((Dvb::Demuxer*)eit_demuxer->data)->get_fd();
			fds[count].events = POLLIN;
			count++;
			eit_demuxer = g_slist_next(eit_demuxer);
		}

		guint result = ::poll(fds, demuxer_count, 5000);
		if (result < 0)
		{
			throw SystemException ("Failed to poll EIT demuxers");
		}
		
		eit_demuxer = eit_demuxers;
		while (eit_demuxer != NULL && selected_eit_demuxer == NULL)
		{
			Dvb::Demuxer* current = (Dvb::Demuxer*)eit_demuxer->data;
			if (current->poll(1))
			{
				selected_eit_demuxer = current;
			}
			eit_demuxer = g_slist_next(eit_demuxer);				
		}

		if (selected_eit_demuxer == NULL)
		{
			throw Exception("Failed to get an EIT demuxer with events");
		}
		
		if (is_atsc)
		{
			parser.parse_psip_eis(*selected_eit_demuxer, section);
		}
		else
		{
			parser.parse_eis(*selected_eit_demuxer, section);
		}
	}
};

void EpgThread::run()
{
	TRY;

	Data data;
	Dvb::Frontend& frontend = get_application().get_device_manager().get_frontend();
	Profile& profile = get_application().get_profile_manager().get_current_profile();
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
	const Dvb::Transponder* transponder = frontend.get_current_transponder();
	EITDemuxers demuxers(demux_path);
	Dvb::SI::SectionParser parser;
	Dvb::SI::MasterGuideTable master_guide_table;

	gboolean is_atsc = frontend.get_frontend_type() == FE_ATSC;
	if (is_atsc)
	{
		Dvb::Demuxer demuxer_mgt(demux_path);
		demuxer_mgt.set_filter(PSIP_PID, MGT_ID, 0xFF);
		parser.parse_psip_mgt(demuxer_mgt, master_guide_table);
		
		gsize size = master_guide_table.tables.size();
		for (guint i = 0; i < size; i++)
		{
			Dvb::SI::MasterGuideTableTable mgtt = master_guide_table.tables[i];
			if (mgtt.type >= 0100 && mgtt.type <= 0x017F)
			{		
				demuxers.add()->set_filter(mgtt.pid, PSIP_EIT_ID, 0);
				g_debug("Set up PID 0x%02X for events", mgtt.pid);
			}
		}
	}
	else
	{
		demuxers.add()->set_filter(EIT_PID, EIT_ID, 0);
	}
	
	guint processed_event_count = 0;
	guint processed_events[10000];
	guint frequency = transponder->frontend_parameters.frequency;
	while (!is_terminated())
	{
		try
		{
			Dvb::SI::EventInformationSection section;
			
			demuxers.get_next_eit(parser, section, is_atsc);

			guint service_id = section.service_id;
			Channel* channel = profile.find_channel(frequency, service_id);
			if (channel != NULL)
			{
				for( unsigned int k = 0; section.events.size() > k; k++ )
				{
					gboolean found = false;
					Dvb::SI::Event& event	= section.events[k];

					for (guint i = 0; i < processed_event_count && !found; i++)
					{
						if (processed_events[i] == event.event_id)
						{
							found = true;
						}
					}
					
					if (processed_event_count < 10000)
					{
						processed_events[processed_event_count++] = event.event_id;
					
						if (!found)
						{
							EpgEvent epg_event;

							epg_event.epg_event_id	= 0;
							epg_event.channel_id	= channel->channel_id;
							epg_event.event_id		= event.event_id;
							epg_event.start_time	= event.start_time;
							epg_event.duration		= event.duration;
							
							for (Dvb::SI::EventTextList::iterator i = event.texts.begin(); i != event.texts.end(); i++)
							{
								EpgEventText epg_event_text;
								const Dvb::SI::EventText& event_text = *i;
								
								epg_event_text.epg_event_text_id	= 0;
								epg_event_text.epg_event_id			= 0;
								epg_event_text.is_extended			= event_text.is_extended;
								epg_event_text.language				= event_text.language;
								epg_event_text.title				= event_text.title;
								epg_event_text.description			= event_text.description;
								
								epg_event.texts.push_back(epg_event_text);
							}
							
							data.replace_epg_event(epg_event);
							get_application().update_epg_time();
						}
					}
				}
			}
		}
		catch(const TimeoutException& ex)
		{
			g_debug("Timeout in EPG thread: %s", ex.what().c_str());
			terminate();
		}
		catch(const Glib::Exception& ex)
		{
			g_debug("Exception in EPG thread: %s", ex.what().c_str());
		}
	}

	THREAD_CATCH;

	g_debug(_("Exiting EPG thread"));
}

void Application::update_epg_time()
{
	last_epg_update_time = time(NULL);
}

guint Application::get_last_epg_update_time() const
{
	return last_epg_update_time;
}

void Application::toggle_visibility()
{
	if (main_window != NULL)
	{
		main_window->property_visible() = !main_window->property_visible();
	}
}
