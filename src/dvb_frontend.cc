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
#include "dvb_frontend.h"
#include "dvb_transponder.h"
#include "exception.h"
#include "me-tv-i18n.h"

using namespace Dvb;

Frontend::Frontend(const Adapter& frontend_adapter, guint frontend_index)
	: adapter(frontend_adapter)
{
	fd = -1;
	memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
	frontend = frontend_index;
}

Frontend::~Frontend()
{
	close();
	g_debug("Frontend destroyed");
}

void Frontend::open()
{
	if (fd == -1)
	{
		Glib::ustring path = adapter.get_frontend_path(frontend);
		g_debug("Opening frontend device: %s", path.c_str());
		if ( ( fd = ::open ( path.c_str(), O_RDWR | O_NONBLOCK ) ) < 0 )
		{
			throw SystemException(_("Failed to open tuner"));
		}
	
		if ( ioctl ( fd, FE_GET_INFO, &frontend_info ) < 0 )
		{
			throw SystemException(_("Failed to get tuner info"));
		}
	}
}

void Frontend::close()
{
	if (fd != -1)
	{
		::close(fd);
		fd = -1;
	}
}

void Frontend::tune_to(Transponder& transponder, guint wait_seconds)
{
	struct dvb_frontend_parameters parameters = transponder.frontend_parameters;
	struct dvb_frontend_event ev;
	g_debug("Trying to tune to freq %d, symbol rate %d, inner fec %d", parameters.frequency, parameters.u.qpsk.symbol_rate, parameters.u.qpsk.fec_inner);
	
	if(frontend_info.type == FE_QPSK)
	{
		transponder.hi_band = 0;
		
		if(LNBHighValue > 0 && LNBSwitchValue > 0 && parameters.frequency >= LNBSwitchValue)
		{
			transponder.hi_band = 1;
		}
		
		diseqc(transponder);
		
		if(transponder.hi_band == 1)
		{
			parameters.frequency = abs(parameters.frequency - LNBHighValue);
		}
		else
		{
			parameters.frequency = abs(parameters.frequency - LNBLowValue);
		}
		
		g_debug("Diseqc'd, as this is a dvb-s device. We're hiband? %d new freq: %d polarisation: %d", transponder.hi_band, parameters.frequency, transponder.polarisation);
		usleep(500000);
	}

	// Discard stale events
	do {} while (ioctl(fd, FE_GET_EVENT, &ev) != -1);
		
	gint return_code = ioctl ( fd, FE_SET_FRONTEND, &parameters );
	if (return_code  < 0 )
	{
		g_debug("return code: %d", return_code);
		throw SystemException(_("Failed to tune device") );
	}
	
	
	g_message(_("Waiting for signal lock ..."));
	wait_lock(wait_seconds);
	g_message(_("Got signal lock"));
	
	frontend_parameters = parameters;
}

void Frontend::diseqc(const Transponder& transponder)
{
	// hiband 1, polarisation 0 seems to work.
	int satellite_number	= transponder.satellite_number;
	int polarisation		= transponder.polarisation;
	int hi_band			= transponder.hi_band;
	
	struct dvb_diseqc_master_cmd cmd = { {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4};
	cmd.msg[3] = 0xf0 | (((satellite_number * 4) & 0x0f) | (hi_band ? 1 : 0) | (polarisation ? 0 : 2));
	
	g_debug("diseqc - satnum %d , pol %d , hiband %d msg_3 = %d", satellite_number, polarisation, hi_band, cmd.msg[3]);
	
	fe_sec_voltage_t	voltage	= (polarisation == POLARISATION_VERTICAL) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t	tone	= (hi_band == 1) ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t	burst	= (satellite_number / 4) % 2 ? SEC_MINI_B : SEC_MINI_A;
	
	g_debug("diseqc - created commands: voltage %d - tone %d - burst %d - msg3 %d.", voltage, tone, burst, cmd.msg[3]);
	

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

	usleep(19 * 1000);
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
	guint		start_time = time(NULL);
	
	while ((start_time + wait_seconds) > (guint)time(NULL))
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
		g_debug("status: %d", status);
		struct dvb_frontend_parameters parameters;
		ioctl(fd, FE_GET_FRONTEND, &parameters);
		g_debug("currently tuned to freq %d, symbol rate %d, inner fec %d", parameters.frequency, parameters.u.qpsk.symbol_rate, parameters.u.qpsk.fec_inner);
		
		throw Exception(_("Failed to lock to channel"));
	}
}

guint Frontend::get_signal_strength()
{
	guint result = 0;
	if (ioctl(fd, FE_READ_SIGNAL_STRENGTH, &result) == -1)
	{
		throw SystemException(_("Failed to get signal strength"));
	}
	return result;
}

guint Frontend::get_snr()
{
	guint result = 0;
	if (ioctl(fd, FE_READ_SNR, &result) == -1)
	{
		throw SystemException(_("Failed to get signal to noise ratio"));
	}
	return result;
}

const struct dvb_frontend_parameters& Frontend::get_frontend_parameters() const
{
	return frontend_parameters;
}
