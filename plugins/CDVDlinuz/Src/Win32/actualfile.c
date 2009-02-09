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

 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *

 *  PCSX2 members can be contacted through their website at www.pcsx2.net.

 */





#include <windows.h>



#include <stddef.h> // NULL



#include "logfile.h"

#include "actualfile.h"





int IsActualFile(const char *filename) {

  DWORD retval;



  if(filename == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: IsActualFile(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  retval = GetFileAttributes(filename);

  if(retval == INVALID_FILE_ATTRIBUTES)  return(-1); // Name doesn't exist.

  if((retval & FILE_ATTRIBUTE_DIRECTORY) != 0)  return(-2);



  return(0); // Yep, that's a file.

} // END IsActualFile()





void ActualFileDelete(const char *filename) {

#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileDelete(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  DeleteFile(filename);

} // END ActualFileDelete()





void ActualFileRename(const char *origname, const char *newname) {

#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileDelete(%s->%s)", origname, newname);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  MoveFile(origname, newname);

  return;

} // END ActualFileRename()





ACTUALHANDLE ActualFileOpenForRead(const char *filename) {

  HANDLE newhandle;



  if(filename == NULL)  return(NULL);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileOpenForRead(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  newhandle = CreateFile(filename,

                         GENERIC_READ,

                         FILE_SHARE_READ,

                         NULL,

                         OPEN_EXISTING,

                         FILE_FLAG_RANDOM_ACCESS,

                         NULL);

  if(newhandle == INVALID_HANDLE_VALUE) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error opening file %s", filename);

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(NULL);

  } // ENDIF- Error? Abort



  return(newhandle);

} // END ActualFileOpenForRead()





off64_t ActualFileSize(ACTUALHANDLE handle) {

  int retval;

  BY_HANDLE_FILE_INFORMATION info;

  off64_t retsize;



  if(handle == NULL)  return(-1);

  if(handle == INVALID_HANDLE_VALUE)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileSize()");

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  retval = GetFileInformationByHandle(handle, &info);

  if(retval == 0)  return(-1); // Handle doesn't exist...



  retsize = info.nFileSizeHigh;

  retsize *= 0x10000;

  retsize *= 0x10000;

  retsize += info.nFileSizeLow;

  return(retsize);

} // END ActualFileSize()





int ActualFileSeek(ACTUALHANDLE handle, off64_t position) {

  // int retval;

  LARGE_INTEGER realpos;

  DWORD errcode;



  if(handle == NULL)  return(-1);

  if(handle == INVALID_HANDLE_VALUE)  return(-1);

  if(position < 0)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileSeek(%llu)", position);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  realpos.QuadPart = position;

////// WinXP code for seek

//   retval = SetFilePointerEx(handle,

//                             realpos,

//                             NULL,

//                             FILE_BEGIN);

//   if(retval == 0) {



////// Win98 code for seek

  realpos.LowPart = SetFilePointer(handle,

                                   realpos.LowPart,

                                   &realpos.HighPart,

                                   FILE_BEGIN);

  errcode = GetLastError();

  if((realpos.LowPart == 0xFFFFFFFF) && (errcode != NO_ERROR)) {



#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error on seek (%llu)", position);

    PrintError("CDVDiso file", errcode);

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1);

  } // ENDIF- Error? Abort



  return(0);

} // END ActualFileSeek()





int ActualFileRead(ACTUALHANDLE handle, int bytes, char *buffer) {

  int retval;

  DWORD bytesread;

#ifdef VERBOSE_WARNING_ACTUALFILE

  DWORD errcode;

#endif /* VERBOSE_WARNING_ACTUALFILE */



  if(handle == NULL)  return(-1);

  if(handle == INVALID_HANDLE_VALUE)  return(-1);

  if(bytes < 1)  return(-1);

  if(buffer == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileRead(%i)", bytes);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  retval = ReadFile(handle, buffer, bytes, &bytesread, NULL);

  if(retval == 0) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    errcode = GetLastError();

    PrintLog("CDVDiso file:   Error reading from file");

    PrintError("CDVDiso file", errcode);

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(-1);

  } // ENDIF- Error? Abort

  if(bytesread < bytes) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Short Block! Only read %i out of %i bytes", bytesread, bytes);

#endif /* VERBOSE_WARNING_ACTUALFILE */

  } // ENDIF- Error? Abort



  return(bytesread); // Send back how many bytes read

} // END ActualFileRead()





void ActualFileClose(ACTUALHANDLE handle) {

  if(handle == NULL)  return;

  if(handle == INVALID_HANDLE_VALUE)  return;



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileClose()");

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  CloseHandle(handle);

  return;

} // END ActualFileClose()





ACTUALHANDLE ActualFileOpenForWrite(const char *filename) {

  HANDLE newhandle;



  if(filename == NULL)  return(NULL);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileOpenForWrite(%s)", filename);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  newhandle = CreateFile(filename,

                         GENERIC_WRITE,

                         0,

                         NULL,

                         CREATE_ALWAYS,

                         FILE_FLAG_SEQUENTIAL_SCAN,

                         NULL);

  if(newhandle == INVALID_HANDLE_VALUE) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error opening file %s", filename);

#endif /* VERBOSE_WARNING_ACTUALFILE */

    return(NULL);

  } // ENDIF- Error? Abort



  return(newhandle);

} // END ActualFileOpenForWrite()





int ActualFileWrite(ACTUALHANDLE handle, int bytes, char *buffer) {

  int retval;

  DWORD byteswritten;



  if(handle == NULL)  return(-1);

  if(handle == INVALID_HANDLE_VALUE)  return(-1);

  if(bytes < 1)  return(-1);

  if(buffer == NULL)  return(-1);



#ifdef VERBOSE_FUNCTION_ACTUALFILE

  PrintLog("CDVDiso file: ActualFileWrite(%i)", bytes);

#endif /* VERBOSE_FUNCTION_ACTUALFILE */



  retval = WriteFile(handle, buffer, bytes, &byteswritten, NULL);

  if(retval == 0) {

#ifdef VERBOSE_WARNING_ACTUALFILE

    PrintLog("CDVDiso file:   Error writing to file!");

#endif /* VERBOSE_WARNING_ACTUALFILE */

    // return(-1);

  } // ENDIF- Error? Abort



  return(byteswritten); // Send back how many bytes written

} // END ActualFileWrite()

