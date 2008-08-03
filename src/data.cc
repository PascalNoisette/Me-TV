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

Glib::RecMutex statement_mutex;

class SQLiteException : public Exception
{
public:
	SQLiteException(sqlite3* database, const Glib::ustring& message) :
		Exception(Glib::ustring::compose("%1: %2", message, Glib::ustring(sqlite3_errmsg(database)))) {}
};

Statement::Statement(sqlite3* database, const Glib::ustring& command) : database(database), command(command), lock(statement_mutex)
{
	statement = NULL;
	const char* remaining = NULL;
	//g_debug("Command: %s", command.c_str());
	
	if (sqlite3_prepare_v2(database, command.c_str(), -1, &statement, &remaining) != 0)
	{
		if (remaining == NULL || *remaining == 0)
		{
			throw SQLiteException(database, Glib::ustring::compose("Failed to prepare statement: %1", command));
		}
		else
		{
			throw SQLiteException(database, Glib::ustring::compose(_("Failed to prepare statement: %1"), Glib::ustring(remaining)));
		}
	}
	
	if (remaining != NULL && *remaining != 0)
	{
		throw SQLiteException(database, Glib::ustring::compose(_("Prepare statement had remaining data: %1"), Glib::ustring(remaining)));
	}
	
	if (statement == NULL)
	{
		throw SQLiteException(database, _("Failed to create statement"));
	}
}

Statement::~Statement()
{
	if (sqlite3_finalize(statement) != 0)
	{
		throw SQLiteException(database, _("Failed to finalise statement"));
	}
}
	
gint Statement::step()
{
	return sqlite3_step(statement);
}
	
gint Statement::get_int(guint column)
{
	return sqlite3_column_int(statement, column);
}

const Glib::ustring Statement::get_text(guint column)
{
	return (gchar*)sqlite3_column_text(statement, column);
}

Data::Data(gboolean initialise)
{
	Glib::ustring database_path = Glib::build_filename(Glib::get_home_dir(), ".me-tv/me-tv.db");
	
	if (sqlite3_open(database_path.c_str(), &database) != 0)
	{
		throw SQLiteException(database, _("Failed to connect to Me TV database"));
	}
	
	if (initialise)
	{
		execute_non_query(
			"CREATE TABLE IF NOT EXISTS PROFILE ("\
			"PROFILE_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
			"NAME CHAR(50) NOT NULL, "\
			"UNIQUE (NAME));");
		
		execute_non_query(
			"CREATE TABLE IF NOT EXISTS CHANNEL ("\
			"CHANNEL_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
			"PROFILE_ID INTEGER NOT NULL, "\
			"NAME CHAR(50) NOT NULL, "\
			"FLAGS INTEGER NOT NULL, "\
			"SORT_ORDER INTEGER NOT NULL, "\
			"MRL CHAR(1024), "\
			"SERVICE_ID INTEGER, "\
			"FREQUENCY INTEGER, "\
			"BANDWIDTH INTEGER, "\
			"CODE_RATE_HP INTEGER, "\
			"CODE_RATE_LP INTEGER, "\
			"CONSTELLATION INTEGER, "\
			"TRANSMISSION_MODE INTEGER, "\
			"GUARD_INTERVAL INTEGER, "\
			"HIERARCHY_INFORMATION INTEGER, "\
			"INVERSION INTEGER, "\
			"UNIQUE (NAME));");
		
		execute_non_query(
			"CREATE TABLE IF NOT EXISTS EPG_EVENT ("\
			"EPG_EVENT_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
			"CHANNEL_ID INTEGER NOT NULL, "\
			"EVENT_ID INTEGER NOT NULL, "\
			"START_TIME INTEGER NOT NULL, "\
			"DURATION INTEGER NOT NULL, "\
			"UNIQUE (CHANNEL_ID, EVENT_ID));");

		execute_non_query(
			"CREATE TABLE IF NOT EXISTS EPG_EVENT_TEXT ("\
			"EPG_EVENT_TEXT_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
			"EPG_EVENT_ID INTEGER NOT NULL, "\
			"LANGUAGE CHAR(3) NOT NULL, "\
			"TITLE CHAR(100) NOT NULL, "\
			"DESCRIPTION CHAR(200) NOT NULL, "\
			"UNIQUE (EPG_EVENT_ID, LANGUAGE));");

		execute_non_query(
			"CREATE TABLE IF NOT EXISTS SCHEDULED_RECORDING ("\
			"SCHEDULED_RECORDING_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
			"DESCRIPTION CHAR(100) NOT NULL, "\
			"TYPE INTEGER NOT NULL, " \
			"CHANNEL_ID INTEGER NOT NULL, " \
			"START_TIME INTEGER NOT NULL, " \
			"DURATION INTEGER NOT NULL);");
	}
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
	replace_text(text, "'", "''");
}

