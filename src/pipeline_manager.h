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

#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include <gtkmm.h>
#include "thread.h"
#include "source.h"
#include "sink.h"
#include "packet_queue.h"

class Pipeline
{
private:
	Glib::ustring	name;
	Source*			source;
	SinkList		sinks;
	PacketQueue		packet_queue;
		
public:
	Pipeline(const Glib::ustring& name);
	~Pipeline();
	
	const Glib::ustring& get_name() const { return name; }
	void start();
	void stop();
	void set_source(const Channel& channel);
	void set_source(const Glib::ustring& mrl);
	void add_sink(Gtk::DrawingArea& drawing_area);
	void add_sink(const Glib::ustring& mrl, const Glib::ustring& video_codec, const Glib::ustring& audio_codec);
	PacketQueue& get_packet_queue() { return packet_queue; }
	Source& get_source();
	SinkList& get_sinks() { return sinks; }
};

typedef std::list<Pipeline*> PipelineList;

class PipelineManager
{
private:
	PipelineList			pipelines;
	Glib::StaticRecMutex	mutex;
public:
	PipelineManager();
	~PipelineManager();
	Pipeline& create(const Glib::ustring& name);
	Pipeline* find_pipeline(const Glib::ustring& name);
	Pipeline& get_pipeline(const Glib::ustring& name);
	void remove(Pipeline* pipeline);
};

#endif
