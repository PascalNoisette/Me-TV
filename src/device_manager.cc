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

Glib::ustring DeviceManager::get_adapter_string(guint adapter)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1", adapter);
}

Glib::ustring DeviceManager::get_frontend_string(guint adapter, guint frontend)
{
	return Glib::ustring::compose("/dev/dvb/adapter%1/frontend%2", adapter, frontend);
}
	
DeviceManager::DeviceManager()
{
	g_debug("Device manager starting");
	guint adapter_count = 0;
	while (Gio::File::create_for_path(get_adapter_string(adapter_count))->query_exists())
	{
		Dvb::Adapter* adapter = new Dvb::Adapter(adapter_count);
		adapters.push_back(adapter);
		
		guint frontend_count = 0;
		while (Gio::File::create_for_path(get_frontend_string(adapter_count, frontend_count))->query_exists())
		{
			Dvb::Frontend* frontend = new Dvb::Frontend(*adapter, frontend_count);
			g_debug("Registered %s", frontend->get_frontend_info().name);
			frontends.push_back(frontend);
			frontend_count++;
		}

		adapter_count++;
	}
	
	g_debug("Device manager initialised with %d DVB adapter(s)", adapter_count);
}

DeviceManager::~DeviceManager()
{
	while (frontends.size() > 0)
	{
		Dvb::Frontend* frontend = *(frontends.begin());
		frontends.pop_front();
		Glib::ustring device_name = frontend->get_frontend_info().name;
		free(frontend);
		g_debug("Deregistered %s", device_name.c_str());
	}
	
	while (adapters.size() > 0)
	{
		free(*(adapters.begin()));
		adapters.pop_front();
	}
}
	
const std::list<Dvb::Frontend*> DeviceManager::get_frontends() const
{
	return frontends;
}
	
Dvb::Frontend& DeviceManager::get_frontend_by_name(const Glib::ustring& frontend_name)
{
	Dvb::Frontend* result = NULL;
	
	std::list<Dvb::Frontend*>::const_iterator frontend_iterator = frontends.begin();
	while (frontend_iterator != frontends.end() && result == NULL)
	{
		Dvb::Frontend* frontend = *frontend_iterator;
		if (frontend_name == frontend->get_frontend_info().name)
		{
			result = frontend;
		}
		frontend_iterator++;
	}

	if (result == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find frontend '%1'"), frontend_name));
	}

	return *result;
}
	
Dvb::Frontend* DeviceManager::request_frontend(Event event)
{
	Dvb::Frontend* result = NULL;
	if (frontends.size() > 0)
	{
		result = *(frontends.begin());
	}
	return result;
}
