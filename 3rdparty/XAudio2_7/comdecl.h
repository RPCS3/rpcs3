// comdecl.h: Macros to facilitate COM interface and GUID declarations.
// Copyright (c) Microsoft Corporation.  All rights reserved.

#ifndef _COMDECL_H_
#define _COMDECL_H_

#ifndef _XBOX
    #include <basetyps.h>   // For standard COM interface macros
#else
    #pragma warning(push)
    #pragma warning(disable:4061)
    #include <xtl.h>        // Required by xobjbase.h
    #include <xobjbase.h>   // Special definitions for Xbox build
    #pragma warning(pop)
#endif

// The DEFINE_CLSID() and DEFINE_IID() macros defined below allow COM GUIDs to
// be declared and defined in such a way that clients can obtain the GUIDs using
// either the __uuidof() extension or the old-style CLSID_Foo / IID_IFoo names.
// If using the latter approach, the client can also choose whether to get the
// GUID definitions by defining the INITGUID preprocessor constant or by linking
// to a GUID library.  This works in either C or C++.

#ifdef __cplusplus

    #define DECLSPEC_UUID_WRAPPER(x) __declspec(uuid(#x))
    #ifdef INITGUID

        #define DEFINE_CLSID(className, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            class DECLSPEC_UUID_WRAPPER(l##-##w1##-##w2##-##b1##b2##-##b3##b4##b5##b6##b7##b8) className; \
            EXTERN_C const GUID DECLSPEC_SELECTANY CLSID_##className = __uuidof(className)

        #define DEFINE_IID(interfaceName, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            interface DECLSPEC_UUID_WRAPPER(l##-##w1##-##w2##-##b1##b2##-##b3##b4##b5##b6##b7##b8) interfaceName; \
            EXTERN_C const GUID DECLSPEC_SELECTANY IID_##interfaceName = __uuidof(interfaceName)

    #else // INITGUID

        #define DEFINE_CLSID(className, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            class DECLSPEC_UUID_WRAPPER(l##-##w1##-##w2##-##b1##b2##-##b3##b4##b5##b6##b7##b8) className; \
            EXTERN_C const GUID CLSID_##className

        #define DEFINE_IID(interfaceName, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            interface DECLSPEC_UUID_WRAPPER(l##-##w1##-##w2##-##b1##b2##-##b3##b4##b5##b6##b7##b8) interfaceName; \
            EXTERN_C const GUID IID_##interfaceName

    #endif // INITGUID

#else // __cplusplus

    #define DEFINE_CLSID(className, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        DEFINE_GUID(CLSID_##className, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)

    #define DEFINE_IID(interfaceName, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        DEFINE_GUID(IID_##interfaceName, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)

#endif // __cplusplus

#endif // #ifndef _COMDECL_H_
