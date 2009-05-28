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

#ifndef __METERS_DIALOG_H__
#define __METERS_DIALOG_H__

#include <libgnomeuimm.h>
#include "me-tv-ui.h"
#include "thread.h"

#define METERS_POLL_INTERVAL 100000

class MetersDialog : public Gtk::Dialog
{
private:
		
	class MetersThread : public Thread
	{
	private:
		MetersDialog& meters_dialog;
	public:
		MetersThread(MetersDialog& dialog) : Thread("Meters"), meters_dialog(dialog) {}

		void run()
		{
			while (!is_terminated())
			{
				meters_dialog.update_meters();
				usleep(METERS_POLL_INTERVAL);
			}
		}
	};

	const Glib::RefPtr<Gtk::Builder>	builder;
	Gtk::ProgressBar*					progress_bar_signal_strength;
	Gtk::ProgressBar*					progress_bar_signal_noise;
	MetersThread						meters_thread;
	Dvb::Frontend&						frontend;

	void on_hide();

public:	
	MetersDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder> builder);
	~MetersDialog();

	static MetersDialog& create(Glib::RefPtr<Gtk::Builder> builder);
		
	void start();
	void stop();
	void update_meters();
	void set_meters(gdouble strength, gdouble snr);
};

#endif
