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
#include "me-tv-i18n.h"
#include "dvb_demuxer.h"
#include "exception.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

using namespace Dvb;

#define POLL_TIMEOUT 2000

Demuxer::Demuxer(const Glib::ustring& device_path)
{
	fd = -1;
	
	if ((fd = open(device_path.c_str(),O_RDWR|O_NONBLOCK)) < 0)
	{
		throw SystemException(_("Failed to open demux device"));
	}

	pfd[0].fd = fd;
	pfd[0].events = POLLIN | POLLOUT | POLLPRI;
}

Demuxer::~Demuxer()
{
	if (fd != -1)
	{
		stop();
		::close(fd);
		fd = -1;
	}
}

void Demuxer::set_pes_filter(uint16_t pid, dmx_pes_type_t pestype)
{
	struct dmx_pes_filter_params parameters;
	
	memset( &parameters, 0, sizeof( dmx_pes_filter_params ) );

	parameters.pid     = pid;
	parameters.input   = DMX_IN_FRONTEND;
	parameters.output  = DMX_OUT_TS_TAP;
	parameters.pes_type = pestype;
	parameters.flags   = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
	
	if (ioctl(fd, DMX_SET_PES_FILTER, &parameters) < 0)
	{
		throw SystemException(_("Failed to set PES filter"));
	}
}

void Demuxer::set_filter(ushort pid, ushort table_id, ushort mask)
{
	struct dmx_sct_filter_params parameters;

	memset( &parameters, 0, sizeof( dmx_sct_filter_params ) );
	
	parameters.pid = pid;
	parameters.timeout = 0;
	parameters.filter.filter[0] = table_id;
	parameters.filter.mask[0] = mask;
	parameters.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl(fd, DMX_SET_FILTER, &parameters) < 0)
	{
		throw SystemException(_("Failed to set section filter for demuxer"));
	}
}

void Demuxer::set_buffer_size(unsigned int buffer_size)
{
	if (ioctl(fd, DMX_SET_BUFFER_SIZE, buffer_size) < 0)
	{
		throw SystemException(_("Failed to set demuxer buffer size"));
	}
}

gint Demuxer::read(unsigned char* buffer, size_t length)
{
	if (!poll(POLL_TIMEOUT))
	{
		throw TimeoutException(_("Timeout while reading"));
	}

	gint bytes_read = ::read(fd, buffer, length);
	if (bytes_read == -1)
	{
		throw SystemException(_("Failed to read data from demuxer"));
	}
	
	return bytes_read;
}

void Demuxer::stop()
{
	if (ioctl(fd, DMX_STOP) < 0)
	{
		throw SystemException(_("Failed to stop demuxer"));
	}
}

int Demuxer::get_fd() const
{
	return fd;
}

gboolean Demuxer::poll(guint timeout)
{
	gint result = ::poll(pfd, 1, timeout);
	
	if (result == -1)
	{
		throw SystemException (_("Failed to poll"));
	}
	
	return result > 0;
}
