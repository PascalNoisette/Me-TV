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
	dirty = true;
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
		const EpgEvent& existing = *i;
		if (epg_event.event_id == existing.event_id)
		{
			if (epg_event.version_number > existing.version_number ||
			    (epg_event.version_number == 0 && existing.version_number == 31))
			{
				g_debug("Old EPG Event %d (%s) (CHANNEL: %d) (%s) version %d removed",
				    existing.event_id,
				    existing.get_title().c_str(),
				    existing.channel_id,
				    existing.get_start_time_text().c_str(),
				    existing.version_number);
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
		g_debug("EPG Event %d (%s) (CHANNEL: %d) (%s) version %d added",
		    epg_event.event_id,
		    epg_event.get_title().c_str(),
		    epg_event.channel_id,
		    epg_event.get_start_time_text().c_str(),
		    epg_event.version_number);

		dirty = true;
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

EpgEventList EpgEvents::get_list(guint start_time, guint end_time)
{
	EpgEventList result;
	
	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::iterator i = list.begin(); i != list.end(); i++)
	{
		EpgEvent& epg_event = *i;
		guint epg_event_end_time = epg_event.get_end_time();
		if(
			(epg_event.start_time >= start_time && epg_event.start_time <= end_time) ||
			(epg_event_end_time >= start_time && epg_event_end_time <= end_time) ||
			(epg_event.start_time <= start_time && epg_event_end_time >= end_time)
		)
		{
			result.push_back(epg_event);
		}
	}
	
	return result;
}

guint prune_before_time = 0;

bool is_old(EpgEvent& epg_event)
{
	return epg_event.get_end_time() < prune_before_time;
}

void EpgEvents::prune()
{
	Glib::RecMutex::Lock lock(mutex);
	prune_before_time = get_local_time() - 36000; // Back 10 hours
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
			epg_event_text.subtitle				= row_epg_event_text["subtitle"].string_value;
			epg_event_text.description			= row_epg_event_text["description"].string_value;
		
			// This is temporary
			if (epg_event_text.subtitle == "-")
			{
				epg_event_text.subtitle.clear();
				epg_event.save = true;
			}
	
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
				row_epg_event_text["subtitle"].string_value			= epg_event_text.subtitle;
				row_epg_event_text["description"].string_value		= epg_event_text.description;
			
				data_table_epg_event_text.rows.add(row_epg_event_text);
			}
			
			epg_event.save = false;
		}
	}
	adapter_epg_event_text.replace_rows(data_table_epg_event_text);
	dirty = false;
	
	g_debug("EPG events saved");
}

EpgEventList EpgEvents::search(const Glib::ustring& text, gboolean search_description)
{
	EpgEventList result;

	guint now = get_local_time();

	Glib::RecMutex::Lock lock(mutex);
	for (EpgEventList::iterator i = list.begin(); i != list.end(); i++)
	{
		EpgEvent& epg_event = *i;
		if (
		    epg_event.get_end_time() >= now &&
		    (
			    epg_event.get_title().uppercase().find(text) != Glib::ustring::npos ||
				(
					search_description && (
						(epg_event.get_subtitle().uppercase().find(text) != Glib::ustring::npos) ||
						(epg_event.get_description().uppercase().find(text) != Glib::ustring::npos)
					)
				)
			)
		)
		{
			result.push_back(epg_event);
		}
	}
	
	return result;
}
