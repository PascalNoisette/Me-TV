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

StreamManager::~StreamManager()
{
	g_debug("Destroying StreamManager");
	stop();

	FrontendThreadList::iterator i = frontend_threads.begin(); 
	while (i != frontend_threads.end())
	{
		FrontendThread* frontend_thread = *i;
		delete frontend_thread;
		i = frontend_threads.erase(i);
	}
	g_debug("StreamManager destroyed");
}

void StreamManager::load()
{
	g_debug("Creating stream manager");
	FrontendList& frontends = get_application().device_manager.get_frontends();
	for(FrontendList::iterator i = frontends.begin(); i != frontends.end(); i++)
	{
		g_debug("Creating frontend thread");
		FrontendThread* frontend_thread = new FrontendThread(**i);
		frontend_threads.push_back(frontend_thread);
	}
}

guint StreamManager::get_last_epg_update_time()
{
	guint result = 0;
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
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
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		if (frontend_thread.is_recording())
		{
			return true;
		}
	}
	return false;
}

gboolean StreamManager::is_recording(const Channel& channel)
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
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
	
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		g_debug("CHECKING: %s", frontend_thread.frontend.get_path().c_str());
		if (frontend_thread.frontend.get_path() == scheduled_recording.device)
		{
			frontend_thread.start_recording(channel, scheduled_recording.description, true);
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
	
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
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
		for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
		{
			FrontendThread& frontend_thread = **i;
			if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
			{
				g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
				continue;
			}

			if (frontend_thread.frontend.get_frontend_parameters().frequency ==
			    channel.transponder.frontend_parameters.frequency)
			{
				g_debug("Found a frontend already tuned to the correct transponder");
				frontend_thread.start_recording(channel, channel.name, false);
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
			{
				FrontendThread& frontend_thread = **i;
				if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
				{
					g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
					continue;
				}
				
				if (frontend_thread.get_streams().empty())
				{
					g_debug("Selected idle frontend '%s' for recording", frontend_thread.frontend.get_name().c_str());
					frontend_thread.start_recording(channel, channel.name, false);
					found = true;
					break;
				}
			}

			if (!found)
			{
				throw Exception(_("Failed to get available frontend"));
			}
		}
	}
}

void StreamManager::stop_recording(const Channel& channel)
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		frontend_thread.stop_recording(channel);
	}
}

void StreamManager::start()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		g_debug("Starting frontend thread");
		FrontendThread& frontend_thread = **i;
		frontend_thread.start();
	}
}

void StreamManager::stop()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		g_debug("Stopping frontend thread");
		FrontendThread& frontend_thread = **i;
		frontend_thread.stop();
	}
}

void StreamManager::start_display(Channel& channel)
{
	gboolean found = false;

	g_debug("Not currently displaying '%s'", channel.name.c_str());
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
		{
			g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
			continue;
		}

		if (frontend_thread.frontend.get_frontend_parameters().frequency ==
		    channel.transponder.frontend_parameters.frequency)
		{
			g_debug("Found a frontend already tuned to the correct transponder");
			stop_display();
			frontend_thread.start_display(channel);
			found = true;
			break;
		}
	}
	
	if (!found)
	{
		for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
		{
			FrontendThread& frontend_thread = **i;
			if (frontend_thread.frontend.get_frontend_type() != channel.transponder.frontend_type)
			{
				g_debug("'%s' incompatible", frontend_thread.frontend.get_name().c_str());
				continue;
			}
			
			if (!frontend_thread.is_recording())
			{
				g_debug("Selected idle frontend '%s' for display", frontend_thread.frontend.get_name().c_str());
				stop_display();
				frontend_thread.start_display(channel);
				found = true;
				break;
			}
		}

		if (!found)
		{
			throw Exception(_("Failed to get available frontend"));
		}
	}
}

void StreamManager::stop_display()
{
	FrontendThread* free_frontend_thread = NULL;
	
	g_debug("StreamManager stopping display");
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		frontend_thread.stop_display();
	}
}

Channel& StreamManager::get_display_channel()
{
	return get_display_stream().channel;
}

ChannelStream& StreamManager::get_display_stream()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		ChannelStreamList& streams = frontend_thread.get_streams();
		for (ChannelStreamList::iterator j = streams.begin(); j != streams.end(); j++)
		{
			ChannelStream& stream = **j;
			if (stream.type == CHANNEL_STREAM_TYPE_DISPLAY)
			{
				return stream;
			}
		}
	}

	throw Exception(_("Failed to get display stream"));
}

gboolean StreamManager::has_display_stream()
{
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		if (frontend_thread.is_display())
		{
			return true;
		}
	}

	return false;
}

FrontendThread& StreamManager::get_display_frontend_thread()
{
	FrontendThread* free_frontend_thread = NULL;
	
	Dvb::Transponder& current_transponder = get_application().stream_manager.get_display_channel().transponder;
	for (FrontendThreadList::iterator i = frontend_threads.begin(); i != frontend_threads.end(); i++)
	{
		FrontendThread& frontend_thread = **i;
		ChannelStreamList& streams = frontend_thread.get_streams();

		if (free_frontend_thread == NULL && streams.empty())
		{
			free_frontend_thread = &frontend_thread;
		}
		
		for (ChannelStreamList::iterator j = streams.begin(); j != streams.end(); j++)
		{
			ChannelStream& stream = **j;
			if (stream.type == CHANNEL_STREAM_TYPE_DISPLAY)
			{
				return frontend_thread;
			}
		}
	}

	if (free_frontend_thread != NULL)
	{
		return *free_frontend_thread;
	}

	throw Exception(_("Failed to get display frontend thread"));
}
