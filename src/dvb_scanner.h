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

#ifndef __DVB_SCANNER_H__
#define __DVB_SCANNER_H__

#include <vector>
#include "dvb_frontend.h"
#include "dvb_service.h"
#include "dvb_transponder.h"

namespace Dvb {
	class Scanner {
	private:
		gboolean terminated;
		TransponderList transponders;
		void tune_to(Frontend & frontend, Transponder const & transponder);
		void atsc_tune_to(Frontend & frontend, Transponder const & transponder);
	public:
		Scanner();
		void start(Frontend & frontend, TransponderList & transponders);
		void terminate();
		sigc::signal<void, dvb_frontend_parameters const &, guint, Glib::ustring const &, guint const, guint> signal_service;
		sigc::signal<void, guint, gsize> signal_progress;
		sigc::signal<void> signal_complete;
	};
}

#endif