void Data::replace_epg_event(EpgEvent& epg_event)
{
	if (epg_event.channel_id == 0)
	{
		throw Exception("ASSERT: epg_event.channel_id == 0");
	}

	Glib::ustring insert_command = Glib::ustring::compose
	(
		"REPLACE INTO EPG_EVENT "\
	 	"(CHANNEL_ID, EVENT_ID, START_TIME, DURATION) "\
	 	"VALUES (%1, %2, %3, %4);",
		epg_event.channel_id, epg_event.event_id, epg_event.start_time, epg_event.duration
	);
	
	execute_non_query(insert_command);
	
	if (epg_event.epg_event_id == 0)
	{
		epg_event.epg_event_id = sqlite3_last_insert_rowid(database);
	}	
	
	if (epg_event.epg_event_id == 0)
	{
		Glib::ustring select_command = Glib::ustring::compose
		(
			"SELECT EPG_EVENT_ID FROM EPG_EVENT WHERE CHANNEL_ID = %1 AND EVENT_ID = %2",
			epg_event.channel_id, epg_event.event_id
		);
		
		Statement statement(database, select_command);
		if (statement.step() == SQLITE_ROW)
		{
			epg_event.epg_event_id = statement.get_int(0);
		}
		
		if (epg_event.epg_event_id == 0)
		{
			throw Exception("Failed to get epg_event_id");
		}
	}
	
	for (EpgEventTextList::iterator i = epg_event.texts.begin(); i != epg_event.texts.end(); i++)
	{
		EpgEventText& epg_event_text = *i;
		epg_event_text.epg_event_id = epg_event.epg_event_id;
		replace_epg_event_text(epg_event_text);
	}
}

void Data::replace_epg_event_text(EpgEventText& epg_event_text)
{
	Glib::ustring command;

	if (epg_event_text.epg_event_id == 0)
	{
		throw Exception(_("Event ID was 0"));
	}

	Glib::ustring fixed_title = epg_event_text.title;
	Glib::ustring fixed_description = epg_event_text.description;
	
	fix_quotes(fixed_title);
	fix_quotes(fixed_description);

	guint existing_epg_event_text_id = 0;

	// Make sure that statement exists before we start the next command
	{
		Statement statement(database, Glib::ustring::compose(
			"SELECT EPG_EVENT_TEXT_ID FROM EPG_EVENT_TEXT WHERE EPG_EVENT_ID=%1;",
			epg_event_text.epg_event_id));
		if (statement.step() == SQLITE_ROW)
		{
			existing_epg_event_text_id = statement.get_int(0);
		}
	}
	
	if (existing_epg_event_text_id != 0)
	{
		if (epg_event_text.is_extended)
		{
			command = Glib::ustring::compose
			(
				"UPDATE EPG_EVENT_TEXT SET DESCRIPTION = '%1' WHERE EPG_EVENT_TEXT_ID=%2;",
				fixed_description, existing_epg_event_text_id
			);
		}
		else
		{
			command = Glib::ustring::compose
			(
				"UPDATE EPG_EVENT_TEXT SET TITLE = '%1' WHERE EPG_EVENT_TEXT_ID=%2;",
				fixed_title, existing_epg_event_text_id
			);
		}
	}
	else
	{
		if (epg_event_text.is_extended)
		{
			command = Glib::ustring::compose
			(
				"INSERT INTO EPG_EVENT_TEXT "\
				"(EPG_EVENT_ID, LANGUAGE, TITLE, DESCRIPTION) "\
				"VALUES (%1, '%2', '%3', '%4');",
				epg_event_text.epg_event_id, epg_event_text.language, fixed_title, fixed_description
			);
		}
		else
		{
			command = Glib::ustring::compose
			(
				"INSERT INTO EPG_EVENT_TEXT "\
				"(EPG_EVENT_ID, LANGUAGE, TITLE, DESCRIPTION) "\
				"VALUES (%1, '%2', '%3', '%4');",
				epg_event_text.epg_event_id, epg_event_text.language, fixed_title, fixed_description
			);
		}
	}
	
	execute_non_query(command);

	if (epg_event_text.epg_event_text_id == 0)
	{
		epg_event_text.epg_event_text_id = sqlite3_last_insert_rowid(database);
	}	
}

