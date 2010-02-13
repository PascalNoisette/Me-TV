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

void StreamManager::start_recording(const Channel& channel, const Glib::ustring& filename, gboolean scheduled)
{
	// TODO
	throw Exception("Not implemented");
}

void StreamManager::stop_recording(const Channel& channel)
{
	// TODO
	throw Exception("Not implemented");
}

void StreamManager::start()
{
	FrontendList& frontends = get_application().device_manager.get_frontends();
	for(FrontendList::iterator i = frontends.begin(); i != frontends.end(); i++)
	{
		frontend_threads.push_back(FrontendThread(**i));
		(*frontend_threads.rbegin()).start();
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