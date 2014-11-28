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

#ifndef __DVB_TRANSPONDER_H__
#define __DVB_TRANSPONDER_H__

#include <glibmm.h>
#include <linux/dvb/frontend.h>
#include "dvb_service.h"

namespace Dvb {

	enum polarisation {
		POLARISATION_HORIZONTAL = 0x00,
		POLARISATION_VERTICAL = 0x01,
		POLARISATION_CIRCULAR_LEFT = 0x02,
		POLARISATION_CIRCULAR_RIGHT = 0x03
	};

	class Transponder {
	public:
		Transponder();
		bool operator==(Transponder const & transponder) const;
		bool operator!=(Transponder const & transponder) const;
		bool operator==(dvb_frontend_parameters frontend_parameters) const;
		bool operator!=(dvb_frontend_parameters frontend_parameters) const;
		fe_type_t frontend_type;
		dvb_frontend_parameters frontend_parameters;
		guint polarisation;
		guint satellite_number;
		guint hi_band;
	};

	class TransponderList : public std::list<Transponder> {
	public:
		gboolean exists(Transponder const & transponder);
		void add(Transponder const & transponder);
	};

}

#endif
