/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014  Russel Winder
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

#ifndef INITIAL_SCAN_LINE
#define INITIAL_SCAN_LINE

#include "me-tv.h"
#include <linux/dvb/frontend.h>

class InitialScanLine {
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
	InitialScanLine(Glib::ustring const & line);
	guint get_parameter_count() const { return parts.size(); }
	guint get_frequency(guint index);
	fe_bandwidth_t get_bandwidth(guint index);
	fe_code_rate_t get_fec(guint index);
	fe_modulation_t get_modulation(guint index);
	fe_transmit_mode_t get_transmit_mode(guint index);
	fe_guard_interval_t get_guard_interval(guint index);
	fe_hierarchy_t get_hierarchy(guint index);
	guint get_symbol_rate(guint index);
	guint get_polarisation(guint index);
};

#endif
