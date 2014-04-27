///////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/afterstd.h
// Purpose:     #include after STL headers
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07/07/03
// RCS-ID:      $Id: afterstd.h 42906 2006-11-01 14:16:42Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

/**
    See the comments in beforestd.h.
 */

#if defined(__WXMSW__)
    #include "wx/msw/winundef.h"
#endif

// undo what we did in wx/beforestd.h
#if defined(__VISUALC__) && __VISUALC__ <= 1201
    // MSVC 5 does not have this
    #if _MSC_VER > 1100
        // don't restore this one for VC6, it gives it in each try/catch which is a
        // bit annoying to say the least
        #if _MSC_VER >= 0x1300
            // unreachable code
            #pragma warning(default:4702)
        #endif // VC++ >= 7

        #pragma warning(pop)
    #else
        // 'expression' : signed/unsigned mismatch
        #pragma warning(default:4018)

        // 'identifier' : unreferenced formal parameter
        #pragma warning(default:4100)

        // 'conversion' : conversion from 'type1' to 'type2',
        // possible loss of data
        #pragma warning(default:4244)

        // C++ language change: to explicitly specialize class template
        // 'identifier' use the following syntax
        #pragma warning(default:4663)
    #endif
#endif

