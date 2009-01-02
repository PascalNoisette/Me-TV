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

#include "epg_thread.h"
#include "application.h"

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
		
	void get_next_eit(Dvb::SI::SectionParser& parser, Dvb::SI::EventInformationSection& section, gboolean is_atsc);
	
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
};

void EITDemuxers::get_next_eit(Dvb::SI::SectionParser& parser, Dvb::SI::EventInformationSection& section, gboolean is_atsc)
{
	if (eit_demuxers == NULL)
	{
		throw Exception(_("No demuxers"));
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

	gint result = ::poll(fds, demuxer_count, 5000);
	if (result < 0)
	{
		throw SystemException (_("Failed to poll EIT demuxers"));
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
		throw Exception(_("Failed to get an EIT demuxer with events"));
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

void EpgThread::run()
{
	TRY;

	Data data;
	Dvb::Frontend& frontend = get_application().get_device_manager().get_frontend();
	Profile& profile = get_application().get_profile_manager().get_current_profile();
	Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
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
	guint frequency = frontend.get_frontend_parameters().frequency;
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
						if (!found)
						{
							processed_events[processed_event_count++] = event.event_id;

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

							g_debug("EPG event %d added", event.event_id);
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
