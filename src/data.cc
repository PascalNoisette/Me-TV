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

#include "data.h"
#include "me-tv.h"
#include "exception.h"
#include <giomm.h>

Glib::RecMutex statement_mutex;

class SQLiteException : public Exception
{
public:
	SQLiteException(sqlite3* database, const Glib::ustring& exception_message) :
		Exception(Glib::ustring::compose("%1: %2", exception_message, Glib::ustring(sqlite3_errmsg(database)))) {}
};

typedef enum
{
	PARAMETER_TYPE_NULL,
	PARAMETER_TYPE_STRING,
	PARAMETER_TYPE_INTEGER
} ParameterType;

class Parameter
{
public:
	Parameter(const Glib::ustring& parameter_name) :
		name(parameter_name), parameter_type(PARAMETER_TYPE_NULL) {}
	Parameter(const Glib::ustring& parameter_name, const Glib::ustring& value) :
		name(parameter_name), string_value(value), parameter_type(PARAMETER_TYPE_STRING) {}
	Parameter(const Glib::ustring& parameter_name, gint value) :
		name(parameter_name), int_value(value), parameter_type(PARAMETER_TYPE_INTEGER) {}
		
	Glib::ustring	name;
	guint			int_value;
	Glib::ustring	string_value;
	ParameterType	parameter_type;
};

class ParameterList : public std::list<Parameter>
{
public:
	Glib::ustring get_names();
	Glib::ustring get_values();
	void add(const Glib::ustring& name, const Glib::ustring& value);
	void add(const Glib::ustring& name, gint value);
	void add(const Glib::ustring& name);
};

Glib::ustring ParameterList::get_names()
{
	gboolean first = true;
	Glib::ustring result;
	for (std::list<Parameter>::iterator i = begin(); i != end(); i++)
	{
		if (!first)
		{
			result += ", ";
		}
		else
		{
			first = false;
		}

		result += (*i).name;
	}
	return result;
}

Glib::ustring ParameterList::get_values()
{
	gboolean first = true;
	Glib::ustring result;

	for (std::list<Parameter>::iterator i = begin(); i != end(); i++)
	{
		if (!first)
		{
			result += ", ";
		}
		else
		{
			first = false;
		}

		Parameter& parameter = *i;
		if (parameter.parameter_type == PARAMETER_TYPE_NULL)
		{
			result += "NULL";
		}
		else if (parameter.parameter_type == PARAMETER_TYPE_INTEGER)
		{
			result += Glib::ustring::compose("%1", parameter.int_value);
		}
		else if (parameter.parameter_type == PARAMETER_TYPE_STRING)
		{
			result += Glib::ustring::compose("\"%1\"", parameter.string_value);
		}
	}
	return result;
}

void ParameterList::add(const Glib::ustring& name, const Glib::ustring& value)
{
	push_back(Parameter(name, value));
}

void ParameterList::add(const Glib::ustring& name, gint value)
{
	push_back(Parameter(name, value));
}

void ParameterList::add(const Glib::ustring& name)
{
	push_back(Parameter(name));
}

