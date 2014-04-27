///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/private.h
// Purpose:     miscellaneous private things for Unix wx ports
// Author:      Vadim Zeitlin
// Created:     2005-09-25
// RCS-ID:      $Id: private.h 35688 2005-09-25 19:59:19Z VZ $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_PRIVATE_H_
#define _WX_UNIX_PRIVATE_H_

// standard linux headers produce many warnings when used with icc
#if defined(__INTELC__) && defined(__LINUX__)
    inline void wxFD_ZERO(fd_set *fds)
    {
        #pragma warning(push)
        #pragma warning(disable:593)
        FD_ZERO(fds);
        #pragma warning(pop)
    }

    inline void wxFD_SET(int fd, fd_set *fds)
    {
        #pragma warning(push, 1)
        #pragma warning(disable:1469)
        FD_SET(fd, fds);
        #pragma warning(pop)
    }

    inline bool wxFD_ISSET(int fd, fd_set *fds)
    {
        #pragma warning(push, 1)
        #pragma warning(disable:1469)
        return FD_ISSET(fd, fds);
        #pragma warning(pop)
    }
#else // !__INTELC__
    #define wxFD_ZERO(fds) FD_ZERO(fds)
    #define wxFD_SET(fd, fds) FD_SET(fd, fds)
    #define wxFD_ISSET(fd, fds) FD_ISSET(fd, fds)
#endif // __INTELC__/!__INTELC__


#endif // _WX_UNIX_PRIVATE_H_

