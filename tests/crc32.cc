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
 * These test values stem from the entry for CRC32/MPEG-2 at
 * http://reveng.sourceforge.net/crc-catalogue/all.htm
 */

constexpr guchar const * zero_length_sequence = (guchar const *)"";
constexpr gsize const length_of_zero_length_sequence = (gsize)0;
constexpr guint32 result_for_zero_length_sequence = 0xffffffff;

constexpr guchar const * check_sequence = (guchar const *)"123456789";
constexpr gsize const length_of_check_sequence = (gsize)9;
constexpr guint32 result_for_check_sequence = 0x0376e6e7;


TEST_CASE("CRC32 of empty sequence, address, length") {
	REQUIRE(CRC32::calculate(zero_length_sequence, length_of_zero_length_sequence) == result_for_zero_length_sequence);
}

TEST_CASE("CRC32 of test sequence, address, length") {
	REQUIRE(CRC32::calculate(check_sequence, length_of_check_sequence) == result_for_check_sequence);
}

TEST_CASE("CRC32 of empty sequence, address, address") {
	REQUIRE(CRC32::calculate(zero_length_sequence, zero_length_sequence) == result_for_zero_length_sequence);
}

TEST_CASE("CRC32 of test sequence, address, address") {
	REQUIRE(CRC32::calculate(check_sequence, &(check_sequence[length_of_check_sequence])) == result_for_check_sequence);
}
