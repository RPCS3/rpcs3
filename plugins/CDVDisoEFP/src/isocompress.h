/*  isocompress.h

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





#ifndef ISOCOMPRESS_H

#define ISOCOMPRESS_H





#include <sys/types.h>



#include "isofile.h"

#include "actualfile.h"





// #define VERBOSE_FUNCTION_ISOCOMPRESS

// #define VERBOSE_WARNING_ISOCOMPRESS





struct CompressExt {

  const char *name;

  int method;

};



#ifdef _WIN32

#pragma pack(1)

#endif /* _WIN32 */



struct TableData {

  off64_t offset;

  unsigned short size;

#ifdef _WIN32

};

#else

} __attribute__ ((packed));

#endif /* _WIN32 */



union TableMap {

  struct TableData table;

  char ch[sizeof(struct TableData)];

};





extern const char *compressnames[];

extern const char *compressdesc[];



extern struct CompressExt compressext[];





extern int IsoNameStripCompress(struct IsoFile *isofile);



// 0 = success, -1 = Failure w/data, -2 = Failure w/table

extern int CompressOpenForRead(struct IsoFile *isofile);



extern int CompressSeek(struct IsoFile *isofile, off64_t sector);

extern int CompressRead(struct IsoFile *isofile, char *buffer);

extern void CompressClose(struct IsoFile *isofile);



extern int CompressOpenForWrite(struct IsoFile *isofile);

extern int CompressWrite(struct IsoFile *isofile, char *buffer);





#endif /* ISOCOMPRESS_H */

