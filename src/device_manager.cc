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

#include "device_manager.h"

Glib::ustring DeviceManager::get_adapter_path(guint adapter)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1", adapter);
}

Glib::ustring DeviceManager::get_frontend_path(guint adapter, guint frontend_index)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1/frontend%2", adapter, frontend_index);
}
	
DeviceManager::DeviceManager()
{
	frontend = NULL;
	Glib::ustring frontend_path;
	
	g_debug("Scanning DVB devices ...");
	guint adapter_count = 0;
	Glib::ustring adapter_path = get_adapter_path(adapter_count);
	while (Gio::File::create_for_path(adapter_path)->query_exists())
	{
		// TODO: This leaks
		Dvb::Adapter* adapter = new Dvb::Adapter(adapter_count);
		
		guint frontend_count = 0;
		frontend_path = get_frontend_path(adapter_count, frontend_count);
		while (Gio::File::create_for_path(frontend_path)->query_exists())
		{
			try
			{
				Dvb::Frontend* current = new Dvb::Frontend(*adapter, frontend_count);
				if (!is_frontend_supported(*current))
				{
					g_debug("Frontend not supported");
				}
				else
				{
					frontends.push_back(current);
				}
			}
			catch(...)
			{
				g_debug("Failed to load '%s'", frontend_path.c_str());
			}
			
			frontend_path = get_frontend_path(adapter_count, ++frontend_count);
		}

		adapter_path = get_adapter_path(++adapter_count);
	}
	
	if (frontends.size() > 0)
	{
		frontend = *(frontends.begin());
	}
	
	if (frontend != NULL)
	{
		g_debug("Using '%s' (%s) ", frontend->get_frontend_info().name, frontend->get_path().c_str());
	}
	else
	{
		throw Exception(_("No frontend available"));
	}
}

DeviceManager::~DeviceManager()
{
	if (frontend != NULL)
	{
		delete frontend;
		g_debug("Closed DVB device");
	}
}

gboolean DeviceManager::is_frontend_supported(const Dvb::Frontend& test_frontend)
{
	gboolean result = false;

	switch(test_frontend.get_frontend_type())
	{
	case FE_OFDM: result = true; break;	// DVB-T
	case FE_QAM: result = true; break;	// DVB-C
	case FE_ATSC: result = true; break;
	default: result = false; break;
	}

	return result;
}

Dvb::Frontend& DeviceManager::get_frontend()
{	
	return *frontend;
}

Dvb::Frontend& DeviceManager::get_frontend_by_path(const Glib::ustring& path)
{
	Dvb::Frontend* result = NULL;
	
	for (FrontendList::iterator iterator = frontends.begin(); iterator != frontends.end(); iterator++)
	{
		Dvb::Frontend* current_frontend = *iterator;
		if (current_frontend->get_path() == path)
		{
			result = current_frontend;
		}
	}
	
	if (result == NULL)
	{
		throw Exception(_("Failed to get frontend by path"));
	}
	
	return *result;
}
