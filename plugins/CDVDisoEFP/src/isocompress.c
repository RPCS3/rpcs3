/*  isocompress.c

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





#include <stddef.h> // NULL



// #ifndef __LINUX__

// #ifdef __linux__

// #define __LINUX__

// #endif /* __linux__ */

// #endif /* No __LINUX__ */



// #define CDVDdefs

// #include "PS2Edefs.h"



#include "logfile.h"

#include "isofile.h"

#include "actualfile.h"

#include "gzipv1.h"

#include "blockv2.h"

#include "gzipv2.h"

#include "bzip2v2.h"

#include "bzip2v3.h"

#include "isocompress.h"





const char *compressnames[] = {

  "No Compression",

  ".Z (zlib) for speed",

  ".BZ2 (bzip2) for speed",

  ".bz2 (bzip2) for size",

 NULL }; // Compress types 0, 3, 4, and 5



const char *compressdesc[] = {

  "",

  " zlib orig",

  " block.dump",

  " zlib speed",

  " bzip2 speed",

  " bzip2 size",

  NULL };



const char *compressid[] = {

  "BVD2",

  "Z V2",

  "BZV2",

  "BZV3",

  NULL }; // Starts at compress type 2



struct CompressExt compressext[] = {

  { ".z", 1 },

  { ".Z", 1 },

  { ".bz2", 5 },

  { ".BZ2", 3 },

  { ".bZ2", 3 },

  { ".Bz2", 3 },

  { ".bz", 3 },

  { ".BZ", 3 },

  { ".bZ", 3 },

  { ".Bz", 3 },

  { ".dump", 2 },

  { NULL, 0 }

};





int IsoNameStripCompress(struct IsoFile *isofile) {

  int tempext;

  int tempnamepos;

  int tempextpos;

  int retmethod;



  retmethod = 0;

  tempext = 0;

  while(compressext[tempext].name != NULL) {

    tempextpos = 0;

    while(*(compressext[tempext].name + tempextpos) != 0)  tempextpos++;



    tempnamepos = isofile->namepos;

    while((tempnamepos > 0) && (tempextpos > 0) &&

          (isofile->name[tempnamepos - 1] == *(compressext[tempext].name + tempextpos - 1))) {

      tempnamepos--;

      tempextpos--;

    } // ENDWHILE- Comparing one extension to the end of the file name

    if(tempextpos == 0) {

      isofile->namepos = tempnamepos; // Move name pointer in front of ext.

      return(compressext[tempext].method); // Found a match... say which one.

    } else {

      tempext++; // Next ext in the list to test...

    } // ENDIF- Did we find a match?

  } // ENDWHILE- looking through extension list



  return(0); // No compress extension found.

} // END IsoNameStripCompress()





int CompressOpenForRead(struct IsoFile *isofile) {

  int retval;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: OpenForRead()");

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      retval = GZipV1OpenForRead(isofile);

      if(retval >= 0)  retval = GZipV1OpenTableForRead(isofile);

      break;

    case 2:

      retval = BlockV2OpenForRead(isofile);

      if(retval >= 0)  retval = BlockV2OpenTableForRead(isofile);

      break;

    case 3:

      retval = GZipV2OpenForRead(isofile);

      if(retval >= 0)  retval = GZipV2OpenTableForRead(isofile);

      break;

    case 4:

      retval = BZip2V2OpenForRead(isofile);

      if(retval >= 0)  retval = BZip2V2OpenTableForRead(isofile);

      break;

    case 5:

      retval = BZip2V3OpenForRead(isofile);

      if(retval >= 0)  retval = BZip2V3OpenTableForRead(isofile);

      break;

    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  return(retval);

} // END CompressOpenForRead()





int CompressSeek(struct IsoFile *isofile, off64_t sector) {

  int retval;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: Seek(%lli)", sector);

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      retval = GZipV1SeekTable(isofile, sector);

      break;

    case 2:

      retval = BlockV2SeekTable(isofile, sector);

      break;

    case 3:

      retval = GZipV2SeekTable(isofile, sector);

      break;

    case 4:

      retval = BZip2V2SeekTable(isofile, sector);

      break;

    case 5:

      retval = BZip2V3SeekTable(isofile, sector);

      break;

    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  return(retval);

} // END CompressSeek()





