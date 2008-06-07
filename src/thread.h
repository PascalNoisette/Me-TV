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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <glibmm.h>
#include "exception.h"

class Thread
{
private:
	gboolean		terminated;
	Glib::Thread*	thread;
public:
	Thread()
	{
		terminated = true;
		thread = NULL;
	}
	
	void start()
	{
		if (thread != NULL)
		{
			throw Exception("Thread has already been started");
		}
		
		terminated = false;
		thread = Glib::Thread::create(sigc::mem_fun(*this, &Thread::on_run), true);
	}
		
	virtual void run() = 0;
		
	void on_run()
	{		
		try
		{
			run();
		}
		CATCH
	}
		
	void join(gboolean terminate = false)
	{
		if (thread != NULL)
		{
			if (terminate)
			{
				terminated = true;
			}
			
			thread->join();
		}
	}
		
	void terminate()
	{
		terminated = true;
	}
		
	gboolean is_terminated() const
	{
		return terminated;
	}
};

#endif
