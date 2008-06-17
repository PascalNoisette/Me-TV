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
	struct StringTable
	{
		const char*	text;
		guint		value;
	};
	
	class Adapter
	{
	private:
		Glib::ustring path;
	public:
		Adapter(guint adapter)
		{
			path = Glib::ustring::compose("/dev/dvb/adapter%1", adapter);
		}

		Glib::ustring get_frontend_path(guint frontend) const
		{
			return Glib::ustring::compose(path + "/frontend%1", frontend);
		}
			
		Glib::ustring get_demux_path() const { return path + "/demux0"; }
		Glib::ustring get_dvr_path() const { return path + "/dvr0"; }
			
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

		void tune_to (const Transponder& transponder, guint timeout = 10);
			
		int get_frontend_type() const { return frontend_info.type; }
		const struct dvb_frontend_info& get_frontend_info() const;
		int get_fd() const { return fd; }
		const Adapter& get_adapter() const { return adapter; }
			
		static struct StringTable* get_bandwidth_table();
		static struct StringTable* get_fec_table();
		static struct StringTable* get_qam_table();
		static struct StringTable* get_modulation_table();
		static struct StringTable* get_guard_table();
		static struct StringTable* get_hierarchy_table();
		static struct StringTable* get_inversion_table();

		static guint convert_string_to_value(const StringTable* table, const Glib::ustring& text);
		static Glib::ustring convert_value_to_string(const StringTable* table, guint value);

		guint get_signal_strength();
		guint get_snr();
	};
}

#endif
