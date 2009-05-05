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
#include "application.h"
#include "exception.h"
#include <giomm.h>

Glib::RecMutex statement_mutex;

using namespace Data;

class SQLiteException : public Exception
{
public:
	SQLiteException(sqlite3* connection, const Glib::ustring& exception_message) :
		Exception(Glib::ustring::compose("%1: %2", exception_message, Glib::ustring(sqlite3_errmsg(connection)))) {}
	SQLiteException(Connection& connection, const Glib::ustring& exception_message) :
		Exception(Glib::ustring::compose("%1: %2", exception_message, connection.get_error_message())) {}
};

Statement::Statement(Connection& connection, const Glib::ustring& command) :
	lock(statement_mutex), connection(connection), command(command)
{
	statement = NULL;
	const char* remaining = NULL;
	
	if (sqlite3_prepare_v2(connection.get_connection(), command.c_str(), -1, &statement, &remaining) != 0)
	{
		if (remaining == NULL || *remaining == 0)
		{
			throw SQLiteException(connection, Glib::ustring::compose(
				_("Failed to prepare statement: %1"), command));
		}
		else
		{
			throw SQLiteException(connection, Glib::ustring::compose(
				_("Failed to prepare statement: %1"), Glib::ustring(remaining)));
		}
	}
	
	if (remaining != NULL && *remaining != 0)
	{
		throw SQLiteException(connection, Glib::ustring::compose(
			_("Prepared statement had remaining data: %1"), Glib::ustring(remaining)));
	}
	
	if (statement == NULL)
	{
		throw SQLiteException(connection, _("Failed to create statement"));
	}
}

Statement::~Statement()
{
	if (sqlite3_finalize(statement) != 0)
	{
		throw SQLiteException(connection, _("Failed to finalise statement"));
	}
}

void Statement::reset()
{
	sqlite3_reset(statement);
}
	
