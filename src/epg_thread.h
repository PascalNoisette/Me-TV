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

#ifndef __EPG_THREAD_H__
#define __EPG_THREAD_H__

#include "thread.h"
#include "dvb_frontend.h"

class EpgThread: public Thread {
private:
	guint last_update_time;
	Dvb::Frontend & frontend;
public:
	EpgThread(Dvb::Frontend & frontend);
	void run();
	guint get_last_epg_update_time() const { return last_update_time; }
};

#endif
