/////////////////////////////////////////////////////////////////////////////
// Name:        wx/access.h
// Purpose:     Accessibility classes
// Author:      Julian Smart
// Modified by:
// Created:     2003-02-12
// RCS-ID:      $Id: access.h 51246 2008-01-16 12:56:37Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ACCESSBASE_H_
#define _WX_ACCESSBASE_H_

// ----------------------------------------------------------------------------
// headers we have to include here
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_ACCESSIBILITY

#include "wx/variant.h"

typedef enum
{
    wxACC_FAIL,
    wxACC_FALSE,
    wxACC_OK,
    wxACC_NOT_IMPLEMENTED,
    wxACC_NOT_SUPPORTED
} wxAccStatus;

// Child ids are integer identifiers from 1 up.
// So zero represents 'this' object.
#define wxACC_SELF 0

// Navigation constants

typedef enum
{
    wxNAVDIR_DOWN,
    wxNAVDIR_FIRSTCHILD,
    wxNAVDIR_LASTCHILD,
    wxNAVDIR_LEFT,
    wxNAVDIR_NEXT,
    wxNAVDIR_PREVIOUS,
    wxNAVDIR_RIGHT,
    wxNAVDIR_UP
} wxNavDir;

// Role constants

typedef enum {
    wxROLE_NONE,
    wxROLE_SYSTEM_ALERT,
    wxROLE_SYSTEM_ANIMATION,
    wxROLE_SYSTEM_APPLICATION,
    wxROLE_SYSTEM_BORDER,
    wxROLE_SYSTEM_BUTTONDROPDOWN,
    wxROLE_SYSTEM_BUTTONDROPDOWNGRID,
    wxROLE_SYSTEM_BUTTONMENU,
    wxROLE_SYSTEM_CARET,
    wxROLE_SYSTEM_CELL,
    wxROLE_SYSTEM_CHARACTER,
    wxROLE_SYSTEM_CHART,
    wxROLE_SYSTEM_CHECKBUTTON,
    wxROLE_SYSTEM_CLIENT,
    wxROLE_SYSTEM_CLOCK,
    wxROLE_SYSTEM_COLUMN,
    wxROLE_SYSTEM_COLUMNHEADER,
    wxROLE_SYSTEM_COMBOBOX,
    wxROLE_SYSTEM_CURSOR,
    wxROLE_SYSTEM_DIAGRAM,
    wxROLE_SYSTEM_DIAL,
    wxROLE_SYSTEM_DIALOG,
    wxROLE_SYSTEM_DOCUMENT,
    wxROLE_SYSTEM_DROPLIST,
    wxROLE_SYSTEM_EQUATION,
    wxROLE_SYSTEM_GRAPHIC,
    wxROLE_SYSTEM_GRIP,
    wxROLE_SYSTEM_GROUPING,
    wxROLE_SYSTEM_HELPBALLOON,
    wxROLE_SYSTEM_HOTKEYFIELD,
    wxROLE_SYSTEM_INDICATOR,
    wxROLE_SYSTEM_LINK,
    wxROLE_SYSTEM_LIST,
    wxROLE_SYSTEM_LISTITEM,
    wxROLE_SYSTEM_MENUBAR,
    wxROLE_SYSTEM_MENUITEM,
    wxROLE_SYSTEM_MENUPOPUP,
    wxROLE_SYSTEM_OUTLINE,
    wxROLE_SYSTEM_OUTLINEITEM,
    wxROLE_SYSTEM_PAGETAB,
    wxROLE_SYSTEM_PAGETABLIST,
    wxROLE_SYSTEM_PANE,
    wxROLE_SYSTEM_PROGRESSBAR,
    wxROLE_SYSTEM_PROPERTYPAGE,
    wxROLE_SYSTEM_PUSHBUTTON,
    wxROLE_SYSTEM_RADIOBUTTON,
    wxROLE_SYSTEM_ROW,
    wxROLE_SYSTEM_ROWHEADER,
    wxROLE_SYSTEM_SCROLLBAR,
    wxROLE_SYSTEM_SEPARATOR,
    wxROLE_SYSTEM_SLIDER,
    wxROLE_SYSTEM_SOUND,
    wxROLE_SYSTEM_SPINBUTTON,
    wxROLE_SYSTEM_STATICTEXT,
    wxROLE_SYSTEM_STATUSBAR,
    wxROLE_SYSTEM_TABLE,
    wxROLE_SYSTEM_TEXT,
    wxROLE_SYSTEM_TITLEBAR,
    wxROLE_SYSTEM_TOOLBAR,
    wxROLE_SYSTEM_TOOLTIP,
    wxROLE_SYSTEM_WHITESPACE,
    wxROLE_SYSTEM_WINDOW
} wxAccRole;

