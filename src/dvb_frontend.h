/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014  Russel Winder
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

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <stdint.h>
#include <giomm.h>
#include <linux/dvb/frontend.h>
#include "dvb_transponder.h"

constexpr auto LNB_HIGH_VALUE = 10600000;
constexpr auto LNB_LOW_VALUE = 9750000;
constexpr auto LNB_SWITCH_VALUE = 11700000;

struct diseqc_cmd {
	dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

namespace Dvb {
	class Adapter {
	private:
		Glib::ustring path;
		guint index;
	public:
		Adapter(guint adapter_index) : index(adapter_index) {
			path = Glib::ustring::compose("/dev/dvb/adapter%1", index);
		}
		Glib::ustring get_frontend_path(guint frontend) const {
			return Glib::ustring::compose(path + "/frontend%1", frontend);
		}
		Glib::ustring get_demux_path() const { return path + "/demux0"; } // TODO: always 0?
		Glib::ustring get_dvr_path() const { return path + "/dvr0"; } // TODO: always 0?
	};

	class Frontend {
	private:
		Adapter const & adapter;
		int fd;
		dvb_frontend_info frontend_info;
		void wait_lock(guint wait_seconds);
		void diseqc(int satellite_number, int polarisation, int hi_band);
		guint frontend;
		dvb_frontend_parameters frontend_parameters;

	public:
		Frontend(const Adapter& adapter, guint frontend);
		~Frontend();
		void open();
		void close();
		void tune_to(Transponder const & transponder, guint timeout = 2000);
		dvb_frontend_parameters const & get_frontend_parameters() const { return frontend_parameters; }
		fe_type_t get_frontend_type() const { return frontend_info.type; }
		dvb_frontend_info const & get_frontend_info() const { return frontend_info; }
		Glib::ustring get_name() const { return get_frontend_info().name; }
		int get_fd() const { return fd; }
		Adapter const & get_adapter() const { return adapter; }
		Glib::ustring get_path() const { return adapter.get_frontend_path(frontend); }
		guint get_signal_strength();
		guint get_snr();
	};
}

#endif
