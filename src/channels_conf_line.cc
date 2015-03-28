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

#include "channels_conf_line.h"

StringTable ChannelsConfLine::bandwidth_table[] = {
	{ "BANDWIDTH_8_MHZ",	BANDWIDTH_8_MHZ },
	{ "BANDWIDTH_7_MHZ",	BANDWIDTH_7_MHZ },
	{ "BANDWIDTH_6_MHZ",	BANDWIDTH_6_MHZ },
	{ "BANDWIDTH_AUTO",		BANDWIDTH_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::fec_table[] = {
	{ "FEC_NONE",	FEC_NONE },
	{ "FEC_1_2",	FEC_1_2 },
	{ "FEC_2_3",	FEC_2_3 },
	{ "FEC_3_4",	FEC_3_4 },
	{ "FEC_4_5",	FEC_4_5 },
	{ "FEC_5_6",	FEC_5_6 },
	{ "FEC_6_7",	FEC_6_7 },
	{ "FEC_7_8",	FEC_7_8 },
	{ "FEC_8_9",	FEC_8_9 },
	{ "FEC_AUTO",	FEC_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::modulation_table[] = {
	{ "QPSK",		QPSK },
	{ "QAM_16",		QAM_16 },
	{ "QAM_32",		QAM_32 },
	{ "QAM_64",		QAM_64 },
	{ "QAM_128",	QAM_128 },
	{ "QAM_256",	QAM_256 },
	{ "QAM_AUTO",	QAM_AUTO },
	{ "VSB_8",		VSB_8 },
	{ "VSB_16",		VSB_16 },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::transmit_mode_table[] = {
	{ "TRANSMISSION_MODE_2K",   TRANSMISSION_MODE_2K },
	{ "TRANSMISSION_MODE_8K",   TRANSMISSION_MODE_8K },
	{ "TRANSMISSION_MODE_AUTO", TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::guard_table[] = {
	{ "GUARD_INTERVAL_1_32",	GUARD_INTERVAL_1_32 },
	{ "GUARD_INTERVAL_1_16",	GUARD_INTERVAL_1_16 },
	{ "GUARD_INTERVAL_1_8",		GUARD_INTERVAL_1_8 },
	{ "GUARD_INTERVAL_1_4",		GUARD_INTERVAL_1_4 },
	{ "GUARD_INTERVAL_AUTO",	GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::hierarchy_table[] = {
	{ "HIERARCHY_NONE", HIERARCHY_NONE },
	{ "HIERARCHY_1",    HIERARCHY_1 },
	{ "HIERARCHY_2",    HIERARCHY_2 },
	{ "HIERARCHY_4",    HIERARCHY_4 },
	{ "HIERARCHY_AUTO", HIERARCHY_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::inversion_table[] = {
	{ "INVERSION_OFF",	INVERSION_OFF },
	{ "INVERSION_ON",	INVERSION_ON },
	{ "INVERSION_AUTO",	INVERSION_AUTO },
	{ NULL, 0 }
};

StringTable ChannelsConfLine::polarisation_table[] = {
	{ "h",	0 },
	{ "v",	1 },
	{ NULL, 0 }
};

ChannelsConfLine::ChannelsConfLine(Glib::ustring const & line) {
	split_string(parts, line, ":", false, 20);
}

const Glib::ustring& ChannelsConfLine::get_name(guint index) {
	return parts[index];
}

fe_spectral_inversion_t ChannelsConfLine::get_inversion(guint index) {
	return (fe_spectral_inversion_t)convert_string_to_value(inversion_table, parts[index]);
}

fe_bandwidth_t ChannelsConfLine::get_bandwidth(guint index) {
	return (fe_bandwidth_t)convert_string_to_value(bandwidth_table, parts[index]);
}

fe_code_rate_t ChannelsConfLine::get_fec(guint index) {
	return (fe_code_rate_t)convert_string_to_value(fec_table, parts[index]);
}

fe_modulation_t ChannelsConfLine::get_modulation(guint index) {
	return (fe_modulation_t)convert_string_to_value(modulation_table, parts[index]);
}

fe_transmit_mode_t ChannelsConfLine::get_transmit_mode(guint index) {
	return (fe_transmit_mode_t)convert_string_to_value(transmit_mode_table, parts[index]);
}

fe_guard_interval_t	ChannelsConfLine::get_guard_interval(guint index) {
	return (fe_guard_interval_t)convert_string_to_value(guard_table, parts[index]);
}

fe_hierarchy_t ChannelsConfLine::get_hierarchy(guint index) {
	return (fe_hierarchy_t)convert_string_to_value(hierarchy_table, parts[index]);
}

guint ChannelsConfLine::get_symbol_rate(guint index) {
	return atoi(parts[index].c_str());
}

guint ChannelsConfLine::get_service_id(guint index) {
	return atoi(parts[index].c_str());
}

guint ChannelsConfLine::get_polarisation(guint index) {
	return convert_string_to_value(polarisation_table, parts[index].c_str());
}

guint ChannelsConfLine::get_int(guint index) {
	return atoi(parts[index].c_str());
}
