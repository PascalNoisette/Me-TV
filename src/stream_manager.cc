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

guint StreamManager::get_last_epg_update_time()
{
	guint result = 0;
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		guint last_update_time = frontend_thread.get_last_epg_update_time();
		if (last_update_time > result)
		{
			result = last_update_time;
		}
	}
	return result;
}

gboolean StreamManager::is_recording()
{
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		if (frontend_thread.is_recording())
		{
			return true;
		}
	}
	return false;
}

std::list<ChannelStream> StreamManager::get_streams()
{
	std::list<ChannelStream> result;
	
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		std::list<ChannelStream> streams = frontend_thread.get_streams();
		for (std::list<ChannelStream>::iterator j = streams.begin(); j != streams.end(); j++)
		{
			result.push_back(*j);
		}
	}

	return result;
}

gboolean StreamManager::is_recording(const Channel& channel)
{
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		if (frontend_thread.is_recording(channel))
		{
			return true;
		}
	}
	return false;
}

void StreamManager::start_recording(Channel& channel, const ScheduledRecording& scheduled_recording)
{
	gboolean found = false;
	
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		if (frontend_thread.frontend.get_path() == scheduled_recording.device)
		{
			frontend_thread.start_recording(channel,
			    make_recording_filename(channel, scheduled_recording.description),
			    true);
			found = true;
		}
	}

	if (!found)
	{
		Glib::ustring message = Glib::ustring::compose(
		    _("Failed to find frontend '%1' for scheduled recording"),
		    scheduled_recording.device);
		throw Exception(message);
	}
}

void StreamManager::start_recording(Channel& channel)
{
	gboolean recording = false;

	g_debug("Request to record '%s'", channel.name.c_str());
	
	// TODO: Need to look into the SRs to see if the selected frontend recording will conflict
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
		{
			g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
			continue;
		}
		
		if (frontend_thread.is_recording(channel))
		{
			g_debug("Already recording on '%s'", channel.name.c_str());
			recording = true;
			break;
		}
	}

	if (!recording)
	{
		gboolean found = false;
		
		g_debug("Not currently recording '%s'", channel.name.c_str());
		for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
		{
			FrontendThread& frontend_thread = *i;
			if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
			{
				g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
				continue;
			}

			if (frontend_thread.frontend.get_frontend_parameters().frequency ==
			    channel.transponder.frontend_parameters.frequency)
			{
				g_debug("Found a frontend already tuned to the correct transponder");
				frontend_thread.start_recording(channel,
				    make_recording_filename(channel),
				    false);
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (std::list<FrontendThread>::reverse_iterator i = frontend_threads.rbegin(); i != frontend_threads.rend(); i++)
			{
				FrontendThread& frontend_thread = *i;
				if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
				{
					g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
					continue;
				}

				if (!frontend_thread.is_recording())
				{
					g_debug("Selected idle frontend '%s' for recording", frontend_thread.frontend.get_name().c_str());
					frontend_thread.start_recording(channel,
					    make_recording_filename(channel),
					    false);
					break;
				}
			}
		}
	}
}

void StreamManager::stop_recording(const Channel& channel)
{
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		frontend_thread.stop_recording(channel);
	}
}

void StreamManager::start()
{
	g_debug("Starting stream manager");
	FrontendList& frontends = get_application().device_manager.get_frontends();
	for(FrontendList::iterator i = frontends.begin(); i != frontends.end(); i++)
	{
		g_debug("Starting frontend thread");
		FrontendThread* frontend_thread = new FrontendThread(**i);
		frontend_threads.push_back(*frontend_thread);
		frontend_threads.back().start();
	}
}

void StreamManager::stop()
{
	for (std::list<FrontendThread>::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = *i;
		frontend_thread.stop();
	}
}

void StreamManager::set_display_stream(const Channel& channel)
{
	get_display_frontend_thread().set_display_stream(channel);
}

const ChannelStream& StreamManager::get_display_stream()
{
	return get_display_frontend_thread().get_display_stream();
}

FrontendThread& StreamManager::get_display_frontend_thread()
{
	if (frontend_threads.empty())
	{
		throw Exception("Failed to get Display Frontend Thread: no threads");
	}

	return *(frontend_threads.begin());
}
