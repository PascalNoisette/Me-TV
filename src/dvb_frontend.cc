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
 
#include "dvb_frontend.h"
#include "exception.h"
#include <glibmm/i18n.h>

using namespace Dvb;

Frontend::Frontend(const Adapter& adapter, guint frontend) : adapter(adapter)
{
	fd = -1;

	Glib::ustring path = adapter.get_frontend_path(frontend);
	g_debug("Opening frontend device: %s", path.c_str());
	if ( ( fd = open ( path.c_str(), O_RDWR | O_NONBLOCK ) ) < 0 )
	{
		throw SystemException(_("Failed to open tuner"));
	}
	
	if ( ioctl ( fd, FE_GET_INFO, &frontend_info ) < 0 )
	{
		throw SystemException(_("Failed to get tuner info"));
	}
}

Frontend::~Frontend()
{
	if (fd != -1)
	{
		close(fd);
	}
}

void Frontend::tune_to (const Transponder& transponder, guint wait_seconds)
{
	struct dvb_frontend_event ev;
	
	if (frontend_info.type == FE_QPSK)
	{
		diseqc(transponder);
	}

	// Discard stale events
	while (ioctl(fd, FE_GET_EVENT, &ev) != -1);

	if ( ioctl ( fd, FE_SET_FRONTEND, &(transponder.frontend_parameters) ) < 0 )
	{
		throw SystemException(_("Failed to tune device"));
	}
	
	g_message(_("Waiting for signal lock ..."));
	wait_lock(wait_seconds);
	g_message(_("Got signal lock"));
}

void Frontend::diseqc(const Transponder& transponder)
{
	int satellite_number	= transponder.satellite_number;
	int polarisation		= transponder.polarisation;
	int hi_band				= transponder.hi_band;
	
	struct dvb_diseqc_master_cmd cmd = { {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4};
	cmd.msg[3] = 0xf0 | (((satellite_number * 4) & 0x0f) | (hi_band ? 1 : 0) | (polarisation ? 0 : 2));
	
	fe_sec_voltage_t	voltage	= polarisation ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t	tone	= hi_band ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t	burst	= (satellite_number / 4) % 2 ? SEC_MINI_B : SEC_MINI_A;

	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
	{
		throw SystemException(_("Failed to set tone off"));
	}
	
	if (ioctl(fd, FE_SET_VOLTAGE, voltage) == -1)
	{
		throw SystemException(_("Failed to set voltage"));
	}

	usleep(15 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1)
	{
		throw SystemException(_("Failed to send master command"));
	}

	usleep(15 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_BURST, burst) == -1)
	{
		throw SystemException(_("Failed to send burst"));
	}
	
	usleep(15 * 1000);
	if (ioctl(fd, FE_SET_TONE, tone) == -1)
	{
		throw SystemException(_("Failed to set tone"));
	}
}

const struct dvb_frontend_info& Frontend::get_frontend_info() const
{
	return frontend_info;
}

void Frontend::wait_lock(guint wait_seconds)
{
	fe_status_t	status;
	time_t		start_time = time(NULL);

	while ((start_time + wait_seconds) > time(NULL))
	{
		if (!ioctl(fd, FE_READ_STATUS, &status))
		{
			if (status & FE_HAS_LOCK)
			{
				break;
			}
		}

		usleep(100000);
	}

	if (!(status & FE_HAS_LOCK))
	{
		throw Exception(_("Failed to lock to channel"));
	}
}