int CompressRead(struct IsoFile *isofile, char *buffer) {

  struct TableData table;

  int retval;

  int compptr;

  int i;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: Read()");

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      retval = GZipV1ReadTable(isofile, &table);

      if(retval >= 0) {

        if(table.offset != isofile->filebytepos) {

          retval = GZipV1Seek(isofile, table.offset);

        } // ENDIF- The data file not in position?

      } // ENDIF- Did we get a table entry?

      if(retval >= 0) {

        retval = GZipV1Read(isofile, table.size, buffer);

      } // ENDIF- Are we still on track?

      break;



    case 2:

      retval = BlockV2ReadTable(isofile, &table);

      if(retval >= 0) {

        if(table.offset != isofile->filebytepos) {

          retval = BlockV2Seek(isofile, table.offset);

        } // ENDIF- The data file not in position?

      } // ENDIF- Did we get a table entry?

      if(retval >= 0) {

        retval = BlockV2Read(isofile, table.size, buffer);

      } // ENDIF- Are we still on track?

      break;





    case 3:

      retval = GZipV2ReadTable(isofile, &table);

      if(retval >= 0) {

        if(table.offset != isofile->filebytepos) {

          retval = GZipV2Seek(isofile, table.offset);

        } // ENDIF- The data file not in position?

      } // ENDIF- Did we get a table entry?

      if(retval >= 0) {

        retval = GZipV2Read(isofile, table.size, buffer);

      } // ENDIF- Are we still on track?

      break;



    case 4:

      retval = 0;

      if((isofile->filesectorpos < isofile->compsector) ||

         (isofile->filesectorpos > isofile->compsector + isofile->numsectors - 1)) {



        retval = BZip2V2ReadTable(isofile, &table);

        if(retval >= 0) {

          if(table.offset != isofile->filebytepos) {

            retval = BZip2V2Seek(isofile, table.offset);

          } // ENDIF- The data file not in position?

        } // ENDIF- Did we get a table entry?

        if(retval >= 0) {

          retval = BZip2V2Read(isofile, table.size, isofile->compblock);

          isofile->compsector = isofile->filesectorpos / isofile->numsectors;

          isofile->compsector *= isofile->numsectors;

        } // ENDIF- Are we still on track?

      } // ENDIF- Did we have to read in another block?



      if(retval >= 0) {

        compptr = isofile->filesectorpos - isofile->compsector;

        compptr *= isofile->blocksize;

        if((compptr < 0) || (compptr > (65535 - isofile->blocksize))) {

          retval = -1;

        } else {

          for(i = 0; i < isofile->blocksize; i++)

            *(buffer + i) = isofile->compblock[compptr + i];

          isofile->filesectorpos++;

        } // ENDIF- Not a good buffer pointer? Say so.

      } // ENDIF- Do we have a valid buffer to draw from?

      break;



    case 5:

      retval = 0;

      if((isofile->filesectorpos < isofile->compsector) ||

         (isofile->filesectorpos > isofile->compsector + isofile->numsectors - 1)) {



        if(isofile->filesectorpos != isofile->compsector + isofile->numsectors) {

          retval = BZip2V3ReadTable(isofile, &table);

          if(retval >= 0) {

            if(table.offset != isofile->filebytepos) {

              retval = BZip2V3Seek(isofile, table.offset);

            } // ENDIF- The data file not in position?

          } // ENDIF- Did we get a table entry?

        } // ENDIF- Not the next block in the batch? Seek then.



        if(retval >= 0) {

          retval = BZip2V3Read(isofile, 0, isofile->compblock);

          isofile->compsector = isofile->filesectorpos / isofile->numsectors;

          isofile->compsector *= isofile->numsectors;

        } // ENDIF- Are we still on track?

      } // ENDIF- Did we have to read in another block?



      if(retval >= 0) {

        compptr = isofile->filesectorpos - isofile->compsector;

        compptr *= isofile->blocksize;

        if((compptr < 0) || (compptr > (65535 - isofile->blocksize))) {

          retval = -1;

        } else {

          for(i = 0; i < isofile->blocksize; i++)

            *(buffer + i) = isofile->compblock[compptr + i];

          isofile->filesectorpos++;

        } // ENDIF- Not a good buffer pointer? Say so.

      } // ENDIF- Do we have a valid buffer to draw from?

      break;



    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  if(retval >= 0)  retval = isofile->blocksize;

  return(retval);

} // END CompressRead()





