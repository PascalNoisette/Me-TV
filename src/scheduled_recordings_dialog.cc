/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scheduled_recordings_dialog.h"
#include "scheduled_recording_dialog.h"
#include "application.h"

ScheduledRecordingsDialog & ScheduledRecordingsDialog::create(Glib::RefPtr<Gtk::Builder> builder) {
	ScheduledRecordingsDialog * scheduled_recordings_dialog = NULL;
	builder->get_widget_derived("dialog_scheduled_recordings", scheduled_recordings_dialog);
	return *scheduled_recordings_dialog;
}

ScheduledRecordingsDialog::ScheduledRecordingsDialog(BaseObjectType * cobject, Glib::RefPtr<Gtk::Builder> const & builder)
	: Gtk::Dialog(cobject), builder(builder) {
	Gtk::Button* button = NULL;
	builder->get_widget("button_scheduled_recordings_add", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked));
	builder->get_widget("button_scheduled_recordings_delete", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_button_scheduled_recordings_delete_clicked));
	builder->get_widget("button_scheduled_recordings_edit", button);
	button->signal_clicked().connect(sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_button_scheduled_recordings_edit_clicked));
	builder->get_widget("tree_view_scheduled_recordings", tree_view_scheduled_recordings);
	tree_view_scheduled_recordings->signal_row_activated().connect(sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_row_activated));
	list_store = Gtk::ListStore::create(columns);
	tree_view_scheduled_recordings->set_model(list_store);
 	tree_view_scheduled_recordings->append_column(_("Description"), columns.column_description);
	tree_view_scheduled_recordings->append_column(_("Channel"), columns.column_channel);
	tree_view_scheduled_recordings->append_column(_("Start Time"), columns.column_start_time);
	tree_view_scheduled_recordings->append_column(_("Duration"), columns.column_duration);
	tree_view_scheduled_recordings->append_column(_("Record"),columns.column_recurring_type);
	tree_view_scheduled_recordings->append_column(_("After"),columns.column_action_after);
	tree_view_scheduled_recordings->append_column(_("Device"), columns.column_device);
	list_store->set_sort_column(columns.column_sort, Gtk::SORT_ASCENDING);
}

guint ScheduledRecordingsDialog::get_selected_scheduled_recording_id() {
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scheduled_recordings->get_selection();
	if (selection->count_selected_rows() == 0) {
		throw Exception(_("No scheduled recording selected"));
	}
	Gtk::TreeModel::Row row = *(selection->get_selected());
	return row[columns.column_scheduled_recording_id];
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked() {
	ScheduledRecordingDialog& scheduled_recordings_dialog = ScheduledRecordingDialog::create(builder);
	scheduled_recordings_dialog.run(this);
	scheduled_recordings_dialog.hide();
	update();
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_delete_clicked() {
	scheduled_recording_manager.remove_scheduled_recording(
	get_selected_scheduled_recording_id());
	update();
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_edit_clicked() {
	show_scheduled_recording(get_selected_scheduled_recording_id());
	update();
}

void ScheduledRecordingsDialog::update() {
	list_store->clear();
	for (auto && scheduled_recording: scheduled_recording_manager.scheduled_recordings) {
		Gtk::TreeModel::Row row = *(list_store->append());
		row[columns.column_sort] = scheduled_recording.start_time;
		row[columns.column_scheduled_recording_id] = scheduled_recording.scheduled_recording_id;
		row[columns.column_description] = scheduled_recording.description;
		row[columns.column_channel] = channel_manager.get_channel_by_id(scheduled_recording.channel_id).name;
		row[columns.column_start_time] = scheduled_recording.get_start_time_text();
		row[columns.column_duration] = scheduled_recording.get_duration_text();
		row[columns.column_device] = scheduled_recording.device;
		switch (scheduled_recording.recurring_type) {
			case 1: row[columns.column_recurring_type] = "Every day";break;
			case 2: row[columns.column_recurring_type] = "Every week";break;
			case 3: row[columns.column_recurring_type] = "Every weekday";break;
			default: row[columns.column_recurring_type] = "Once";break;
		}
		switch (scheduled_recording.action_after) {
			case 1: row[columns.column_action_after] = "Close Me-tv";break;
			case 2: row[columns.column_action_after] = "Shutdown";break;
			default: row[columns.column_action_after] = "Do Nothing";break;
		}
	}
}

void ScheduledRecordingsDialog::show_scheduled_recording(guint scheduled_recording_id) {
	ScheduledRecording scheduled_recording = scheduled_recording_manager.get_scheduled_recording(scheduled_recording_id);
	ScheduledRecordingDialog & scheduled_recording_dialog = ScheduledRecordingDialog::create(builder);
	scheduled_recording_dialog.run(this, scheduled_recording);
	scheduled_recording_dialog.hide();
}

void ScheduledRecordingsDialog::on_row_activated(Gtk::TreeModel::Path const & tree_model_path, Gtk::TreeViewColumn * column) {
	show_scheduled_recording(get_selected_scheduled_recording_id());
	update();
}

void ScheduledRecordingsDialog::on_show() {
	update();
	Widget::on_show();
}
