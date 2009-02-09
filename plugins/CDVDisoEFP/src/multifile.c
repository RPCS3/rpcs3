/*  multifile.c

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

#include <sys/types.h> // off64_t



// #ifndef __LINUX__

// #ifdef __linux__

// #define __LINUX__

// #endif /* __linux__ */

// #endif /* No __LINUX__ */



// #define CDVDdefs

// #include "PS2Edefs.h"



#include "logfile.h"

#include "isofile.h"

#include "isocompress.h"

#include "actualfile.h"

#include "multifile.h"





#define FILESIZELIMIT 2000000000





char *multinames[] = {

  "",

  "Multiple ",

  NULL };





void IsoNameStripMulti(struct IsoFile *isofile) {

  isofile->multi = 0;

  if(isofile->namepos < 2)  return; // Not enough digits

  if(isofile->name[isofile->namepos - 1] < '0')  return; // Ex: -I0[0]

  if(isofile->name[isofile->namepos - 1] > '1')  return; // Ex: -I0[1]

  if(isofile->name[isofile->namepos - 2] != '0')  return; // Ex: -I[0]0



  isofile->multi = 1;

  isofile->multipos = isofile->namepos - 1;

  isofile->namepos -= 2;

  isofile->multistart = isofile->name[isofile->multipos] - '0';

  isofile->multiend = isofile->multistart;

  isofile->multinow = isofile->multistart;

  isofile->multisectorend[0] = 0; // Sometimes the file name starts with '1'

  isofile->multisectorend[isofile->multistart] = isofile->filesectorsize;

  isofile->multioffset = 0;



  if(isofile->namepos < 1)  return;

  if(isofile->name[isofile->namepos - 1] != 'I')  return; // Ex: -[I]00

  isofile->namepos--;



  if(isofile->namepos < 2)  return; // Filename doesn't start with '-'

  if(isofile->name[isofile->namepos - 1] != '-')  return; // Ex: [-]I00

  isofile->namepos--;



  return;

} // END IsoNameStripMulti()





int MultiFileSeek(struct IsoFile *isofile, off64_t sector) {

  int multinext;

  int retval;

  off64_t tempfilesector;



#ifdef VERBOSE_FUNCTION_MULTIFILE

  PrintLog("CDVD multifile: MultiFileSeek(%llu)", sector);

#endif /* VERBOSE_FUNCTION_MULTIFILE */



  multinext = isofile->multinow;



  // Do we need to back up a file or so?

  while((multinext > isofile->multistart) && 

        (sector < isofile->multisectorend[multinext - 1]))  multinext--;



  // Do we need to go forward a file or two (that we know about?)

  while((multinext < isofile->multiend) &&

        (sector >= isofile->multisectorend[multinext]))  multinext++;



  // Do we need to go forward a file or two (that we *don't* know about?)

  while((multinext < 9) &&

        (sector >= isofile->multisectorend[multinext])) {

    if(isofile->compress > 0) {

      CompressClose(isofile);

    } else {

      ActualFileClose(isofile->handle);

      isofile->handle = ACTUALHANDLENULL;

    } // ENDIF- Close a compressed file? (or an uncompressed one?)



    multinext++;



    isofile->name[isofile->multipos] = '0' + multinext;

    if(isofile->compress > 0) {

      retval = CompressOpenForRead(isofile);



    } else {

      isofile->handle = ActualFileOpenForRead(isofile->name);

      retval = 0;

      if(isofile->handle == ACTUALHANDLENULL) {

        retval = -1;



      } else {

        isofile->filebytesize = ActualFileSize(isofile->handle);

        isofile->filesectorsize = isofile->filebytesize / isofile->blocksize;

        isofile->filebytepos = 0;

        isofile->filesectorpos = 0;

      } // ENDIF- Failed to open the file raw?

    } // ENDIF- Compressed or non-compressed? What a question.



    if(retval < 0) {

      if(isofile->compress > 0) {

        CompressClose(isofile);

      } else {

        ActualFileClose(isofile->handle);

        isofile->handle = ACTUALHANDLENULL;

      } // ENDIF- Close a compressed file? (or an uncompressed one?)



      multinext--;



      isofile->name[isofile->multipos] = '0' + multinext;

      if(isofile->compress > 0) {

        CompressOpenForRead(isofile);

      } else {

        isofile->handle = ActualFileOpenForRead(isofile->name);

        isofile->filebytesize = ActualFileSize(isofile->handle);

        isofile->filesectorsize = isofile->filebytesize / isofile->blocksize;

        isofile->filebytepos = 0;

        isofile->filesectorpos = 0;

      } // ENDIF- Compressed or non-compressed? What a question.



      isofile->multinow = multinext;

      if(isofile->multinow == 0) {

        isofile->multioffset = 0;

      } else {

        isofile->multioffset = isofile->multisectorend[isofile->multinow - 1];

      } // ENDIF- At the start of the list? Offset 0.

      return(-1);

    } // ENDIF- Failed to open next in series? Revert and abort.



    isofile->multinow = multinext;

    isofile->multiend = multinext;

    isofile->multioffset = isofile->multisectorend[multinext - 1];

    isofile->multisectorend[multinext] = isofile->multisectorend[multinext - 1]

                                       + isofile->filesectorsize;

#ifdef VERBOSE_DISC_INFO

    PrintLog("CDVD multifile:   File %i opened, %llu sectors found (%llu sectors total)",

             multinext,

             isofile->filesectorsize,

             isofile->multisectorend[multinext]);

#endif /* VERBOSE_DISC_INFO */

  } // ENDWHILE- searching through new files for a high enough end-mark



  if(multinext != isofile->multinow) {

#ifdef VERBOSE_WARNING_MULTIFILE

    PrintLog("CDVD multifile:   Changing to File %i", multinext);

#endif /* VERBOSE_WARNING_MULTIFILE */

    if(isofile->compress > 0) {

      CompressClose(isofile);

    } else {

      ActualFileClose(isofile->handle);

      isofile->handle = ACTUALHANDLENULL;

    } // ENDIF- Close a compressed file? (or an uncompressed one?)



    isofile->name[isofile->multipos] = '0' + multinext;

    if(isofile->compress > 0) {

      CompressOpenForRead(isofile);

    } else {

      isofile->handle = ActualFileOpenForRead(isofile->name);

      if(isofile->handle == ACTUALHANDLENULL)  return(-1); // Couldn't re-open?

      isofile->filebytesize = ActualFileSize(isofile->handle);

      isofile->filesectorsize = isofile->filebytesize / isofile->blocksize;

      isofile->filebytepos = 0;

      isofile->filesectorpos = 0;

    } // ENDIF- Compressed or non-compressed? What a question.



    isofile->multinow = multinext;

    if(multinext == 0) {

      isofile->multioffset = 0;

    } else {

      isofile->multioffset = isofile->multisectorend[multinext - 1];

    } // ENDIF- At the start of the list? Offset 0.

  } // ENDIF- Not looking at the same file? Change to the new one.



  tempfilesector = sector - isofile->multioffset;

  if(isofile->compress > 0) {

    return(CompressSeek(isofile, tempfilesector));

  } else {

    retval = ActualFileSeek(isofile->handle,

                            (tempfilesector * isofile->blocksize) 

                                            + isofile->imageheader);

    if(retval == 0) {

      isofile->filesectorpos = sector;

      isofile->filebytepos = (sector * isofile->blocksize)

                                     + isofile->imageheader;

    } // ENDIF- Sucessful? Adjust internals

    return(retval);

  } // ENDIF- Seek a position in a compressed file?

} // END MultiFileSeek()





