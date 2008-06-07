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
 
#ifndef __DVB_FRONTEND_H__
#define __DVB_FRONTEND_H__

#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <stdint.h>
#include <giomm.h>
#include "dvb_transponder.h"

struct diseqc_cmd
{
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

namespace Dvb
{
	class Adapter
	{
	private:
		Glib::ustring path;
	public:
		Adapter(guint adapter)
		{
			path = Glib::ustring::format("/dev/dvb/adapter", adapter);
		}

		Glib::ustring get_frontend_path(guint frontend) const
		{
			return Glib::ustring::format(path + "/frontend", frontend);
		}
			
		Glib::ustring get_demux_path() const { return path + "/demux0"; }
			
		gboolean frontend_exists(guint frontend)
		{
			Gio::File::create_for_path(get_frontend_path(frontend))->query_exists();
		}
	};
	
	class Frontend
	{
	private:
		const Adapter& adapter;
		int fd;
		struct dvb_frontend_info frontend_info;
		void wait_lock(guint wait_seconds);
		void diseqc(const Transponder& transponder);
		
	public:
		Frontend(const Adapter& adapter, guint frontend);
		~Frontend();

		int get_frontend_type() const { return frontend_info.type; }
		const struct dvb_frontend_info& get_frontend_info() const;
		int get_fd() const { return fd; }
		void tune_to (const Transponder& transponder, guint timeout = 10);
		const Adapter& get_adapter() const { return adapter; }
	};
}

#endif
