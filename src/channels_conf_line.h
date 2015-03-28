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

#ifndef CHANNELS_CONF_LINE
#define CHANNELS_CONF_LINE

#include "me-tv.h"
#include <linux/dvb/frontend.h>

class ChannelsConfLine {
private:
	static StringTable bandwidth_table[];
	static StringTable fec_table[];
	static StringTable transmit_mode_table[];
	static StringTable guard_table[];
	static StringTable hierarchy_table[];
	static StringTable inversion_table[];
	static StringTable modulation_table[];
	static StringTable polarisation_table[];

	std::vector<Glib::ustring> parts;
public:
	ChannelsConfLine(Glib::ustring const & line);
	guint get_parameter_count() const { return parts.size(); }
	Glib::ustring const & get_name(guint index);
	fe_spectral_inversion_t get_inversion(guint index);
	fe_bandwidth_t get_bandwidth(guint index);
	fe_code_rate_t get_fec(guint index);
	fe_modulation_t get_modulation(guint index);
	fe_transmit_mode_t get_transmit_mode(guint index);
	fe_guard_interval_t get_guard_interval(guint index);
	fe_hierarchy_t get_hierarchy(guint index);
	guint get_symbol_rate(guint index);
	guint get_service_id(guint index);
	guint get_polarisation(guint index);
	guint get_int(guint index);
};

#endif
