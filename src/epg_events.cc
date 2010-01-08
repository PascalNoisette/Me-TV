/*
 * Copyright (C) 2010 Michael Lamothe
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
 * e
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

bool sort_function(const EpgEvent& a, const EpgEvent& b)
{
	return a.start_time < b.start_time;
}

gboolean EpgEvents::add_epg_event(const EpgEvent& epg_event)
{
	gboolean add = true;
	
	Glib::RecMutex::Lock lock(mutex);
	EpgEventList::iterator i = list.begin();
	
	while (i != list.end() && add == true)
	{
		const EpgEvent& current = *i;
		if (epg_event.event_id == current.event_id)
		{
			if (epg_event.version_number > current.version_number ||
			    (epg_event.version_number == 0 && current.version_number == 31))
			{
				g_debug("Old EPG Event %d (%s) version %d removed",
				    epg_event.event_id, epg_event.get_title().c_str(), epg_event.version_number);
				i = list.erase(i);
			}
			else
			{
				add = false;
			}
		}
		else
		{
			i++;
		}
	}

	if (add)
	{
		list.push_back(epg_event);
		list.sort(sort_function);
		g_debug("EPG Event %d (%s) (%s) version %d added",
		    epg_event.event_id,
		    epg_event.get_title().c_str(),
		    epg_event.get_start_time_text().c_str(),
		    epg_event.version_number);
	}
	
	return add;
}

gboolean EpgEvents::get_current(EpgEvent& epg_event)
{
	EpgEvent result;
	gboolean found = false;
	guint now = get_local_time();
	
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

const EpgEventList EpgEvents::get_list()
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
	now = get_local_time();
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
		
		epg_event.epg_event_id		= row_epg_event["epg_event_id"].int_value;
		epg_event.channel_id		= channel_id;
		epg_event.version_number	= row_epg_event["version_number"].int_value;
		epg_event.event_id			= row_epg_event["event_id"].int_value;
		epg_event.start_time		= row_epg_event["start_time"].int_value;
		epg_event.duration			= row_epg_event["duration"].int_value;
		
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

void EpgEvents::save(Data::Connection& connection, guint channel_id)
{
	Application& application = get_application();
	
	Data::Table table_epg_event			= application.get_schema().tables["epg_event"];
	Data::Table table_epg_event_text	= application.get_schema().tables["epg_event_text"];

	Data::TableAdapter adapter_epg_event(connection, table_epg_event);
	Data::TableAdapter adapter_epg_event_text(connection, table_epg_event_text);

	Data::DataTable data_table_epg_event(table_epg_event);
	Data::DataTable data_table_epg_event_text(table_epg_event_text);

	Glib::RecMutex::Lock lock(mutex);

	g_debug("Saving %d EPG events", (int)list.size());
	for (EpgEventList::iterator j = list.begin(); j != list.end(); j++)
	{
		EpgEvent& epg_event = *j;
		
		if (epg_event.save)
		{
			Data::Row row_epg_event;
			
			row_epg_event.auto_increment = &(epg_event.epg_event_id);
			row_epg_event["epg_event_id"].int_value		= epg_event.epg_event_id;
			row_epg_event["channel_id"].int_value		= channel_id;
			row_epg_event["version_number"].int_value	= epg_event.version_number;
			row_epg_event["event_id"].int_value			= epg_event.event_id;
			row_epg_event["start_time"].int_value		= epg_event.start_time;
			row_epg_event["duration"].int_value			= epg_event.duration;
			
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
	
	g_debug("EPG events saved");
}
