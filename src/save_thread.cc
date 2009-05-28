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

#include "save_thread.h"
#include "application.h"
#include "data.h"

SaveThread::SaveThread() : Thread("Save Thread", true)
{
}

void SaveThread::run()
{
	g_debug("Save thread running");
	Data::Connection connection(get_application().get_database_filename());
	get_application().scheduled_recording_manager.save(connection);
	get_application().channel_manager.save(connection);
	terminate();
	g_debug("Save thread finished");
}