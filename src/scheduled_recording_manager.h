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

#ifndef __SCHEDULED_RECORDING_MANAGER_H__
#define __SCHEDULED_RECORDING_MANAGER_H__

#include "scheduled_recording.h"
#include "data.h"
#include "channel.h"

typedef std::list<ScheduledRecording> ScheduledRecordingList;

class ScheduledRecordingManager {
private:
	Glib::Threads::RecMutex mutex;
	gboolean dirty;
	gboolean is_device_available(Glib::ustring const & device, ScheduledRecording const & scheduled_recording);
	void select_device(ScheduledRecording & scheduled_recording);
public:
	void initialise();
	ScheduledRecordingList scheduled_recordings;
	void load(Data::Connection & connection);
	void save(Data::Connection & connection);
	void action_after(guint action);
	void set_scheduled_recording(EpgEvent & epg_event);
	void set_scheduled_recording(ScheduledRecording & scheduled_recording);
	void remove_scheduled_recording(guint scheduled_recording_id);
	void remove_scheduled_recording(EpgEvent & epg_event);
	ScheduledRecording get_scheduled_recording(guint scheduled_recording_id);
	ScheduledRecordingList check_scheduled_recordings();
	guint is_recording(Channel const & channel);
	gboolean is_recording(EpgEvent const & epg_event);
};

#endif
