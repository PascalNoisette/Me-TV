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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <glibmm.h>
#include "exception.h"

class Thread {
private:
	gboolean terminated;
	Glib::Thread * thread;
	Glib::Threads::RecMutex mutex;
	Glib::ustring name;
	gboolean join_on_destroy;
	gboolean started;
	void on_run();
public:
	Thread(Glib::ustring const & name, gboolean join_on_destroy = true);
	virtual ~Thread() { if (join_on_destroy) { join(true); } }
	virtual void run() = 0;
	void start();
	gboolean is_started();
	void terminate();
	gboolean is_terminated() { return terminated; }
	void join(gboolean set_terminate = false);
};

#endif
