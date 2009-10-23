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

#define CRC_BYTE_SIZE			4
#define DVB_SECTION_BUFFER_SIZE	16*1024

#define PAT_PID		0x00
#define NIT_PID		0x10
#define SDT_PID		0x11
#define EIT_PID		0x12
#define PSIP_PID	0x1FFB

#define PAT_ID		0x00
#define PMT_ID		0x02
#define NIT_ID			0x40
#define SDT_ID		0x42
#define EIT_ID		0x4E
#define MGT_ID		0xC7
#define TVCT_ID		0xC8
#define PSIP_EIT_ID	0xCB

#define STREAM_TYPE_MPEG1		0x01
#define STREAM_TYPE_MPEG2		0x02
#define STREAM_TYPE_MPEG4		0x10
#define STREAM_TYPE_H264		0x1B

namespace Dvb
{
	namespace SI
	{
		class EventText
		{
		public:
			gboolean is_extended;
			Glib::ustring language;
			Glib::ustring title;
			Glib::ustring description;
		};
		
		class EventTextMap : public std::map<Glib::ustring, EventText>
		{
		public:
			gboolean contains(const Glib::ustring& language);
		};
		
		class Event
		{
		public:
			Event();

			guint	event_id;
			guint	start_time;
			gulong	duration;
			guint	running_status;
			guint	free_CA_mode;
			
			EventTextMap texts;
		};

		typedef std::list<Event> EventList;
		
		class EventInformationSection
		{
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

		class Service
		{
		public:
			guint id;
			guint type;
			Glib::ustring provider_name;
			Glib::ustring name;
		};

		class ServiceDescriptionSection
		{
		public:
			guint transport_stream_id;
			std::vector<Service> services;
		};

		class NetworkInformationSection
		{
		public:
			std::vector<Dvb::Transponder> transponders;
		};
		
		class ProgramAssociation
		{
		public:
			guint program_number;
			guint program_map_pid;
		};

		class ProgramAssociationSection
		{
		public:
			std::vector<ProgramAssociation> program_associations;
		};

		class MasterGuideTable
		{
		public:
			guint type;
			guint pid;
		};

		typedef std::vector<MasterGuideTable> MasterGuideTableArray;

		class VirtualChannelTable
		{
		public:
			guint program_number;
			guint source_id;
		};

		typedef std::vector<VirtualChannelTable> VirtualChannelTableArray;

		class VideoStream
		{
		public:
			VideoStream()
			{
				pid		= 0;
				type	= 2; // Default to MPEG 2
			}
			
			guint pid;
			guint type;
		};

		class AudioStream
		{
		public:
			AudioStream()
			{
				pid		= 0;
				is_ac3	= false;
				language = _("Unknown language");
			}
			
			guint			pid;
			Glib::ustring	language;
			gboolean		is_ac3;
		};

		class TeletextLanguageDescriptor
		{
		public:
			TeletextLanguageDescriptor()
			{
				language		= _("Unknown language");
				type			= 0;
				magazine_number	= 0;
				page_number		= 0;
			}

			Glib::ustring	language;
			guint			type;
			guint			magazine_number;
			guint			page_number;
		};

		class TeletextStream
		{
		public:
			TeletextStream()
			{
				pid				= 0;
			}
			
			guint	pid;
			std::vector<TeletextLanguageDescriptor> languages;
		};

		class SubtitleStream
		{
		public:
			SubtitleStream()
			{
				pid					= 0;
				subtitling_type		= 0;
				ancillary_page_id	= 0;
				composition_page_id	= 0;
				language			= _("Unknown language");
			}
			
			guint pid;
			guint subtitling_type;
			guint ancillary_page_id;
			guint composition_page_id;
			Glib::ustring language;
		};

		class ProgramMapSection
		{
		public:
			std::vector<VideoStream> video_streams;
			std::vector<AudioStream> audio_streams;
			std::vector<SubtitleStream> subtitle_streams;
			std::vector<TeletextStream> teletext_streams;
		};

		class SectionParser
		{
		private:
			guchar buffer[DVB_SECTION_BUFFER_SIZE];
			Glib::ustring text_encoding;
				
			Glib::ustring get_lang_desc(const guchar* buffer);
			gboolean find_descriptor(uint8_t tag, const unsigned char *buf, int descriptors_loop_len, const unsigned char **desc, int *desc_len);
			guint get_bits(const guchar* buffer, guint bitpos, gsize bitcount);
			Glib::ustring convert_iso6937(const guchar* buffer, gsize length);
			gsize decode_event_descriptor (const guchar* buffer, Event& event);
			gsize read_section(Demuxer& demuxer);
		
			fe_code_rate_t parse_fec_inner(guint bitmask);
		public:
			SectionParser();
			
			gsize get_text(Glib::ustring& s, const guchar* buffer);
			const guchar* get_buffer() const { return buffer; };

			void parse_pas (Demuxer& demuxer, ProgramAssociationSection& section);
			void parse_pms (Demuxer& demuxer, ProgramMapSection& section);
			void parse_eis (Demuxer& demuxer, EventInformationSection& section);
			void parse_psip_eis (Demuxer& demuxer, EventInformationSection& section);
			void parse_psip_mgt(Demuxer& demuxer, MasterGuideTableArray& tables);
			void parse_psip_tvct(Demuxer& demuxer, VirtualChannelTableArray& tables);
			void parse_sds (Demuxer& demuxer, ServiceDescriptionSection& section);
			void parse_nis (Demuxer& demuxer, NetworkInformationSection& section);
		};
	}
}

#endif
