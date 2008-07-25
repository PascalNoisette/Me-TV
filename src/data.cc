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

#include "data.h"
#include "me-tv.h"
#include "exception.h"
#include <giomm.h>

class Statement
{
private:
	sqlite3_stmt* statement;

	void prepare(sqlite3* database, const Glib::ustring& command)
	{
		const char* remaining = NULL;
		
		if (sqlite3_prepare_v2(database, command.c_str(), -1, &statement, &remaining) != 0)
		{
			if (remaining == NULL || *remaining == 0)
			{
				throw Exception("Failed to prepare statement");
			}
			else
			{
				throw Exception(Glib::ustring::compose("Failed to prepare statement: %1", Glib::ustring(remaining)));
			}
		}
		
		if (remaining != NULL && *remaining != 0)
		{
			throw Exception(Glib::ustring::compose("Prepare statement had remaining data: %1", Glib::ustring(remaining)));
		}
		
		if (statement == NULL)
		{
			throw Exception("Failed to create statement");
		}
	}

public:
	Statement(sqlite3* database, const Glib::ustring& command)
	{
		statement = NULL;
		prepare(database, command);
	}
	
	~Statement()
	{
		if (sqlite3_finalize(statement) != 0)
		{
			throw Exception(_("Failed to finalise"));
		}
	}
		
	gint step()
	{
		return sqlite3_step(statement);
	}
		
	gint get_int(guint column)
	{
		return sqlite3_column_int(statement, column);
	}

	const Glib::ustring get_text(guint column)
	{
		return (gchar*)sqlite3_column_text(statement, column);
	}
};

Data::Data()
{
	Glib::ustring database_path = Glib::build_filename(Glib::get_home_dir(), ".me-tv.db");
	
	if (sqlite3_open(database_path.c_str(), &database) != 0)
	{
		throw Exception(_("Failed to connect to Me TV database"));
	}
	
	execute_non_query(
		"CREATE TABLE IF NOT EXISTS EPG_EVENT ("\
		"FREQUENCY INT NOT NULL, "\
		"SERVICE_ID INT NOT NULL, "\
		"EVENT_ID INT NOT NULL, "\
		"START_TIME INT NOT NULL, "\
		"DURATION INT NOT NULL, "\
		"TITLE CHAR(100) NOT NULL, "\
		"DESCRIPTION CHAR(200) NOT NULL, "\
		"PRIMARY KEY (FREQUENCY, SERVICE_ID, EVENT_ID))");
}

Data::~Data()
{
	if (database != NULL)
	{
		sqlite3_close(database);
		database = NULL;
	}
}

guint Data::execute_non_query(const Glib::ustring& command)
{
	Statement statement(database, command);
	statement.step();
	return 0;
}

void fix_quotes(Glib::ustring& text)
{
	Glib::ustring::size_type position = 0;
	while ((position = text.find("'", position)) != Glib::ustring::npos)
	{
		text.insert(position, "'");
		position += 2;
	}
}

void Data::insert_or_ignore_epg_event(guint frequency, guint service_id, guint event_id,
	guint start_time, guint duration, const Glib::ustring& title, const Glib::ustring& description)
{
	Glib::ustring fixed_title = title;
	Glib::ustring fixed_description = description;
	
	fix_quotes(fixed_title);
	fix_quotes(fixed_description);
	
	Glib::ustring insert_command = Glib::ustring::compose
	(
		"INSERT OR IGNORE INTO EPG_EVENT VALUES (%1, %2, %3, %4, %5, '%6', '%7')",
		frequency, service_id, event_id, start_time, duration, fixed_title, fixed_description
	);

	execute_non_query(insert_command);
}

EpgEventList Data::get_epg_events(guint frequency, guint service_id,
	guint start_time, guint end_time)
{
	EpgEventList result;
	
	Glib::ustring select_command = Glib::ustring::compose
	(
		"SELECT * FROM EPG_EVENT WHERE FREQUENCY=%1 AND SERVICE_ID=%2 "\
	 	"AND (START_TIME+DURATION) > %3 AND START_TIME < %4 "\
	 	"ORDER BY START_TIME",
		frequency, service_id , start_time, end_time
	);

	Statement statement(database, select_command);
	
	while (statement.step() == SQLITE_ROW)
	{
		EpgEvent epg_event;
		epg_event.frequency		= statement.get_int(0);
		epg_event.service_id	= statement.get_int(1);
		epg_event.event_id		= statement.get_int(2);
		epg_event.start_time	= statement.get_int(3);
		epg_event.duration		= statement.get_int(4);
		epg_event.title			= statement.get_text(5);
		epg_event.description	= statement.get_text(6);
		result.push_back(epg_event);
	}
	
	return result;
}