// Object types

typedef enum {
    wxOBJID_WINDOW =    0x00000000,
    wxOBJID_SYSMENU =   0xFFFFFFFF,
    wxOBJID_TITLEBAR =  0xFFFFFFFE,
    wxOBJID_MENU =      0xFFFFFFFD,
    wxOBJID_CLIENT =    0xFFFFFFFC,
    wxOBJID_VSCROLL =   0xFFFFFFFB,
    wxOBJID_HSCROLL =   0xFFFFFFFA,
    wxOBJID_SIZEGRIP =  0xFFFFFFF9,
    wxOBJID_CARET =     0xFFFFFFF8,
    wxOBJID_CURSOR =    0xFFFFFFF7,
    wxOBJID_ALERT =     0xFFFFFFF6,
    wxOBJID_SOUND =     0xFFFFFFF5
} wxAccObject;

// Accessible states

#define wxACC_STATE_SYSTEM_ALERT_HIGH       0x00000001
#define wxACC_STATE_SYSTEM_ALERT_MEDIUM     0x00000002
#define wxACC_STATE_SYSTEM_ALERT_LOW        0x00000004
#define wxACC_STATE_SYSTEM_ANIMATED         0x00000008
#define wxACC_STATE_SYSTEM_BUSY             0x00000010
#define wxACC_STATE_SYSTEM_CHECKED          0x00000020
#define wxACC_STATE_SYSTEM_COLLAPSED        0x00000040
#define wxACC_STATE_SYSTEM_DEFAULT          0x00000080
#define wxACC_STATE_SYSTEM_EXPANDED         0x00000100
#define wxACC_STATE_SYSTEM_EXTSELECTABLE    0x00000200
#define wxACC_STATE_SYSTEM_FLOATING         0x00000400
#define wxACC_STATE_SYSTEM_FOCUSABLE        0x00000800
#define wxACC_STATE_SYSTEM_FOCUSED          0x00001000
#define wxACC_STATE_SYSTEM_HOTTRACKED       0x00002000
#define wxACC_STATE_SYSTEM_INVISIBLE        0x00004000
#define wxACC_STATE_SYSTEM_MARQUEED         0x00008000
#define wxACC_STATE_SYSTEM_MIXED            0x00010000
#define wxACC_STATE_SYSTEM_MULTISELECTABLE  0x00020000
#define wxACC_STATE_SYSTEM_OFFSCREEN        0x00040000
#define wxACC_STATE_SYSTEM_PRESSED          0x00080000
#define wxACC_STATE_SYSTEM_PROTECTED        0x00100000
#define wxACC_STATE_SYSTEM_READONLY         0x00200000
#define wxACC_STATE_SYSTEM_SELECTABLE       0x00400000
#define wxACC_STATE_SYSTEM_SELECTED         0x00800000
#define wxACC_STATE_SYSTEM_SELFVOICING      0x01000000
#define wxACC_STATE_SYSTEM_UNAVAILABLE      0x02000000

// Selection flag

typedef enum
{
    wxACC_SEL_NONE            = 0,
    wxACC_SEL_TAKEFOCUS       = 1,
    wxACC_SEL_TAKESELECTION   = 2,
    wxACC_SEL_EXTENDSELECTION = 4,
    wxACC_SEL_ADDSELECTION    = 8,
    wxACC_SEL_REMOVESELECTION = 16
} wxAccSelectionFlags;

// Accessibility event identifiers

