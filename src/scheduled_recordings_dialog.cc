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

#include "scheduled_recordings_dialog.h"
#include "scheduled_recording_dialog.h"
#include "application.h"

ScheduledRecordingsDialog& ScheduledRecordingsDialog::create(Glib::RefPtr<Gtk::Builder> builder)
{
	ScheduledRecordingsDialog* scheduled_recordings_dialog = NULL;
	builder->get_widget_derived("dialog_scheduled_recordings", scheduled_recordings_dialog);
	return *scheduled_recordings_dialog;
}

ScheduledRecordingsDialog::ScheduledRecordingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) :
	Gtk::Dialog(cobject), builder(builder)
{
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
	tree_view_scheduled_recordings->append_column(_("Device"), columns.column_device);
	
	list_store->set_sort_column(columns.column_sort, Gtk::SORT_ASCENDING);
}

guint ScheduledRecordingsDialog::get_selected_scheduled_recording_id()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_scheduled_recordings->get_selection();	
	if (selection->count_selected_rows() == 0)
	{
		throw Exception(_("No scheduled recording selected"));
	}
	
	Gtk::TreeModel::Row row = *(selection->get_selected());

	return row[columns.column_scheduled_recording_id];
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked()
{
	TRY
	ScheduledRecordingDialog& scheduled_recordings_dialog = ScheduledRecordingDialog::create(builder);
	scheduled_recordings_dialog.run(this);
	scheduled_recordings_dialog.hide();
	update();
	CATCH
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_delete_clicked()
{
	TRY
	get_application().scheduled_recording_manager.remove_scheduled_recording(
		get_selected_scheduled_recording_id());
	update();
	CATCH
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_edit_clicked()
{
	TRY
	show_scheduled_recording(get_selected_scheduled_recording_id());
	CATCH
}

void ScheduledRecordingsDialog::update()
{
	ChannelManager& channel_manager = get_application().channel_manager;
	list_store->clear();
	ScheduledRecordingList& scheduled_recordings = get_application().scheduled_recording_manager.scheduled_recordings;
	for (ScheduledRecordingList::iterator i = scheduled_recordings.begin(); i != scheduled_recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		Gtk::TreeModel::Row row = *(list_store->append());
		row[columns.column_sort]					= scheduled_recording.start_time;
		row[columns.column_scheduled_recording_id]	= scheduled_recording.scheduled_recording_id;
		row[columns.column_description]				= scheduled_recording.description;
		row[columns.column_channel]					= channel_manager.get_channel_by_id(scheduled_recording.channel_id).name;
		row[columns.column_start_time]				= scheduled_recording.get_start_time_text();
		row[columns.column_duration]				= scheduled_recording.get_duration_text();
		row[columns.column_device]					= scheduled_recording.device;
	}
}

void ScheduledRecordingsDialog::show_scheduled_recording(guint scheduled_recording_id)
{
	ScheduledRecording scheduled_recording = get_application().scheduled_recording_manager.get_scheduled_recording(scheduled_recording_id);

	ScheduledRecordingDialog& scheduled_recording_dialog = ScheduledRecordingDialog::create(builder);
	scheduled_recording_dialog.run(this, scheduled_recording);
	scheduled_recording_dialog.hide();

	update();
}

void ScheduledRecordingsDialog::on_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column)
{
	TRY
	show_scheduled_recording(get_selected_scheduled_recording_id());
	CATCH
}

void ScheduledRecordingsDialog::on_show()
{
	update();
	Widget::on_show();
}
