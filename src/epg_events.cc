/*
 * Copyright (C) 2009 Michael Lamothe
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

#include "epg_events.h"
#include "me-tv.h"
#include "data.h"
#include "application.h"

EpgEvents::EpgEvents()
{
	g_static_rec_mutex_init(mutex.gobj());
}

EpgEvents::~EpgEvents()
{
}

gboolean EpgEvents::exists(const EpgEvent& epg_event)
{
	gboolean result = false;
	
	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::const_iterator i = list.begin(); i != list.end() && result == false; i++)
	{
		if (epg_event.event_id == (*i).event_id)
		{
			result = true;
		}
	}
	
	return result;
}

bool sort_function(const EpgEvent& a, const EpgEvent& b)
{
	return a.start_time < b.start_time;
}

gboolean EpgEvents::add_epg_event(const EpgEvent& epg_event)
{
	Glib::RecMutex::Lock lock(mutex);
	gboolean event_exists = exists(epg_event);
	if (!event_exists)
	{
		list.push_back(epg_event);
		list.sort(sort_function);
		g_debug("EPG Event %d (%s) added", epg_event.event_id, epg_event.get_title().c_str());
	}
	return !event_exists;
}

void EpgEvents::add_epg_events(const EpgEventList& epg_event_list)
{
	Glib::RecMutex::Lock lock(mutex);
	list = epg_event_list;
}

gboolean EpgEvents::get_current(EpgEvent& epg_event)
{
	EpgEvent result;
	gboolean found = false;
	guint now = convert_to_local_time(time(NULL));
	
	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::iterator i = list.begin(); i != list.end() && found == false; i++)
	{
		EpgEvent& current = *i;
		if (current.start_time <= now && current.get_end_time() >= now)
		{
			found = true;
			epg_event = current;
		}
	}
	
	return found;
}

const EpgEventList& EpgEvents::get_list()
{
	Glib::RecMutex::Lock lock(mutex);
	return list;
}

guint now = 0;

bool is_old(EpgEvent& epg_event)
{
	return epg_event.get_end_time() < now;
}

void EpgEvents::prune()
{
	now = convert_to_local_time(time(NULL));

	list.remove_if(is_old);
}

void EpgEvents::load(Data::Connection& connection, guint channel_id)
{
	Application& application = get_application();

	Data::Table table_epg_event			= application.get_schema().tables["epg_event"];
	Data::Table table_epg_event_text	= application.get_schema().tables["epg_event_text"];

	Data::TableAdapter adapter_epg_event(connection, table_epg_event);
	Data::TableAdapter adapter_epg_event_text(connection, table_epg_event_text);

	Glib::ustring clause = Glib::ustring::compose("channel_id = %1", channel_id);
	Data::DataTable data_table_epg_event = adapter_epg_event.select_rows(clause, "start_time");		
	for (Data::Rows::iterator j = data_table_epg_event.rows.begin(); j != data_table_epg_event.rows.end(); j++)
	{
		Data::Row row_epg_event = *j;
		EpgEvent epg_event;
		
		epg_event.save = false;
		
		epg_event.epg_event_id	= row_epg_event["epg_event_id"].int_value;
		epg_event.channel_id	= channel_id;
		epg_event.event_id		= row_epg_event["event_id"].int_value;
		epg_event.start_time	= row_epg_event["start_time"].int_value;
		epg_event.duration		= row_epg_event["duration"].int_value;
		
		Glib::ustring clause = Glib::ustring::compose("epg_event_id = %1", epg_event.epg_event_id);
		Data::DataTable data_table_epg_event_text = adapter_epg_event_text.select_rows(clause);
		for (Data::Rows::iterator k = data_table_epg_event_text.rows.begin(); k != data_table_epg_event_text.rows.end(); k++)
		{
			Data::Row row_epg_event_text = *k;
			EpgEventText epg_event_text;
			
			epg_event_text.epg_event_text_id	= row_epg_event_text["epg_event_text_id"].int_value;
			epg_event_text.epg_event_id			= epg_event.epg_event_id;
			epg_event_text.language				= row_epg_event_text["language"].string_value;
			epg_event_text.title				= row_epg_event_text["title"].string_value;
			epg_event_text.description			= row_epg_event_text["description"].string_value;
							
			epg_event.texts.push_back(epg_event_text);
		}			
		
		add_epg_event(epg_event);
	}
}

void EpgEvents::save(Data::Connection& connection, guint channel_id, EpgEvents& epg_events)
{
	Application& application = get_application();
	
	Data::Table table_epg_event			= application.get_schema().tables["epg_event"];
	Data::Table table_epg_event_text	= application.get_schema().tables["epg_event_text"];

	Data::TableAdapter adapter_epg_event(connection, table_epg_event);
	Data::TableAdapter adapter_epg_event_text(connection, table_epg_event_text);

	Data::DataTable data_table_epg_event(table_epg_event);
	Data::DataTable data_table_epg_event_text(table_epg_event_text);

	g_debug("Deleting old EPG events");
	Glib::ustring clause_epg_event = Glib::ustring::compose("(START_TIME+DURATION)<%1", convert_to_local_time(time(NULL)));
	adapter_epg_event.delete_rows(clause_epg_event);

	g_debug("Deleting old EPG event texts");
	Glib::ustring clause_epg_event_text =
		"NOT EXISTS (SELECT epg_event_id FROM epg_event WHERE epg_event.epg_event_id = epg_event_text.epg_event_id)";
	adapter_epg_event_text.delete_rows(clause_epg_event_text);

	g_debug("Saving %d EPG events", list.size());
	for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
	{
		EpgEvent& epg_event = *j;
		
		if (epg_event.save)
		{
			Data::Row row_epg_event;
			
			row_epg_event.auto_increment = &(epg_event.epg_event_id);
			row_epg_event["epg_event_id"].int_value	= epg_event.epg_event_id;
			row_epg_event["channel_id"].int_value	= channel_id;
			row_epg_event["event_id"].int_value		= epg_event.event_id;
			row_epg_event["start_time"].int_value	= epg_event.start_time;
			row_epg_event["duration"].int_value		= epg_event.duration;
			
			data_table_epg_event.rows.add(row_epg_event);
		}
	}

	// Have to do this before updating the EPG event texts to get the correct epg_event_id
	adapter_epg_event.replace_rows(data_table_epg_event);

	g_debug("Saving EPG event texts");
	for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
	{
		EpgEvent& epg_event = *j;
		
		if (epg_event.save)
		{
			for (EpgEventTextList::iterator k = epg_event.texts.begin(); k != epg_event.texts.end(); k++)
			{
				EpgEventText epg_event_text = *k;
				Data::Row row_epg_event_text;
			
				row_epg_event_text.auto_increment = &(epg_event_text.epg_event_text_id);
				row_epg_event_text["epg_event_text_id"].int_value	= epg_event_text.epg_event_text_id;
				row_epg_event_text["epg_event_id"].int_value		= epg_event.epg_event_id;
				row_epg_event_text["language"].string_value			= epg_event_text.language;
				row_epg_event_text["title"].string_value			= epg_event_text.title;
				row_epg_event_text["description"].string_value		= epg_event_text.description;
			
				data_table_epg_event_text.rows.add(row_epg_event_text);
			}
			
			epg_event.save = false;
		}
	}
	
	adapter_epg_event_text.replace_rows(data_table_epg_event_text);		

	g_debug("Updating original event IDs");

	Glib::RecMutex::Lock lock(epg_events.mutex);
	EpgEventList& list_real = epg_events.list;
	EpgEventList& list_copy = list;
	for (EpgEventList::iterator j = list_copy.begin(); j != list_copy.end(); j++)
	{
		for (EpgEventList::iterator k = list_real.begin(); k != list_real.end(); k++)
		{
			if ((*k).event_id == (*j).event_id)
			{
				(*k).epg_event_id = (*j).epg_event_id;
				(*k).save = false;
			}
		}
	}
	
	g_debug("EPG events saved");
}
