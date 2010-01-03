/*
 * Copyright (C) 2010 Michael Lamothe
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

Scanner::Scanner()
{
	terminated = false;
}

void Scanner::tune_to(Frontend& frontend, const Transponder& transponder)
{
	if (terminated)
	{
		return;
	}
	
	g_debug("Tuning to transponder at %d Hz", transponder.frontend_parameters.frequency);
	
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
		
		for (guint i = 0; i < sds.services.size(); i++)
		{
			signal_service(
				transponder.frontend_parameters,
				sds.services[i].id,
				sds.services[i].name,
				transponder.polarisation);
		}

		g_debug("Got %u transponders from NIT", (guint)nis.transponders.size());
		for (guint i = 0; i < nis.transponders.size(); i++)
		{
			Transponder& new_transponder = nis.transponders[i];
			if (!transponders.exists(new_transponder))
			{
				g_debug("%d: Adding %d Hz", i + 1, new_transponder.frontend_parameters.frequency);
				transponders.push_back(new_transponder);
			}
			else
			{
				g_debug("%d: Skipping %d Hz", i + 1, new_transponder.frontend_parameters.frequency);
			}
		}
	}
	catch(const Exception& exception)
	{
		g_debug("Failed to tune to transponder at %d Hz", transponder.frontend_parameters.frequency);
	}
}

void Scanner::atsc_tune_to(Frontend& frontend, const Transponder& transponder)
{
	if (terminated)
	{
		return;
	}
	
	g_debug("Tuning to transponder at %d Hz", transponder.frontend_parameters.frequency);
	
	try
	{
		SI::SectionParser parser;
		SI::VirtualChannelTable virtual_channel_table;
		
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		Demuxer demuxer_vct(demux_path);
		
		frontend.tune_to(transponder);
		
		demuxer_vct.set_filter(PSIP_PID, TVCT_ID, 0xFE);
		parser.parse_psip_vct(demuxer_vct, virtual_channel_table);
		demuxer_vct.stop();
		
		for (guint i = 0; i < virtual_channel_table.channels.size(); i++)
		{
			SI::VirtualChannel* vc = &virtual_channel_table.channels[i];
			if (vc->channel_TSID == virtual_channel_table.transport_stream_id && vc->service_type == 0x02)
				signal_service(
					transponder.frontend_parameters,
					vc->program_number,
					Glib::ustring::compose("%1-%2 %3", vc->major_channel_number, vc->minor_channel_number, vc->short_name),
					transponder.polarisation);
		}
	}
	catch(const Exception& exception)
	{
		g_debug("Failed to tune to transponder at %d Hz", transponder.frontend_parameters.frequency);
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
		// Remove comments
		Glib::ustring::size_type index = line.find("#");
		if (index != Glib::ustring::npos)
		{
			line = line.substr(0, index);
		}
		
		// Remove trailing whitespace
		index = line.find_last_not_of(" \t\r\n");
		if (index == Glib::ustring::npos)
		{
			line.clear();
		}
		else
		{
			line = line.substr(0, index + 1);
		}

		if (!line.empty()) // Ignore empty lines or comments
		{
			lines.push_back(line);
		}
		
		status = initial_tuning_file->read_line(line);
	}
	
	guint size = lines.size();
	
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

	gboolean is_atsc = frontend.get_frontend_type() == FE_ATSC;
	guint transponder_count = 0;
	signal_progress(0, transponders.size());
	for (TransponderList::const_iterator i = transponders.begin(); i != transponders.end() && !terminated; i++)
	{
		if (is_atsc) atsc_tune_to(frontend, *i);
		else tune_to(frontend, *i);
		signal_progress(++transponder_count, transponders.size());
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
