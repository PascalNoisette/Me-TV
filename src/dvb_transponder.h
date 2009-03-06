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
 
#ifndef __DVB_TRANSPONDER_H__
#define __DVB_TRANSPONDER_H__

#include <glibmm.h>
#include <linux/dvb/frontend.h>
#include "dvb_service.h"

enum polarisation
{
	POLARISATION_HORIZONTAL     = 0x00,
	POLARISATION_VERTICAL       = 0x01,
	POLARISATION_CIRCULAR_LEFT  = 0x02,
	POLARISATION_CIRCULAR_RIGHT = 0x03
};

namespace Dvb
{
	class Transponder
	{
	public:
		Transponder();
			
		struct dvb_frontend_parameters	frontend_parameters;
		guint							polarisation;
		guint							satellite_number;
		guint							hi_band;
		
		Transponder& operator=(const Transponder& transponder);
	};
}

#endif
