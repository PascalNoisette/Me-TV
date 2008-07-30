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
}

void ScheduledRecordingsDialog::on_button_scheduled_recordings_add_clicked()
{
	ScheduledRecordingDialog* scheduled_recordings_dialog = ScheduledRecordingDialog::create(glade);
	scheduled_recordings_dialog->run();
	scheduled_recordings_dialog->hide();
}
