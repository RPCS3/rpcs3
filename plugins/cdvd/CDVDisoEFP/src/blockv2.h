/*  blockv2.h

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





#ifndef BLOCKV2_H

#define BLOCKV2_H





#include <sys/types.h>



#include "isofile.h"

#include "isocompress.h"





// #define VERBOSE_FUNCTION_BLOCKV2

// #define VERBOSE_WARNING_BLOCKV2





extern int BlockV2OpenTableForRead(struct IsoFile *isofile);

extern int BlockV2SeekTable(struct IsoFile *isofile, off64_t sector);

extern int BlockV2ReadTable(struct IsoFile *isofile, struct TableData *table);



extern int BlockV2OpenTableForWrite(struct IsoFile *isofile);

extern int BlockV2WriteTable(struct IsoFile *isofile, struct TableData table);



extern int BlockV2OpenForRead(struct IsoFile *isofile);

extern int BlockV2Seek(struct IsoFile *isofile, off64_t sector);

extern int BlockV2Read(struct IsoFile *isofile, int bytes, char *buffer);

extern void BlockV2Close(struct IsoFile *isofile);



extern int BlockV2OpenForWrite(struct IsoFile *isofile);

extern int BlockV2Write(struct IsoFile *isofile, char *buffer);





#endif /* BLOCKV2_H */

