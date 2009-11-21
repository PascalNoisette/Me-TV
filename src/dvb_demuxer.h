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
 
#ifndef __DVB_DEMUXER_H__
#define __DVB_DEMUXER_H__

#include <sys/poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <glibmm.h>
#include <linux/dvb/dmx.h>
#include "me-tv.h"

namespace Dvb
{
	class Demuxer
	{
	private:
		int fd;
		struct pollfd pfd[1];

	public:
		Demuxer(const Glib::ustring& device_path);
		~Demuxer();

		void set_pes_filter(uint16_t pid, dmx_pes_type_t pestype);		
		void set_filter(ushort pid, ushort table_id, ushort mask = 0);
		void set_buffer_size(unsigned int buffer_size);
		gint read(unsigned char* buffer, size_t length);
		gboolean poll(gint timeout = read_timeout * 1000);
		void stop();
		int get_fd() const;
	};
}

typedef std::list<Dvb::Demuxer*> DemuxerList;

#endif
