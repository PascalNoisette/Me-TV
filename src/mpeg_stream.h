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

#ifndef __MPEG_STREAM_H__
#define __MPEG_STREAM_H__

#define STREAM_TYPE_MPEG1		0x01
#define STREAM_TYPE_MPEG2		0x02
#define STREAM_TYPE_MPEG4		0x10
#define STREAM_TYPE_H264		0x1B

#include <linux/dvb/frontend.h>
#include <list>
#include <vector>
#include "me-tv-i18n.h"
#include "buffer.h"

namespace Mpeg
{
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

	class Stream
	{
	private:
		gint CRC32[256];
		guint program_map_pid;

		void calculate_crc(guchar *p_begin, guchar *p_end);
		gboolean is_pid_used(guint pid);
		gboolean find_descriptor(guchar tag, const unsigned char *buf, int descriptors_loop_len, const unsigned char **desc, int *desc_len);
		Glib::ustring get_lang_desc(const guchar* buffer);
			
	public:
		Stream();

		std::vector<VideoStream>	video_streams;
		std::vector<AudioStream>	audio_streams;
		std::vector<SubtitleStream>	subtitle_streams;
		std::vector<TeletextStream>	teletext_streams;

		void set_program_map_pid(const Buffer& buffer, guint service_id);
		void parse_pms(const Buffer& buffer);
		void build_pat(guchar* buffer);
		void build_pmt(guchar* buffer);

		void clear();
	};
}

#endif
