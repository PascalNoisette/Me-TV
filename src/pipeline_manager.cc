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

class ChannelPipeline : public Pipeline
{
private:
	Channel channel;
		
	void create_source()
	{
		source = new Source(channel);
	}
public:
	ChannelPipeline(const Glib::ustring& name, const Channel& channel, Gtk::DrawingArea& drawing_area) :
		Pipeline(name, drawing_area), channel(channel) {}
};

class MrlPipeline : public Pipeline
{
private:
	Glib::ustring mrl;

	void create_source()
	{
		source = new Source(mrl);
	}
public:
	MrlPipeline(const Glib::ustring& name, const Glib::ustring& mrl, Gtk::DrawingArea& drawing_area) :
		Pipeline(name, drawing_area), mrl(mrl) {}
};

PipelineManager::PipelineManager()
{
	g_static_rec_mutex_init(mutex.gobj());
}

PipelineManager::~PipelineManager()
{
	while (!pipelines.empty())
	{
		remove(*(pipelines.begin()));
	}
}
	
Pipeline& PipelineManager::create(const Glib::ustring& name, const Glib::ustring& mrl, Gtk::DrawingArea& drawing_area)
{	
	if (find_pipeline(name) != NULL)
	{
		throw Exception("A pipeline with that name already exists");
	}
	
	Glib::RecMutex::Lock lock(mutex);

	Pipeline* pipeline = new MrlPipeline(name, mrl, drawing_area);
	g_debug("Pipeline created");
	pipelines.push_back(pipeline);
	g_debug("Pipeline added");
	return *pipeline;
}
	
Pipeline& PipelineManager::create(const Glib::ustring& name, const Channel& channel, Gtk::DrawingArea& drawing_area)
{	
	if (find_pipeline(name) != NULL)
	{
		throw Exception("A pipeline with that name already exists");
	}
	
	Glib::RecMutex::Lock lock(mutex);

	Pipeline* pipeline = new ChannelPipeline(name, channel, drawing_area);
	g_debug("Pipeline created");
	pipelines.push_back(pipeline);
	g_debug("Pipeline added");
	return *pipeline;
}

Pipeline* PipelineManager::find_pipeline(const Glib::ustring& name)
{
	Glib::RecMutex::Lock lock(mutex);
	Pipeline* result = NULL;
	PipelineList::iterator iterator = pipelines.begin();
	while (iterator != pipelines.end() && result == NULL)
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

Pipeline& PipelineManager::get_pipeline(const Glib::ustring& name)
{
	Pipeline* pipeline = find_pipeline(name);

	if (pipeline == NULL)
	{
		throw Exception(Glib::ustring::compose(_("Failed to find pipeline '%1'"), name));
	}
	
	return *pipeline;
}

void PipelineManager::remove(Pipeline* pipeline)
{
	Glib::RecMutex::Lock lock(mutex);
	if (pipeline == NULL)
	{
		throw Exception("Failed to remove pipeline: Pipeline was NULL");
	}
	
	pipeline->join(true);
	
	pipelines.remove(pipeline);
	g_debug("Pipeline removed");

	delete pipeline;
	g_debug("Pipeline destroyed");
}

Pipeline::Pipeline(const Glib::ustring& name, Gtk::DrawingArea& drawing_area) :
	Thread(Glib::ustring::compose(_("'%1' pipeline"), name)), drawing_area(drawing_area)
{
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

Source& Pipeline::get_source()
{
	if (source == NULL)
	{
		throw Exception(_("Source has not been set"));
	}
	return *source;
}

void Pipeline::run()
{
	AVPacket packet;
	gboolean first = true;
	
	g_debug("Starting pipeline");
	
	create_source();
	sinks.push_back(new Sink(*this, drawing_area));
	
	g_debug("Entering source thread loop");
	while (!is_terminated())
	{
		if (!source->read(&packet))
		{
			terminate();
		}
		else
		{			
			SinkList::iterator iterator = sinks.begin();
			while (iterator != sinks.end())
			{
				Sink* sink = *iterator;

				if (first)
				{
					sink->reset_timer();
				}

				sink->write(&packet);
				iterator++;
			}
			
			first = false;
		}

		av_free_packet(&packet);
	}
		
	g_debug("Pipeline thread finished");
}
