/* crc32.c
	Copyright (C) 2009-2020 Free Software Foundation, Inc.

	This file is part of the libiberty library.

	This file is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	In addition to the permissions in the GNU General Public License, the
	Free Software Foundation gives you unlimited permission to link the
	compiled version of this file into combinations with other programs,
	and to distribute those combinations without any restriction coming
	from the use of this file.  (The General Public License restrictions
	do apply in other respects; for example, they cover modification of
	the file, and distribution when not linked into a combined
	executable.)

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include <stdint.h>

#include "crc32.h"

extern const uint32_t crc32_table[];

/*
@deftypefn Extension {unsigned int} crc32 (const unsigned char *@var{buf}, @
	int @var{len}, unsigned int @var{init})

Compute the 32-bit CRC of @var{buf} which has length @var{len}.  The
starting value is @var{init}; this may be used to compute the CRC of
data split across multiple buffers by passing the return value of each
call as the @var{init} parameter of the next.

This is used by the @command{gdb} remote protocol for the @samp{qCRC}
command.  In order to get the same results as gdb for a block of data,
you must pass the first CRC parameter as @code{0xffffffff}.

This CRC can be specified as:

	Width  : 32
	Poly   : 0x04c11db7
	Init   : parameter, typically 0xffffffff
	RefIn  : false
	RefOut : false
	XorOut : 0

This differs from the "standard" CRC-32 algorithm in that the values
are not reflected, and there is no final XOR value.  These differences
make it easy to compose the values of multiple blocks.

@end deftypefn

*/

uint32_t xcrc32 (const uint8_t *buf, uint32_t len, uint32_t init)
{
	uint32_t crc = init;
	while (len--)
	{
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}
