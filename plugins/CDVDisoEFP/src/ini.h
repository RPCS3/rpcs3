/*  ini.h
 *  Copyright (C) 2002-2005  PCSX2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#ifndef INI_H
#define INI_H


// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */

// #define CDVDdefs
// #include "PS2Edefs.h"


// File format:
// [section]
// keyword=value

// file - Name of the INI file
// section - Section within the file
// keyword - Identifier for a value
// value - value to store with a keyword in a section in the file
// buffer - place to retrieve the value of a keyword

// return values: 0 = success, -1 = failure


// #define VERBOSE_FUNCTION_INI

#define INIMAXLEN 255


extern int INISaveString(char *file, char *section, char *keyword, char *value);
extern int INILoadString(char *file, char *section, char *keyword, char *buffer);

extern int INISaveUInt(char *file, char *section, char *keyword, unsigned int value);
extern int INILoadUInt(char *file, char *section, char *keyword, unsigned int *buffer);

// NULL in the keyword below removes the whole section.
extern int INIRemove(char *file, char *section, char *keyword);


#endif /* INI_H */
