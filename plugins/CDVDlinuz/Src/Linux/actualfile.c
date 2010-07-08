/*  actualfile.c

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





#include <errno.h> // errno

#include <fcntl.h> // open()

#include <stdio.h> // rename()

#include <string.h> // strerror()

#include <sys/stat.h> // stat64(), open(), fstat()

#include <sys/types.h> // stat64(), open(), fstat(), lseek64()

#include <unistd.h> // stat64(), fstat(), lseek64(), read(), close(), write()

// unlink()



#include "logfile.h"

#include "actualfile.h"





int IsActualFile(const char *filename) {

  int retval;

  struct stat64 filestat;



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: IsActualFile(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  retval = stat64(filename, &filestat);

  if((retval < 0) || (errno != 0)) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error retrieving stats on %s", filename);

    PrintLog("CDVDiso file:     %i:%s\n", errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1); // Name doesn't exist.

  } // ENDIF- Trouble getting stat on a file?



  if(S_ISREG(filestat.st_mode) == 0)  return(-2); // Not a regular file.

  return(0); // Yep, that's a file.

} // END IsActualFile()





void ActualFileDelete(const char *filename) {

#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileDelete(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  unlink(filename);

} // END ActualFileDelete()





void ActualFileRename(const char *origname, const char *newname) {

#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileRename(%s->%s)", origname, newname);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  rename(origname, newname);

  return;

} // END ActualFileRename()





ACTUALHANDLE ActualFileOpenForRead(const char *filename) {

  int newhandle;



  if(filename == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileOpenForRead(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  newhandle = open(filename, O_RDONLY | O_LARGEFILE);

  if((newhandle < 0) || (errno != 0)) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error opening file %s\n", filename);

    PrintLog("CDVDiso file:     (%i) %i:%s\n", newhandle, errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1);

  } // ENDIF- Error? Abort



  return(newhandle);

} // END ActualFileOpenForRead()





off64_t ActualFileSize(ACTUALHANDLE handle) {

  int retval;

  struct stat64 filestat;



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileSize()\n");

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  retval = fstat64(handle, &filestat);

  if((retval < 0) || (errno != 0))  return(-1); // Name doesn't exist.

  return(filestat.st_size);

} // END ActualFileSize()





int ActualFileSeek(ACTUALHANDLE handle, off64_t position) {

  off64_t moved;



  if(handle < 0)  return(-1);

  if(position < 0)  return(-1); // Maybe... position = 0?



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileSeek(%lli)", position);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  moved = lseek64(handle, position, SEEK_SET);

  if(errno != 0) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error on seek (%lli)", position);

    PrintLog("CDVDiso file:     %i:%s\n", errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1);

  } // ENDIF- Error? Abort



  return(0);

} // END ActualFileSeek()





int ActualFileRead(ACTUALHANDLE handle, int bytes, char *buffer) {

  int retval;



  if(handle == ACTUALHANDLENULL)  return(-1);

  if(bytes < 1)  return(-1);

  if(buffer == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileRead(%i)", bytes);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  retval = read(handle, buffer, bytes);

  if((retval < 0) || (errno != 0)) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error reading from file!");

    PrintLog("CDVDiso file:     %i:%s", errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    // return(-1);

  } // ENDIF- Error? Abort



  return(retval); // Send back how many bytes read

} // END ActualFileRead()





void ActualFileClose(ACTUALHANDLE handle) {

  if(handle < 0)  return;



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileClose()");

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  close(handle);

  return;

} // END ActualFileClose()





ACTUALHANDLE ActualFileOpenForWrite(const char *filename) {

  int newhandle;



  if(filename == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileOpenForWrite(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  newhandle = open(filename, O_WRONLY | O_CREAT | O_LARGEFILE, 0644);

  if((newhandle < 0) || (errno != 0)) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error opening file %s", filename);

    PrintLog("CDVDiso file:     (%i) %i:%s", newhandle, errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1);

  } // ENDIF- Error? Abort



  return(newhandle);

} // END ActualFileOpenForWrite()





int ActualFileWrite(ACTUALHANDLE handle, int bytes, char *buffer) {

  int retval;



  if(handle < 0)  return(-1);

  if(bytes < 1)  return(-1);

  if(buffer == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileWrite(%i)", bytes);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  errno = 0;

  retval = write(handle, buffer, bytes);

  if((retval < 0) || (errno != 0)) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error writing to file!");

    PrintLog("CDVDiso file:     %i:%s", errno, strerror(errno));

#endif /* VERBOSE_WARNING_ACTUALFILE */

    // return(-1);

  } // ENDIF- Error? Abort



  return(retval); // Send back how many bytes written

} // END ActualFileWrite()

