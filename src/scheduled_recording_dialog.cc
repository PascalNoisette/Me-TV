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
	channel_combo_box = NULL;
	const Profile& profile = get_application().get_profile_manager().get_current_profile();
	
	date_edit_start_date = dynamic_cast<Gnome::UI::DateEdit*>(glade->get_widget("date_edit_start_date"));
	entry_description = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_description"));
	glade->get_widget_derived("combo_box_channel", channel_combo_box);
	channel_combo_box->load(profile.get_channels());

	spinbutton_start_time_hour = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_start_time_hour"));
	spinbutton_start_time_minute = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_start_time_minute"));
	spinbutton_duration = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_duration"));

	time_t now = get_local_time();
	struct tm t;
	gmtime_r(&now, &t);
	entry_description->set_text(UNKNOWN_TEXT);
	spinbutton_start_time_hour->set_value(t.tm_hour);
	spinbutton_start_time_minute->set_value(t.tm_min);	
	spinbutton_duration->set_value(30);
}

guint ScheduledRecordingDialog::run(Gtk::Window* transient_for, ScheduledRecording& scheduled_recording)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	time_t now = scheduled_recording.start_time;
	struct tm t;
	gmtime_r(&now, &t);
	entry_description->set_text(scheduled_recording.description);
	spinbutton_start_time_hour->set_value(t.tm_hour);
	spinbutton_start_time_minute->set_value(t.tm_min);
	spinbutton_duration->set_value(scheduled_recording.duration/60);
	return run(transient_for);
}

guint ScheduledRecordingDialog::run(Gtk::Window* transient_for, EpgEvent& epg_event)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	time_t now = epg_event.start_time;	
	struct tm t;
	gmtime_r(&now, &t);
	entry_description->set_text(epg_event.get_title());
	spinbutton_start_time_hour->set_value(t.tm_hour);
	spinbutton_start_time_minute->set_value(t.tm_min);
	spinbutton_duration->set_value(epg_event.duration/60);
	return run(transient_for);
}

guint ScheduledRecordingDialog::run(Gtk::Window* transient_for)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	guint response = Gtk::Dialog::run();
	
	if (response == Gtk::RESPONSE_OK)
	{
		Data data;
		ScheduledRecording scheduled_recording = get_scheduled_recording();
		data.replace_scheduled_recording(scheduled_recording);
	}
	
	return response;
}

ScheduledRecording ScheduledRecordingDialog::get_scheduled_recording()
{
	ScheduledRecording scheduled_recording;
	scheduled_recording.scheduled_recording_id = 0;
	scheduled_recording.description = entry_description->get_text();
	scheduled_recording.type = 0;
	scheduled_recording.channel_id = channel_combo_box->get_selected_channel_id();
	scheduled_recording.start_time = date_edit_start_date->get_time() +
		(spinbutton_start_time_hour->get_value() * 60 * 60) +
		(spinbutton_start_time_minute->get_value() * 60);
	scheduled_recording.duration = spinbutton_duration->get_value() * 60;
	return scheduled_recording;
}
