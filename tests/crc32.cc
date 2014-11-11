/*
 * Copyright © 2014  Russel Winder
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

#include "catch.hpp"

#include "crc32.h"

/*
 * This implementation of CRC32 always uses 0xffffffffUL as the seed.
 */


/*
 * Some test values gleaned from http://dox.ipxe.org/crc32__test_8c.html
 */

TEST_CASE("0: CRC32 of empty string") {
	std::string str {""};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xffffffff);
}

TEST_CASE("0: CRC32 of hello") {
	std::string str {"hello"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xc9ef5979);
}

TEST_CASE("0: CRC32 of the hello in hello world") {
	std::string str {"hello world"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)5) == 0xc9ef5979);
}

TEST_CASE("0: CRC32 of hello world") {
	std::string str {"hello world"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xf2b5ee7a);
}


/*
 * From http://stackoverflow.com/questions/20963944/test-vectors-for-crc32c
 */

TEST_CASE("1: CRC32 of the quick brown, etc.") {
	std::string str {"The quick brown fox jumps over the lazy dog"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0x22620404);
}

TEST_CASE("1: CRC32 of 123456789") {
	std::string str {"123456789"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xe3069283);
}


/*
 * By using Boost::CRC..
 */

TEST_CASE("Boost: CRC32 of empty string") {
	std::string str {""};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0);
}

TEST_CASE("Boost: CRC32 of hello") {
	std::string str {"hello"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0x3610a686);
}

TEST_CASE("Boost: CRC32 of the hello in hello world") {
	std::string str {"hello world"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)5) == 0x3610a686);
}

TEST_CASE("Boost : CRC32 of hello world") {
	std::string str {"hello world"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xd4a1185);
}

TEST_CASE("Boost: CRC32 of the quick brown, etc.") {
	std::string str {"The quick brown fox jumps over the lazy dog"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xa4d8f35e);
}

TEST_CASE("Boost: CRC32 of 123456789") {
	std::string str {"123456789"};
	REQUIRE(CRC32::calculate((guchar const *)str.c_str(), (gsize)str.size()) == 0xcbf43926);
}