guint Statement::step()
{
	int result = sqlite3_step(statement);
	
	while (result == SQLITE_BUSY)
	{
		g_debug("Database busy, trying again");
		usleep(10000);
		result = sqlite3_step(statement);
	}

	switch (result)
	{
	case SQLITE_DONE:
	case SQLITE_OK:
	case SQLITE_ROW:
		break;
	default:
		throw SQLiteException(
			connection,
			Glib::ustring::compose(_("Failed to execute statement: %1"), command));
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

void Statement::set_int_parameter(guint index, int value)
{
	sqlite3_bind_int(statement, index, value);
}

void Statement::set_string_parameter(guint index, const Glib::ustring& value)
{
	sqlite3_bind_text(statement, index, value.c_str(), -1, NULL);
}

guint Statement::get_parameter_index(const Glib::ustring& name)
{
	int index = sqlite3_bind_parameter_index(statement, name.c_str());
	if (index == 0)
	{
		Glib::ustring message = Glib::ustring::compose("Unknown parameter name '%1'", name);
		throw Exception(message);
	}
	return index;
}

void Statement::set_int_parameter(const Glib::ustring& name, int value)
{
	set_int_parameter(get_parameter_index(name), value);
}

void Statement::set_string_parameter(const Glib::ustring& name, const Glib::ustring& value)
{
	set_string_parameter(get_parameter_index(name), value);
}

Connection::Connection(const Glib::ustring& filename)
{
	gboolean database_exists = Gio::File::create_for_path(filename)->query_exists();
	
	g_debug("Database '%s' ", database_exists ? "exists" : "does not exist");
	g_debug("Opening database file '%s'", filename.c_str());
	if (sqlite3_open(filename.c_str(), &connection) != 0)
	{
		Glib::ustring message = Glib::ustring::compose(_("Failed to connect to Me TV database '%1'"), filename);
		throw SQLiteException(connection, message);
	}
	
	database_created = !database_exists;
}

Connection::~Connection()
{
	if (connection != NULL)
	{
		sqlite3_close(connection);
		connection = NULL;
	}
}

guint Connection::get_last_insert_rowid()
{
	return sqlite3_last_insert_rowid(connection);
}

void SchemaAdapter::initialise_schema()
{
	for (Tables::iterator i = schema.tables.begin(); i != schema.tables.end(); i++)
	{
		Table& table = i->second;
		initialise_table(table);
	}
}

void SchemaAdapter::initialise_table(Table& table)
{
	g_debug("Initialising table '%s'", table.name.c_str());
	
	Glib::ustring command = "CREATE TABLE IF NOT EXISTS ";
	command += table.name;
	command += " (";
	gboolean first_column = true;
	for (Columns::const_iterator j = table.columns.begin(); j != table.columns.end(); j++)
	{
		const Column& column = *j;
		
		if (first_column)
		{
			first_column = false;
		}
		else
		{
			command += ", ";
		}
		
		command += column.name;
		command += " ";
		
		switch (column.type)
		{
		case DATA_TYPE_INTEGER:
			command += "INTEGER";
			break;
		case DATA_TYPE_STRING:
			command += Glib::ustring::compose("CHAR(%1)", column.size);
			break;
		default:
			break;
		}
		
		if (column.name == table.primary_key)
		{
			if (column.type != DATA_TYPE_INTEGER)
			{
				throw Exception(_("Only integers can be primary keys"));
			}
			
			command += " PRIMARY KEY AUTOINCREMENT";
		}
		else
		{
			if (!column.nullable)
			{
				command += " NOT NULL";
			}
		}
	}
	
	for (Constraints::const_iterator j = table.constraints.begin(); j != table.constraints.end(); j++)
	{
		const Constraint& constraint = *j;
		if (constraint.type == ConstraintTypeUnique)
		{
			gboolean first_constraint = true;
			command += ", UNIQUE (";
			for (StringList::const_iterator k = constraint.columns.begin(); k != constraint.columns.end(); k++)
			{
				Glib::ustring column = *k;
				
				if (first_constraint)
				{
					first_constraint = false;
				}
				else
				{
					command += ", ";
				}
				
				command += column;
			}
			command += ")";
		}
	}
	
	command += ");";
	
	Statement statement(connection, command);
	statement.step();
}

void SchemaAdapter::drop_schema()
{
	for (Tables::const_iterator i = schema.tables.begin(); i != schema.tables.end(); i++)
	{
		const Table& table = i->second;
		
		g_debug("Dropping table '%s'", table.name.c_str());
		
		Glib::ustring command = Glib::ustring::compose("DROP TABLE %1", table.name);
        Statement statement(connection, command);
		statement.step();
	}
}

TableAdapter::TableAdapter(Connection& connection, Table& table)
	: connection(connection), table(table)
{
	Glib::ustring fields;
	Glib::ustring replace_fields;
	Glib::ustring replace_values;
	
	for (Columns::const_iterator j = table.columns.begin(); j != table.columns.end(); j++)
	{
		const Column& column = *j;
		
		if (fields.length() > 0) fields += ", ";
		if (replace_fields.length() > 0) replace_fields += ", ";
		if (replace_values.length() > 0) replace_values += ", ";
		
		fields += column.name;
		replace_fields += column.name;
		replace_values += ":" + column.name;
	}
	
	select_command = Glib::ustring::compose("SELECT %1 FROM %2", fields, table.name);
	replace_command = Glib::ustring::compose("REPLACE INTO %1 (%2) VALUES (%3)", table.name, replace_fields, replace_values);
	delete_command = Glib::ustring::compose("DELETE FROM %1", table.name);
	update_command = Glib::ustring::compose("UPDATE %1 SET", table.name);
}

void TableAdapter::replace_rows(DataTable& data_table)
{
	Glib::ustring primary_key = data_table.table.primary_key;
			
	if (data_table.rows.size() > 0)
	{
		for (Data::Rows::iterator i = data_table.rows.begin(); i != data_table.rows.end(); i++)
		{
			Data::Row& row = *i;

			Statement statement(connection, replace_command);
			
			for (Columns::iterator j = data_table.table.columns.begin(); j != data_table.table.columns.end(); j++)
			{
				Column& column = *j;

				if (column.name != primary_key || row[column.name].int_value != 0)
				{
					switch(column.type)
					{
					case DATA_TYPE_INTEGER:
						statement.set_int_parameter(":" + column.name, row[column.name].int_value);
						break;
					case DATA_TYPE_STRING:
						statement.set_string_parameter(":" + column.name, row[column.name].string_value);
						break;
					default:
						break;
					}
				}
			}
			
			statement.step();
						
			if (row.auto_increment != NULL && *row.auto_increment == 0)
			{
				*(row.auto_increment) = connection.get_last_insert_rowid();
				g_debug("%s row replaced for id %d",
					data_table.table.name.c_str(), *(row.auto_increment));
			}
			else
			{
				g_debug("%s row replaced", data_table.table.name.c_str());
			}
		}
	}
}

DataTable TableAdapter::select_row(guint key)
{
	Glib::ustring where = Glib::ustring::compose(
		"%1 = %2", table.name, table.primary_key, key);
	return select_rows(where);
}

DataTable TableAdapter::select_rows(const Glib::ustring& where, const Glib::ustring& sort)
{
	DataTable data_table(table);
	Glib::ustring command = select_command;
	if (where.length() > 0)
	{
		command += Glib::ustring::compose(" WHERE %1", where);
	}
	if (sort.length() > 0)
	{
		command += Glib::ustring::compose(" ORDER BY %1", sort);
	}
	
    Statement statement(connection, command);
	while (statement.step() == SQLITE_ROW)
	{
		Row row;
		
		for (Columns::const_iterator j = table.columns.begin(); j != table.columns.end(); j++)
		{
			const Column& column = *j;
			
			switch (column.type)
			{
			case DATA_TYPE_INTEGER:
				row[column.name].int_value = statement.get_int(column.index);
				break;
			case DATA_TYPE_STRING:
				row[column.name].string_value = statement.get_text(column.index);
				break;
			default:
				break;
			}
		}

		data_table.rows.push_back(row);
	}
	return data_table;
}

void TableAdapter::delete_row(guint key)
{
	Glib::ustring where = Glib::ustring::compose("%1 = %2", table.primary_key, key);
	delete_rows(where);
}

void TableAdapter::delete_rows(const Glib::ustring& where)
{
	Glib::ustring command = delete_command;
	if (where.length() > 0)
	{
		command += Glib::ustring::compose(" WHERE %1", where);
	}
    Statement statement(connection, command);
	statement.step();
}

void TableAdapter::update_rows(const Glib::ustring& set, const Glib::ustring& where)
{
	DataTable data_table(table);
	Glib::ustring command = update_command;
	if (set.length() > 0)
	{
		command += Glib::ustring::compose(" %1", set);
	}
	if (where.length() > 0)
	{
		command += Glib::ustring::compose(" WHERE %1", where);
	}
	
    Statement statement(connection, command);
	statement.step();
}

void Connection::vacuum()
{
    Statement statement(*this, "VACUUM");
	statement.step();
}
