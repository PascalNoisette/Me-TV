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

#include "scheduled_recordings_dialog.h"
#include "scheduled_recording_dialog.h"
#include "data.h"
#include "application.h"

ScheduledRecordingsDialog* ScheduledRecordingsDialog::create(Glib::RefPtr<Gnome::Glade::Xml> glade)
{
	ScheduledRecordingsDialog* scheduled_recordings_dialog = NULL;
	glade->get_widget_derived("dialog_scheduled_recordings", scheduled_recordings_dialog);
	return scheduled_recordings_dialog;
}

ScheduledRecordingsDialog::ScheduledRecordingsDialog(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade) :
	Gtk::Dialog(cobject), glade(glade)
{
	glade->connect_clicked("button_scheduled_recordings_add", sigc::mem_fun(*this, &ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked));
	tree_view_scheduled_recordings = dynamic_cast<Gtk::TreeView*>(glade->get_widget("tree_view_scheduled_recordings"));
	list_store = Gtk::ListStore::create(columns);
	tree_view_scheduled_recordings->set_model(list_store);
	tree_view_scheduled_recordings->append_column("Description", columns.column_description);
	tree_view_scheduled_recordings->append_column("Channel", columns.column_channel);
	tree_view_scheduled_recordings->append_column("Start Time", columns.column_start_time);
	tree_view_scheduled_recordings->append_column("Duration", columns.column_duration);
	update();
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked()
{
	ScheduledRecordingDialog* scheduled_recordings_dialog = ScheduledRecordingDialog::create(glade);
	scheduled_recordings_dialog->run(this);
	scheduled_recordings_dialog->hide();
	update();
}

void ScheduledRecordingsDialog::update()
{
	Data data;
	
	Profile& current_profile = get_application().get_profile_manager().get_current_profile();
	Glib::RefPtr<Gtk::TreeModel> tree_model = tree_view_scheduled_recordings->get_model();
	ScheduledRecordingList recordings = data.get_scheduled_recordings();
	for (ScheduledRecordingList::iterator i = recordings.begin(); i != recordings.end(); i++)
	{
		ScheduledRecording& scheduled_recording = *i;
		Gtk::TreeModel::Row row = *(list_store->append());
		row[columns.column_description] = scheduled_recording.description;
		row[columns.column_channel] = current_profile.get_channel(scheduled_recording.channel_id).name;
		row[columns.column_start_time] = scheduled_recording.get_start_time_text();
		row[columns.column_duration] = scheduled_recording.get_duration_text();
	}
}
