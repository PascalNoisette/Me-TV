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

class Pipeline : public Thread
{
private:
	Glib::ustring		name;
	SinkList			sinks;
	Gtk::DrawingArea&	drawing_area;
		
	virtual void create_source() = 0;
	void run();
		
protected:
	Source*			source;

public:
	Pipeline(const Glib::ustring& name, Gtk::DrawingArea& drawing_area);
	~Pipeline();
		
	const Glib::ustring& get_name() const { return name; }
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
	Pipeline& create(const Glib::ustring& name, const Channel& channel, Gtk::DrawingArea& drawing_area);
	Pipeline& create(const Glib::ustring& name, const Glib::ustring& mrl, Gtk::DrawingArea& drawing_area);
	Pipeline* find_pipeline(const Glib::ustring& name);
	Pipeline& get_pipeline(const Glib::ustring& name);
	void remove(Pipeline* pipeline);
};

#endif
