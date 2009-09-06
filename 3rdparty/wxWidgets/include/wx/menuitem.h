///////////////////////////////////////////////////////////////////////////////
// Name:        wx/menuitem.h
// Purpose:     wxMenuItem class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.10.99
// RCS-ID:      $Id: menuitem.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MENUITEM_H_BASE_
#define _WX_MENUITEM_H_BASE_

#include "wx/defs.h"

#if wxUSE_MENUS

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/object.h"  // base class

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxAcceleratorEntry;
class WXDLLIMPEXP_FWD_CORE wxMenuItem;
class WXDLLIMPEXP_FWD_CORE wxMenu;

// ----------------------------------------------------------------------------
// wxMenuItem is an item in the menu which may be either a normal item, a sub
// menu or a separator
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMenuItemBase : public wxObject
{
public:
    // creation
    static wxMenuItem *New(wxMenu *parentMenu = (wxMenu *)NULL,
                           int itemid = wxID_SEPARATOR,
                           const wxString& text = wxEmptyString,
                           const wxString& help = wxEmptyString,
                           wxItemKind kind = wxITEM_NORMAL,
                           wxMenu *subMenu = (wxMenu *)NULL);

    // destruction: wxMenuItem will delete its submenu
    virtual ~wxMenuItemBase();

    // the menu we're in
    wxMenu *GetMenu() const { return m_parentMenu; }
    void SetMenu(wxMenu* menu) { m_parentMenu = menu; }

    // get/set id
    void SetId(int itemid) { m_id = itemid; }
    int  GetId() const { return m_id; }
    bool IsSeparator() const { return m_id == wxID_SEPARATOR; }

    // the item's text (or name)
    //
    // NB: the item's text includes the accelerators and mnemonics info (if
    //     any), i.e. it may contain '&' or '_' or "\t..." and thus is
    //     different from the item's label which only contains the text shown
    //     in the menu
    virtual void SetText(const wxString& str);

    wxString GetLabel() const { return GetLabelFromText(m_text); }
    const wxString& GetText() const { return m_text; }

    // get the label from text (implemented in platform-specific code)
    static wxString GetLabelFromText(const wxString& text);

    // what kind of menu item we are
    wxItemKind GetKind() const { return m_kind; }
    void SetKind(wxItemKind kind) { m_kind = kind; }

    virtual void SetCheckable(bool checkable) { m_kind = checkable ? wxITEM_CHECK : wxITEM_NORMAL; }
    bool IsCheckable() const
        { return m_kind == wxITEM_CHECK || m_kind == wxITEM_RADIO; }

    bool IsSubMenu() const { return m_subMenu != NULL; }
    void SetSubMenu(wxMenu *menu) { m_subMenu = menu; }
    wxMenu *GetSubMenu() const { return m_subMenu; }

    // state
    virtual void Enable(bool enable = true) { m_isEnabled = enable; }
    virtual bool IsEnabled() const { return m_isEnabled; }

    virtual void Check(bool check = true) { m_isChecked = check; }
    virtual bool IsChecked() const { return m_isChecked; }
    void Toggle() { Check(!m_isChecked); }

    // help string (displayed in the status bar by default)
    void SetHelp(const wxString& str);
    const wxString& GetHelp() const { return m_help; }

#if wxUSE_ACCEL
    // extract the accelerator from the given menu string, return NULL if none
    // found
    static wxAcceleratorEntry *GetAccelFromString(const wxString& label);

    // get our accelerator or NULL (caller must delete the pointer)
    virtual wxAcceleratorEntry *GetAccel() const;

    // set the accel for this item - this may also be done indirectly with
    // SetText()
    virtual void SetAccel(wxAcceleratorEntry *accel);
#endif // wxUSE_ACCEL

    // compatibility only, use new functions in the new code
    void SetName(const wxString& str) { SetText(str); }
    const wxString& GetName() const { return GetText(); }

    static wxMenuItem *New(wxMenu *parentMenu,
                           int itemid,
                           const wxString& text,
                           const wxString& help,
                           bool isCheckable,
                           wxMenu *subMenu = (wxMenu *)NULL)
    {
        return New(parentMenu, itemid, text, help,
                   isCheckable ? wxITEM_CHECK : wxITEM_NORMAL, subMenu);
    }

protected:
    int           m_id;             // numeric id of the item >= 0 or wxID_ANY or wxID_SEPARATOR
    wxMenu       *m_parentMenu,     // the menu we belong to
                 *m_subMenu;        // our sub menu or NULL
    wxString      m_text,           // label of the item
                  m_help;           // the help string for the item
    wxItemKind    m_kind;           // separator/normal/check/radio item?
    bool          m_isChecked;      // is checked?
    bool          m_isEnabled;      // is enabled?

    // this ctor is for the derived classes only, we're never created directly
    wxMenuItemBase(wxMenu *parentMenu = (wxMenu *)NULL,
                   int itemid = wxID_SEPARATOR,
                   const wxString& text = wxEmptyString,
                   const wxString& help = wxEmptyString,
                   wxItemKind kind = wxITEM_NORMAL,
                   wxMenu *subMenu = (wxMenu *)NULL);

private:
    // and, if we have one ctor, compiler won't generate a default copy one, so
    // declare them ourselves - but don't implement as they shouldn't be used
    wxMenuItemBase(const wxMenuItemBase& item);
    wxMenuItemBase& operator=(const wxMenuItemBase& item);

public:

#if wxABI_VERSION >= 20805
    // Sets the label. This function replaces SetText.
    void SetItemLabel(const wxString& str) { SetText(str); }

    // return the item label including any mnemonics and accelerators.
    // This used to be called GetText.
    // We can't implement this in the base class (no new virtuals in stable branch)
    // wxString GetItemLabel() const;

    // return just the text of the item label, without any mnemonics
    // This used to be called GetLabel.
    wxString GetItemLabelText() const { return GetLabelText(m_text); }

    // return just the text part of the given label. In 2.9 and up, this is implemented in
    // platform-specific code, but is now implemented in terms of GetLabelFromText.
    static wxString GetLabelText(const wxString& label);
#endif
};

// ----------------------------------------------------------------------------
// include the real class declaration
// ----------------------------------------------------------------------------

#ifdef wxUSE_BASE_CLASSES_ONLY
    #define wxMenuItem wxMenuItemBase
#else // !wxUSE_BASE_CLASSES_ONLY
#if defined(__WXUNIVERSAL__)
    #include "wx/univ/menuitem.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/menuitem.h"
#elif defined(__WXMSW__)
    #include "wx/msw/menuitem.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/menuitem.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/menuitem.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/menuitem.h"
#elif defined(__WXMAC__)
    #include "wx/mac/menuitem.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/menuitem.h"
#elif defined(__WXPM__)
    #include "wx/os2/menuitem.h"
#endif
#endif // wxUSE_BASE_CLASSES_ONLY/!wxUSE_BASE_CLASSES_ONLY

#endif // wxUSE_MENUS

#endif
    // _WX_MENUITEM_H_BASE_