int MultiFileRead(struct IsoFile *isofile, char *block) {

  int retval;



#ifdef VERBOSE_FUNCTION_MULTIFILE

  PrintLog("CDVD multifile: MultiFileRead()");

#endif /* VERBOSE_FUNCTION_MULTIFILE */



  if(isofile->filesectorpos >= isofile->filesectorsize)

    MultiFileSeek(isofile, isofile->sectorpos);



  if(isofile->compress > 0) {

    return(CompressRead(isofile, block));

  } else {

    retval = ActualFileRead(isofile->handle, isofile->blocksize, block);

    if(retval > 0)  isofile->filebytepos += retval;

    if(retval == isofile->blocksize)  isofile->filesectorpos++;

    return(retval);

  } // ENDIF- Read a compressed sector?

} // END MultiFileRead()





int MultiFileWrite(struct IsoFile *isofile, char *block) {

  int retval;



#ifdef VERBOSE_FUNCTION_MULTIFILE

  PrintLog("CDVD multifile: MultiFileWrite()");

#endif /* VERBOSE_FUNCTION_MULTIFILE */



  if(isofile->filebytesize + isofile->blocksize > FILESIZELIMIT) {

    if(isofile->compress > 0) {

      CompressClose(isofile);

    } else {

      ActualFileClose(isofile->handle);

      isofile->handle = ACTUALHANDLENULL;

    } // ENDIF- Close a compressed file? (or an uncompressed one?)

    if(isofile->multinow == 9)  return(-1); // Over 10 files? Overflow!



    isofile->multioffset += isofile->filesectorsize;

    isofile->multinow++;

    isofile->multiend++;

#ifdef VERBOSE_WARNING_MULTIFILE

    PrintLog("CDVD multifile:   Changing to File %i", isofile->multinow);

#endif /* VERBOSE_WARNING_MULTIFILE */



    isofile->name[isofile->multipos] = '0' + isofile->multinow;

    if(isofile->compress > 0) {

      retval = CompressOpenForWrite(isofile);

    } else {

      isofile->handle = ActualFileOpenForWrite(isofile->name);

      if(isofile->handle == ACTUALHANDLENULL) {

        retval = -1;

      } else {

        retval = 0;

        isofile->filebytesize = 0;

        isofile->filesectorsize = 0;

        isofile->filebytepos = 0;

        isofile->filesectorpos = 0;

      } // ENDIF- Trouble opening next file?

    } // ENDIF- Opening the next compressed file? (Or uncompressed?)

    if(retval < 0)  return(-1); // Couldn't open another file? Abort.

  } // ENDIF- Hit the size limit? Move on to next file...



  if(isofile->compress > 0) {

    return(CompressWrite(isofile, block));

  } else {

    retval = ActualFileWrite(isofile->handle, isofile->blocksize, block);

    if(retval > 0)  isofile->filebytepos += retval;

    if(retval == isofile->blocksize)  isofile->filesectorpos++;

    return(retval);

  } // ENDIF- Write a compressed sector?

} // END MultiFileWrite()