#define wxACC_EVENT_SYSTEM_SOUND              0x0001
#define wxACC_EVENT_SYSTEM_ALERT              0x0002
#define wxACC_EVENT_SYSTEM_FOREGROUND         0x0003
#define wxACC_EVENT_SYSTEM_MENUSTART          0x0004
#define wxACC_EVENT_SYSTEM_MENUEND            0x0005
#define wxACC_EVENT_SYSTEM_MENUPOPUPSTART     0x0006
#define wxACC_EVENT_SYSTEM_MENUPOPUPEND       0x0007
#define wxACC_EVENT_SYSTEM_CAPTURESTART       0x0008
#define wxACC_EVENT_SYSTEM_CAPTUREEND         0x0009
#define wxACC_EVENT_SYSTEM_MOVESIZESTART      0x000A
#define wxACC_EVENT_SYSTEM_MOVESIZEEND        0x000B
#define wxACC_EVENT_SYSTEM_CONTEXTHELPSTART   0x000C
#define wxACC_EVENT_SYSTEM_CONTEXTHELPEND     0x000D
#define wxACC_EVENT_SYSTEM_DRAGDROPSTART      0x000E
#define wxACC_EVENT_SYSTEM_DRAGDROPEND        0x000F
#define wxACC_EVENT_SYSTEM_DIALOGSTART        0x0010
#define wxACC_EVENT_SYSTEM_DIALOGEND          0x0011
#define wxACC_EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define wxACC_EVENT_SYSTEM_SCROLLINGEND       0x0013
#define wxACC_EVENT_SYSTEM_SWITCHSTART        0x0014
#define wxACC_EVENT_SYSTEM_SWITCHEND          0x0015
#define wxACC_EVENT_SYSTEM_MINIMIZESTART      0x0016
#define wxACC_EVENT_SYSTEM_MINIMIZEEND        0x0017
#define wxACC_EVENT_OBJECT_CREATE                 0x8000
#define wxACC_EVENT_OBJECT_DESTROY                0x8001
#define wxACC_EVENT_OBJECT_SHOW                   0x8002
#define wxACC_EVENT_OBJECT_HIDE                   0x8003
#define wxACC_EVENT_OBJECT_REORDER                0x8004
#define wxACC_EVENT_OBJECT_FOCUS                  0x8005
#define wxACC_EVENT_OBJECT_SELECTION              0x8006
#define wxACC_EVENT_OBJECT_SELECTIONADD           0x8007
#define wxACC_EVENT_OBJECT_SELECTIONREMOVE        0x8008
#define wxACC_EVENT_OBJECT_SELECTIONWITHIN        0x8009
#define wxACC_EVENT_OBJECT_STATECHANGE            0x800A
#define wxACC_EVENT_OBJECT_LOCATIONCHANGE         0x800B
#define wxACC_EVENT_OBJECT_NAMECHANGE             0x800C
#define wxACC_EVENT_OBJECT_DESCRIPTIONCHANGE      0x800D
#define wxACC_EVENT_OBJECT_VALUECHANGE            0x800E
#define wxACC_EVENT_OBJECT_PARENTCHANGE           0x800F
#define wxACC_EVENT_OBJECT_HELPCHANGE             0x8010
#define wxACC_EVENT_OBJECT_DEFACTIONCHANGE        0x8011
#define wxACC_EVENT_OBJECT_ACCELERATORCHANGE      0x8012

// ----------------------------------------------------------------------------
// wxAccessible
// All functions return an indication of success, failure, or not implemented.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxAccessible;
class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxPoint;
class WXDLLIMPEXP_FWD_CORE wxRect;
class WXDLLEXPORT wxAccessibleBase : public wxObject
{
    DECLARE_NO_COPY_CLASS(wxAccessibleBase)

public:
    wxAccessibleBase(wxWindow* win): m_window(win) {}
    virtual ~wxAccessibleBase() {}

// Overridables

        // Can return either a child object, or an integer
        // representing the child element, starting from 1.
        // pt is in screen coordinates.
    virtual wxAccStatus HitTest(const wxPoint& WXUNUSED(pt), int* WXUNUSED(childId), wxAccessible** WXUNUSED(childObject))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns the rectangle for this object (id = 0) or a child element (id > 0).
        // rect is in screen coordinates.
    virtual wxAccStatus GetLocation(wxRect& WXUNUSED(rect), int WXUNUSED(elementId))
         { return wxACC_NOT_IMPLEMENTED; }

