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
 
#ifndef __DVB_SERVICE_H__
#define __DVB_SERVICE_H__

#include <glibmm.h>

namespace Dvb
{
	class Transponder;

	class Service
	{
	private:
	public:
		Service(const Transponder& transponder);
			
		Glib::ustring		name;
		guint				id;
		const Transponder&	transponder;
		
		gboolean operator ==(const Service& service) const;
		gboolean operator !=(const Service& service) const;
	};

	typedef std::list<Service> ServiceList;
}

#endif