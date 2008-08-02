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

#include "scheduled_recording_dialog.h"
#include "scheduled_recordings_dialog.h"
#include "main_window.h"
#include "application.h"

ScheduledRecordingDialog* ScheduledRecordingDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	ScheduledRecordingDialog* scheduled_recording_dialog = NULL;
	glade->get_widget_derived("dialog_scheduled_recording", scheduled_recording_dialog);
	return scheduled_recording_dialog;
}

ScheduledRecordingDialog::ScheduledRecordingDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::Dialog(cobject), glade(glade)
{
}

void ScheduledRecordingDialog::populate_controls()
{
	ChannelComboBox* channel_combo_box = NULL;
	const Profile& profile = get_application().get_profile_manager().get_current_profile();
	
	dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_description"))->set_text(UNKNOWN_TEXT);
	glade->get_widget_derived("combo_box_channel", channel_combo_box);
	channel_combo_box->load(profile.get_channels());
	
	Gtk::SpinButton* spinbutton_start_time_hour = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_start_time_hour"));
	Gtk::SpinButton* spinbutton_start_time_minute = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_start_time_minute"));
	Gtk::SpinButton* spinbutton_duration = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_duration"));
	
	time_t now = get_local_time();
	
	struct tm t;
	gmtime_r(&now, &t);
		
	spinbutton_start_time_hour->set_value(t.tm_hour);
	spinbutton_start_time_minute->set_value(t.tm_min);
	
	spinbutton_duration->set_value(30);
}

guint ScheduledRecordingDialog::run(EpgEvent& epg_event)
{
	MainWindow* main_window = MainWindow::create(glade);
	set_transient_for(*main_window);
	populate_controls();
	return Gtk::Dialog::run();
}

guint ScheduledRecordingDialog::run()
{
	ScheduledRecordingsDialog* scheduled_recordings_dialog = ScheduledRecordingsDialog::create(glade);
	set_transient_for(*scheduled_recordings_dialog);
	populate_controls();
	return Gtk::Dialog::run();
}
