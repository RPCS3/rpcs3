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
 * Default cursor - it happens to be the Mac cursor, but could be anything   */

#define DEFAULT_CWIDTH	16
#define DEFAULT_CHEIGHT	16
#define DEFAULT_CHOTX	0
#define DEFAULT_CHOTY	0

/* Added a real MacOS cursor, at the request of Luc-Olivier de Charrière */
#define USE_MACOS_CURSOR

#ifdef USE_MACOS_CURSOR

static const unsigned char default_cdata[] = {
    0x00, 0x00,
    0x40, 0x00,
    0x60, 0x00,
    0x70, 0x00,
    0x78, 0x00,
    0x7C, 0x00,
    0x7E, 0x00,
    0x7F, 0x00,
    0x7F, 0x80,
    0x7C, 0x00,
    0x6C, 0x00,
    0x46, 0x00,
    0x06, 0x00,
    0x03, 0x00,
    0x03, 0x00,
    0x00, 0x00
};

static const unsigned char default_cmask[] = {
    0xC0, 0x00,
    0xE0, 0x00,
    0xF0, 0x00,
    0xF8, 0x00,
    0xFC, 0x00,
    0xFE, 0x00,
    0xFF, 0x00,
    0xFF, 0x80,
    0xFF, 0xC0,
    0xFF, 0xE0,
    0xFE, 0x00,
    0xEF, 0x00,
    0xCF, 0x00,
    0x87, 0x80,
    0x07, 0x80,
    0x03, 0x00
};

#else

static const unsigned char default_cdata[] = {
    0x00, 0x00,
    0x40, 0x00,
    0x60, 0x00,
    0x70, 0x00,
    0x78, 0x00,
    0x7C, 0x00,
    0x7E, 0x00,
    0x7F, 0x00,
    0x7F, 0x80,
    0x7C, 0x00,
    0x6C, 0x00,
    0x46, 0x00,
    0x06, 0x00,
    0x03, 0x00,
    0x03, 0x00,
    0x00, 0x00
};

static const unsigned char default_cmask[] = {
    0x40, 0x00,
    0xE0, 0x00,
    0xF0, 0x00,
    0xF8, 0x00,
    0xFC, 0x00,
    0xFE, 0x00,
    0xFF, 0x00,
    0xFF, 0x80,
    0xFF, 0xC0,
    0xFF, 0x80,
    0xFE, 0x00,
    0xEF, 0x00,
    0x4F, 0x00,
    0x07, 0x80,
    0x07, 0x80,
    0x03, 0x00
};

#endif /* TRUE_MACINTOSH_CURSOR */
/* vi: set ts=4 sw=4 expandtab: */
