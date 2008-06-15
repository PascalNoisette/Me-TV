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
#include "dvb_frontend.h"

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

Scanner::Scanner()
{
	terminated = false;
}

void Scanner::process_terrestrial_line(Frontend& frontend, const Glib::ustring& line, guint wait_timeout)
{
	struct dvb_frontend_parameters frontend_parameters;

	StringSplitter splitter(line, " ", 100);
	
	frontend_parameters.frequency						= splitter.get_int_value(1);
	frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth_t)Frontend::convert_string_to_value(Frontend::get_bandwidth_table(),		splitter.get_value(2));
	frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate_t)Frontend::convert_string_to_value(Frontend::get_fec_table(),				splitter.get_value(3));
	frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate_t)Frontend::convert_string_to_value(Frontend::get_fec_table(),				splitter.get_value(4));
	frontend_parameters.u.ofdm.constellation			= (fe_modulation_t)Frontend::convert_string_to_value(Frontend::get_qam_table(),				splitter.get_value(5));
	frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode_t)Frontend::convert_string_to_value(Frontend::get_modulation_table(),	splitter.get_value(6));
	frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval_t)Frontend::convert_string_to_value(Frontend::get_guard_table(),		splitter.get_value(7));
	frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy_t)Frontend::convert_string_to_value(Frontend::get_hierarchy_table(),		splitter.get_value(8));
	frontend_parameters.inversion						= INVERSION_AUTO;

	Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;
	
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
				signal_service(frontend_parameters, sds.services[i].id, sds.services[i].name);
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
	while (status == Glib::IO_STATUS_NORMAL && !terminated)
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
		
		signal_progress(++count, size);
		iterator++;
	}

	signal_progress(size, size);
	
	g_debug("Scanner finished");
}

void Scanner::terminate()
{
	terminated = true;
}
