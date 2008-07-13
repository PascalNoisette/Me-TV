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
		Glib::RecMutex::Lock lock(mutex);
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
		Glib::RecMutex::Lock lock(mutex);
		source = new Source(mrl);
	}
public:
	MrlPipeline(const Glib::ustring& name, const Glib::ustring& mrl, Gtk::DrawingArea& drawing_area) :
		Pipeline(name, drawing_area), mrl(mrl) {}
};

PipelineManager::PipelineManager()
{
	g_static_rec_mutex_init(mutex.gobj());
//	Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &PipelineManager::on_timeout), 1);
}

PipelineManager::~PipelineManager()
{
	Glib::RecMutex::Lock lock(mutex);
	while (!pipelines.empty())
	{
		remove(*(pipelines.begin()));
	}
}

bool PipelineManager::on_timeout()
{
	PipelineList orpans;

	g_debug("Reaper entered");

	Glib::RecMutex::Lock lock(mutex);	

	PipelineList::iterator iterator = pipelines.begin();
	while (iterator != pipelines.end())
	{
		Pipeline* pipeline = *iterator;
		if (pipeline->is_terminated())
		{
			g_debug("Pipeline reaper found orphaned pipeline '%s'", pipeline->get_name().c_str());
			orpans.push_back(pipeline);
		}
		iterator++;
	}

	// Remove orphans
	gdk_threads_leave();
	iterator = orpans.begin();
	while (iterator != orpans.end())
	{
		Pipeline* pipeline = *iterator;
		remove(pipeline);
		g_debug("Pipeline reaper collected pipeline '%s'", pipeline->get_name().c_str());
		
		iterator++;
	}
	
	g_debug("Reaper exited");
	
	return true;
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
	
	{
		GdkUnlock gdk_unlock;
		pipeline->join(true);
	}
	
	pipelines.remove(pipeline);
	g_debug("Pipeline removed");

	delete pipeline;
	g_debug("Pipeline destroyed");
}

Pipeline::Pipeline(const Glib::ustring& name, Gtk::DrawingArea& drawing_area) :
	Thread(Glib::ustring::compose(_("'%1' pipeline"), name)), drawing_area(drawing_area)
{
	this->name = name;
	source = NULL;
	start_time = 0;
	g_static_rec_mutex_init(mutex.gobj());
}

Pipeline::~Pipeline()
{	
	Glib::RecMutex::Lock lock(mutex);
	
	while (!sinks.empty())
	{
		Sink* sink = *(sinks.begin());
		delete sink;
		sinks.pop_front();
	}

	if (source != NULL)
	{
		delete source;
		source = NULL;
	}
}

Source& Pipeline::get_source()
{
	Glib::RecMutex::Lock lock(mutex);
		
	if (source == NULL)
	{
		throw Exception(_("Source has not been set"));
	}
	return *source;
}

void Pipeline::run()
{
	AVPacket packet;
	
	g_debug("Starting pipeline");
	
	create_source();
//	sinks.push_back(new Sink(*this, "/home/michael/data.mpeg"));

	//sinks.push_back(new Sink(*this, drawing_area));
	
	g_debug("Entering source thread loop");
	start_time = get_elapsed();
	while (!is_terminated())
	{
		if (source->read(&packet))
		{
			SinkList::iterator iterator = sinks.begin();
			while (iterator != sinks.end())
			{
				Sink* sink = *iterator;
				sink->write(&packet);
				iterator++;
			}
			
			av_free_packet(&packet);
		}
		else
		{
			// Trash all the sinks
			gdk_threads_leave();
			while (!sinks.empty())
			{
				Sink* sink = *(sinks.begin());
				delete sink;
				sinks.pop_front();
			}
		}
	}
	g_debug("Pipeline thread finished");
}

guint Pipeline::get_duration()
{
	Glib::RecMutex::Lock lock(mutex);
	
	if (source == NULL)
	{
		throw Exception("Source is NULL");
	}
	
	return source->get_duration();
}

guint Pipeline::get_elapsed()
{
	guint elapsed = (av_gettime() - start_time) * AV_TIME_BASE;
	guint duration = get_duration();
	if (elapsed > duration)
	{
		elapsed = duration;
	}

	if (elapsed < 0)
	{
		elapsed = 0;
	}
	
	return elapsed;
}

