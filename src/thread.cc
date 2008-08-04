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

#include "thread.h"
#include "me-tv-ui.h"

Thread::Thread(const Glib::ustring& name)
{
	g_static_rec_mutex_init(mutex.gobj());
	terminated = true;
	thread = NULL;
	this->name = name;
	g_debug("Thread '%s' created", name.c_str());
}

Thread::~Thread()
{
	join(true);
}

void Thread::start()
{
	Glib::RecMutex::Lock lock(mutex);
	if (thread != NULL)
	{
		throw Exception("'" + name + "' thread has already been started");
	}
	
	terminated = false;
	thread = Glib::Thread::create(sigc::mem_fun(*this, &Thread::on_run), true);
	g_debug("Thread '%s' started", name.c_str());
}
	
void Thread::on_run()
{		
	TRY
	run();
	g_debug("Thread '%s' exited", name.c_str());
	THREAD_CATCH
}
	
void Thread::join(gboolean set_terminate)
{
	gboolean do_join = false;
	
	{
		Glib::RecMutex::Lock lock(mutex);
		if (thread != NULL)
		{
			if (set_terminate)
			{
				terminated = true;
				g_debug("Thread '%s' marked for termination", name.c_str());
			}
			
			do_join = true;
		}		
	}
	
	if (do_join)
	{
		g_debug("Thread '%s' waiting for join ...", name.c_str());
		thread->join();
		g_debug("Thread '%s' joined", name.c_str());

		Glib::RecMutex::Lock lock(mutex);
		thread = NULL;
		terminated = true;
	}
	
}
	
void Thread::terminate()
{
	Glib::RecMutex::Lock lock(mutex);
	terminated = true;
	g_debug("Thread '%s' marked for termination", name.c_str());
}
	
gboolean Thread::is_terminated()
{
	Glib::RecMutex::Lock lock(mutex);
	return terminated;
}
