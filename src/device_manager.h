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

namespace Dvb
{
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

	public:
		DeviceManager()
		{
			g_debug("Device manager starting");
			guint adapter_count = 0;
			while (Gio::File::create_for_path(get_adapter_string(adapter_count))->query_exists())
			{
				Adapter adapter(adapter_count);
				
				guint frontend_count = 0;
				while (Gio::File::create_for_path(get_frontend_string(adapter_count, frontend_count))->query_exists())
				{
					Frontend frontend(adapter, frontend_count);
					g_debug("Registered %s", frontend.get_frontend_info().name);
					frontend_count++;
				}

				adapter_count++;
			}
			
			g_debug("Device manager initialised with %d DVB adapter(s)", adapter_count);
		}
	};
}

#endif
