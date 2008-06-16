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

#include "packet_queue.h"
#include "exception.h"

PacketQueue::PacketQueue()
{
	finished = false;
}
	
void PacketQueue::push(AVPacket* packet)
{
	while (get_size() > 100 && !finished)
	{
		usleep(100);
	}

	if (!finished)
	{
		Glib::RecMutex::Lock lock(mutex);	
		if (av_dup_packet(packet) < 0)
		{
			throw Exception("Failed to dup packet");
		}
		AVPacket* new_packet = new AVPacket();
		*new_packet = *packet;
		queue.push(new_packet);
	}
}

AVPacket* PacketQueue::pop()
{
	while (get_size() == 0 && !finished)
	{
		usleep(1000);
	}

	AVPacket* packet = NULL;
	if (!finished)
	{
		Glib::RecMutex::Lock lock(mutex);
		packet = queue.front();
		queue.pop();
	}
	return packet;
}

gsize PacketQueue::get_size()
{
	Glib::RecMutex::Lock lock(mutex);
	return queue.size();
}
	
gboolean PacketQueue::is_empty()
{
	Glib::RecMutex::Lock lock(mutex);
	return queue.empty();
}
	
void PacketQueue::finish()
{
	finished = true;
}

gboolean PacketQueue::is_finished()
{
	return finished && is_empty();
}
