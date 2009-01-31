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

ScheduledRecordingDialog::ScheduledRecordingDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml) :
	Gtk::Dialog(cobject), glade(glade_xml)
{
	channel_combo_box = NULL;
	scheduled_recording_id = 0;
	const Profile& profile = get_application().get_profile_manager().get_current_profile();
	
	date_edit_start_time = dynamic_cast<Gnome::UI::DateEdit*>(glade->get_widget("date_edit_start_time"));
	entry_description = dynamic_cast<Gtk::Entry*>(glade->get_widget("entry_description"));
	glade->get_widget_derived("combo_box_channel", channel_combo_box);
	channel_combo_box->load(profile.get_channels());
	spinbutton_duration = dynamic_cast<Gtk::SpinButton*>(glade->get_widget("spinbutton_duration"));

	if (get_application().get_boolean_configuration_value("use_24_hour_workaround"))
	{
		date_edit_start_time->set_flags(date_edit_start_time->get_flags() | Gnome::UI::DATE_EDIT_24_HR);
	}
}

gint ScheduledRecordingDialog::run(Gtk::Window* transient_for, ScheduledRecording& scheduled_recording)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	channel_combo_box->set_selected_channel_id(scheduled_recording.channel_id);
	scheduled_recording_id = scheduled_recording.scheduled_recording_id;
	entry_description->set_text(scheduled_recording.description);
	date_edit_start_time->set_time(scheduled_recording.start_time);
	spinbutton_duration->set_value(scheduled_recording.duration/60);
	return run(transient_for, false);
}

gint ScheduledRecordingDialog::run(Gtk::Window* transient_for, EpgEvent& epg_event)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	Application& application = get_application();
	guint before = application.get_int_configuration_value("record_extra_before");
	guint after = application.get_int_configuration_value("record_extra_after");

	channel_combo_box->set_selected_channel_id(epg_event.channel_id);
	entry_description->set_text(epg_event.get_title());
	date_edit_start_time->set_time(convert_to_utc_time(epg_event.start_time) - (before * 60));
	spinbutton_duration->set_value((epg_event.duration/60) + before + after);
	return run(transient_for, false);
}

gint ScheduledRecordingDialog::run(Gtk::Window* transient_for, gboolean populate_default)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	if (populate_default)
	{
		entry_description->set_text(_("Unknown description"));
		date_edit_start_time->set_time(time(NULL));
		spinbutton_duration->set_value(30);
	}
	
	gint dialog_response = Gtk::Dialog::run();
	hide();
	
	if (dialog_response == Gtk::RESPONSE_OK)
	{
		Data data;
		ScheduledRecording scheduled_recording = get_scheduled_recording();
		data.replace_scheduled_recording(scheduled_recording);
		get_application().check_scheduled_recordings(data);
	}
	
	return dialog_response;
}

ScheduledRecording ScheduledRecordingDialog::get_scheduled_recording()
{
	ScheduledRecording scheduled_recording;
	scheduled_recording.scheduled_recording_id = scheduled_recording_id;
	scheduled_recording.description = entry_description->get_text();
	scheduled_recording.type = 0;
	scheduled_recording.channel_id = channel_combo_box->get_selected_channel_id();
	scheduled_recording.start_time = date_edit_start_time->get_time();
	scheduled_recording.duration = spinbutton_duration->get_value() * 60;
	return scheduled_recording;
}
