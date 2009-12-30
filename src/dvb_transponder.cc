/*
 * Copyright (C) 2010 Michael Lamothe
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
 
#include "dvb_transponder.h"
#include "dvb_service.h"
#include "exception.h"
#include "me-tv.h"
#include "me-tv-i18n.h"

using namespace Dvb;

Transponder::Transponder() : polarisation(0), satellite_number(0), hi_band(0)
{
	memset(&frontend_parameters, 0, sizeof(frontend_parameters));
}

gboolean TransponderList::exists(const Transponder& transponder)
{
	for (const_iterator i = begin(); i != end(); i++)
	{
		if (transponder == *i)
		{
			return true;
		}
	}
	
	return false;
}

void TransponderList::add(const Transponder& transponder)
{
	if (!exists(transponder))
	{
		push_back(transponder);
	}
}

bool Transponder::operator==(const Transponder& transponder) const
{
	return transponder.frontend_parameters.frequency == frontend_parameters.frequency;
}

bool Transponder::operator!=(const Transponder& transponder) const
{
	return transponder.frontend_parameters.frequency != frontend_parameters.frequency;
}
