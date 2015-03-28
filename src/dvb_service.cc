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

#include "me-tv.h"
#include "dvb_service.h"
#include "dvb_transponder.h"

using namespace Dvb;

Service::Service(Transponder & service_transponder): id(0), transponder(service_transponder) { }

gboolean Service::operator ==(Service const & service) const {
	return service.id == id && service.transponder == transponder;
}

gboolean Service::operator !=(Service const & service) const {
	return !(*this == service);
}
