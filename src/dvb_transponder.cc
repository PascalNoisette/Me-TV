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

#include "dvb_transponder.h"
#include "dvb_service.h"
#include "exception.h"
#include "me-tv.h"
#include "me-tv-i18n.h"

using namespace Dvb;

Transponder::Transponder() : polarisation(0), satellite_number(0), hi_band(0) {
	frontend_type = FE_OFDM;
	memset(&frontend_parameters, 0, sizeof(frontend_parameters));
}

gboolean TransponderList::exists(Transponder const & transponder) {
	for (const_iterator i = begin(); i != end(); ++i) {
		if (transponder == *i) { return true; }
	}
	return false;
}

void TransponderList::add(Transponder const & transponder) {
	if (!exists(transponder)) { push_back(transponder); }
}

bool Transponder::operator==(Transponder const & transponder) const {
	return transponder == frontend_parameters;
}

bool Transponder::operator!=(Transponder const & transponder) const {
	return transponder != frontend_parameters;
}

bool Transponder::operator==(dvb_frontend_parameters p) const {
	return frontend_parameters.frequency == p.frequency && (
		(frontend_type == FE_OFDM && frontend_parameters.u.ofdm.bandwidth == p.u.ofdm.bandwidth) ||
		(frontend_type == FE_ATSC && frontend_parameters.u.vsb.modulation == p.u.vsb.modulation) ||
		(frontend_type == FE_QAM && frontend_parameters.u.qam.modulation == p.u.qam.modulation) ||
	    frontend_type == FE_QPSK
	);
}

bool Transponder::operator!=(dvb_frontend_parameters p) const {
	return !(*this == p);
}
