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

#include "meters_dialog.h"
#include "application.h"

MetersDialog::MetersDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::Dialog(cobject), glade(glade), meters_thread(*this), frontend(get_application().get_device_manager().get_frontend())
{
	progress_bar_signal_strength = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_strength"));
	progress_bar_signal_noise = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_noise"));
	glade->get_widget_derived("combo_box_meters_device_name", combo_box_meters_device_name);
	glade->connect_clicked("button_meters_close", sigc::mem_fun(*this, &Gtk::Widget::hide));
	set_meters(0, 0);
}

MetersDialog::~MetersDialog()
{
	stop();
}

void MetersDialog::start()
{
	meters_thread.start();
}
	
void MetersDialog::stop()
{
	gdk_threads_leave();
	meters_thread.join(true);
	gdk_threads_enter();
}

void MetersDialog::update_meters()
{
	gdouble strength = frontend.get_signal_strength();
	gdouble snr = frontend.get_snr();
	
	usleep(METERS_POLL_INTERVAL);

	GdkLock gdk_lock;
	set_meters(strength, snr);
}

void MetersDialog::set_meters(gdouble strength, gdouble snr)
{
	gdouble bits16 = 1 << 16;
	progress_bar_signal_strength->set_fraction(strength/bits16);
	progress_bar_signal_noise->set_fraction(snr/bits16);
}

void MetersDialog::on_hide()
{
	stop();
	Widget::on_hide();
}

