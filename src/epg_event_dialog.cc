/*
 * Copyright (C) 2011 Michael Lamothe
 * Copyright © 2014  Russel Winder
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

#include "epg_event_dialog.h"
#include "me-tv.h"
#include "application.h"
#include "main_window.h"
#include "scheduled_recording_dialog.h"

EpgEventDialog & EpgEventDialog::create(Glib::RefPtr<Gtk::Builder> builder) {
	EpgEventDialog * dialog_epg_event = NULL;
	builder->get_widget_derived("dialog_epg_event", dialog_epg_event);
	return *dialog_epg_event;
}

EpgEventDialog::EpgEventDialog(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder)
	: Gtk::Dialog(cobject), builder(builder) {
}

void EpgEventDialog::show_epg_event(EpgEvent & epg_event) {
	EpgEventText const & epg_event_text = epg_event.get_default_text();
	time_t end_time = epg_event.start_time + epg_event.duration;
	time_t now = get_local_time();
	Glib::ustring information = Glib::ustring::compose(
	    	"<b>%1</b>\n<b><i>%2</i></b>\n<i>%4 (%5)</i>\n\n%3",
	    	encode_xml(epg_event_text.title),
	    	epg_event_text.description.empty() ? "" : encode_xml(epg_event_text.subtitle),
	    	epg_event_text.description.empty() ? encode_xml(epg_event_text.subtitle) : encode_xml(epg_event_text.description),
		    epg_event.get_start_time_text() + " - " + get_local_time_text(convert_to_utc_time(end_time), "%H:%M"),
	    	epg_event.get_duration_text());
	Gtk::Label * label_epg_event_information = NULL;
	builder->get_widget("label_epg_event_information", label_epg_event_information);
	label_epg_event_information->set_label(information);
	gboolean is_scheduled = scheduled_recording_manager.is_recording(epg_event);
	Gtk::HBox*  hbox_epg_event_dialog_scheduled = NULL;
	builder->get_widget("hbox_epg_event_dialog_scheduled", hbox_epg_event_dialog_scheduled);
	hbox_epg_event_dialog_scheduled->property_visible() = is_scheduled;
	Gtk::Button * button_epg_event_dialog_record = NULL;
	builder->get_widget("button_epg_event_dialog_record", button_epg_event_dialog_record);
	button_epg_event_dialog_record->property_visible() = !is_scheduled;
	Gtk::Button * button_epg_event_dialog_watch_now = NULL;
	builder->get_widget("button_epg_event_dialog_watch_now", button_epg_event_dialog_watch_now);
	button_epg_event_dialog_watch_now->property_visible() = epg_event.start_time <= now && now <= end_time;
	Gtk::Button * button_epg_event_dialog_view_schedule = NULL;
	builder->get_widget("button_epg_event_dialog_view_schedule", button_epg_event_dialog_view_schedule);
	button_epg_event_dialog_view_schedule->property_visible() = is_scheduled;
	gint result = run();
	hide();
	switch(result) {
	case 0: // Close
		break;
	case 1: { // Record
			ScheduledRecordingDialog& scheduled_recording_dialog = ScheduledRecordingDialog::create(builder);
			scheduled_recording_dialog.run(MainWindow::create(builder), epg_event);
			scheduled_recording_dialog.hide();
			signal_update();
		}
		break;
	case 2: // Record
		action_scheduled_recordings->activate();
		break;
	case 3: // Watch Now
		signal_start_display(epg_event.channel_id);
		break;
	default:
		break;
	}
}
