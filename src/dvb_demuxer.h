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

#ifndef __DVB_DEMUXER_H__
#define __DVB_DEMUXER_H__

#include <sys/poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <glibmm.h>
#include <linux/dvb/dmx.h>
#include "me-tv.h"
#include "buffer.h"

namespace Dvb {

	class Demuxer {
	private:
		int fd;
		pollfd pfd[1];
	public:
		Demuxer(Glib::ustring const & device_path);
		~Demuxer();
		void set_pes_filter(uint16_t pid, dmx_pes_type_t pestype);
		void set_filter(ushort pid, ushort table_id, ushort mask = 0xFF);
		void set_buffer_size(unsigned int buffer_size);
		gint read(unsigned char * buffer, size_t length, gint timeout = read_timeout * 1000);
		void read_section(Buffer & buffer, gint timeout = read_timeout * 1000);
		gboolean poll(gint timeout = read_timeout * 1000);
		void stop();
		int get_fd() const { return fd; };
	};

	typedef std::list<Dvb::Demuxer *> DemuxerList;

}

#endif
