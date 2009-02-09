//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

extern FILE *spu2Log;

void FileLog(const char *fmt, ...);
void ConLog(const char *fmt, ...);

void DoFullDump();

extern int wavedump_ok;

int wavedump_open();
void wavedump_close();
void wavedump_write(s16 left,s16 right);


#endif // DEBUG_H_INCLUDED //
