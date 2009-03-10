/////////////////////////////////////////////////////////////////////////////
// Name:        listimpl.cpp
// Purpose:     second-part of macro based implementation of template lists
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16/11/98
// RCS-ID:      $Id: listimpl.cpp 38893 2006-04-24 17:59:10Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if wxUSE_STL

#undef  WX_DEFINE_LIST
#define WX_DEFINE_LIST(name)                                                  \
    void _WX_LIST_HELPER_##name::DeleteFunction( _WX_LIST_ITEM_TYPE_##name X )\
    {                                                                         \
        delete X;                                                             \
    }                                                                         \
    name::BaseListType name::EmptyList;

#else // !wxUSE_STL

    #define _DEFINE_LIST(T, name)         \
        void wx##name##Node::DeleteData() \
        {                                 \
            delete (T *)GetData();        \
        }

    // redefine the macro so that now it will generate the class implementation
    // old value would provoke a compile-time error if this file is not included
    #undef  WX_DEFINE_LIST
    #define WX_DEFINE_LIST(name) _DEFINE_LIST(_WX_LIST_ITEM_TYPE_##name, name)

    // don't pollute preprocessor's name space
    //#undef  _DEFINE_LIST

#endif // wxUSE_STL/!wxUSE_STL

