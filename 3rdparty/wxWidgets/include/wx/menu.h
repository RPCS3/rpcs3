///////////////////////////////////////////////////////////////////////////////
// Name:        wx/menu.h
// Purpose:     wxMenu and wxMenuBar classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.10.99
// RCS-ID:      $Id: menu.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MENU_H_BASE_
#define _WX_MENU_H_BASE_

#include "wx/defs.h"

#if wxUSE_MENUS

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/list.h"        // for "template" list classes
#include "wx/window.h"      // base class for wxMenuBar

// also include this one to ensure compatibility with old code which only
// included wx/menu.h
#include "wx/menuitem.h"

class WXDLLIMPEXP_FWD_CORE wxMenu;
class WXDLLIMPEXP_FWD_CORE wxMenuBarBase;
class WXDLLIMPEXP_FWD_CORE wxMenuBar;
class WXDLLIMPEXP_FWD_CORE wxMenuItem;

// pseudo template list classes
WX_DECLARE_EXPORTED_LIST(wxMenu, wxMenuList);
WX_DECLARE_EXPORTED_LIST(wxMenuItem, wxMenuItemList);

// ----------------------------------------------------------------------------
// wxMenu
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMenuBase : public wxEvtHandler
{
public:
    // create a menu
    static wxMenu *New(const wxString& title = wxEmptyString, long style = 0);

    // ctors
    wxMenuBase(const wxString& title, long style = 0) : m_title(title)
        { Init(style); }
    wxMenuBase(long style = 0)
        { Init(style); }

    // dtor deletes all the menu items we own
    virtual ~wxMenuBase();

    // menu construction
    // -----------------

    // append any kind of item (normal/check/radio/separator)
    wxMenuItem* Append(int itemid,
                       const wxString& text = wxEmptyString,
                       const wxString& help = wxEmptyString,
                       wxItemKind kind = wxITEM_NORMAL)
    {
        return DoAppend(wxMenuItem::New((wxMenu *)this, itemid, text, help, kind));
    }

    // append a separator to the menu
    wxMenuItem* AppendSeparator() { return Append(wxID_SEPARATOR, wxEmptyString); }

    // append a check item
    wxMenuItem* AppendCheckItem(int itemid,
                                const wxString& text,
                                const wxString& help = wxEmptyString)
    {
        return Append(itemid, text, help, wxITEM_CHECK);
    }

    // append a radio item
    wxMenuItem* AppendRadioItem(int itemid,
                                const wxString& text,
                                const wxString& help = wxEmptyString)
    {
        return Append(itemid, text, help, wxITEM_RADIO);
    }

    // append a submenu
    wxMenuItem* AppendSubMenu(wxMenu *submenu,
                              const wxString& text,
                              const wxString& help = wxEmptyString)
    {
        return DoAppend(wxMenuItem::New((wxMenu *)this, wxID_ANY, text, help,
                                        wxITEM_NORMAL, submenu));
    }

    // the most generic form of Append() - append anything
    wxMenuItem* Append(wxMenuItem *item) { return DoAppend(item); }

    // insert a break in the menu (only works when appending the items, not
    // inserting them)
    virtual void Break() { }

    // insert an item before given position
    wxMenuItem* Insert(size_t pos, wxMenuItem *item);

    // insert an item before given position
    wxMenuItem* Insert(size_t pos,
                       int itemid,
                       const wxString& text = wxEmptyString,
                       const wxString& help = wxEmptyString,
                       wxItemKind kind = wxITEM_NORMAL)
    {
        return Insert(pos, wxMenuItem::New((wxMenu *)this, itemid, text, help, kind));
    }

    // insert a separator
    wxMenuItem* InsertSeparator(size_t pos)
    {
        return Insert(pos, wxMenuItem::New((wxMenu *)this, wxID_SEPARATOR));
    }

    // insert a check item
    wxMenuItem* InsertCheckItem(size_t pos,
                                int itemid,
                                const wxString& text,
                                const wxString& help = wxEmptyString)
    {
        return Insert(pos, itemid, text, help, wxITEM_CHECK);
    }

    // insert a radio item
     wxMenuItem* InsertRadioItem(size_t pos,
                                 int itemid,
                                 const wxString& text,
                                 const wxString& help = wxEmptyString)
    {
        return Insert(pos, itemid, text, help, wxITEM_RADIO);
    }

    // insert a submenu
    wxMenuItem* Insert(size_t pos,
                       int itemid,
                       const wxString& text,
                       wxMenu *submenu,
                       const wxString& help = wxEmptyString)
    {
        return Insert(pos, wxMenuItem::New((wxMenu *)this, itemid, text, help,
                                           wxITEM_NORMAL, submenu));
    }

    // prepend an item to the menu
    wxMenuItem* Prepend(wxMenuItem *item)
    {
        return Insert(0u, item);
    }

    // prepend any item to the menu
    wxMenuItem* Prepend(int itemid,
                        const wxString& text = wxEmptyString,
                        const wxString& help = wxEmptyString,
                        wxItemKind kind = wxITEM_NORMAL)
    {
        return Insert(0u, itemid, text, help, kind);
    }

    // prepend a separator
    wxMenuItem* PrependSeparator()
    {
        return InsertSeparator(0u);
    }

    // prepend a check item
    wxMenuItem* PrependCheckItem(int itemid,
                                 const wxString& text,
                                 const wxString& help = wxEmptyString)
    {
        return InsertCheckItem(0u, itemid, text, help);
    }

    // prepend a radio item
    wxMenuItem* PrependRadioItem(int itemid,
                                 const wxString& text,
                                 const wxString& help = wxEmptyString)
    {
        return InsertRadioItem(0u, itemid, text, help);
    }

    // prepend a submenu
    wxMenuItem* Prepend(int itemid,
                        const wxString& text,
                        wxMenu *submenu,
                        const wxString& help = wxEmptyString)
    {
        return Insert(0u, itemid, text, submenu, help);
    }

    // detach an item from the menu, but don't delete it so that it can be
    // added back later (but if it's not, the caller is responsible for
    // deleting it!)
    wxMenuItem *Remove(int itemid) { return Remove(FindChildItem(itemid)); }
    wxMenuItem *Remove(wxMenuItem *item);

    // delete an item from the menu (submenus are not destroyed by this
    // function, see Destroy)
    bool Delete(int itemid) { return Delete(FindChildItem(itemid)); }
    bool Delete(wxMenuItem *item);

    // delete the item from menu and destroy it (if it's a submenu)
    bool Destroy(int itemid) { return Destroy(FindChildItem(itemid)); }
    bool Destroy(wxMenuItem *item);

    // menu items access
    // -----------------

    // get the items
    size_t GetMenuItemCount() const { return m_items.GetCount(); }

    const wxMenuItemList& GetMenuItems() const { return m_items; }
    wxMenuItemList& GetMenuItems() { return m_items; }

    // search
    virtual int FindItem(const wxString& item) const;
    wxMenuItem* FindItem(int itemid, wxMenu **menu = NULL) const;

    // find by position
    wxMenuItem* FindItemByPosition(size_t position) const;

    // get/set items attributes
    void Enable(int itemid, bool enable);
    bool IsEnabled(int itemid) const;

    void Check(int itemid, bool check);
    bool IsChecked(int itemid) const;

    void SetLabel(int itemid, const wxString& label);
    wxString GetLabel(int itemid) const;

    virtual void SetHelpString(int itemid, const wxString& helpString);
    virtual wxString GetHelpString(int itemid) const;

    // misc accessors
    // --------------

    // the title
    virtual void SetTitle(const wxString& title) { m_title = title; }
    const wxString GetTitle() const { return m_title; }

    // event handler
    void SetEventHandler(wxEvtHandler *handler) { m_eventHandler = handler; }
    wxEvtHandler *GetEventHandler() const { return m_eventHandler; }

    // invoking window
    void SetInvokingWindow(wxWindow *win) { m_invokingWindow = win; }
    wxWindow *GetInvokingWindow() const { return m_invokingWindow; }

    // style
    long GetStyle() const { return m_style; }

    // implementation helpers
    // ----------------------

    // Updates the UI for a menu and all submenus recursively. source is the
    // object that has the update event handlers defined for it. If NULL, the
    // menu or associated window will be used.
    void UpdateUI(wxEvtHandler* source = (wxEvtHandler*)NULL);

    // get the menu bar this menu is attached to (may be NULL, always NULL for
    // popup menus).  Traverse up the menu hierarchy to find it.
    wxMenuBar *GetMenuBar() const;

    // called when the menu is attached/detached to/from a menu bar
    virtual void Attach(wxMenuBarBase *menubar);
    virtual void Detach();

    // is the menu attached to a menu bar (or is it a popup one)?
    bool IsAttached() const { return GetMenuBar() != NULL; }

    // set/get the parent of this menu
    void SetParent(wxMenu *parent) { m_menuParent = parent; }
    wxMenu *GetParent() const { return m_menuParent; }

    // implementation only from now on
    // -------------------------------

    // unlike FindItem(), this function doesn't recurse but only looks through
    // our direct children and also may return the index of the found child if
    // pos != NULL
    wxMenuItem *FindChildItem(int itemid, size_t *pos = NULL) const;

    // called to generate a wxCommandEvent, return true if it was processed,
    // false otherwise
    //
    // the checked parameter may have boolean value or -1 for uncheckable items
    bool SendEvent(int itemid, int checked = -1);

    // compatibility: these functions are deprecated, use the new ones instead
    // -----------------------------------------------------------------------

    // use the versions taking wxItem_XXX now instead, they're more readable
    // and allow adding the radio items as well
    void Append(int itemid,
                const wxString& text,
                const wxString& help,
                bool isCheckable)
    {
        Append(itemid, text, help, isCheckable ? wxITEM_CHECK : wxITEM_NORMAL);
    }

    // use more readable and not requiring unused itemid AppendSubMenu() instead
    wxMenuItem* Append(int itemid,
                       const wxString& text,
                       wxMenu *submenu,
                       const wxString& help = wxEmptyString)
    {
        return DoAppend(wxMenuItem::New((wxMenu *)this, itemid, text, help,
                                        wxITEM_NORMAL, submenu));
    }

    void Insert(size_t pos,
                int itemid,
                const wxString& text,
                const wxString& help,
                bool isCheckable)
    {
        Insert(pos, itemid, text, help, isCheckable ? wxITEM_CHECK : wxITEM_NORMAL);
    }

    void Prepend(int itemid,
                 const wxString& text,
                 const wxString& help,
                 bool isCheckable)
    {
        Insert(0u, itemid, text, help, isCheckable);
    }

    static void LockAccels(bool locked)
    {
        ms_locked = locked;
    }

protected:
    // virtuals to override in derived classes
    // ---------------------------------------

    virtual wxMenuItem* DoAppend(wxMenuItem *item);
    virtual wxMenuItem* DoInsert(size_t pos, wxMenuItem *item);

    virtual wxMenuItem *DoRemove(wxMenuItem *item);
    virtual bool DoDelete(wxMenuItem *item);
    virtual bool DoDestroy(wxMenuItem *item);

    // helpers
    // -------

    // common part of all ctors
    void Init(long style);

    // associate the submenu with this menu
    void AddSubMenu(wxMenu *submenu);

    wxMenuBar     *m_menuBar;           // menubar we belong to or NULL
    wxMenu        *m_menuParent;        // parent menu or NULL

    wxString       m_title;             // the menu title or label
    wxMenuItemList m_items;             // the list of menu items

    wxWindow      *m_invokingWindow;    // for popup menus

    long           m_style;             // combination of wxMENU_XXX flags

    wxEvtHandler  *m_eventHandler;      // a pluggable in event handler

    static bool      ms_locked;

    DECLARE_NO_COPY_CLASS(wxMenuBase)

public:

#if wxABI_VERSION >= 20805
    //  Returns the stripped label
    wxString GetLabelText(int itemid) const { return wxMenuItem::GetLabelFromText(GetLabel(itemid)); }
#endif

};

