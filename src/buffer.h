/*
 * Me TV — A GTK+ client for watching and recording DVB.
 *
 *  Copyright (C) 2011 Michael Lamothe
 *  Copyright © 2014  Russel Winder
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <glib.h>

class Buffer {
private:
	guchar * buffer;
	gsize length;
	static guint get_bits(guchar * buffer, guint position, gsize count);

public:
	Buffer();
	Buffer(gsize length);
	~Buffer();
	void dump() const;
	void clear();
	void set_length(gsize length);
	gsize get_length() const { return length; }
	guchar * get_buffer() const { return buffer; }
	guint get_bits(guint offset, guint position, gsize count) const;
	guint get_bits(guint position, gsize count) const;
	guint32 crc32() const;

	guchar operator[](int index) const { return buffer[index]; };
};

#endif