void Data::load_epg_event(Statement& statement, EpgEvent& epg_event)
{
	epg_event.epg_event_id	= statement.get_int(0);
	epg_event.channel_id	= statement.get_int(1);
	epg_event.event_id		= statement.get_int(2);
	epg_event.start_time	= statement.get_int(3);
	epg_event.duration		= statement.get_int(4);

	Glib::ustring text_select_command = Glib::ustring::compose
	(
		"SELECT * FROM EPG_EVENT_TEXT WHERE EPG_EVENT_ID=%1;",
		epg_event.epg_event_id
	);
	
	Statement text_statement(database, text_select_command);
	while (text_statement.step() == SQLITE_ROW)
	{
		EpgEventText epg_event_text;
		epg_event_text.epg_event_text_id	= text_statement.get_int(0);
		epg_event_text.epg_event_id			= text_statement.get_int(1);
		epg_event_text.language				= text_statement.get_text(2);
		epg_event_text.title				= text_statement.get_text(3);
		epg_event_text.description			= text_statement.get_text(4);
		epg_event.texts.push_back(epg_event_text);
	}
}

EpgEventList Data::get_epg_events(const Channel& channel, guint start_time, guint end_time)
{
	EpgEventList result;
	
	Glib::ustring select_command = Glib::ustring::compose
	(
		"SELECT * FROM EPG_EVENT WHERE CHANNEL_ID=%1 AND ("\
	 	"(START_TIME > %2 AND START_TIME < %3) OR "\
	 	"((START_TIME+DURATION) > %2 AND (START_TIME+DURATION) < %3) OR "\
	 	"(START_TIME < %2 AND (START_TIME+DURATION) > %3)"\
	 	") ORDER BY START_TIME;",
		channel.channel_id , start_time, end_time
	);

	Statement statement(database, select_command);
	while (statement.step() == SQLITE_ROW)
	{
		EpgEvent epg_event;
		load_epg_event(statement, epg_event);
		result.push_back(epg_event);
	}
	
	return result;
}

