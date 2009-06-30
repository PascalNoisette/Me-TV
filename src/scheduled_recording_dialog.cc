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
	scheduled_recording_id = 0;
	
	builder->get_widget_derived("combo_box_start_time_hour", combo_box_start_time_hour);
	builder->get_widget_derived("combo_box_start_time_minute", combo_box_start_time_minute);
	builder->get_widget("calendar_start_time_date", calendar_start_time_date);

	builder->get_widget("entry_description", entry_description);
	builder->get_widget_derived("combo_box_channel", channel_combo_box);
	channel_combo_box->load(get_application().channel_manager.get_channels());
	builder->get_widget("spinbutton_duration", spinbutton_duration);

	for (guint hour = 0; hour < 24; hour++)
	{
		Glib::ustring text = Glib::ustring::format(hour);
		combo_box_start_time_hour->append_text(text);
	}

	for (guint minute = 0; minute < 60; minute++)
	{
		Glib::ustring text = Glib::ustring::format(std::setfill(L'0'), std::setw(2), minute);
		combo_box_start_time_minute->append_text(text);
	}
}

void ScheduledRecordingDialog::set_date_time(time_t t)
{
	if (t != 0)
	{
		t = convert_to_utc_time(t);
	}
	
	struct tm* start_time = localtime(t == 0 ? NULL : &t);
	combo_box_start_time_hour->set_active(start_time->tm_hour);
	combo_box_start_time_minute->set_active(start_time->tm_min);

	calendar_start_time_date->property_day() = start_time->tm_mday;
	calendar_start_time_date->property_month() = start_time->tm_mon;
	calendar_start_time_date->property_year() = 1900 + start_time->tm_year;
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
	set_date_time((time_t)scheduled_recording.start_time);
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
	set_date_time((time_t)(epg_event.start_time - (before * 60)));
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
		Channel* channel = get_application().channel_manager.get_display_channel();
		if (channel == NULL)
		{
			throw Exception(_("Failed to create scheduled recording dialog: No display channel"));
		}
		
		channel_combo_box->set_selected_channel_id(channel->channel_id);
		entry_description->set_text(_("Unknown description"));

		set_date_time(0);

		spinbutton_duration->set_value(30);
	}
	
	gint dialog_response = Gtk::Dialog::run();
	hide();
	
	if (dialog_response == Gtk::RESPONSE_OK)
	{
		ScheduledRecording scheduled_recording = get_scheduled_recording();
		get_application().scheduled_recording_manager.add_scheduled_recording(scheduled_recording);
	}
	
	return dialog_response;
}

ScheduledRecording ScheduledRecordingDialog::get_scheduled_recording()
{
	Glib::Date date;
	struct tm start_time;

	calendar_start_time_date->get_date(date);
	date.to_struct_tm(start_time);
	start_time.tm_hour = combo_box_start_time_hour->get_active();
	start_time.tm_min = combo_box_start_time_minute->get_active();
	
	ScheduledRecording scheduled_recording;
	scheduled_recording.scheduled_recording_id	= scheduled_recording_id;
	scheduled_recording.description				= entry_description->get_text();
	scheduled_recording.type					= 0;
	scheduled_recording.channel_id				= channel_combo_box->get_selected_channel_id();
	scheduled_recording.start_time				= mktime(&start_time);
	scheduled_recording.duration				= (int)spinbutton_duration->get_value() * 60;
	scheduled_recording.device					= get_application().device_manager.get_frontend().get_path();
	return scheduled_recording;
}
