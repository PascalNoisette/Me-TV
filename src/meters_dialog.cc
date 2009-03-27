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

#include "meters_dialog.h"
#include "application.h"

MetersDialog* MetersDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	MetersDialog* meters_dialog = NULL;
	glade->get_widget_derived("dialog_meters", meters_dialog);
	return meters_dialog;
}

MetersDialog::MetersDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml) :
	Gtk::Dialog(cobject), glade(glade_xml), meters_thread(*this), frontend(get_application().device_manager.get_frontend())
{
	progress_bar_signal_strength = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_strength"));
	progress_bar_signal_noise = dynamic_cast<Gtk::ProgressBar*>(glade->get_widget("progress_bar_signal_noise"));
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
	gdouble bits16 = 65535;
	Glib::ustring signal_strength_text = Glib::ustring::compose(_("Signal Strength (%1%%)"), (guint)((strength/bits16)*100));
	Glib::ustring signal_noise_text = Glib::ustring::compose(_("S/N Ratio (%1%%)"), (guint)((snr/bits16)*100));
	progress_bar_signal_strength->set_text(signal_strength_text);
	progress_bar_signal_strength->set_fraction(strength/bits16);
	progress_bar_signal_noise->set_text(signal_noise_text);
	progress_bar_signal_noise->set_fraction(snr/bits16);
}

void MetersDialog::on_hide()
{
	stop();
	Widget::on_hide();
}

