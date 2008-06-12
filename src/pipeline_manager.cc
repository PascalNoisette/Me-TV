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

#include "pipeline_manager.h"
#include "application.h"
#include "me-tv.h"
#include "sink.h"
#include "exception.h"

PipelineManager::~PipelineManager()
{
	while (!pipelines.empty())
	{
		remove(*(pipelines.begin()));
	}
}
	
Pipeline& PipelineManager::create(const Glib::ustring& name)
{
	Pipeline* pipeline = new Pipeline(name);
	g_debug("Pipeline created");
	pipelines.push_back(pipeline);
	g_debug("Pipeline added");
	return *pipeline;
}
	
Pipeline* PipelineManager::get_pipeline(const Glib::ustring& name)
{
	Pipeline* result = NULL;
	PipelineList::iterator iterator = pipelines.begin();
	while (iterator != pipelines.end() && result != NULL)
	{
		Pipeline* pipeline = *iterator;
		if (pipeline->get_name() == name)
		{
			result = pipeline;
		}
		iterator++;
	}
	return result;
}

void PipelineManager::remove(Pipeline* pipeline)
{
	if (pipeline == NULL)
	{
		throw Exception("Failed to remove pipeline: Pipeline was NULL");
	}
	
	g_debug("Stopping pipeline '%s'", pipeline->get_name().c_str());
	pipeline->stop();
	g_debug("Pipeline stopped");
	
	pipelines.remove(pipeline);
	g_debug("Pipeline removed");

	delete pipeline;
	g_debug("Pipeline destroyed");
}

Pipeline::Pipeline(const Glib::ustring& name)
{
	source = NULL;
	this->name = name;
}

Pipeline::~Pipeline()
{
	if (source != NULL)
	{
		delete source;
		source = NULL;
	}
	
	while (!sinks.empty())
	{
		Sink* sink = *(sinks.begin());
		sinks.pop_front();
	}
}

void Pipeline::set_source(const Channel& channel)
{
	if (source != NULL)
	{
		throw Exception("Failed to set channel source: A source has already been assigned to this pipeline");
	}

	source = new Source(packet_queue, channel);
}

void Pipeline::set_source(const Glib::ustring& mrl)
{
	if (source != NULL)
	{
		throw Exception("Failed to set mrl source: A source has already been assigned to this pipeline");
	}
	
	source = new Source(packet_queue, mrl);
}

void Pipeline::add_sink(Gtk::DrawingArea& drawing_area)
{
	sinks.push_back(new GtkAlsaSink(*this, drawing_area));
}

void Pipeline::add_sink(const Glib::ustring& mrl, const Glib::ustring& video_codec, const Glib::ustring& audio_codec)
{
	throw Exception("Not implemented");
}

void Pipeline::start()
{
	g_debug("Starting pipeline");
	get_source().start();
	SinkList::iterator iterator = sinks.begin();
	while (iterator != sinks.end())
	{
		Sink* sink = *iterator;
		sink->start();
		iterator++;
	}	
	g_debug("Pipeline started");
}

void Pipeline::stop()
{
	g_debug("Stopping pipeline");
	get_source().join(true);
	SinkList::iterator iterator = sinks.begin();
	while (iterator != sinks.end())
	{
		Sink* sink = *iterator;
		sink->join(true);
		iterator++;
	}
	g_debug("Pipeline stopped");
}

Source& Pipeline::get_source()
{
	if (source == NULL)
	{
		throw Exception(_("Source has not been set"));
	}
	return *source;
}

