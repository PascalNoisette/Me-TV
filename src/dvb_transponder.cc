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
 
#include "dvb_transponder.h"
#include "dvb_service.h"
#include "exception.h"
#include "me-tv.h"

using namespace Dvb;

void Transponder::add_service(Service& service)
{
	services.push_back(service);
}

Service& Transponder::get_service(guint service_id)
{
	Service* result = NULL;
	gboolean found = false;

	ServiceList::iterator iterator = services.begin();
	while (iterator != services.end() && !found)
	{
		Service& service = *iterator;
		if (service.id == service_id)
		{
			result = &service;
			found = true;
		}
	}

	if (result == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find service with service ID %1"), service_id));
	}
	
	return *result;
}
