/*
 * Copyright (C) 2009 Michael Lamothe
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

#include "initial_scan_line.h"

struct StringTable InitialScanLine::bandwidth_table[] =
{
	{ "8MHz", BANDWIDTH_8_MHZ },
	{ "7MHz", BANDWIDTH_7_MHZ },
	{ "6MHz", BANDWIDTH_6_MHZ },
	{ "AUTO", BANDWIDTH_AUTO },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::fec_table[] =
{
	{ "NONE", FEC_NONE },
	{ "1/2",  FEC_1_2 },
	{ "2/3",  FEC_2_3 },
	{ "3/4",  FEC_3_4 },
	{ "4/5",  FEC_4_5 },
	{ "5/6",  FEC_5_6 },
	{ "6/7",  FEC_6_7 },
	{ "7/8",  FEC_7_8 },
	{ "8/9",  FEC_8_9 },
	{ "AUTO", FEC_AUTO },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::modulation_table[] =
{
	{ "QPSK",   QPSK },
	{ "QAM16",  QAM_16 },
	{ "QAM32",  QAM_32 },
	{ "QAM64",  QAM_64 },
	{ "QAM128", QAM_128 },
	{ "QAM256", QAM_256 },
	{ "AUTO",   QAM_AUTO },
	{ "8VSB",   VSB_8 },
	{ "16VSB",  VSB_16 },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::transmit_mode_table[] =
{
	{ "2k",   TRANSMISSION_MODE_2K },
	{ "8k",   TRANSMISSION_MODE_8K },
	{ "AUTO", TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::guard_table[] =
{
	{ "1/32", GUARD_INTERVAL_1_32 },
	{ "1/16", GUARD_INTERVAL_1_16 },
	{ "1/8",  GUARD_INTERVAL_1_8 },
	{ "1/4",  GUARD_INTERVAL_1_4 },
	{ "AUTO", GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::hierarchy_table[] =
{
	{ "NONE", HIERARCHY_NONE },
	{ "1",    HIERARCHY_1 },
	{ "2",    HIERARCHY_2 },
	{ "4",    HIERARCHY_4 },
	{ "AUTO", HIERARCHY_AUTO },
	{ NULL, 0 }
};

struct StringTable InitialScanLine::inversion_table[] =
{
	{ "INVERSION_OFF",	INVERSION_OFF },
	{ "INVERSION_ON",	INVERSION_ON },
	{ "INVERSION_AUTO",	INVERSION_AUTO },
	{ NULL, 0 }
};

InitialScanLine::InitialScanLine(const Glib::ustring& line) : splitter(line, " ", 20)
{
}

guint InitialScanLine::get_frequency(guint index)
{
	return splitter.get_int_value(index);
}

fe_bandwidth_t InitialScanLine::get_bandwidth(guint index)
{
	return (fe_bandwidth_t)convert_string_to_value(bandwidth_table, splitter.get_value(index));
}

fe_code_rate_t InitialScanLine::get_fec(guint index)
{
	return (fe_code_rate_t)convert_string_to_value(fec_table, splitter.get_value(index));
}

fe_modulation_t InitialScanLine::get_modulation(guint index)
{
	return (fe_modulation_t)convert_string_to_value(modulation_table, splitter.get_value(index));
}

fe_transmit_mode_t InitialScanLine::get_transmit_mode(guint index)
{
	return (fe_transmit_mode_t)convert_string_to_value(transmit_mode_table, splitter.get_value(index));
}

fe_guard_interval_t	InitialScanLine::get_guard_interval(guint index)
{
	return (fe_guard_interval_t)convert_string_to_value(guard_table, splitter.get_value(index));
}

fe_hierarchy_t InitialScanLine::get_hierarchy(guint index)
{
	return (fe_hierarchy_t)convert_string_to_value(hierarchy_table, splitter.get_value(index));
}

guint InitialScanLine::get_symbol_rate(guint index)
{
	return splitter.get_int_value(index);
}
