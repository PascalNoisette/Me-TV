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

#ifndef __DVB_SCANNER_H__
#define __DVB_SCANNER_H__

#include "dvb_frontend.h"
#include "dvb_service.h"
#include <vector>

namespace Dvb
{
	class Scanner
	{
	private:
		guint convert_string_to_value(const StringTable* table, const gchar* text);
		void process_terrestrial_line(Frontend& frontend, const Glib::ustring& line, guint wait_timeout);
		gboolean terminated;
	public:
		Scanner();
			
		void start(Frontend& frontend, const Glib::ustring& region_file_path, guint wait_timeout);
		void terminate();
			
		sigc::signal<void, struct dvb_frontend_parameters&, guint, const Glib::ustring&> signal_service;
		sigc::signal<void, guint, gsize> signal_progress;
	};
}

#endif