void Data::replace_channel(Channel& channel)
{
	Glib::ustring fixed_name = channel.name;
	
	fix_quotes(fixed_name);
	
	Glib::ustring insert_command = Glib::ustring::compose
	(
		"REPLACE INTO CHANNEL "\
	 	"(CHANNEL_ID, PROFILE_ID, NAME, FLAGS, SORT_ORDER, MRL, SERVICE_ID, FREQUENCY, BANDWIDTH, CODE_RATE_HP, CODE_RATE_LP, "\
	 	"CONSTELLATION, TRANSMISSION_MODE, GUARD_INTERVAL, HIERARCHY_INFORMATION, INVERSION) "\
	 	"VALUES (%1, %2, '%3', %4, %5, '%6', %7, %8, ",
	 	channel.channel_id == 0 ? "NULL" : Glib::ustring::compose("%1", channel.channel_id),
	 	channel.profile_id,
		fixed_name,
		channel.flags,
		channel.sort_order,
		channel.mrl,
		channel.service_id,
		(guint)channel.frontend_parameters.frequency
	);

	Glib::ustring insert_command_extra = Glib::ustring::compose
	(
		"%1, %2, %3, %4, %5, %6, %7, %8);",
		(guint)channel.frontend_parameters.u.ofdm.bandwidth,
		(guint)channel.frontend_parameters.u.ofdm.code_rate_HP,
		(guint)channel.frontend_parameters.u.ofdm.code_rate_LP,
		(guint)channel.frontend_parameters.u.ofdm.constellation,
		(guint)channel.frontend_parameters.u.ofdm.transmission_mode,
		(guint)channel.frontend_parameters.u.ofdm.guard_interval,
		(guint)channel.frontend_parameters.u.ofdm.hierarchy_information,
		(guint)channel.frontend_parameters.inversion
	);
	
	execute_non_query(insert_command + insert_command_extra);
	
	if (channel.channel_id == 0)
	{
		channel.channel_id = sqlite3_last_insert_rowid(database);
	}
}

void Data::replace_profile(Profile& profile)
{
	Glib::ustring fixed_name = profile.name;
	
	fix_quotes(fixed_name);
	
	Glib::ustring replace_command = Glib::ustring::compose
	(
		"REPLACE INTO PROFILE (PROFILE_ID, NAME) VALUES (%1, '%2');",
	 	profile.profile_id == 0 ? "NULL" : Glib::ustring::compose("%1", profile.profile_id),
	 	profile.name
	);

	execute_non_query(replace_command);
	
	if (profile.profile_id == 0)
	{
		profile.profile_id = sqlite3_last_insert_rowid(database);
		if (profile.profile_id == 0)
		{
			throw Exception("ASSERT: profile.profile_id == 0");
		}
	}
	
	ChannelList& channels = profile.get_channels();
	for (ChannelList::iterator channel_iterator = channels.begin(); channel_iterator != channels.end(); channel_iterator++)
	{
		Channel& channel = *channel_iterator;
		channel.profile_id = profile.profile_id;
		replace_channel(channel);
	}
}

ProfileList Data::get_all_profiles()
{
	ProfileList profiles;

	Statement statement(database, "SELECT * FROM PROFILE");
	while (statement.step() == SQLITE_ROW)
	{
		Profile profile;
		profile.profile_id	= statement.get_int(0);
		profile.name		= statement.get_text(1);
		
		Statement channel_statement(database,
			Glib::ustring::compose("SELECT * FROM CHANNEL WHERE PROFILE_ID = %1", profile.profile_id));
		while (channel_statement.step() == SQLITE_ROW)
		{
			Channel channel;
			
			channel.channel_id			= channel_statement.get_int(0);
			channel.profile_id			= channel_statement.get_int(1);
			channel.name				= channel_statement.get_text(2);
			channel.flags				= channel_statement.get_int(3);
			channel.sort_order			= channel_statement.get_int(4);
			channel.mrl					= channel_statement.get_text(5);
			channel.service_id			= channel_statement.get_int(6);
			channel.frontend_parameters.frequency						= channel_statement.get_int(7);
			channel.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth_t)channel_statement.get_int(8);
			channel.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate_t)channel_statement.get_int(9);
			channel.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate_t)channel_statement.get_int(10);
			channel.frontend_parameters.u.ofdm.constellation			= (fe_modulation_t)channel_statement.get_int(11);
			channel.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode_t)channel_statement.get_int(12);
			channel.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval_t)channel_statement.get_int(13);
			channel.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy_t)channel_statement.get_int(14);
			channel.frontend_parameters.inversion						= (fe_spectral_inversion_t)channel_statement.get_int(15);
			
			profile.add_channel(channel);
			
			g_debug("Loaded channel: %s", channel.name.c_str());
		}
		
		profiles.push_back(profile);

		g_debug("Loaded profile: %s", profile.name.c_str());
	}
	
	return profiles;
}

