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
 
#include "dvb_transponder.h"
#include "dvb_service.h"

using namespace Dvb;

Transponder::Transponder()
{
	services = g_hash_table_new(g_int_hash, g_int_equal);
}

void Transponder::add_service(Service& service)
{
	g_hash_table_insert (services, &(service.id), &service);
}

Service* Transponder::get_service(guint service_id)
{
	return (Service*)g_hash_table_lookup(services,  &service_id);
}
