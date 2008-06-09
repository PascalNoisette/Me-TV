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
 
#include "dvb_si.h"
#include "application.h"
#include "exception.h"
#include "atsc_text.h"

#define SHORT_EVENT			0x4D
#define EXTENDED_EVENT		0x4E

#define GPS_EPOCH			315964800

using namespace Dvb;
using namespace Dvb::SI;

void dump(guchar* buffer, gsize length)
{
	for (guint i = 0; i < length; i++)
	{
		guchar ch = buffer[i];
		if (g_ascii_isalnum(ch))
		{
			g_debug ("buffer[%d] = 0x%02X; // (%c)", i, ch, ch);
		}
		else
		{
			g_debug ("buffer[%d] = 0x%02X;", i, ch);
		}
	}
}

static guint32 crc_table[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static guint32 crc32(const char *data, int len)
{
    register int i;
    u_long crc = 0xffffffff;

    for (i=0; i<len; i++)
	{
        crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
	}
	
    return crc;
}

Dvb::SI::Event::Event()
{
	event_id = 0;
	start_time = 0;
	duration = 0;
	running_status = 0;
	free_CA_mode = 0;
}

SectionParser::SectionParser()
{
}

gsize SectionParser::read_section(Demuxer& demuxer)
{
	gsize bytes_read = demuxer.read(buffer, 3);
	
	if (bytes_read != 3)
	{
		throw Exception(_("Failed to read header"));
	}
		
	gsize remaining_section_length = get_bits (buffer, 12, 12);
	gsize section_length = remaining_section_length + 3;
	bytes_read = demuxer.read(buffer + 3, remaining_section_length);
	
	if (bytes_read != remaining_section_length)
	{
		throw Exception(_("Failed to read section"));
	}
	
	guint32 crc = crc32((const char *)buffer, section_length);
	
	if (crc != 0)
	{
		throw Exception(_("CRC32 check failed"));
	}
	
	return section_length;
}

void SectionParser::parse_pas(Demuxer& demuxer, ProgramAssociationSection& section)
{
	gsize section_length = read_section(demuxer);
	guint offset = 8;
	
	while (offset < (section_length - 4))
	{
		ProgramAssociation program_association;
		program_association.program_number = get_bits(&buffer[offset], 0, 16);
		offset += 2;
		program_association.program_map_pid = get_bits(&buffer[offset], 3, 13);
		offset += 2;	
		section.program_associations.push_back(program_association);
	}
}

void SectionParser::parse_sds (Demuxer& demuxer, ServiceDescriptionSection& section)
{
	gsize section_length = read_section(demuxer);
	
	guint offset = 3;
	section.transport_stream_id = get_bits(buffer + offset, 0, 16);
	offset += 8;
	
	while (offset < section_length - 4)
	{
		Service service;
		service.id = get_bits(buffer + offset, 0, 16);
		offset += 3;
		guint descriptors_loop_length = get_bits(buffer + offset, 4, 12);
		offset += 2;
		guint descriptors_end_offset = offset + descriptors_loop_length;
		while (offset < descriptors_end_offset)
		{
			guint descriptor_tag = buffer[offset];
			guint descriptor_length = buffer[offset + 1];
			if (descriptor_tag == 0x48)
			{
				service.type = buffer[offset + 2];
				guint service_provider_name_length = get_text(service.provider_name, buffer + offset + 3);
				get_text(service.name, buffer + offset + 3 + service_provider_name_length);
			}

			offset += descriptor_length + 2;
		}
		
		section.services.push_back(service);
	}
}

gsize get_atsc_text(Glib::ustring& string, const guchar* buffer)
{
	size_t text_position = 0;
	gsize offset = 0;
	
	gsize number_strings = buffer[offset++];
	for (guint i = 0; i < number_strings; i++)
	{
		offset += 3;
		gsize number_segments = buffer[offset++];
		for (guint j = 0; j < number_segments; j++)
		{
			struct atsc_text_string_segment* segment = (struct atsc_text_string_segment*)&buffer[offset];
			
			uint8_t* text = NULL;
			size_t text_length = 0;
			
			atsc_text_segment_decode(segment, &text, &text_length, &text_position);

			string.append((const gchar*)text, text_position);
		}
	}
	
	return offset;
}

gboolean SectionParser::find_descriptor(uint8_t tag, const unsigned char *buf, int descriptors_loop_len, 
	const unsigned char **desc, int *desc_len)
{
	while (descriptors_loop_len > 0)
	{
		unsigned char descriptor_tag = buf[0];
		unsigned char descriptor_len = buf[1] + 2;

		if (!descriptor_len)
		{
			break;
		}

		if (tag == descriptor_tag)
		{
			if (desc)
				*desc = buf;

			if (desc_len)
				*desc_len = descriptor_len;
			return true;
		}

		buf += descriptor_len;
		descriptors_loop_len -= descriptor_len;
	}
	
	return false;
}

Glib::ustring SectionParser::get_lang_desc(const guchar* buffer)
{
	char c[4];
	Glib::ustring s;
	memset( mempcpy( c, buffer+2, 3 ), 0, 1 );
	s = c;
	return s;
}

void SectionParser::parse_pms(Demuxer& demuxer, ProgramMapSection& section)
{	
	const guchar* desc = NULL;
	gsize section_length = read_section(demuxer);
	guint offset = 8;
	gsize program_info_length = ((buffer[10] & 0x0f) << 8) | buffer[11];

	offset += program_info_length + 4;
	
	while ((section_length - offset) >= 5)
	{
		guint pid_type = buffer[offset];
		guint elementary_pid = ((buffer[offset+1] & 0x1f) << 8) | buffer[offset+2];
	    gsize descriptor_length = ((buffer[offset+3] & 0x0f) << 8) | buffer[offset+4];
		
		switch (pid_type)
		{
		case STREAM_TYPE_MPEG1:
		case STREAM_TYPE_MPEG2:
		case STREAM_TYPE_MPEG4:
		case STREAM_TYPE_H264:
			g_debug("Video PID: %d", elementary_pid);
			{
				VideoStream video_stream;
				video_stream.pid = elementary_pid;
				video_stream.type = pid_type;
				section.video_streams.push_back(video_stream);
			}
			break;
		case 0x03:
		case 0x04:
			g_debug("Audio PID: %d", elementary_pid);
			{				
				AudioStream stream;
				stream.pid = elementary_pid;
				stream.is_ac3 = false;

				if (find_descriptor(0x0A, buffer + offset + 5, descriptor_length, &desc, NULL))
				{
					stream.language = get_lang_desc (desc);
				}

				section.audio_streams.push_back(stream);
			}
			break;
		case 0x06:
			{
				int descriptor_loop_length = 0;
				if (find_descriptor(0x56, buffer + offset + 5, descriptor_length, &desc, &descriptor_loop_length))
				{
					g_debug("Teletext PID: %d", elementary_pid);

					TeletextStream stream;
					stream.pid = elementary_pid;

					int descriptor_index = 0;
					while (descriptor_index < descriptor_loop_length - 4)
					{
						TeletextLanguageDescriptor descriptor;
						
						descriptor.language			= get_lang_desc(desc + descriptor_index);
						descriptor.type				= get_bits(desc + descriptor_index + 6, 0, 5);
						descriptor.magazine_number	= get_bits(desc + descriptor_index + 6, 5, 3);
						descriptor.page_number		= get_bits(desc + descriptor_index + 7, 0, 8);
						
						g_debug("TeleText: Language: '%s', Type: %d, Magazine Number: %d, Page Number: %d",
							descriptor.language.c_str(),
							descriptor.type,
							descriptor.magazine_number,
							descriptor.page_number);
						
						stream.languages.push_back(descriptor);
						
						descriptor_index += 5;
					}
					section.teletext_streams.push_back(stream);
				}
				else if (find_descriptor (0x59, buffer + offset + 5, descriptor_length, &desc, NULL))
				{
					g_debug("Subtitle PID: %d", elementary_pid);
					SubtitleStream stream;
					stream.pid = elementary_pid;
					if (descriptor_length > 0)
					{
						stream.subtitling_type = get_bits(desc + 5, 0, 8);
						if (stream.subtitling_type>=0x10 && stream.subtitling_type<=0x23)
						{
							stream.language				= get_lang_desc(desc);
							stream.composition_page_id	= get_bits(desc + 6, 0, 16);
							stream.ancillary_page_id	= get_bits(desc + 8, 0, 16);
							
							g_debug(
								"Subtitle composition_page_id: %d, ancillary_page_id: %d, language: %s",
								stream.composition_page_id, stream.ancillary_page_id, stream.language.c_str() );
						}
					}
					section.subtitle_streams.push_back(stream);
				}
				else if (find_descriptor (0x6A, buffer + offset + 5, descriptor_length, NULL, NULL))
				{
					g_debug("AC3 PID: %d", elementary_pid);
					AudioStream stream;
					stream.pid = elementary_pid;
					stream.is_ac3 = true;

					const guchar* desc = NULL;
					if (find_descriptor(0x0A, buffer + offset + 5, descriptor_length, &desc, NULL))
					{
						stream.language = get_lang_desc (desc);
					}
					section.audio_streams.push_back(stream);
				}
			}
			break;
		case 0x81:
			g_debug("AC3 PID: %d", elementary_pid);
			{
				AudioStream stream;
				stream.pid = elementary_pid;
				stream.is_ac3 = true;
			
				const guchar* desc = NULL;
				if (find_descriptor(0x0A, buffer + offset + 5, descriptor_length, &desc, NULL))
				{
					stream.language = get_lang_desc (desc);
				}
				section.audio_streams.push_back(stream);
			}
			break;
		default:
			break;
		}
		
		offset += descriptor_length + 5;
	}
}

void SectionParser::parse_psip_mgt(Demuxer& demuxer, MasterGuideTable& table)
{
	gsize section_length = read_section(demuxer);
	guint offset = 9;
	guint tables_defined = get_bits(&buffer[offset], 0, 16);
	offset += 2;
	
	for (guint i = 0; i < tables_defined; i++)
	{
		MasterGuideTableTable mgtt;
		mgtt.type = get_bits(&buffer[offset], 0, 16);
		offset += 2;
		mgtt.pid = get_bits(&buffer[offset], 3, 13);
		table.tables.push_back(mgtt);	
		offset += 7;
		guint table_type_descriptors_length = get_bits(&buffer[offset], 4, 12);
		offset += table_type_descriptors_length + 2;
	}
}

void SectionParser::parse_psip_eis(Demuxer& demuxer, EventInformationSection& section)
{
	gsize section_length = read_section(demuxer);
	guint offset = 3;

	section.service_id = get_bits(&buffer[offset], 0, 16);
	
	offset += 6;
	guint num_events_in_section = buffer[offset++];
	
	for (int i = 0; i < num_events_in_section; i++)
	{
		Event event;

		event.event_id		= get_bits (&buffer[offset], 2, 14); offset += 2;
		event.start_time	= get_bits (&buffer[offset], 0, 32); offset += 4;
		event.duration		= get_bits (&buffer[offset], 4, 20); offset += 3;

		event.start_time += GPS_EPOCH;
		event.start_time += timezone;
		
		gsize title_length = buffer[offset++];
		if (title_length > 0)
		{
			get_atsc_text (event.title, &buffer[offset++]);
			offset += title_length;
		}
		
		guint descriptors_length = buffer[offset++];
		offset += descriptors_length;
		
		section.events.push_back(event);
	}
}

void SectionParser::parse_eis(Demuxer& demuxer, EventInformationSection& section)
{
	gsize section_length = read_section(demuxer);
	
	section.table_id =						buffer[0];
	section.section_syntax_indicator =		get_bits (buffer, 8, 1);
	section.service_id =					get_bits (buffer, 24, 16);
	section.version_number =				get_bits (buffer, 42, 5);
	section.current_next_indicator =		get_bits (buffer, 47, 1);
	section.section_number =				get_bits (buffer, 48, 8);
	section.last_section_number =			get_bits (buffer, 56, 8);
	section.transport_stream_id =			get_bits (buffer, 64, 16);
	section.original_network_id =			get_bits (buffer, 80, 16);
	section.segment_last_section_number =	get_bits (buffer, 96, 8);
	section.last_table_id =					get_bits (buffer, 104, 8);

	unsigned int offset = 14;
	
	while (offset < (section_length - CRC_BYTE_SIZE))
	{
		Event event;

		gulong	start_time_MJD;  // 16
		gulong	start_time_UTC;  // 24
		gulong	duration;

		event.event_id			= get_bits (&buffer[offset], 0, 16);
		start_time_MJD			= get_bits (&buffer[offset], 16, 16);
		start_time_UTC			= get_bits (&buffer[offset], 32, 24);
		duration				= get_bits (&buffer[offset], 56, 24);
		event.running_status	= get_bits (&buffer[offset], 80, 3);
		event.free_CA_mode		= get_bits (&buffer[offset], 83, 1);

		unsigned int descriptors_loop_length  = get_bits (&buffer[offset], 84, 12);
		offset += 12;
		unsigned int end_descriptor_offset = descriptors_loop_length + offset;

		guint event_dur_hour = ((duration >> 20)&0xf)*10+((duration >> 16)&0xf);
		guint event_dur_min =  ((duration >> 12)&0xf)*10+((duration >>  8)&0xf);
		guint event_dur_sec =  ((duration >>  4)&0xf)*10+((duration      )&0xf);

		event.duration = (event_dur_hour*60 + event_dur_min) * 60;
		
		guint event_start_day = 0;
		guint event_start_month = 0;
		guint event_start_year = 0;
		
		if (start_time_MJD > 0 && event.event_id != 0)
		{
			long year =  (long) ((start_time_MJD  - 15078.2) / 365.25);
			long month =  (long) ((start_time_MJD - 14956.1 - (long)(year * 365.25) ) / 30.6001);
			event_start_day =  (long) (start_time_MJD - 14956 - (long)(year * 365.25) - (long)(month * 30.6001));

			long k = (month == 14 || month == 15) ? 1 : 0;

			event_start_year = year + k + 1900;
			event_start_month = month - 1 - k * 12;

			guint event_start_hour = ((start_time_UTC >> 20)&0xf)*10+((start_time_UTC >> 16)&0xf);
			guint event_start_min =  ((start_time_UTC >> 12)&0xf)*10+((start_time_UTC >>  8)&0xf);
			guint event_start_sec =  ((start_time_UTC >>  4)&0xf)*10+((start_time_UTC      )&0xf);

			struct tm t;

			memset(&t, 0, sizeof(struct tm));

			t.tm_min	= event_start_min;
			t.tm_hour	= event_start_hour;
			t.tm_mday	= event_start_day;
			t.tm_mon	= event_start_month - 1;
			t.tm_year	= event_start_year - 1900;
			
			event.start_time = mktime(&t);
		
			while (offset < end_descriptor_offset)
			{
				offset += decode_event_descriptor(&buffer[offset], event);
			}

			if (offset > end_descriptor_offset)
			{
				throw Exception("ASSERT: offset > end_descriptor_offset");
			}

			section.events.push_back( event );
		}
	}
	section.crc = get_bits (&buffer[offset], 0, 32);
	offset += 4;
	
	if (offset > section_length)
	{
		throw Exception("ASSERT: offset > end_section_offset");
	}
}

gsize SectionParser::decode_event_descriptor (const guchar* event_buffer, Event& event)
{
	if (event_buffer[1] == 0)
	{
		return 2;
	}

	gsize descriptor_length = event_buffer[1] + 2;
	
	guint descriptor_tag = event_buffer[0];	
	switch(descriptor_tag)
	{		
	case EXTENDED_EVENT:
		{
			Glib::ustring language;
			Glib::ustring title;
			Glib::ustring description;

			language = get_lang_desc (event_buffer);
			unsigned int offset = 6;
			guint length_of_items = event_buffer[offset];
			offset++;
			while (length_of_items > offset - 7)
			{
				offset += get_text(description, &event_buffer[offset]);			
				offset += get_text(title, &event_buffer[offset]);
			}
			offset += get_text(description, &event_buffer[offset]);
			
			if (event.description.empty())
			{
				event.description = description;
			}

			if (event.title.empty())
			{
				event.title = title;
			}
		}
		break;

	case SHORT_EVENT:
		{
			Glib::ustring language;
			Glib::ustring title;
			Glib::ustring description;
			
			language = get_lang_desc (event_buffer);
			unsigned int offset = 5;
			offset += get_text(title, &event_buffer[offset]);
			offset += get_text(description, &event_buffer[offset]);
			
			if (event.title.empty())
			{
				event.title = title;
			}
			else if (event.title != title)
			{
				event.title += "-" + title;
			}
			
			if (event.description.empty())
			{
				event.description = description;
			}
			else if (event.description != description)
			{
				event.description += "-" + description;
			}
		}
		break;
	default:
		break;
	}

	return descriptor_length;
}

guint SectionParser::get_bits(const guchar *buffer, guint bitpos, gsize bitcount)
{
	gsize i;
	gsize val = 0;

	for (i = bitpos; i < bitcount + bitpos; i++)
	{
		val = val << 1;
		val = val + ((buffer[i >> 3] & (0x80 >> (i & 7))) ? 1 : 0);
	}
	return val;
}


gsize SectionParser::get_text(Glib::ustring& s, const guchar* text_buffer)
{
	gsize length = text_buffer[0];
	guint text_index = 0;
	gchar text[length];
	guint index = 0;
	const gchar* codeset = "ISO-8859-15";
	
	if (length > 0)
	{
		// Skip over length byte
		index++;

		if (epg_encoding != "auto")
		{
			codeset = epg_encoding.c_str();
		}
		else // Determine codeset from stream
		{			
			if (text_buffer[index] < 0x20)
			{
				switch (text_buffer[index])
				{
				case 0x01: codeset = "ISO-8859-5"; break;
				case 0x02: codeset = "ISO-8859-6"; break;
				case 0x03: codeset = "ISO-8859-7"; break;
				case 0x04: codeset = "ISO-8859-8"; break;
				case 0x05: codeset = "ISO-8859-9"; break;
				case 0x06: codeset = "ISO-8859-10"; break;
				case 0x07: codeset = "ISO-8859-11"; break;
				case 0x08: codeset = "ISO-8859-12"; break;
				case 0x09: codeset = "ISO-8859-13"; break;
				case 0x0A: codeset = "ISO-8859-14"; break;
				case 0x0B: codeset = "ISO-8859-15"; break;
				case 0x14: codeset = "UTF-16BE"; break;

				case 0x10:
					{
						// Skip 0x00
						index++;
						if (text_buffer[index] != 0x00)
						{
							g_warning("Second byte of code table id was not 0");
						}
						
						index++;
						
						switch(text_buffer[index])
						{
						case 0x01: codeset = "ISO-8859-1"; break;
						case 0x02: codeset = "ISO-8859-2"; break;
						case 0x03: codeset = "ISO-8859-3"; break;
						case 0x04: codeset = "ISO-8859-4"; break;
						case 0x05: codeset = "ISO-8859-5"; break;
						case 0x06: codeset = "ISO-8859-6"; break;
						case 0x07: codeset = "ISO-8859-7"; break;
						case 0x08: codeset = "ISO-8859-8"; break;
						case 0x09: codeset = "ISO-8859-9"; break;
						case 0x0A: codeset = "ISO-8859-10"; break;
						case 0x0B: codeset = "ISO-8859-11"; break;
						case 0x0C: codeset = "ISO-8859-12"; break;
						case 0x0D: codeset = "ISO-8859-13"; break;
						case 0x0E: codeset = "ISO-8859-14"; break;
						case 0x0F: codeset = "ISO-8859-15"; break;
						}
					}
				}
				
				index++;
			}
		}
		
		s = "";

		if (epg_encoding == "iso6937")
		{
			s = convert_iso6937(text_buffer + index, length);
		}
		else
		{
			if (index < (length + 1))
			{
				while (index < (length + 1))
				{
					u_char ch = text_buffer[index];

					if (codeset == "UTF-16BE")
					{
						text[text_index++] = ch;
					}
					else
					{
						if (ch == 0x86 || ch == 0x87)
						{
							// Ignore formatting
						}
						else if (ch == 0x8A)
						{
							text[text_index++] = '\n';
						}
						else if (ch >= 0x80 && ch < 0xA0)
						{
							text[text_index++] = '.';
						}
						else
						{
							text[text_index++] = ch;
						}
					}
					
					index++;
				}
				
				gsize bytes_read;
				gsize bytes_written;
				GError* error = NULL;

				gchar* result = g_convert(
					text,
					text_index,
					"UTF-8",
					codeset,
					&bytes_read,
					&bytes_written,
					&error);

				if (error != NULL)
				{
					Glib::ustring message = Glib::ustring::format("Failed to convert to UTF-8: %s", error->message);
					g_debug(message.c_str());
					g_debug("Codeset: %s", codeset);
					g_debug("Length: %d", length);
					for (guint i = 0; i < (length+1); i++)
					{
						gchar ch = text_buffer[i];
						if (!isprint(ch))
						{
							g_debug("text_buffer[%d]\t= 0x%02X", i, text_buffer[i]);
						}
						else
						{
							g_debug("text_buffer[%d]\t= 0x%02X '%c'", i, text_buffer[i], text_buffer[i]);
						}
					}
					throw Exception(message);
				}
				
				if (result == NULL)
				{
					throw Exception(_("Failed to convert to UTF-8"));
				}
				s = result;
				g_free(result);
			}
		}
	}
	
	return length + 1;
}

Glib::ustring SectionParser::convert_iso6937(const guchar* text_buffer, gsize length)
{
	Glib::ustring result;
	gint val;
	guint i;
	
	for (i=0; i<length; i++ )
	{
		if ( text_buffer[i]<0x20 || (text_buffer[i]>=0x80 && text_buffer[i]<=0x9f) )
		{
			continue; // control codes
		}
		
		if ( text_buffer[i]>=0xC0 && text_buffer[i]<=0xCF ) // next char has accent
		{
			if ( i==length-1 ) // Accent char not followed by char
			{
				continue;
			}
			
			val = ( text_buffer[i]<<8 ) + text_buffer[i+1];
			++i;
			
			switch (val)
			{
				case 0xc141: result += gunichar(0x00C0); break; //LATIN CAPITAL LETTER A WITH GRAVE
				case 0xc145: result += gunichar(0x00C8); break; //LATIN CAPITAL LETTER E WITH GRAVE
				case 0xc149: result += gunichar(0x00CC); break; //LATIN CAPITAL LETTER I WITH GRAVE
				case 0xc14f: result += gunichar(0x00D2); break; //LATIN CAPITAL LETTER O WITH GRAVE
				case 0xc155: result += gunichar(0x00D9); break; //LATIN CAPITAL LETTER U WITH GRAVE
				case 0xc161: result += gunichar(0x00E0); break; //LATIN SMALL LETTER A WITH GRAVE
				case 0xc165: result += gunichar(0x00E8); break; //LATIN SMALL LETTER E WITH GRAVE
				case 0xc169: result += gunichar(0x00EC); break; //LATIN SMALL LETTER I WITH GRAVE
				case 0xc16f: result += gunichar(0x00F2); break; //LATIN SMALL LETTER O WITH GRAVE
				case 0xc175: result += gunichar(0x00F9); break; //LATIN SMALL LETTER U WITH GRAVE
				case 0xc220: result += gunichar(0x00B4); break; //ACUTE ACCENT
				case 0xc241: result += gunichar(0x00C1); break; //LATIN CAPITAL LETTER A WITH ACUTE
				case 0xc243: result += gunichar(0x0106); break; //LATIN CAPITAL LETTER C WITH ACUTE
				case 0xc245: result += gunichar(0x00C9); break; //LATIN CAPITAL LETTER E WITH ACUTE
				case 0xc249: result += gunichar(0x00CD); break; //LATIN CAPITAL LETTER I WITH ACUTE
				case 0xc24c: result += gunichar(0x0139); break; //LATIN CAPITAL LETTER L WITH ACUTE
				case 0xc24e: result += gunichar(0x0143); break; //LATIN CAPITAL LETTER N WITH ACUTE
				case 0xc24f: result += gunichar(0x00D3); break; //LATIN CAPITAL LETTER O WITH ACUTE
				case 0xc252: result += gunichar(0x0154); break; //LATIN CAPITAL LETTER R WITH ACUTE
				case 0xc253: result += gunichar(0x015A); break; //LATIN CAPITAL LETTER S WITH ACUTE
				case 0xc255: result += gunichar(0x00DA); break; //LATIN CAPITAL LETTER U WITH ACUTE
				case 0xc259: result += gunichar(0x00DD); break; //LATIN CAPITAL LETTER Y WITH ACUTE
				case 0xc25a: result += gunichar(0x0179); break; //LATIN CAPITAL LETTER Z WITH ACUTE
				case 0xc261: result += gunichar(0x00E1); break; //LATIN SMALL LETTER A WITH ACUTE
				case 0xc263: result += gunichar(0x0107); break; //LATIN SMALL LETTER C WITH ACUTE
				case 0xc265: result += gunichar(0x00E9); break; //LATIN SMALL LETTER E WITH ACUTE
				case 0xc269: result += gunichar(0x00ED); break; //LATIN SMALL LETTER I WITH ACUTE
				case 0xc26c: result += gunichar(0x013A); break; //LATIN SMALL LETTER L WITH ACUTE
				case 0xc26e: result += gunichar(0x0144); break; //LATIN SMALL LETTER N WITH ACUTE
				case 0xc26f: result += gunichar(0x00F3); break; //LATIN SMALL LETTER O WITH ACUTE
				case 0xc272: result += gunichar(0x0155); break; //LATIN SMALL LETTER R WITH ACUTE
				case 0xc273: result += gunichar(0x015B); break; //LATIN SMALL LETTER S WITH ACUTE
				case 0xc275: result += gunichar(0x00FA); break; //LATIN SMALL LETTER U WITH ACUTE
				case 0xc279: result += gunichar(0x00FD); break; //LATIN SMALL LETTER Y WITH ACUTE
				case 0xc27a: result += gunichar(0x017A); break; //LATIN SMALL LETTER Z WITH ACUTE
				case 0xc341: result += gunichar(0x00C2); break; //LATIN CAPITAL LETTER A WITH CIRCUMFLEX
				case 0xc343: result += gunichar(0x0108); break; //LATIN CAPITAL LETTER C WITH CIRCUMFLEX
				case 0xc345: result += gunichar(0x00CA); break; //LATIN CAPITAL LETTER E WITH CIRCUMFLEX
				case 0xc347: result += gunichar(0x011C); break; //LATIN CAPITAL LETTER G WITH CIRCUMFLEX
				case 0xc348: result += gunichar(0x0124); break; //LATIN CAPITAL LETTER H WITH CIRCUMFLEX
				case 0xc349: result += gunichar(0x00CE); break; //LATIN CAPITAL LETTER I WITH CIRCUMFLEX
				case 0xc34a: result += gunichar(0x0134); break; //LATIN CAPITAL LETTER J WITH CIRCUMFLEX
				case 0xc34f: result += gunichar(0x00D4); break; //LATIN CAPITAL LETTER O WITH CIRCUMFLEX
				case 0xc353: result += gunichar(0x015C); break; //LATIN CAPITAL LETTER S WITH CIRCUMFLEX
				case 0xc355: result += gunichar(0x00DB); break; //LATIN CAPITAL LETTER U WITH CIRCUMFLEX
				case 0xc357: result += gunichar(0x0174); break; //LATIN CAPITAL LETTER W WITH CIRCUMFLEX
				case 0xc359: result += gunichar(0x0176); break; //LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
				case 0xc361: result += gunichar(0x00E2); break; //LATIN SMALL LETTER A WITH CIRCUMFLEX
				case 0xc363: result += gunichar(0x0109); break; //LATIN SMALL LETTER C WITH CIRCUMFLEX
				case 0xc365: result += gunichar(0x00EA); break; //LATIN SMALL LETTER E WITH CIRCUMFLEX
				case 0xc367: result += gunichar(0x011D); break; //LATIN SMALL LETTER G WITH CIRCUMFLEX
				case 0xc368: result += gunichar(0x0125); break; //LATIN SMALL LETTER H WITH CIRCUMFLEX
				case 0xc369: result += gunichar(0x00EE); break; //LATIN SMALL LETTER I WITH CIRCUMFLEX
				case 0xc36a: result += gunichar(0x0135); break; //LATIN SMALL LETTER J WITH CIRCUMFLEX
				case 0xc36f: result += gunichar(0x00F4); break; //LATIN SMALL LETTER O WITH CIRCUMFLEX
				case 0xc373: result += gunichar(0x015D); break; //LATIN SMALL LETTER S WITH CIRCUMFLEX
				case 0xc375: result += gunichar(0x00FB); break; //LATIN SMALL LETTER U WITH CIRCUMFLEX
				case 0xc377: result += gunichar(0x0175); break; //LATIN SMALL LETTER W WITH CIRCUMFLEX
				case 0xc379: result += gunichar(0x0177); break; //LATIN SMALL LETTER Y WITH CIRCUMFLEX
				case 0xc441: result += gunichar(0x00C3); break; //LATIN CAPITAL LETTER A WITH TILDE
				case 0xc449: result += gunichar(0x0128); break; //LATIN CAPITAL LETTER I WITH TILDE
				case 0xc44e: result += gunichar(0x00D1); break; //LATIN CAPITAL LETTER N WITH TILDE
				case 0xc44f: result += gunichar(0x00D5); break; //LATIN CAPITAL LETTER O WITH TILDE
				case 0xc455: result += gunichar(0x0168); break; //LATIN CAPITAL LETTER U WITH TILDE
				case 0xc461: result += gunichar(0x00E3); break; //LATIN SMALL LETTER A WITH TILDE
				case 0xc469: result += gunichar(0x0129); break; //LATIN SMALL LETTER I WITH TILDE
				case 0xc46e: result += gunichar(0x00F1); break; //LATIN SMALL LETTER N WITH TILDE
				case 0xc46f: result += gunichar(0x00F5); break; //LATIN SMALL LETTER O WITH TILDE
				case 0xc475: result += gunichar(0x0169); break; //LATIN SMALL LETTER U WITH TILDE
				case 0xc520: result += gunichar(0x00AF); break; //MACRON
				case 0xc541: result += gunichar(0x0100); break; //LATIN CAPITAL LETTER A WITH MACRON
				case 0xc545: result += gunichar(0x0112); break; //LATIN CAPITAL LETTER E WITH MACRON
				case 0xc549: result += gunichar(0x012A); break; //LATIN CAPITAL LETTER I WITH MACRON
				case 0xc54f: result += gunichar(0x014C); break; //LATIN CAPITAL LETTER O WITH MACRON
				case 0xc555: result += gunichar(0x016A); break; //LATIN CAPITAL LETTER U WITH MACRON
				case 0xc561: result += gunichar(0x0101); break; //LATIN SMALL LETTER A WITH MACRON
				case 0xc565: result += gunichar(0x0113); break; //LATIN SMALL LETTER E WITH MACRON
				case 0xc569: result += gunichar(0x012B); break; //LATIN SMALL LETTER I WITH MACRON
				case 0xc56f: result += gunichar(0x014D); break; //LATIN SMALL LETTER O WITH MACRON
				case 0xc575: result += gunichar(0x016B); break; //LATIN SMALL LETTER U WITH MACRON
				case 0xc620: result += gunichar(0x02D8); break; //BREVE
				case 0xc641: result += gunichar(0x0102); break; //LATIN CAPITAL LETTER A WITH BREVE
				case 0xc647: result += gunichar(0x011E); break; //LATIN CAPITAL LETTER G WITH BREVE
				case 0xc655: result += gunichar(0x016C); break; //LATIN CAPITAL LETTER U WITH BREVE
				case 0xc661: result += gunichar(0x0103); break; //LATIN SMALL LETTER A WITH BREVE
				case 0xc667: result += gunichar(0x011F); break; //LATIN SMALL LETTER G WITH BREVE
				case 0xc675: result += gunichar(0x016D); break; //LATIN SMALL LETTER U WITH BREVE
				case 0xc720: result += gunichar(0x02D9); break; //DOT ABOVE (Mandarin Chinese light tone)
				case 0xc743: result += gunichar(0x010A); break; //LATIN CAPITAL LETTER C WITH DOT ABOVE
				case 0xc745: result += gunichar(0x0116); break; //LATIN CAPITAL LETTER E WITH DOT ABOVE
				case 0xc747: result += gunichar(0x0120); break; //LATIN CAPITAL LETTER G WITH DOT ABOVE
				case 0xc749: result += gunichar(0x0130); break; //LATIN CAPITAL LETTER I WITH DOT ABOVE
				case 0xc75a: result += gunichar(0x017B); break; //LATIN CAPITAL LETTER Z WITH DOT ABOVE
				case 0xc763: result += gunichar(0x010B); break; //LATIN SMALL LETTER C WITH DOT ABOVE
				case 0xc765: result += gunichar(0x0117); break; //LATIN SMALL LETTER E WITH DOT ABOVE
				case 0xc767: result += gunichar(0x0121); break; //LATIN SMALL LETTER G WITH DOT ABOVE
				case 0xc77a: result += gunichar(0x017C); break; //LATIN SMALL LETTER Z WITH DOT ABOVE
				case 0xc820: result += gunichar(0x00A8); break; //DIAERESIS
				case 0xc841: result += gunichar(0x00C4); break; //LATIN CAPITAL LETTER A WITH DIAERESIS
				case 0xc845: result += gunichar(0x00CB); break; //LATIN CAPITAL LETTER E WITH DIAERESIS
				case 0xc849: result += gunichar(0x00CF); break; //LATIN CAPITAL LETTER I WITH DIAERESIS
				case 0xc84f: result += gunichar(0x00D6); break; //LATIN CAPITAL LETTER O WITH DIAERESIS
				case 0xc855: result += gunichar(0x00DC); break; //LATIN CAPITAL LETTER U WITH DIAERESIS
				case 0xc859: result += gunichar(0x0178); break; //LATIN CAPITAL LETTER Y WITH DIAERESIS
				case 0xc861: result += gunichar(0x00E4); break; //LATIN SMALL LETTER A WITH DIAERESIS
				case 0xc865: result += gunichar(0x00EB); break; //LATIN SMALL LETTER E WITH DIAERESIS
				case 0xc869: result += gunichar(0x00EF); break; //LATIN SMALL LETTER I WITH DIAERESIS
				case 0xc86f: result += gunichar(0x00F6); break; //LATIN SMALL LETTER O WITH DIAERESIS
				case 0xc875: result += gunichar(0x00FC); break; //LATIN SMALL LETTER U WITH DIAERESIS
				case 0xc879: result += gunichar(0x00FF); break; //LATIN SMALL LETTER Y WITH DIAERESIS
				case 0xca20: result += gunichar(0x02DA); break; //RING ABOVE
				case 0xca41: result += gunichar(0x00C5); break; //LATIN CAPITAL LETTER A WITH RING ABOVE
				case 0xca55: result += gunichar(0x016E); break; //LATIN CAPITAL LETTER U WITH RING ABOVE
				case 0xca61: result += gunichar(0x00E5); break; //LATIN SMALL LETTER A WITH RING ABOVE
				case 0xca75: result += gunichar(0x016F); break; //LATIN SMALL LETTER U WITH RING ABOVE
				case 0xcb20: result += gunichar(0x00B8); break; //CEDILLA
				case 0xcb43: result += gunichar(0x00C7); break; //LATIN CAPITAL LETTER C WITH CEDILLA
				case 0xcb47: result += gunichar(0x0122); break; //LATIN CAPITAL LETTER G WITH CEDILLA
				case 0xcb4b: result += gunichar(0x0136); break; //LATIN CAPITAL LETTER K WITH CEDILLA
				case 0xcb4c: result += gunichar(0x013B); break; //LATIN CAPITAL LETTER L WITH CEDILLA
				case 0xcb4e: result += gunichar(0x0145); break; //LATIN CAPITAL LETTER N WITH CEDILLA
				case 0xcb52: result += gunichar(0x0156); break; //LATIN CAPITAL LETTER R WITH CEDILLA
				case 0xcb53: result += gunichar(0x015E); break; //LATIN CAPITAL LETTER S WITH CEDILLA
				case 0xcb54: result += gunichar(0x0162); break; //LATIN CAPITAL LETTER T WITH CEDILLA
				case 0xcb63: result += gunichar(0x00E7); break; //LATIN SMALL LETTER C WITH CEDILLA
				case 0xcb67: result += gunichar(0x0123); break; //LATIN SMALL LETTER G WITH CEDILLA
				case 0xcb6b: result += gunichar(0x0137); break; //LATIN SMALL LETTER K WITH CEDILLA
				case 0xcb6c: result += gunichar(0x013C); break; //LATIN SMALL LETTER L WITH CEDILLA
				case 0xcb6e: result += gunichar(0x0146); break; //LATIN SMALL LETTER N WITH CEDILLA
				case 0xcb72: result += gunichar(0x0157); break; //LATIN SMALL LETTER R WITH CEDILLA
				case 0xcb73: result += gunichar(0x015F); break; //LATIN SMALL LETTER S WITH CEDILLA
				case 0xcb74: result += gunichar(0x0163); break; //LATIN SMALL LETTER T WITH CEDILLA
				case 0xcd20: result += gunichar(0x02DD); break; //DOUBLE ACUTE ACCENT
				case 0xcd4f: result += gunichar(0x0150); break; //LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
				case 0xcd55: result += gunichar(0x0170); break; //LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
				case 0xcd6f: result += gunichar(0x0151); break; //LATIN SMALL LETTER O WITH DOUBLE ACUTE
				case 0xcd75: result += gunichar(0x0171); break; //LATIN SMALL LETTER U WITH DOUBLE ACUTE
				case 0xce20: result += gunichar(0x02DB); break; //OGONEK
				case 0xce41: result += gunichar(0x0104); break; //LATIN CAPITAL LETTER A WITH OGONEK
				case 0xce45: result += gunichar(0x0118); break; //LATIN CAPITAL LETTER E WITH OGONEK
				case 0xce49: result += gunichar(0x012E); break; //LATIN CAPITAL LETTER I WITH OGONEK
				case 0xce55: result += gunichar(0x0172); break; //LATIN CAPITAL LETTER U WITH OGONEK
				case 0xce61: result += gunichar(0x0105); break; //LATIN SMALL LETTER A WITH OGONEK
				case 0xce65: result += gunichar(0x0119); break; //LATIN SMALL LETTER E WITH OGONEK
				case 0xce69: result += gunichar(0x012F); break; //LATIN SMALL LETTER I WITH OGONEK
				case 0xce75: result += gunichar(0x0173); break; //LATIN SMALL LETTER U WITH OGONEK
				case 0xcf20: result += gunichar(0x02C7); break; //CARON (Mandarin Chinese third tone)
				case 0xcf43: result += gunichar(0x010C); break; //LATIN CAPITAL LETTER C WITH CARON
				case 0xcf44: result += gunichar(0x010E); break; //LATIN CAPITAL LETTER D WITH CARON
				case 0xcf45: result += gunichar(0x011A); break; //LATIN CAPITAL LETTER E WITH CARON
				case 0xcf4c: result += gunichar(0x013D); break; //LATIN CAPITAL LETTER L WITH CARON
				case 0xcf4e: result += gunichar(0x0147); break; //LATIN CAPITAL LETTER N WITH CARON
				case 0xcf52: result += gunichar(0x0158); break; //LATIN CAPITAL LETTER R WITH CARON
				case 0xcf53: result += gunichar(0x0160); break; //LATIN CAPITAL LETTER S WITH CARON
				case 0xcf54: result += gunichar(0x0164); break; //LATIN CAPITAL LETTER T WITH CARON
				case 0xcf5a: result += gunichar(0x017D); break; //LATIN CAPITAL LETTER Z WITH CARON
				case 0xcf63: result += gunichar(0x010D); break; //LATIN SMALL LETTER C WITH CARON
				case 0xcf64: result += gunichar(0x010F); break; //LATIN SMALL LETTER D WITH CARON
				case 0xcf65: result += gunichar(0x011B); break; //LATIN SMALL LETTER E WITH CARON
				case 0xcf6c: result += gunichar(0x013E); break; //LATIN SMALL LETTER L WITH CARON
				case 0xcf6e: result += gunichar(0x0148); break; //LATIN SMALL LETTER N WITH CARON
				case 0xcf72: result += gunichar(0x0159); break; //LATIN SMALL LETTER R WITH CARON
				case 0xcf73: result += gunichar(0x0161); break; //LATIN SMALL LETTER S WITH CARON
				case 0xcf74: result += gunichar(0x0165); break; //LATIN SMALL LETTER T WITH CARON
				case 0xcf7a: result += gunichar(0x017E); break; //LATIN SMALL LETTER Z WITH CARON
				default: break; // unknown
			}
		}
		else
		{
			switch ( text_buffer[i] )
			{
				case 0xa0: result += gunichar(0x00A0); break; //NO-BREAK SPACE
				case 0xa1: result += gunichar(0x00A1); break; //INVERTED EXCLAMATION MARK
				case 0xa2: result += gunichar(0x00A2); break; //CENT SIGN
				case 0xa3: result += gunichar(0x00A3); break; //POUND SIGN
				case 0xa5: result += gunichar(0x00A5); break; //YEN SIGN
				case 0xa7: result += gunichar(0x00A7); break; //SECTION SIGN
				case 0xa8: result += gunichar(0x00A4); break; //CURRENCY SIGN
				case 0xa9: result += gunichar(0x2018); break; //LEFT SINGLE QUOTATION MARK
				case 0xaa: result += gunichar(0x201C); break; //LEFT DOUBLE QUOTATION MARK
				case 0xab: result += gunichar(0x00AB); break; //LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
				case 0xac: result += gunichar(0x2190); break; //LEFTWARDS ARROW
				case 0xad: result += gunichar(0x2191); break; //UPWARDS ARROW
				case 0xae: result += gunichar(0x2192); break; //RIGHTWARDS ARROW
				case 0xaf: result += gunichar(0x2193); break; //DOWNWARDS ARROW
				case 0xb0: result += gunichar(0x00B0); break; //DEGREE SIGN
				case 0xb1: result += gunichar(0x00B1); break; //PLUS-MINUS SIGN
				case 0xb2: result += gunichar(0x00B2); break; //SUPERSCRIPT TWO
				case 0xb3: result += gunichar(0x00B3); break; //SUPERSCRIPT THREE
				case 0xb4: result += gunichar(0x00D7); break; //MULTIPLICATION SIGN
				case 0xb5: result += gunichar(0x00B5); break; //MICRO SIGN
				case 0xb6: result += gunichar(0x00B6); break; //PILCROW SIGN
				case 0xb7: result += gunichar(0x00B7); break; //MIDDLE DOT
				case 0xb8: result += gunichar(0x00F7); break; //DIVISION SIGN
				case 0xb9: result += gunichar(0x2019); break; //RIGHT SINGLE QUOTATION MARK
				case 0xba: result += gunichar(0x201D); break; //RIGHT DOUBLE QUOTATION MARK
				case 0xbb: result += gunichar(0x00BB); break; //RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
				case 0xbc: result += gunichar(0x00BC); break; //VULGAR FRACTION ONE QUARTER
				case 0xbd: result += gunichar(0x00BD); break; //VULGAR FRACTION ONE HALF
				case 0xbe: result += gunichar(0x00BE); break; //VULGAR FRACTION THREE QUARTERS
				case 0xbf: result += gunichar(0x00BF); break; //INVERTED QUESTION MARK
				case 0xd0: result += gunichar(0x2014); break; //EM DASH
				case 0xd1: result += gunichar(0x00B9); break; //SUPERSCRIPT ONE
				case 0xd2: result += gunichar(0x00AE); break; //REGISTERED SIGN
				case 0xd3: result += gunichar(0x00A9); break; //COPYRIGHT SIGN
				case 0xd4: result += gunichar(0x2122); break; //TRADE MARK SIGN
				case 0xd5: result += gunichar(0x266A); break; //EIGHTH NOTE
				case 0xd6: result += gunichar(0x00AC); break; //NOT SIGN
				case 0xd7: result += gunichar(0x00A6); break; //BROKEN BAR
				case 0xdc: result += gunichar(0x215B); break; //VULGAR FRACTION ONE EIGHTH
				case 0xdd: result += gunichar(0x215C); break; //VULGAR FRACTION THREE EIGHTHS
				case 0xde: result += gunichar(0x215D); break; //VULGAR FRACTION FIVE EIGHTHS
				case 0xdf: result += gunichar(0x215E); break; //VULGAR FRACTION SEVEN EIGHTHS
				case 0xe0: result += gunichar(0x2126); break; //OHM SIGN
				case 0xe1: result += gunichar(0x00C6); break; //LATIN CAPITAL LETTER AE
				case 0xe2: result += gunichar(0x00D0); break; //LATIN CAPITAL LETTER ETH (Icelandic)
				case 0xe3: result += gunichar(0x00AA); break; //FEMININE ORDINAL INDICATOR
				case 0xe4: result += gunichar(0x0126); break; //LATIN CAPITAL LETTER H WITH STROKE
				case 0xe6: result += gunichar(0x0132); break; //LATIN CAPITAL LIGATURE IJ
				case 0xe7: result += gunichar(0x013F); break; //LATIN CAPITAL LETTER L WITH MIDDLE DOT
				case 0xe8: result += gunichar(0x0141); break; //LATIN CAPITAL LETTER L WITH STROKE
				case 0xe9: result += gunichar(0x00D8); break; //LATIN CAPITAL LETTER O WITH STROKE
				case 0xea: result += gunichar(0x0152); break; //LATIN CAPITAL LIGATURE OE
				case 0xeb: result += gunichar(0x00BA); break; //MASCULINE ORDINAL INDICATOR
				case 0xec: result += gunichar(0x00DE); break; //LATIN CAPITAL LETTER THORN (Icelandic)
				case 0xed: result += gunichar(0x0166); break; //LATIN CAPITAL LETTER T WITH STROKE
				case 0xee: result += gunichar(0x014A); break; //LATIN CAPITAL LETTER ENG (Sami)
				case 0xef: result += gunichar(0x0149); break; //LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
				case 0xf0:	result += gunichar(0x0138); break; //LATIN SMALL LETTER KRA (Greenlandic)
				case 0xf1:	result += gunichar(0x00E6); break; //LATIN SMALL LETTER AE
				case 0xf2:	result += gunichar(0x0111); break; //LATIN SMALL LETTER D WITH STROKE
				case 0xf3:	result += gunichar(0x00F0); break; //LATIN SMALL LETTER ETH (Icelandic)
				case 0xf4:	result += gunichar(0x0127); break; //LATIN SMALL LETTER H WITH STROKE
				case 0xf5:	result += gunichar(0x0131); break; //LATIN SMALL LETTER DOTLESS I
				case 0xf6:	result += gunichar(0x0133); break; //LATIN SMALL LIGATURE IJ
				case 0xf7:	result += gunichar(0x0140); break; //LATIN SMALL LETTER L WITH MIDDLE DOT
				case 0xf8:	result += gunichar(0x0142); break; //LATIN SMALL LETTER L WITH STROKE
				case 0xf9:	result += gunichar(0x00F8); break; //LATIN SMALL LETTER O WITH STROKE
				case 0xfa:	result += gunichar(0x0153); break; //LATIN SMALL LIGATURE OE
				case 0xfb:	result += gunichar(0x00DF); break; //LATIN SMALL LETTER SHARP S (German)
				case 0xfc:	result += gunichar(0x00FE); break; //LATIN SMALL LETTER THORN (Icelandic)
				case 0xfd:	result += gunichar(0x0167); break; //LATIN SMALL LETTER T WITH STROKE
				case 0xfe:	result += gunichar(0x014B); break; //LATIN SMALL LETTER ENG (Sami)
				case 0xff: result += gunichar(0x00AD); break; //SOFT HYPHEN
				default: result += (char)text_buffer[i];
			}
		}
	}
	return result;
}