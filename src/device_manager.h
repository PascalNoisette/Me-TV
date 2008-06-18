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

#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include <glibmm.h>
#include <giomm.h>
#include "dvb_frontend.h"
#include "exception.h"
#include "me-tv.h"
#include "scheduler.h"

namespace Dvb
{
	typedef enum
	{
		USAGE_TYPE_SCANNING,
		USAGE_TYPE_RECORDING,		
		USAGE_TYPE_VIEWING,
		USAGE_TYPE_EPG_UPDATE
	} UsageType;
	
	class FrontendEvent : public Event
	{
	private:
		UsageType usage_type;
	public:
		FrontendEvent(UsageType usage_type, Event event) : Event(event), usage_type(usage_type) {}
	};
	
	class DeviceManager
	{
	private:
		Glib::ustring get_adapter_string(guint adapter)
		{
			return Glib::ustring::compose("/dev/dvb/adapter%1", adapter);
		}

		Glib::ustring get_frontend_string(guint adapter, guint frontend)
		{
			return Glib::ustring::compose("/dev/dvb/adapter%1/frontend%2", adapter, frontend);
		}
		
		std::list<Adapter*> adapters;
		std::list<Frontend*> frontends;
		Scheduler scheduler;

	public:
		DeviceManager()
		{
			g_debug("Device manager starting");
			guint adapter_count = 0;
			while (Gio::File::create_for_path(get_adapter_string(adapter_count))->query_exists())
			{
				Adapter* adapter = new Adapter(adapter_count);
				adapters.push_back(adapter);
				
				guint frontend_count = 0;
				while (Gio::File::create_for_path(get_frontend_string(adapter_count, frontend_count))->query_exists())
				{
					Frontend* frontend = new Frontend(*adapter, frontend_count);
					g_debug("Registered %s", frontend->get_frontend_info().name);
					frontends.push_back(frontend);
					frontend_count++;
				}

				adapter_count++;
			}
			
			g_debug("Device manager initialised with %d DVB adapter(s)", adapter_count);
		}
		
		~DeviceManager()
		{
			while (frontends.size() > 0)
			{
				Frontend* frontend = *(frontends.begin());
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
			
		const std::list<Frontend*> get_frontends() const { return frontends; }
			
		Frontend& get_frontend_by_name(const Glib::ustring& frontend_name)
		{
			Frontend* result = NULL;
			
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
			
		Frontend* request_frontend(Event event)
		{
			Frontend* result = NULL;
			if (frontends.size() > 0)
			{
				result = *(frontends.begin());
			}
			return result;
		}
	};
}

#endif
