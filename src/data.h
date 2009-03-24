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

#ifndef __DATA_H__
#define __DATA_H__

#include "me-tv.h"
#include <sqlite3.h>
#include <linux/dvb/frontend.h>
#include "channel.h"
#include "epg_event.h"
#include "scheduled_recording.h"

namespace Data
{
	class Value
	{
	public:
		int				int_value;
		Glib::ustring	string_value;
	};
	
	typedef std::map<Glib::ustring, Value> Parameters;
	
	class Statement
	{
	private:
		Glib::RecMutex::Lock	lock;
		sqlite3_stmt*			statement;
		sqlite3*				connection;
		Glib::ustring			command;

	public:
		Statement(sqlite3* connection, const Glib::ustring& command);
		~Statement();

		void reset();
		gint step();
		gint get_int(guint column);
		const Glib::ustring get_text(guint column);

		int get_parameter_index(const Glib::ustring& name);
			
		void set_int_parameter(guint index, int value);
		void set_string_parameter(guint index, const Glib::ustring& value);
		void set_int_parameter(const Glib::ustring& name, int value);
		void set_string_parameter(const Glib::ustring& name, const Glib::ustring& value);
	};

	class Connection
	{
	private:
		sqlite3*	connection;
	public:
		Connection();
		~Connection();
			
		Statement& create_statement(const Glib::ustring& command)
		{
			return *(new Statement(connection, command));
		}

		int get_last_insert_rowid();
	};
	
	typedef enum
	{
		DATA_TYPE_INTEGER,
		DATA_TYPE_STRING
	} DataType;

	class Column
	{
	public:
		int index;
		Glib::ustring name;
		DataType type;
		guint size;
		gboolean nullable;
		gboolean auto_increment;
	};

	class Columns : public std::vector<Column>
	{
	private:
		std::map<Glib::ustring, Column> columns_by_name;
	public:
		Column& operator[](int index) { return at(index); }
		Column& operator[](const Glib::ustring& name) { return columns_by_name[name]; }

		void add(
			const Glib::ustring& name,
			DataType type,
			guint size,
			gboolean nullable,
			gboolean auto_increment)
		{
			Column column;
			
			column.index = std::vector<Column>::size();
			column.name = name;
			column.type = type;
			column.size = size;
			column.nullable = nullable;
			column.auto_increment = auto_increment;
			
			push_back(column);
			columns_by_name[name] = column;
		}
	};

	typedef enum
	{
		ConstraintTypeUnique
	} ConstraintType;
	
	class Constraint
	{
	public:
		ConstraintType type;
		StringList columns;
	};
	
	class Constraints : public std::list<Constraint>
	{
	public:
		void add_unique(const StringList& columns)
		{
			add(ConstraintTypeUnique, columns);
		}
			
		void add(ConstraintType constraint_type, const StringList& columns)
		{
			Constraint constraint;
			constraint.type = constraint_type;
			constraint.columns = columns;
			push_back(constraint);
		}
	};
	
	class Table
	{
	public:
		Glib::ustring	name;
		Glib::ustring	primary_key;
		Columns			columns;
		Constraints		constraints;
	};
	
	class Tables : public std::map<Glib::ustring, Table>
	{
	public:
		void add(Table& table)
		{
			(*this)[table.name] = table;
		}
	};
	
	class Schema
	{
	public:
		Tables tables;
	};
	
	class SchemaAdapter
	{
	private:
		const Schema& schema;
		Connection& connection;
	public:
		SchemaAdapter(Schema& schema, Connection& connection) :
			schema(schema), connection(connection) {}

		void initialise_schema();
	};
		
	class Row : public std::map<Glib::ustring, Value>
	{
	};

	class Rows : public std::vector<Row>
	{
	public:
		void add(Row& row)
		{
			std::vector<Row>::push_back(row);
		}
	};
		
	class DataTable
	{
	public:
		DataTable(Table& table) : table(table) {}

		Table& table;
		Rows rows;
	};

	class IntegerIndex : std::map<int, Row&>
	{
	};
	
	class StringIndex : std::map<Glib::ustring, Row&>
	{
	};

	class TableAdapter
	{
	private:
		Connection& connection;
		Glib::ustring select_command;
		Glib::ustring replace_command;
		Glib::ustring delete_command;
	public:
		TableAdapter(Connection& connection, Table& table);

		Table& table;

		void delete_row(const Glib::ustring& key);
		void delete_row(guint key);
		void delete_rows(const Glib::ustring& clause = "");

		DataTable select_row(const Glib::ustring& key);
		DataTable select_row(guint key);
		DataTable select_rows(const Glib::ustring& clause = "", const Glib::ustring& sort = "");
			
		void replace(DataTable& data_table);
	};
}

#endif