gboolean Data::get_current_epg_event(const Channel& channel, EpgEvent& epg_event)
{
	gboolean result = false;
	
	Glib::ustring select_command = Glib::ustring::compose
	(
		"SELECT * FROM EPG_EVENT WHERE CHANNEL_ID=%1 "\
	 	"AND START_TIME <= %2 AND (START_TIME + DURATION) > %2",
		channel.channel_id, time(NULL) + timezone
	);

	Statement statement(database, select_command);
	if (statement.step() == SQLITE_ROW)
	{
		load_epg_event(statement, epg_event);
		result = true;
	}
	
	return result;
}

void Data::replace_scheduled_recording(ScheduledRecording& scheduled_recording)
{
	Glib::ustring fixed_description = scheduled_recording.description;
	
	fix_quotes(fixed_description);
	
	Glib::ustring replace_command = Glib::ustring::compose
	(
		"REPLACE INTO SCHEDULED_RECORDING " \
	 	"(SCHEDULED_RECORDING_ID, DESCRIPTION, TYPE, CHANNEL_ID, START_TIME, DURATION) VALUES " \
	 	"(%1, '%2', %3, %4, %5, %6);",
	 	scheduled_recording.scheduled_recording_id == 0 ? "NULL" : Glib::ustring::compose("%1", scheduled_recording.scheduled_recording_id),
	 	fixed_description,
	 	scheduled_recording.type,
	 	scheduled_recording.channel_id,
	 	scheduled_recording.start_time,
	 	scheduled_recording.duration
	);

	execute_non_query(replace_command);
	
	if (scheduled_recording.scheduled_recording_id == 0)
	{
		scheduled_recording.scheduled_recording_id = sqlite3_last_insert_rowid(database);
		if (scheduled_recording.scheduled_recording_id == 0)
		{
			throw Exception("ASSERT: scheduled_recording.scheduled_recording_id == 0");
		}
	}
}

ScheduledRecordingList Data::get_scheduled_recordings()
{
	ScheduledRecordingList result;
	
	Glib::ustring select_command = "SELECT * FROM SCHEDULED_RECORDING";

	Statement statement(database, select_command);
	while (statement.step() == SQLITE_ROW)
	{
		ScheduledRecording scheduled_recording;
		load_scheduled_recording(statement, scheduled_recording);
		result.push_back(scheduled_recording);
	}
	
	return result;
}

void Data::delete_scheduled_recording(guint scheduled_recording_id)
{
	Glib::ustring command = Glib::ustring::compose(
		"DELETE FROM SCHEDULED_RECORDING WHERE SCHEDULED_RECORDING_ID=%1",
		scheduled_recording_id);
	execute_non_query(command);
}

void Data::load_scheduled_recording(Statement& statement, ScheduledRecording& scheduled_recording)
{
	scheduled_recording.scheduled_recording_id	= statement.get_int(0);
	scheduled_recording.description				= statement.get_text(1);
	scheduled_recording.type					= statement.get_int(2);
	scheduled_recording.channel_id				= statement.get_int(3);
	scheduled_recording.start_time				= statement.get_int(4);
	scheduled_recording.duration				= statement.get_int(5);
}

gboolean Data::get_scheduled_recording(guint scheduled_recording_id, ScheduledRecording& scheduled_recording)
{
	gboolean result = false;
	Glib::ustring select_command = Glib::ustring::compose(
		"SELECT * FROM SCHEDULED_RECORDING WHERE SCHEDULED_RECORDING_ID=%1",
		scheduled_recording_id
	);
	
	Statement statement(database, select_command);
	if (statement.step() == SQLITE_ROW)
	{
		load_scheduled_recording(statement, scheduled_recording);
		result = true;
	}
	
	return result;
}