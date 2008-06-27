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
#include "me-tv-ui.h"

#define METERS_POLL_INTERVAL 100000

class MetersDialog : public Gtk::Dialog
{
private:
	class MetersThread : public Thread
	{
	private:
		MetersDialog& meters_dialog;
	public:
		MetersThread(MetersDialog& meters_dialog) : Thread("Meters"), meters_dialog(meters_dialog) {}

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
	ComboBoxFrontend*	combo_box_meters_device_name;
	MetersThread		meters_thread;
	Dvb::Frontend*		frontend;

public:	
	MetersDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
		Gtk::Dialog(cobject), glade(glade), meters_thread(*this)
	{
		progress_bar_signal_strength = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_strength"));
		progress_bar_signal_noise = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_noise"));
		glade->get_widget_derived("combo_box_meters_device_name", combo_box_meters_device_name);
		glade->connect_clicked("button_meters_close", sigc::mem_fun(*this, &Gtk::Widget::hide));

		set_meters(0, 0);
	}

	~MetersDialog()
	{
		stop();
	}

	void start()
	{
		frontend = &combo_box_meters_device_name->get_selected_frontend();
		meters_thread.start();
	}
		
	void stop()
	{
		gdk_threads_leave();
		meters_thread.join(true);
		gdk_threads_enter();
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
		gdouble bits16 = 1 << 16;
		progress_bar_signal_strength->set_fraction(strength/bits16);
		progress_bar_signal_noise->set_fraction(snr/bits16);
	}

	void on_hide()
	{
		stop();
		Widget::on_hide();
	}
};

#endif
