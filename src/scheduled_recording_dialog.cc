/*
 * Copyright (C) 2011 Michael Lamothe
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
#include "application.h"
#include <iomanip>

ScheduledRecordingDialog& ScheduledRecordingDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ScheduledRecordingDialog* scheduled_recording_dialog = NULL;
	builder->get_widget_derived("dialog_scheduled_recording", scheduled_recording_dialog);
	return *scheduled_recording_dialog;
}

ScheduledRecordingDialog::ScheduledRecordingDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
	channel_combo_box = NULL;
	recurring_combo_box = NULL;
	action_after_combo_box = NULL;
	scheduled_recording_id = 0;
	
	builder->get_widget("entry_description", entry_description);
	builder->get_widget_derived("combo_box_channel", channel_combo_box);
	channel_combo_box->load(channel_manager.get_channels());
	builder->get_widget("calendar_start_time_date", calendar_start_time_date);
	builder->get_widget("spin_button_start_time_hour", spin_button_start_time_hour);
	builder->get_widget("spin_button_start_time_minute", spin_button_start_time_minute);
	builder->get_widget("spinbutton_duration", spin_button_duration);
	builder->get_widget("combo_box_recurring", recurring_combo_box);
	builder->get_widget("combo_box_action_after", action_after_combo_box);
}

void ScheduledRecordingDialog::set_date_time(time_t t)
{
	if (t == 0)
	{
		t = time(NULL);
	}
	else
	{
		t = convert_to_utc_time(t);
	}
	
	struct tm* start_time = localtime(&t);
	spin_button_start_time_hour->set_value(start_time->tm_hour);
	spin_button_start_time_minute->set_value(start_time->tm_min);

	calendar_start_time_date->select_day(start_time->tm_mday);
	calendar_start_time_date->select_month(start_time->tm_mon, 1900 + start_time->tm_year);
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
	set_date_time(convert_to_local_time((time_t)scheduled_recording.start_time));
	spin_button_duration->set_value(scheduled_recording.duration/60);
	recurring_combo_box->set_active(scheduled_recording.recurring_type);
	action_after_combo_box->set_active(scheduled_recording.action_after);

	return run(transient_for, false);
}

gint ScheduledRecordingDialog::run(Gtk::Window* transient_for, EpgEvent& epg_event)
{
	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	scheduled_recording_id = 0;

	Application& application = get_application();
	guint before = configuration_manager.get_int_value("record_extra_before");
	guint after = configuration_manager.get_int_value("record_extra_after");

	channel_combo_box->set_selected_channel_id(epg_event.channel_id);
	entry_description->set_text(epg_event.get_title());
	set_date_time((time_t)epg_event.start_time - (before * 60));
	spin_button_duration->set_value((epg_event.duration/60) + before + after);
	recurring_combo_box->set_active(0);
	action_after_combo_box->set_active(0);
	
	return run(transient_for, false);
}

gint ScheduledRecordingDialog::run(Gtk::Window* transient_for, gboolean populate_default)
{
	gint dialog_response = Gtk::RESPONSE_CANCEL;

	if (transient_for != NULL)
	{
		set_transient_for(*transient_for);
	}
	
	if (populate_default)
	{	
		scheduled_recording_id = 0;	
		Channel& channel = stream_manager.get_display_channel();		
		channel_combo_box->set_selected_channel_id(channel.channel_id);
		entry_description->set_text(_("Unknown description"));
		recurring_combo_box->set_active(0);
		action_after_combo_box->set_active(0);
		set_date_time(0);

		spin_button_duration->set_value(30);
	}
	
	dialog_response = Gtk::Dialog::run();
	hide();

	if (dialog_response == Gtk::RESPONSE_OK)
	{
		g_debug("Pressed OK on scheduled recording dialog");
		ScheduledRecording scheduled_recording = get_scheduled_recording();
		g_debug("Got SR details for '%s'", scheduled_recording.description.c_str());
		scheduled_recording_manager.set_scheduled_recording(scheduled_recording);
	}
	signal_update();
		
	return dialog_response;
}

ScheduledRecording ScheduledRecordingDialog::get_scheduled_recording()
{
	Glib::Date date;
	struct tm start_time;

	calendar_start_time_date->get_date(date);
	date.to_struct_tm(start_time);
	start_time.tm_hour = spin_button_start_time_hour->get_value();
	start_time.tm_min = spin_button_start_time_minute->get_value();
	
	ScheduledRecording scheduled_recording;
	scheduled_recording.scheduled_recording_id	= scheduled_recording_id;
	scheduled_recording.description				= entry_description->get_text();
	scheduled_recording.recurring_type			= recurring_combo_box->get_active_row_number();
	scheduled_recording.action_after			= action_after_combo_box->get_active_row_number();
	scheduled_recording.channel_id				= channel_combo_box->get_selected_channel_id();
	scheduled_recording.start_time				= mktime(&start_time);
	scheduled_recording.duration				= (int)spin_button_duration->get_value() * 60;
	scheduled_recording.device					= "";
	
	return scheduled_recording;
}
