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

#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include "me-tv.h"
#include <glibmm.h>
#include <giomm.h>
#include "dvb_frontend.h"
#include "exception.h"

typedef std::list<Dvb::Frontend *> FrontendList;

class DeviceManager {
private:
	FrontendList frontends;
	static Glib::ustring get_adapter_path(guint adapter);
	static Glib::ustring get_frontend_path(guint adapter, guint frontend);
	static gboolean is_frontend_supported(Dvb::Frontend const & frontend);
public:
	void initialise();
	Dvb::Frontend * find_frontend_by_path(Glib::ustring const & path);
	FrontendList & get_frontends() { return frontends; };
	void check_frontend();
};

#endif
