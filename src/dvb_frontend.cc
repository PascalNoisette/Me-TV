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
#include "me-tv.h"

using namespace Dvb;

struct StringTable Dvb::bandwidth_table[] =
{
	{ "8MHz", BANDWIDTH_8_MHZ },
	{ "7MHz", BANDWIDTH_7_MHZ },
	{ "6MHz", BANDWIDTH_6_MHZ },
	{ "AUTO", BANDWIDTH_AUTO },
	{ NULL, 0 }
};

struct StringTable Dvb::fec_table[] =
{
	{ "NONE", FEC_NONE },
	{ "1/2",  FEC_1_2 },
	{ "2/3",  FEC_2_3 },
	{ "3/4",  FEC_3_4 },
	{ "4/5",  FEC_4_5 },
	{ "5/6",  FEC_5_6 },
	{ "6/7",  FEC_6_7 },
	{ "7/8",  FEC_7_8 },
	{ "8/9",  FEC_8_9 },
	{ "AUTO", FEC_AUTO },
	{ NULL, 0 }
};

struct StringTable Dvb::modulation_table[] =
{
	{ "QPSK",   QPSK },
	{ "QAM16",  QAM_16 },
	{ "QAM32",  QAM_32 },
	{ "QAM64",  QAM_64 },
	{ "QAM128", QAM_128 },
	{ "QAM256", QAM_256 },
	{ "AUTO",   QAM_AUTO },
	{ "8VSB",   VSB_8 },
	{ "16VSB",  VSB_16 },
	{ NULL, 0 }
};

struct StringTable Dvb::transmit_mode_table[] =
{
	{ "2k",   TRANSMISSION_MODE_2K },
	{ "8k",   TRANSMISSION_MODE_8K },
	{ "AUTO", TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

struct StringTable Dvb::guard_table[] =
{
	{ "1/32", GUARD_INTERVAL_1_32 },
	{ "1/16", GUARD_INTERVAL_1_16 },
	{ "1/8",  GUARD_INTERVAL_1_8 },
	{ "1/4",  GUARD_INTERVAL_1_4 },
	{ "AUTO", GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

struct StringTable Dvb::hierarchy_table[] =
{
	{ "NONE", HIERARCHY_NONE },
	{ "1",    HIERARCHY_1 },
	{ "2",    HIERARCHY_2 },
	{ "4",    HIERARCHY_4 },
	{ "AUTO", HIERARCHY_AUTO },
	{ NULL, 0 }
};

struct StringTable Dvb::inversion_table[] =
{
	{ "INVERSION_OFF",	INVERSION_OFF },
	{ "INVERSION_ON",	INVERSION_ON },
	{ "INVERSION_AUTO",	INVERSION_AUTO },
	{ NULL, 0 }
};

Frontend::Frontend(const Adapter& frontend_adapter, guint frontend_index)
	: adapter(frontend_adapter)
{
	fd = -1;
	memset(&frontend_parameters, 0, sizeof(struct dvb_frontend_parameters));
	frontend = frontend_index;

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
	g_debug("Frontend destroyed");
	if (fd != -1)
	{
		close(fd);
	}
}

guint Frontend::convert_string_to_value(const StringTable* table, const Glib::ustring& text)
{
	gboolean found = false;
	const StringTable*	current = table;

	while (current->text != NULL && !found)
	{
		if (text == current->text)
		{
			found = true;
		}
		else
		{
			current++;
		}
	}
	
	if (!found)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find a value for '%1'"), text));
	}
	
	return (guint)current->value;
}

Glib::ustring Frontend::convert_value_to_string(const StringTable* table, guint value)
{
	gboolean found = false;
	const StringTable*	current = table;

	while (current->text != NULL && !found)
	{
		if (value == current->value)
		{
			found = true;
		}
		else
		{
			current++;
		}
	}
	
	if (!found)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find a text value for '%1'"), value));
	}
	
	return current->text;
}

void Frontend::tune_to (const struct dvb_frontend_parameters& parameters, guint wait_seconds)
{
	struct dvb_frontend_event ev;

	// Discard stale events
	while (ioctl(fd, FE_GET_EVENT, &ev) != -1);

	if ( ioctl ( fd, FE_SET_FRONTEND, &(parameters) ) < 0 )
	{
		throw SystemException(_("Failed to tune device"));
	}
	
	g_message(_("Waiting for signal lock ..."));
	wait_lock(wait_seconds);
	g_message(_("Got signal lock"));
	
	frontend_parameters = parameters;
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
