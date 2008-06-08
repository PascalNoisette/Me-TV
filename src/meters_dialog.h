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

#ifndef __METERS_DIALOG_H__
#define __METERS_DIALOG_H__

#include <libgnomeuimm.h>
#include <libglademm.h>
#include "thread.h"

#define METERS_POLL_INTERVAL 1000000

class MetersDialog : public Gtk::Dialog
{
private:
	class MetersThread : public Thread
	{
	private:
		MetersDialog& meters_dialog;
	public:
		MetersThread(MetersDialog& meters_dialog) : meters_dialog(meters_dialog)
		{
		}

		void run()
		{
			while (!is_terminated())
			{
				meters_dialog.update_meters();
				usleep(METERS_POLL_INTERVAL);
			}
		}
	};

	const Glib::RefPtr<Gnome::Glade::Xml>& glade;
	Gtk::ProgressBar*	progress_bar_signal_strength;
	Gtk::ProgressBar*	progress_bar_signal_noise;
	Gtk::Label*			label_meters_device_name;
	MetersThread		meters_thread;
	Dvb::Frontend*		frontend;

public:	
	MetersDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
		Gtk::Dialog(cobject), glade(glade), meters_thread(*this)
	{
		progress_bar_signal_strength = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_strength"));
		progress_bar_signal_noise = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_noise"));
		label_meters_device_name = dynamic_cast<Gtk::Label*>(glade->get_widget("label_meters_device_name"));
		set_meters(0, 0);
	}

	~MetersDialog()
	{
		meters_thread.terminate();
		meters_thread.join();
	}

	void start(Dvb::Frontend& f)
	{
		frontend = &f;
		label_meters_device_name->set_label(frontend->get_frontend_info().name);
		meters_thread.start();
	}

	void update_meters()
	{
		if (frontend == NULL)
		{
			throw Exception(_("Frontend was not set"));
		}

		gdouble strength = frontend->get_signal_strength();
		gdouble snr = frontend->get_snr();
		
		usleep(METERS_POLL_INTERVAL);

		GdkLock gdk_lock;
		set_meters(strength, snr);
	}

	void set_meters(gdouble strength, gdouble snr)
	{
		progress_bar_signal_strength->set_fraction(strength);
		progress_bar_signal_noise->set_fraction(snr);
	}
};

#endif
