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

#include "me-tv.h"
#include "exception.h"
#include "application.h"
#include "dvb_si.h"
#include "dvb_scanner.h"
#include "dvb_demuxer.h"
#include "dvb_frontend.h"
#include "string_splitter.h"
#include "initial_scan_line.h"

using namespace Dvb;

Scanner::Scanner(guint timeout) : wait_timeout(timeout)
{
	terminated = false;
}

void Scanner::tune_to(Frontend& frontend, const Transponder& transponder)
{
	static guint count = 0;
	
	if (terminated)
	{
		return;
	}

	signal_progress(count++, transponders.size());
	
	try
	{
		SI::SectionParser parser;
		SI::ServiceDescriptionSection sds;
		SI::NetworkInformationSection nis;
		
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		Demuxer demuxer_sds(demux_path);
		Demuxer demuxer_nis(demux_path);
		
		frontend.tune_to(transponder);
		
		demuxer_sds.set_filter(SDT_PID, SDT_ID);
		demuxer_nis.set_filter(NIT_PID, NIT_ID);
		parser.parse_sds(demuxer_sds, sds);
		parser.parse_nis(demuxer_nis, nis);
	
		demuxer_sds.stop();
		demuxer_nis.stop();
		
		guint number_of_services = sds.services.size();
		if (number_of_services > 0)
		{
			for (guint i = 0; i < number_of_services; i++)
			{
				signal_service(transponder.frontend_parameters, sds.services[i].id, sds.services[i].name, transponder.polarisation);
			}
		}
		
		guint number_of_transponders = nis.transponders.size();
		if (number_of_transponders > 0)
		{
			for(guint i = 0; i < number_of_transponders; i++ )
			{
				transponders.add(nis.transponders[i]);
			}
		}
	}
	catch(const Exception& exception)
	{
		g_debug("Failed to tune to transponder at '%d'", transponder.frontend_parameters.frequency);
	}
}

void Scanner::process_terrestrial_line(Frontend& frontend, const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;

	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency						= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion						= INVERSION_AUTO;
	frontend_parameters.u.ofdm.bandwidth				= initial_scan_line.get_bandwidth(2);
	frontend_parameters.u.ofdm.code_rate_HP				= initial_scan_line.get_fec(3);
	frontend_parameters.u.ofdm.code_rate_LP				= initial_scan_line.get_fec(4);
	frontend_parameters.u.ofdm.constellation			= initial_scan_line.get_modulation(5);
	frontend_parameters.u.ofdm.transmission_mode		= initial_scan_line.get_transmit_mode(6);
	frontend_parameters.u.ofdm.guard_interval			= initial_scan_line.get_guard_interval(7);
	frontend_parameters.u.ofdm.hierarchy_information	= initial_scan_line.get_hierarchy(8);

	Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;

	transponders.add(transponder);
}

void Scanner::process_atsc_line(Frontend& frontend, const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;

	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.u.vsb.modulation	= initial_scan_line.get_modulation(2);
	frontend_parameters.inversion			= INVERSION_AUTO;

	Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;
	
	transponders.add(transponder);
}

void Scanner::process_satellite_line(Frontend& frontend, const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;
	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion			= INVERSION_AUTO;
	
	frontend_parameters.u.qpsk.symbol_rate	= initial_scan_line.get_symbol_rate(3);
	frontend_parameters.u.qpsk.fec_inner	= initial_scan_line.get_fec(4);

	Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;
	transponder.polarisation		= initial_scan_line.get_polarisation(2);
	
	g_debug("Frequency %d, Symbol rate %d, FEC %d, polarisation %d", frontend_parameters.frequency, frontend_parameters.u.qpsk.symbol_rate, frontend_parameters.u.qpsk.fec_inner, transponder.polarisation);
	
	transponders.add(transponder);
}

void Scanner::process_cable_line(Frontend& frontend, const Glib::ustring& line)
{
	struct dvb_frontend_parameters frontend_parameters;
	InitialScanLine initial_scan_line(line);
	
	frontend_parameters.frequency			= initial_scan_line.get_frequency(1);
	frontend_parameters.inversion			= INVERSION_AUTO;
	
	frontend_parameters.u.qam.symbol_rate	= initial_scan_line.get_symbol_rate(2);
	frontend_parameters.u.qam.fec_inner		= initial_scan_line.get_fec(3);
	frontend_parameters.u.qam.modulation	= initial_scan_line.get_modulation(4);

	Transponder transponder;
	transponder.frontend_parameters = frontend_parameters;
	
	transponders.add(transponder);
}

void Scanner::start(Frontend& frontend, const Glib::ustring& region_file_path)
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
	
	for (StringList::iterator iterator = lines.begin(); iterator != lines.end() && !terminated; iterator++)
	{
		Glib::ustring process_line = *iterator;

		if (!process_line.empty())
		{
			g_debug("Processing line: '%s'", process_line.c_str());

			if (Glib::str_has_prefix(process_line, "T "))
			{
				process_terrestrial_line(frontend, process_line);
			}
			else if (Glib::str_has_prefix(process_line, "C "))
			{
				process_cable_line(frontend, process_line);
			}
			else if (Glib::str_has_prefix(process_line, "S "))
			{
				process_satellite_line(frontend, process_line);
			}
			else if (Glib::str_has_prefix(process_line, "A "))
			{
				process_atsc_line(frontend, process_line);
			}
			else
			{
				throw Exception(_("Me TV cannot process a line in the initial tuning file"));
			}
		}
	}

	for (TransponderList::const_iterator i = transponders.begin(); i != transponders.end() && !terminated; i++)
	{
		tune_to(frontend, *i);
	}
	
	g_debug("Scanner loop exited");

	signal_complete();
	
	g_debug("Scanner finished");
}

void Scanner::terminate()
{
	g_debug("Scanner marked for termination");
	terminated = true;
}
