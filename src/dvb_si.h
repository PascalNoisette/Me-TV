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

#ifndef __DVB_SI_H__
#define __DVB_SI_H__

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <linux/dvb/frontend.h>
#include "dvb_demuxer.h"
#include "dvb_transponder.h"
#include "me-tv.h"
#include "me-tv-i18n.h"

constexpr auto DVB_SECTION_BUFFER_SIZE = 16*1024;
constexpr auto TS_PACKET_SIZE = 188;
constexpr auto PACKET_BUFFER_SIZE = 50;

constexpr auto PAT_PID = 0x00;
constexpr auto NIT_PID = 0x10;
constexpr auto SDT_PID = 0x11;
constexpr auto EIT_PID = 0x12;
constexpr auto PSIP_PID = 0x1FFB;

constexpr auto PAT_ID = 0x00;
constexpr auto PMT_ID = 0x02;
constexpr auto NIT_ID = 0x40;
constexpr auto SDT_ID = 0x42;
constexpr auto EIT_ID = 0x4E;
constexpr auto MGT_ID = 0xC7;
constexpr auto TVCT_ID = 0xC8;
constexpr auto CVCT_ID = 0xC9;
constexpr auto PSIP_EIT_ID = 0xCB;
constexpr auto STT_ID = 0xCD;

namespace Dvb {

	namespace SI {

		class EventText {
		public:
			Glib::ustring language;
			Glib::ustring title;
			Glib::ustring subtitle;
			Glib::ustring description;
		};

		class EventTextMap : public std::map<Glib::ustring, EventText> {
		public:
			gboolean contains(const Glib::ustring& language);
		};

		class Event {
		public:
			Event();
			guint event_id;
			guint version_number;
			guint start_time;
			gulong duration;
			EventTextMap texts;
		};

		typedef std::list<Event> EventList;

		class EventInformationSection {
		public:
			u_int table_id;
			u_int section_syntax_indicator;
			u_int service_id;
			u_int version_number;
			u_int current_next_indicator;
			u_int section_number;
			u_int last_section_number;
			u_int transport_stream_id;
			u_int original_network_id;
			u_int segment_last_section_number;
			u_int last_table_id;
			unsigned long crc;
			std::vector<Event> events;
		};

		class Service {
		public:
			guint id;
			guint type;
			gboolean eit_schedule_flag;
			Glib::ustring provider_name;
			Glib::ustring name;
		};

		class ServiceDescriptionSection {
		public:
			guint transport_stream_id;
			gboolean epg_events_available;
			std::vector<Service> services;
		};

		class NetworkInformationSection {
		public:
			std::vector<Dvb::Transponder> transponders;
		};

		class SystemTimeTable {
		public:
			gulong system_time;
			guint GPS_UTC_offset;
			guint daylight_savings;
		};

		class MasterGuideTable {
		public:
			guint type;
			guint pid;
		};

		typedef std::vector<MasterGuideTable> MasterGuideTableArray;

		class VirtualChannel {
		public:
			Glib::ustring short_name;
			guint major_channel_number;
			guint minor_channel_number;
			guint channel_TSID;
			guint program_number;
			guint service_type;
			guint source_id;
		};

		class VirtualChannelTable {
		public:
			guint transport_stream_id;
			std::vector<VirtualChannel> channels;
		};

		class SectionParser {
		private:
			guchar buffer[DVB_SECTION_BUFFER_SIZE];
			Glib::ustring text_encoding;
			guint get_bits(guchar const * buffer, guint bitpos, gsize bitcount);
			Glib::ustring convert_iso6937(guchar const * buffer, gsize length);
			gsize decode_event_descriptor (guchar const * buffer, Event & event);
			gsize read_section(Demuxer & demuxer);
			fe_code_rate_t parse_fec_inner(guint bitmask);

		public:
			SectionParser();
			gsize get_text(Glib::ustring & s, guchar const * buffer);
			guchar const * get_buffer() const { return buffer; };
			void parse_eis (Demuxer & demuxer, EventInformationSection & section);
			void parse_psip_eis (Demuxer & demuxer, EventInformationSection & section);
			void parse_psip_mgt(Demuxer & demuxer, MasterGuideTableArray & tables);
			void parse_psip_vct(Demuxer & demuxer, VirtualChannelTable & section);
			void parse_psip_stt(Demuxer & demuxer, SystemTimeTable & table);
			void parse_sds (Demuxer & demuxer, ServiceDescriptionSection & section);
			void parse_nis (Demuxer & demuxer, NetworkInformationSection & section);
		};

	}

}

#endif
