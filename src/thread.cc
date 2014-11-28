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

#include "thread.h"
#include "me-tv-ui.h"
#include "me-tv-i18n.h"

Thread::Thread(const Glib::ustring& thread_name, gboolean join_thread_on_destroy)
	: join_on_destroy(join_thread_on_destroy) {
	terminated = true;
	started = false;
	thread = NULL;
	name = thread_name;
	g_debug("Thread '%s' created", name.c_str());
}

void Thread::start() {
	Glib::Threads::RecMutex::Lock lock(mutex);
	if (thread != NULL) { throw Exception("'" + name + "'" + _(" thread has already been started")); }
	terminated = false;
	started = false;
	thread = Glib::Thread::create(sigc::mem_fun(*this, &Thread::on_run), true);
	while (!started) {
		g_debug("Waiting for '%s' to start", name.c_str());
		usleep(1000);
	}
	g_debug("Thread '%s' started", name.c_str());
}

void Thread::on_run() {
	started = true;
	run();
	terminated = true;
	g_debug("Thread '%s' exited", name.c_str());
}

void Thread::join(gboolean set_terminate) {
	gboolean do_join = false;
	{
		Glib::Threads::RecMutex::Lock lock(mutex);
		if (thread != NULL) {
			if (set_terminate) {
				terminated = true;
				g_debug("Thread '%s' marked for termination", name.c_str());
			}
			do_join = true;
		}
	}
	if (do_join) {
		g_debug("Thread '%s' waiting for join ...", name.c_str());
		thread->join();
		g_debug("Thread '%s' joined", name.c_str());
		Glib::Threads::RecMutex::Lock lock(mutex);
		thread = NULL;
		terminated = true;
	}
}

void Thread::terminate() {
	Glib::Threads::RecMutex::Lock lock(mutex);
	terminated = true;
	g_debug("Thread '%s' marked for termination", name.c_str());
}

gboolean Thread::is_started() {
	Glib::Threads::RecMutex::Lock lock(mutex);
	return (thread != NULL);
}
