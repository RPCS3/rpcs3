/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/filefn.cpp
// Purpose:     File- and directory-related functions
// Author:      Julian Smart
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: filefnwce.cpp 52111 2008-02-26 14:15:12Z JS $
// Copyright:   (c) 1998 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/file.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WXWINCE__
#include "wx/msw/wince/missing.h"

int wxOpen(const wxChar *filename, int oflag, int WXUNUSED(pmode))
{
    DWORD access = 0;
    DWORD shareMode = 0;
    DWORD disposition = 0;

    if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDONLY)
    {
        access = GENERIC_READ;
        shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
        disposition = OPEN_EXISTING;
    }
    else if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_WRONLY)
    {
        access = GENERIC_WRITE;
        disposition = OPEN_ALWAYS;
    }
    else if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDWR)
    {
        access = GENERIC_READ|GENERIC_WRITE;
        disposition = OPEN_ALWAYS;
    }

    if (oflag & O_APPEND)
    {
        if ( wxFile::Exists(filename) )
        {
            access |= GENERIC_WRITE;
            shareMode = FILE_SHARE_READ;
            disposition = OPEN_EXISTING;
        }
        else
        {
            oflag |= O_TRUNC;
        }
    }
    if (oflag & O_TRUNC)
    {
        access |= GENERIC_WRITE;
        shareMode = 0;
        disposition = oflag & O_CREAT ? CREATE_ALWAYS : TRUNCATE_EXISTING;
    }
    else if (oflag & O_CREAT)
    {
        access |= GENERIC_WRITE;
        shareMode = 0;
        disposition = CREATE_NEW;
    }
    else if (oflag & O_EXCL)
    {
        access |= GENERIC_WRITE;
        shareMode = 0;
        disposition = TRUNCATE_EXISTING;
    }

    int fd = 0;
    HANDLE fileHandle = ::CreateFile(filename, access, shareMode, NULL,
        disposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
        fd = -1;
    else
        fd = (int) fileHandle;

    return fd;
}

int wxAccess(const wxChar *name, int WXUNUSED(how))
{
    HANDLE fileHandle = ::CreateFile(name, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    CloseHandle(fileHandle);

    return 0;
}

int wxClose(int fd)
{
    if (CloseHandle((HANDLE)fd))
        return 0;
    return -1;
}

int wxEof(int fd)
{
    DWORD off0 = SetFilePointer((HANDLE) fd, 0, NULL, FILE_CURRENT);
    if (off0 == 0xFFFFFFFF && GetLastError() != NO_ERROR)
        return -1;

    DWORD off1 = SetFilePointer((HANDLE) fd, 0, NULL, FILE_END);
    if (off1 == 0xFFFFFFFF && GetLastError() != NO_ERROR)
        return -1;

    if (off0 == off1)
        return 1;
    else
    {
        SetFilePointer((HANDLE) fd, off0, NULL, FILE_BEGIN);
        return 0;
    }
}

int wxRead(int fd, void *buf, unsigned int count)
{
    DWORD bytesRead = 0;

    if (ReadFile((HANDLE) fd, buf, (DWORD) count, &bytesRead, NULL))
        return bytesRead;
    else
        return -1;
}

int wxWrite(int fd, const void *buf, unsigned int count)
{
    DWORD bytesWritten = 0;

    if (WriteFile((HANDLE) fd, buf, (DWORD) count, &bytesWritten, NULL))
        return bytesWritten;
    else
        return -1;
}

__int64 wxSeek(int fd, __int64 offset, int origin)
{
    int method;
    switch ( origin ) {
        default:
            wxFAIL_MSG(_("unknown seek origin"));

        case SEEK_SET:
            method = FILE_BEGIN;
            break;

        case SEEK_CUR:
            method = FILE_CURRENT;
            break;

        case SEEK_END:
            method = FILE_END;
            break;
    }

    DWORD res = SetFilePointer((HANDLE) fd, offset, NULL, method) ;
    if (res == 0xFFFFFFFF && GetLastError() != NO_ERROR)
    {
        wxLogSysError(_("can't seek on file descriptor %d"), fd);
        return wxInvalidOffset;
    }
    else
        return (off_t)res;
}

__int64 wxTell(int fd)
{
    // WinCE apparently doesn't support lpDistanceToMoveHigh.
    // LONG high = 0;
    DWORD res = SetFilePointer((HANDLE) fd, 0, NULL, FILE_CURRENT) ;
    if (res == 0xFFFFFFFF && GetLastError() != NO_ERROR)
    {
        wxLogSysError(_("can't get seek position on file descriptor %d"), fd);
        return wxInvalidOffset;
    }
    else
        return res ; // + (((__int64)high) << 32);
}

int wxFsync(int WXUNUSED(fd))
{
    return 0;
}

#endif //__WXWINCE__
