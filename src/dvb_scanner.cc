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

#include "me-tv.h"
#include "exception.h"
#include "application.h"
#include "dvb_si.h"
#include "dvb_scanner.h"
#include "dvb_demuxer.h"

using namespace Dvb;

Scanner::Scanner(): terminated(false) { }

void Scanner::tune_to(Frontend & frontend, Transponder const & transponder) {
	if (terminated) { return; }
	g_debug("Tuning to transponder at %d Hz", transponder.frontend_parameters.frequency);
	try {
		SI::SectionParser parser;
		SI::ServiceDescriptionSection sds;
		SI::NetworkInformationSection nis;
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		Demuxer demuxer_sds(demux_path);
		Demuxer demuxer_nis(demux_path);
		frontend.tune_to(transponder, 1500);
		demuxer_sds.set_filter(SDT_PID, SDT_ID);
		demuxer_nis.set_filter(NIT_PID, NIT_ID);
		parser.parse_sds(demuxer_sds, sds);
		parser.parse_nis(demuxer_nis, nis);
		demuxer_sds.stop();
		demuxer_nis.stop();
		for (guint i = 0; i < sds.services.size(); ++i) {
			Glib::ustring service_name = sds.services[i].name;
			if (service_name.empty()) {
				service_name = Glib::ustring::compose("Unknown Service %1-%2",
				   	transponder.frontend_parameters.frequency/1000,
				    sds.services[i].id);
			}
			signal_service(
				transponder.frontend_parameters,
				sds.services[i].id,
				service_name,
				transponder.polarisation,
			    frontend.get_signal_strength());
		}
		g_debug("Got %u transponders from NIT", (guint)nis.transponders.size());
		for (guint i = 0; i < nis.transponders.size(); ++i) {
			Transponder& new_transponder = nis.transponders[i];
			if (!transponders.exists(new_transponder)) {
				g_debug("%d: Adding %d Hz", i + 1, new_transponder.frontend_parameters.frequency);
				transponders.push_back(new_transponder);
			}
			else {
				g_debug("%d: Skipping %d Hz", i + 1, new_transponder.frontend_parameters.frequency);
			}
		}
	}
	catch(Exception const & exception) {
		g_debug("Failed to tune to transponder at %d Hz", transponder.frontend_parameters.frequency);
	}
}

void Scanner::atsc_tune_to(Frontend & frontend, Transponder const & transponder) {
	if (terminated) { return; }
	g_debug("Tuning to transponder at %d Hz", transponder.frontend_parameters.frequency);
	try {
		SI::SectionParser parser;
		SI::VirtualChannelTable virtual_channel_table;
		Glib::ustring demux_path = frontend.get_adapter().get_demux_path();
		Demuxer demuxer_vct(demux_path);
		frontend.tune_to(transponder,  1500);
		demuxer_vct.set_filter(PSIP_PID, TVCT_ID, 0xFE);
		parser.parse_psip_vct(demuxer_vct, virtual_channel_table);
		demuxer_vct.stop();
		for (guint i = 0; i < virtual_channel_table.channels.size(); ++i) {
			SI::VirtualChannel* vc = &virtual_channel_table.channels[i];
			if (vc->channel_TSID == virtual_channel_table.transport_stream_id && vc->service_type == 0x02)
				signal_service(
					transponder.frontend_parameters,
					vc->program_number,
					Glib::ustring::compose("%1-%2 %3", vc->major_channel_number, vc->minor_channel_number, vc->short_name),
					transponder.polarisation,
				    frontend.get_signal_strength());
		}
	}
	catch(Exception const & exception) {
		g_debug("Failed to tune to transponder at %d Hz", transponder.frontend_parameters.frequency);
	}
}

void Scanner::start(Frontend & frontend, TransponderList & t) {
	transponders = t;
	gboolean is_atsc = frontend.get_frontend_type() == FE_ATSC;
	guint transponder_count = 0;
	signal_progress(0, transponders.size());
	for (TransponderList::const_iterator i = transponders.begin(); i != transponders.end() && !terminated; ++i) {
		if (is_atsc) { atsc_tune_to(frontend, *i); }
		else { tune_to(frontend, *i); }
		signal_progress(++transponder_count, transponders.size());
	}
	g_debug("Scanner loop exited");
	signal_complete();
	g_debug("Scanner finished");
}

void Scanner::terminate() {
	g_debug("Scanner marked for termination");
	terminated = true;
}
