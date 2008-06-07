/*
 * Copyright (C) 2008 Michael Lamothe
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

#include "exception.h"
#include "application.h"
#include "dvb_si.h"
#include "dvb_scanner.h"
#include "dvb_demuxer.h"

using namespace Dvb;

class StringSplitter
{
private:
	gchar** parts;
	gsize	count;
public:
	StringSplitter(const Glib::ustring& text, const char* deliminator, gsize max_length)
	{
		parts = g_strsplit(text.c_str(), deliminator, max_length);
		count = 0;
		gchar** iterator = parts;
		while (*iterator++ != NULL && count < max_length)
		{
			count++;
		}
	}
		
	~StringSplitter()
	{
		g_strfreev(parts);
	}

	const gchar* get_value(guint index)
	{
		if (index >= count)
		{
			throw Exception("Index out of bounds");
		}
		return parts[index];
	}

	gint get_int_value(guint index)
	{
		return atoi(get_value(index));
	}
	
	gsize get_count() const { return count; }
};

struct StringTable bandwidth_table[] =
{
	{ "8MHz", BANDWIDTH_8_MHZ },
	{ "7MHz", BANDWIDTH_7_MHZ },
	{ "6MHz", BANDWIDTH_6_MHZ },
	{ "AUTO", BANDWIDTH_AUTO },
	{ NULL, 0 }
};

struct StringTable fec_table[] =
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

struct StringTable qam_table[] =
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

struct StringTable modulation_table[] =
{
	{ "2k",   TRANSMISSION_MODE_2K },
	{ "8k",   TRANSMISSION_MODE_8K },
	{ "AUTO", TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

struct StringTable guard_table[] =
{
	{ "1/32", GUARD_INTERVAL_1_32 },
	{ "1/16", GUARD_INTERVAL_1_16 },
	{ "1/8",  GUARD_INTERVAL_1_8 },
	{ "1/4",  GUARD_INTERVAL_1_4 },
	{ "AUTO", GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

struct StringTable hierarchy_table[] =
{
	{ "NONE", HIERARCHY_NONE },
	{ "1",    HIERARCHY_1 },
	{ "2",    HIERARCHY_2 },
	{ "4",    HIERARCHY_4 },
	{ "AUTO", HIERARCHY_AUTO },
	{ NULL, 0 }
};

Scanner::Scanner()
{
}

guint Scanner::convert_string_to_value(const StringTable* table, const gchar* text)
{
	gboolean found = false;
	const StringTable*	current = table;

	while (current->text != NULL && !found)
	{
		if (g_str_equal(text,current->text))
		{
			found = true;
		}
		else
		{
			current++;
		}
	}
	
	if (!found)
	{
		throw Exception(Glib::ustring::format("Failed to find a value for ", text));
	}
	
	return (guint)current->value;
}

void Scanner::process_terrestrial_line(Frontend& frontend, const Glib::ustring& line, guint wait_timeout)
{
	StringSplitter splitter(line, " ", 100);
	Transponder transponder;
	transponder.frontend_parameters.frequency						= splitter.get_int_value(1);
	transponder.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth_t)convert_string_to_value(bandwidth_table, splitter.get_value(2));
	transponder.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate_t)convert_string_to_value(fec_table, splitter.get_value(3));
	transponder.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate_t)convert_string_to_value(fec_table, splitter.get_value(4));
	transponder.frontend_parameters.u.ofdm.constellation			= (fe_modulation_t)convert_string_to_value(qam_table, splitter.get_value(5));
	transponder.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode_t)convert_string_to_value(modulation_table, splitter.get_value(6));
	transponder.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval_t)convert_string_to_value(guard_table, splitter.get_value(7));
	transponder.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy_t)convert_string_to_value(hierarchy_table, splitter.get_value(8));
	transponder.frontend_parameters.inversion						= INVERSION_AUTO;
	
	try
	{
		SI::SectionParser parser;
		SI::ServiceDescriptionSection sds;
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		Demuxer demuxer_sds(demux_path);

		frontend.tune_to(transponder, wait_timeout);		
		demuxer_sds.set_filter(SDT_PID, SDT_ID);
		parser.parse_sds(demuxer_sds, sds);
		
		guint number_of_services = sds.services.size();
		if (number_of_services > 0)
		{
			for (guint i = 0; i < number_of_services; i++)
			{
				SI::Service dvb_service = sds.services[i];
				Service service(transponder);
				service.id = dvb_service.id;
				service.name = dvb_service.name;
				transponder.add_service(service);
				signal_service(service);
			}
		}
	}
	catch(const Exception& exception)
	{
		g_debug("Failed to tune to transponder at '%d'", transponder.frontend_parameters.frequency);
	}
}

void Scanner::start(Frontend& frontend, const Glib::ustring& region_file_path, guint wait_timeout)
{
	Glib::RefPtr<Glib::IOChannel> initial_tuning_file = Glib::IOChannel::create_from_file(region_file_path, "r");
	
	std::list<Glib::ustring> lines;
	Glib::ustring line;
	Glib::IOStatus status = initial_tuning_file->read_line(line);
	while (status == Glib::IO_STATUS_NORMAL)
	{
		if (Glib::str_has_prefix(line, "#")|| line.empty())
		{
			// Ignore empty lines or comments
		}
		else
		{
			if (Glib::str_has_suffix(line, "\n"))
			{
				line = line.substr(0, line.length()-1);
			}
			lines.push_back(line);
		}
		
		status = initial_tuning_file->read_line(line);
	}
	
	guint size = lines.size();
	guint count = 0;
	
	std::list<Glib::ustring>::iterator iterator = lines.begin();
	while (iterator != lines.end())
	{
		Glib::ustring line = *iterator;

		g_debug("Processing line: '%s'", line.c_str());

		if (Glib::str_has_prefix(line, "T "))
		{
			process_terrestrial_line(frontend, line, wait_timeout);
		}
		else
		{
			throw Exception("Me TV cannot process a line in the initial tuning file");
		}
		
		signal_progress(++count/(gdouble)size);
		iterator++;
	}
}
