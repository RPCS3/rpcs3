/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * A default blank 8x8 cursor                                                */

#define BLANK_CWIDTH	8
#define BLANK_CHEIGHT	8
#define BLANK_CHOTX	0
#define BLANK_CHOTY	0

static const unsigned char blank_cdata[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char blank_cmask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/* vi: set ts=4 sw=4 expandtab: */
