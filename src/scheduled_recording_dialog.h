/*
 * Copyright (C) 2010 Michael Lamothe
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

#ifndef __SCHEDULED_RECORDING_DIALOG_H__
#define __SCHEDULED_RECORDING_DIALOG_H__

#include "epg_event.h"
#include "me-tv-ui.h"
#include "scheduled_recording.h"

class ScheduledRecordingDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gtk::Builder> builder;

	Gtk::Calendar*		calendar_start_time_date;
	Gtk::SpinButton*	spin_button_start_time_hour;
	Gtk::SpinButton*	spin_button_start_time_minute;
	Gtk::SpinButton*	spin_button_duration;
	Gtk::Entry*			entry_description;
	Gtk::ComboBox*		recurring_combo_box;
	Gtk::ComboBox*		action_after_combo_box;
	ChannelComboBox*	channel_combo_box;
	guint				scheduled_recording_id;

	void set_date_time(time_t t);

public:
	ScheduledRecordingDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

	static ScheduledRecordingDialog& create(Glib::RefPtr<Gtk::Builder> builder);

	gint run(Gtk::Window* transient_for, ScheduledRecording& scheduled_recording);
	gint run(Gtk::Window* transient_for, EpgEvent& epg_event);
	gint run(Gtk::Window* transient_for, gboolean populate_default = true);
		
	ScheduledRecording get_scheduled_recording();
};

#endif
