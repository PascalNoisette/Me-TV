/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DVB_SERVICE_H__
#define __DVB_SERVICE_H__

#include <glibmm.h>
#include <list>

namespace Dvb {

	class Transponder;

	class Service {
	public:
		Service(Transponder & transponder);
		Glib::ustring name;
		guint id;
		Transponder & transponder;
		gboolean operator ==(Service const & service) const;
		gboolean operator !=(Service const & service) const;
	};

	typedef std::list<Service> ServiceList;
}

#endif