// ----------------------------------------------------------------------------
// wxMenuBar
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMenuBarBase : public wxWindow
{
public:
    // default ctor
    wxMenuBarBase();

    // dtor will delete all menus we own
    virtual ~wxMenuBarBase();

    // menu bar construction
    // ---------------------

    // append a menu to the end of menubar, return true if ok
    virtual bool Append(wxMenu *menu, const wxString& title);

    // insert a menu before the given position into the menubar, return true
    // if inserted ok
    virtual bool Insert(size_t pos, wxMenu *menu, const wxString& title);

    // menu bar items access
    // ---------------------

    // get the number of menus in the menu bar
    size_t GetMenuCount() const { return m_menus.GetCount(); }

    // get the menu at given position
    wxMenu *GetMenu(size_t pos) const;

    // replace the menu at given position with another one, returns the
    // previous menu (which should be deleted by the caller)
    virtual wxMenu *Replace(size_t pos, wxMenu *menu, const wxString& title);

    // delete the menu at given position from the menu bar, return the pointer
    // to the menu (which should be  deleted by the caller)
    virtual wxMenu *Remove(size_t pos);

    // enable or disable a submenu
    virtual void EnableTop(size_t pos, bool enable) = 0;

    // is the menu enabled?
    virtual bool IsEnabledTop(size_t WXUNUSED(pos)) const { return true; }

    // get or change the label of the menu at given position
    virtual void SetLabelTop(size_t pos, const wxString& label) = 0;
    virtual wxString GetLabelTop(size_t pos) const = 0;

    // item search
    // -----------

    // by menu and item names, returns wxNOT_FOUND if not found or id of the
    // found item
    virtual int FindMenuItem(const wxString& menu, const wxString& item) const;

    // find item by id (in any menu), returns NULL if not found
    //
    // if menu is !NULL, it will be filled with wxMenu this item belongs to
    virtual wxMenuItem* FindItem(int itemid, wxMenu **menu = NULL) const;

    // find menu by its caption, return wxNOT_FOUND on failure
    int FindMenu(const wxString& title) const;

    // item access
    // -----------

    // all these functions just use FindItem() and then call an appropriate
    // method on it
    //
    // NB: under MSW, these methods can only be used after the menubar had
    //     been attached to the frame

    void Enable(int itemid, bool enable);
    void Check(int itemid, bool check);
    bool IsChecked(int itemid) const;
    bool IsEnabled(int itemid) const;
    virtual bool IsEnabled() const { return wxWindow::IsEnabled(); }

    void SetLabel(int itemid, const wxString &label);
    wxString GetLabel(int itemid) const;

    void SetHelpString(int itemid, const wxString& helpString);
    wxString GetHelpString(int itemid) const;

    // implementation helpers

    // get the frame we are attached to (may return NULL)
    wxFrame *GetFrame() const { return m_menuBarFrame; }

    // returns true if we're attached to a frame
    bool IsAttached() const { return GetFrame() != NULL; }

    // associate the menubar with the frame
    virtual void Attach(wxFrame *frame);

    // called before deleting the menubar normally
    virtual void Detach();

    // need to override these ones to avoid virtual function hiding
    virtual bool Enable(bool enable = true) { return wxWindow::Enable(enable); }
    virtual void SetLabel(const wxString& s) { wxWindow::SetLabel(s); }
    virtual wxString GetLabel() const { return wxWindow::GetLabel(); }

    // don't want menu bars to accept the focus by tabbing to them
    virtual bool AcceptsFocusFromKeyboard() const { return false; }

    // update all menu item states in all menus
    virtual void UpdateMenus();

protected:
    // the list of all our menus
    wxMenuList m_menus;

    // the frame we are attached to (may be NULL)
    wxFrame *m_menuBarFrame;

    DECLARE_NO_COPY_CLASS(wxMenuBarBase)

public:

#if wxABI_VERSION >= 20805
    // Replacement for SetLabelTop
    void SetMenuLabel(size_t pos, const wxString& label) { SetLabelTop(pos, label); }

    // Gets the original label at the top-level of the menubar
    // Implemented per port, since we can't have virtual functions in the stable branch.
    // wxString GetMenuLabel(size_t pos) const;

    // Get the text only, from the label at the top-level of the menubar
    wxString GetMenuLabelText(size_t pos) const;
#endif

};

// ----------------------------------------------------------------------------
// include the real class declaration
// ----------------------------------------------------------------------------

#ifdef wxUSE_BASE_CLASSES_ONLY
    #define wxMenuItem wxMenuItemBase
#else // !wxUSE_BASE_CLASSES_ONLY
#if defined(__WXUNIVERSAL__)
    #include "wx/univ/menu.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/menu.h"
#elif defined(__WXMSW__)
    #include "wx/msw/menu.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/menu.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/menu.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/menu.h"
#elif defined(__WXMAC__)
    #include "wx/mac/menu.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/menu.h"
#elif defined(__WXPM__)
    #include "wx/os2/menu.h"
#endif
#endif // wxUSE_BASE_CLASSES_ONLY/!wxUSE_BASE_CLASSES_ONLY

#endif // wxUSE_MENUS

#endif
    // _WX_MENU_H_BASE_