void CompressClose(struct IsoFile *isofile) {

  int retval;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: Close()");

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      GZipV1Close(isofile);

      break;

    case 2:

      BlockV2Close(isofile);

      break;

    case 3:

      GZipV2Close(isofile);

      break;

    case 4:

      BZip2V2Close(isofile);

      break;

    case 5:

      BZip2V3Close(isofile);

      break;

    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  return;

} // END CompressClose()





int CompressOpenForWrite(struct IsoFile *isofile) {

  int retval;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: OpenForWrite()");

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      retval = GZipV1OpenForWrite(isofile);

      if(retval >= 0)  retval = GZipV1OpenTableForWrite(isofile);

      break;

    case 2:

      retval = -1;

      break;

    case 3:

      retval = GZipV2OpenForWrite(isofile);

      if(retval >= 0)  retval = GZipV2OpenTableForWrite(isofile);

      break;

    case 4:

      retval = BZip2V2OpenForWrite(isofile);

      if(retval >= 0)  retval = BZip2V2OpenTableForWrite(isofile);

      break;

    case 5:

      retval = BZip2V3OpenForWrite(isofile);

      if(retval >= 0)  retval = BZip2V3OpenTableForWrite(isofile);

      break;

    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  return(retval);

} // END CompressOpenForWrite()





int CompressWrite(struct IsoFile *isofile, char *buffer) {

  struct TableData table;

  int compptr;

  int retval;

  int i;



#ifdef VERBOSE_FUNCTION_ISOCOMPRESS

  PrintLog("CDVDiso compress: Write()");

#endif /* VERBOSE_FUNCTION_ISOCOMPRESS */



  switch(isofile->compress) {

    case 1:

      retval = GZipV1Write(isofile, buffer);

      if(retval > 0) {

        table.offset = isofile->filebytepos - retval;

        table.size = retval;

        retval = GZipV1WriteTable(isofile, table);

      } // ENDIF- Wrote the data out? Update the table as well.

      break;



    case 2:

      retval = -1;

      break;



    case 3:

      retval = GZipV2Write(isofile, buffer);

      if(retval > 0) {

        table.offset = isofile->filebytepos - retval;

        table.size = retval;

        retval = GZipV2WriteTable(isofile, table);

      } // ENDIF- Wrote the data out? Update the table as well.

      break;



    case 4:

      retval = 0;

      if((isofile->filesectorpos < isofile->compsector) ||

         (isofile->filesectorpos > isofile->compsector + isofile->numsectors - 1)) {

        retval = BZip2V2Write(isofile, isofile->compblock);

        isofile->compsector += isofile->numsectors;

        if(retval > 0) {

          table.offset = isofile->filebytepos - retval;

          table.size = retval;

          retval = BZip2V2WriteTable(isofile, table);

        } // ENDIF- Wrote the data out? Update the table as well.

      } // ENDIF- Do we have a full buffer to write out?



      if(retval >= 0) {

        compptr = isofile->filesectorpos - isofile->compsector;

        compptr *= isofile->blocksize;

        for(i = 0; i < isofile->blocksize; i++)

          isofile->compblock[compptr + i] = *(buffer + i);

      } // ENDIF- Do we have a valid buffer to draw from?

      isofile->filesectorpos++;

      break;



    case 5:

      retval = 0;

      if((isofile->filesectorpos < isofile->compsector) ||

         (isofile->filesectorpos > isofile->compsector + isofile->numsectors - 1)) {

        retval = BZip2V3Write(isofile, isofile->compblock);

        isofile->compsector += isofile->numsectors;

        if(retval > 0) {

          table.offset = isofile->filebytepos - retval;

          table.size = retval;

          retval = BZip2V3WriteTable(isofile, table);

        } // ENDIF- Wrote the data out? Update the table as well.

      } // ENDIF- Do we have a full buffer to write out?



      if(retval >= 0) {

        compptr = isofile->filesectorpos - isofile->compsector;

        compptr *= isofile->blocksize;

        for(i = 0; i < isofile->blocksize; i++)

          isofile->compblock[compptr + i] = *(buffer + i);

      } // ENDIF- Do we have a valid buffer to draw from?

      isofile->filesectorpos++;

      break;



    default:

      retval = -1;

      break;

  } // ENDSWITCH compress- which method do we try to get header info from?



  if(retval >= 0)  retval = isofile->blocksize;

  return(retval);

} // END CompressWrite()