Statement::Statement(sqlite3* statement_database, const Glib::ustring& statement_command) :
	lock(statement_mutex), database(statement_database), command(statement_command)
{
	statement = NULL;
	const char* remaining = NULL;
//	g_debug("Command: %s", command.c_str());
	
	if (sqlite3_prepare_v2(database, command.c_str(), -1, &statement, &remaining) != 0)
	{
		if (remaining == NULL || *remaining == 0)
		{
			throw SQLiteException(database, Glib::ustring::compose(_("Failed to prepare statement: %1"), command));
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
	int result = sqlite3_step(statement);
	
	while (result == SQLITE_BUSY)
	{
		g_debug("Database busy, trying again");
		usleep(10000);
		result = sqlite3_step(statement);
	}
	
	return result;
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
			"INVERSION INTEGER, "\
						  
			"BANDWIDTH INTEGER, "\
			"CODE_RATE_HP INTEGER, "\
			"CODE_RATE_LP INTEGER, "\
			"CONSTELLATION INTEGER, "\
			"TRANSMISSION_MODE INTEGER, "\
			"GUARD_INTERVAL INTEGER, "\
			"HIERARCHY_INFORMATION INTEGER, "\
						  
			"SYMBOL_RATE INTEGER, "\
			"FEC_INNER INTEGER, "\
			"MODULATION INTEGER, "\

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
		
		delete_old_sceduled_recordings();
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
	return statement.step();
}

void fix_quotes(Glib::ustring& text)
{
	replace_text(text, "'", "''");
}

void Data::replace_epg_event(EpgEvent& epg_event)
{
	if (epg_event.channel_id == 0)
	{
		throw Exception(_("ASSERT: epg_event.channel_id == 0"));
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
			throw Exception(_("Failed to get epg_event_id"));
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
				"UPDATE EPG_EVENT_TEXT SET DESCRIPTION = DESCRIPTION || '%1' WHERE EPG_EVENT_TEXT_ID=%2;",
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
	ParameterList parameters;
	
	Glib::ustring fixed_name = channel.name;
	fix_quotes(fixed_name);

	// General
	if (channel.channel_id == 0)
	{
		parameters.add("CHANNEL_ID");
	}
	else
	{
		parameters.add("CHANNEL_ID", channel.channel_id);
	}
	parameters.add("PROFILE_ID",	channel.profile_id);
	parameters.add("NAME",			fixed_name);
	parameters.add("FLAGS",			channel.flags);
	parameters.add("SORT_ORDER",	channel.sort_order);
	parameters.add("MRL",			channel.mrl);
	parameters.add("SERVICE_ID",	channel.service_id);
	parameters.add("FREQUENCY",		(guint)channel.frontend_parameters.frequency);
	parameters.add("INVERSION",		(guint)channel.frontend_parameters.inversion);

	if (channel.flags & CHANNEL_FLAG_DVB_T)
	{
		parameters.add("BANDWIDTH",				(guint)channel.frontend_parameters.u.ofdm.bandwidth);
		parameters.add("CODE_RATE_HP",			(guint)channel.frontend_parameters.u.ofdm.code_rate_HP);
		parameters.add("CODE_RATE_LP",			(guint)channel.frontend_parameters.u.ofdm.code_rate_LP);
		parameters.add("CONSTELLATION",			(guint)channel.frontend_parameters.u.ofdm.constellation);
		parameters.add("TRANSMISSION_MODE", 	(guint)channel.frontend_parameters.u.ofdm.transmission_mode);
		parameters.add("GUARD_INTERVAL",		(guint)channel.frontend_parameters.u.ofdm.guard_interval);
		parameters.add("HIERARCHY_INFORMATION", (guint)channel.frontend_parameters.u.ofdm.hierarchy_information);
	}
	else if (channel.flags & CHANNEL_FLAG_DVB_C)
	{
		parameters.add("SYMBOL_RATE",	(guint)channel.frontend_parameters.u.qam.symbol_rate);
		parameters.add("FEC_INNER",		(guint)channel.frontend_parameters.u.qam.fec_inner);
		parameters.add("MODULATION",	(guint)channel.frontend_parameters.u.qam.modulation);
	}
	else if (channel.flags & CHANNEL_FLAG_ATSC)
	{
		parameters.add("MODULATION", (guint)channel.frontend_parameters.u.vsb.modulation);
	}
	else
	{
		throw Exception(_("Invalid channel flag"));
	}
	
	Glib::ustring insert_command = Glib::ustring::compose
	(
		"REPLACE INTO CHANNEL (%1) VALUES (%2);",
		parameters.get_names(), parameters.get_values()
	);

	execute_non_query(insert_command);
	
	if (channel.channel_id == 0)
	{
		channel.channel_id = sqlite3_last_insert_rowid(database);
	}
}

void Data::replace_profile(Profile& profile)
{
	ParameterList parameters;
	Glib::ustring fixed_name = profile.name;
	
	fix_quotes(fixed_name);
	
	if (profile.profile_id == 0)
	{
		parameters.add("PROFILE_ID");
	}
	else
	{
		parameters.add("PROFILE_ID", profile.profile_id);
	}
	parameters.add("NAME", profile.name);
	
	Glib::ustring replace_command = Glib::ustring::compose
	(
		"REPLACE INTO PROFILE (%1) VALUES (%2);",
		parameters.get_names(), parameters.get_values()
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
	
	// First, remove channels
	Statement statement(database,
		Glib::ustring::compose("SELECT CHANNEL_ID FROM CHANNEL WHERE PROFILE_ID=%1;", profile.profile_id));
	while (statement.step() == SQLITE_ROW)
	{
		gint channel_id = statement.get_int(0);
		if (profile.find_channel(channel_id) == NULL)
		{
			delete_channel(channel_id);
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

	Statement statement(database, "SELECT * FROM PROFILE ORDER BY PROFILE_ID");
	while (statement.step() == SQLITE_ROW)
	{
		Profile profile;
		profile.profile_id	= statement.get_int(0);
		profile.name		= statement.get_text(1);

		Statement channel_statement(database,
			Glib::ustring::compose("SELECT * FROM CHANNEL WHERE PROFILE_ID = %1 ORDER BY SORT_ORDER", profile.profile_id));
		while (channel_statement.step() == SQLITE_ROW)
		{
			Channel channel;
			
			channel.channel_id	= channel_statement.get_int(0);
			channel.profile_id	= channel_statement.get_int(1);
			channel.name		= channel_statement.get_text(2);
			channel.flags		= channel_statement.get_int(3);
			channel.sort_order	= channel_statement.get_int(4);
			channel.mrl			= channel_statement.get_text(5);
			channel.service_id	= channel_statement.get_int(6);

			channel.frontend_parameters.frequency	= channel_statement.get_int(7);
			channel.frontend_parameters.inversion	= (fe_spectral_inversion_t)channel_statement.get_int(8);

			if (channel.flags & CHANNEL_FLAG_DVB_T)
			{
				channel.frontend_parameters.u.ofdm.bandwidth				= (fe_bandwidth_t)channel_statement.get_int(9);
				channel.frontend_parameters.u.ofdm.code_rate_HP				= (fe_code_rate_t)channel_statement.get_int(10);
				channel.frontend_parameters.u.ofdm.code_rate_LP				= (fe_code_rate_t)channel_statement.get_int(11);
				channel.frontend_parameters.u.ofdm.constellation			= (fe_modulation_t)channel_statement.get_int(12);
				channel.frontend_parameters.u.ofdm.transmission_mode		= (fe_transmit_mode_t)channel_statement.get_int(13);
				channel.frontend_parameters.u.ofdm.guard_interval			= (fe_guard_interval_t)channel_statement.get_int(14);
				channel.frontend_parameters.u.ofdm.hierarchy_information	= (fe_hierarchy_t)channel_statement.get_int(15);
			}
			else if (channel.flags & CHANNEL_FLAG_DVB_C)
			{
				channel.frontend_parameters.u.qam.symbol_rate	= channel_statement.get_int(16);
				channel.frontend_parameters.u.qam.fec_inner		= (fe_code_rate_t)channel_statement.get_int(17);
				channel.frontend_parameters.u.qam.modulation	= (fe_modulation_t)channel_statement.get_int(18);
			}
			else if (channel.flags & CHANNEL_FLAG_ATSC)
			{
				channel.frontend_parameters.u.vsb.modulation	= (fe_modulation_t)channel_statement.get_int(18);
			}
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
		channel.channel_id, convert_to_local_time(time(NULL))
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
	
	Glib::ustring select_command = Glib::ustring::compose
	(
		"SELECT * FROM SCHEDULED_RECORDING WHERE "\
		"((START_TIME >= %1 AND START_TIME <= %2) OR "\
		"(START_TIME+DURATION >= %1 AND START_TIME+DURATION <= %2) OR "\
		"(START_TIME <= %1 AND START_TIME+DURATION >= %2)) AND "\
		"CHANNEL_ID != %3",
		scheduled_recording.start_time,
		scheduled_recording.start_time + scheduled_recording.duration,
		scheduled_recording.channel_id
	);

	Statement statement(database, select_command);
	if (statement.step() == SQLITE_ROW)
	{
		Glib::ustring message = Glib::ustring::compose
		(
			_("Failed to save scheduled recording because it conflicts with another scheduled recording called '%1'"),
			statement.get_text(1)
		);
		throw Exception(message);
	}	
	
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
			throw Exception(_("ASSERT: scheduled_recording.scheduled_recording_id == 0"));
		}
	}
}

ScheduledRecordingList Data::get_scheduled_recordings()
{
	ScheduledRecordingList result;
	
	Glib::ustring select_command = "SELECT * FROM SCHEDULED_RECORDING ORDER BY START_TIME";

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
		"SELECT * FROM SCHEDULED_RECORDING WHERE SCHEDULED_RECORDING_ID=%1;",
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

void Data::delete_channel(guint channel_id)
{
	execute_non_query(Glib::ustring::compose(
		"DELETE FROM EPG_EVENT_TEXT WHERE EPG_EVENT_ID IN "\
		"(SELECT EPG_EVENT_ID FROM EPG_EVENT WHERE CHANNEL_ID=%1);",
		channel_id));
	execute_non_query(Glib::ustring::compose(
		"DELETE FROM EPG_EVENT WHERE CHANNEL_ID=%1;",
		channel_id));
	execute_non_query(Glib::ustring::compose(
		"DELETE FROM CHANNEL WHERE CHANNEL_ID=%1;",
		channel_id));
}

void Data::delete_old_sceduled_recordings()
{
	execute_non_query(Glib::ustring::compose(
		"DELETE FROM SCHEDULED_RECORDING WHERE (START_TIME+DURATION)<%1;",
		time(NULL)));
}
