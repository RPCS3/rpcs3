///////////////////////////////////////////////////////////////////////////////
// Name:        wx/stack.h
// Purpose:     STL stack clone
// Author:      Lindsay Mathieson
// Modified by:
// Created:     30.07.2001
// Copyright:   (c) 2001 Lindsay Mathieson <lindsay@mathieson.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STACK_H_
#define _WX_STACK_H_

#include "wx/vector.h"

#define WX_DECLARE_STACK(obj, cls)\
class cls : public wxVectorBase\
{\
    WX_DECLARE_VECTORBASE(obj, cls);\
public:\
    void push(const obj& o)\
    {\
        bool rc = Alloc(size() + 1);\
        wxASSERT(rc);\
        Append(new obj(o));\
    };\
\
    void pop()\
    {\
        RemoveAt(size() - 1);\
    };\
\
    obj& top()\
    {\
        return *(obj *) GetItem(size() - 1);\
    };\
    const obj& top() const\
    {\
        return *(obj *) GetItem(size() - 1);\
    };\
}

#endif // _WX_STACK_H_

