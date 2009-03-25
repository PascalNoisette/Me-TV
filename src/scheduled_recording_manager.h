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

#ifndef __SCHEDULED_RECORDING_MANAGER_H__
#define __SCHEDULED_RECORDING_MANAGER_H__

#include "scheduled_recording.h"

typedef std::list<ScheduledRecording> ScheduledRecordingList;

class ScheduledRecordingManager
{
private:
	Glib::StaticRecMutex mutex;
public:
	ScheduledRecordingManager();
		
	void load();
	void save();
	void add_scheduled_recording(const ScheduledRecording& scheduled_recording);
		
	ScheduledRecordingList scheduled_recordings;
};

#endif