        // Navigates from fromId to toId/toObject.
    virtual wxAccStatus Navigate(wxNavDir WXUNUSED(navDir), int WXUNUSED(fromId),
                int* WXUNUSED(toId), wxAccessible** WXUNUSED(toObject))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the name of the specified object.
    virtual wxAccStatus GetName(int WXUNUSED(childId), wxString* WXUNUSED(name))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the number of children.
    virtual wxAccStatus GetChildCount(int* WXUNUSED(childCount))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the specified child (starting from 1).
        // If *child is NULL and return value is wxACC_OK,
        // this means that the child is a simple element and
        // not an accessible object.
    virtual wxAccStatus GetChild(int WXUNUSED(childId), wxAccessible** WXUNUSED(child))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the parent, or NULL.
    virtual wxAccStatus GetParent(wxAccessible** WXUNUSED(parent))
         { return wxACC_NOT_IMPLEMENTED; }

        // Performs the default action. childId is 0 (the action for this object)
        // or > 0 (the action for a child).
        // Return wxACC_NOT_SUPPORTED if there is no default action for this
        // window (e.g. an edit control).
    virtual wxAccStatus DoDefaultAction(int WXUNUSED(childId))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the default action for this object (0) or > 0 (the action for a child).
        // Return wxACC_OK even if there is no action. actionName is the action, or the empty
        // string if there is no action.
        // The retrieved string describes the action that is performed on an object,
        // not what the object does as a result. For example, a toolbar button that prints
        // a document has a default action of "Press" rather than "Prints the current document."
    virtual wxAccStatus GetDefaultAction(int WXUNUSED(childId), wxString* WXUNUSED(actionName))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns the description for this object or a child.
    virtual wxAccStatus GetDescription(int WXUNUSED(childId), wxString* WXUNUSED(description))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns help text for this object or a child, similar to tooltip text.
    virtual wxAccStatus GetHelpText(int WXUNUSED(childId), wxString* WXUNUSED(helpText))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns the keyboard shortcut for this object or child.
        // Return e.g. ALT+K
    virtual wxAccStatus GetKeyboardShortcut(int WXUNUSED(childId), wxString* WXUNUSED(shortcut))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns a role constant.
    virtual wxAccStatus GetRole(int WXUNUSED(childId), wxAccRole* WXUNUSED(role))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns a state constant.
    virtual wxAccStatus GetState(int WXUNUSED(childId), long* WXUNUSED(state))
         { return wxACC_NOT_IMPLEMENTED; }

        // Returns a localized string representing the value for the object
        // or child.
    virtual wxAccStatus GetValue(int WXUNUSED(childId), wxString* WXUNUSED(strValue))
         { return wxACC_NOT_IMPLEMENTED; }

        // Selects the object or child.
    virtual wxAccStatus Select(int WXUNUSED(childId), wxAccSelectionFlags WXUNUSED(selectFlags))
         { return wxACC_NOT_IMPLEMENTED; }

        // Gets the window with the keyboard focus.
        // If childId is 0 and child is NULL, no object in
        // this subhierarchy has the focus.
        // If this object has the focus, child should be 'this'.
    virtual wxAccStatus GetFocus(int* WXUNUSED(childId), wxAccessible** WXUNUSED(child))
         { return wxACC_NOT_IMPLEMENTED; }

#if wxUSE_VARIANT
        // Gets a variant representing the selected children
        // of this object.
        // Acceptable values:
        // - a null variant (IsNull() returns TRUE)
        // - a list variant (GetType() == wxT("list"))
        // - an integer representing the selected child element,
        //   or 0 if this object is selected (GetType() == wxT("long"))
        // - a "void*" pointer to a wxAccessible child object
    virtual wxAccStatus GetSelections(wxVariant* WXUNUSED(selections))
         { return wxACC_NOT_IMPLEMENTED; }
#endif // wxUSE_VARIANT

// Accessors

        // Returns the window associated with this object.

    wxWindow* GetWindow() { return m_window; }

        // Sets the window associated with this object.

    void SetWindow(wxWindow* window) { m_window = window; }

// Operations

        // Each platform's implementation must define this
    // static void NotifyEvent(int eventType, wxWindow* window, wxAccObject objectType,
    //                         int objectId);

private:

// Data members

    wxWindow*   m_window;
};


// ----------------------------------------------------------------------------
// now include the declaration of the real class
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #include "wx/msw/ole/access.h"
#endif

#endif // wxUSE_ACCESSIBILITY

#endif // _WX_ACCESSBASE_H_

