/*  actualfile.h
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#ifndef ACTUALFILE_H
#define ACTUALFILE_H


#include <sys/types.h> // off64_t


#define ACTUALHANDLE int
#define ACTUALHANDLENULL -1

// #define VERBOSE_FUNCTION_ACTUALFILE
// #define VERBOSE_WARNING_ACTUALFILE


extern int IsActualFile(const char *filename);
extern void ActualFileDelete(const char *filename);
extern void ActualFileRename(const char *origname, const char *newname);

extern ACTUALHANDLE ActualFileOpenForRead(const char *filename);
extern off64_t ActualFileSize(ACTUALHANDLE handle);
extern int ActualFileSeek(ACTUALHANDLE handle, off64_t position);
extern int ActualFileRead(ACTUALHANDLE handle, int bytes, char *buffer);
extern void ActualFileClose(ACTUALHANDLE handle);

extern ACTUALHANDLE ActualFileOpenForWrite(const char *filename);
extern int ActualFileWrite(ACTUALHANDLE handle, int bytes, char *buffer);


#endif /* ACTUALFILE_H */
