///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/menucmn.cpp
// Purpose:     wxMenu and wxMenuBar methods common to all ports
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.10.99
// RCS-ID:      $Id: menucmn.cpp 57852 2009-01-06 09:40:34Z SC $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_MENUS

#include <ctype.h>

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/menu.h"
#endif

#include "wx/stockitem.h"

// ----------------------------------------------------------------------------
// template lists
// ----------------------------------------------------------------------------

#include "wx/listimpl.cpp"

WX_DEFINE_LIST(wxMenuList)
WX_DEFINE_LIST(wxMenuItemList)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxAcceleratorEntry
// ----------------------------------------------------------------------------


#if wxUSE_ACCEL

static const struct wxKeyName
{
    wxKeyCode code;
    const wxChar *name;
} wxKeyNames[] =
{
    { WXK_DELETE, wxTRANSLATE("DEL") },
    { WXK_DELETE, wxTRANSLATE("DELETE") },
    { WXK_BACK, wxTRANSLATE("BACK") },
    { WXK_INSERT, wxTRANSLATE("INS") },
    { WXK_INSERT, wxTRANSLATE("INSERT") },
    { WXK_RETURN, wxTRANSLATE("ENTER") },
    { WXK_RETURN, wxTRANSLATE("RETURN") },
    { WXK_PAGEUP, wxTRANSLATE("PGUP") },
    { WXK_PAGEDOWN, wxTRANSLATE("PGDN") },
    { WXK_LEFT, wxTRANSLATE("LEFT") },
    { WXK_RIGHT, wxTRANSLATE("RIGHT") },
    { WXK_UP, wxTRANSLATE("UP") },
    { WXK_DOWN, wxTRANSLATE("DOWN") },
    { WXK_HOME, wxTRANSLATE("HOME") },
    { WXK_END, wxTRANSLATE("END") },
    { WXK_SPACE, wxTRANSLATE("SPACE") },
    { WXK_TAB, wxTRANSLATE("TAB") },
    { WXK_ESCAPE, wxTRANSLATE("ESC") },
    { WXK_ESCAPE, wxTRANSLATE("ESCAPE") },
    { WXK_CANCEL, wxTRANSLATE("CANCEL") },
    { WXK_CLEAR, wxTRANSLATE("CLEAR") },
    { WXK_MENU, wxTRANSLATE("MENU") },
    { WXK_PAUSE, wxTRANSLATE("PAUSE") },
    { WXK_CAPITAL, wxTRANSLATE("CAPITAL") },
    { WXK_SELECT, wxTRANSLATE("SELECT") },
    { WXK_PRINT, wxTRANSLATE("PRINT") },
    { WXK_EXECUTE, wxTRANSLATE("EXECUTE") },
    { WXK_SNAPSHOT, wxTRANSLATE("SNAPSHOT") },
    { WXK_HELP, wxTRANSLATE("HELP") },
    { WXK_ADD, wxTRANSLATE("ADD") },
    { WXK_SEPARATOR, wxTRANSLATE("SEPARATOR") },
    { WXK_SUBTRACT, wxTRANSLATE("SUBTRACT") },
    { WXK_DECIMAL, wxTRANSLATE("DECIMAL") },
    { WXK_DIVIDE, wxTRANSLATE("DIVIDE") },
    { WXK_NUMLOCK, wxTRANSLATE("NUM_LOCK") },
    { WXK_SCROLL, wxTRANSLATE("SCROLL_LOCK") },
    { WXK_PAGEUP, wxTRANSLATE("PAGEUP") },
    { WXK_PAGEDOWN, wxTRANSLATE("PAGEDOWN") },
    { WXK_NUMPAD_SPACE, wxTRANSLATE("KP_SPACE") },
    { WXK_NUMPAD_TAB, wxTRANSLATE("KP_TAB") },
    { WXK_NUMPAD_ENTER, wxTRANSLATE("KP_ENTER") },
    { WXK_NUMPAD_HOME, wxTRANSLATE("KP_HOME") },
    { WXK_NUMPAD_LEFT, wxTRANSLATE("KP_LEFT") },
    { WXK_NUMPAD_UP, wxTRANSLATE("KP_UP") },
    { WXK_NUMPAD_RIGHT, wxTRANSLATE("KP_RIGHT") },
    { WXK_NUMPAD_DOWN, wxTRANSLATE("KP_DOWN") },
    { WXK_NUMPAD_PAGEUP, wxTRANSLATE("KP_PRIOR") },
    { WXK_NUMPAD_PAGEUP, wxTRANSLATE("KP_PAGEUP") },
    { WXK_NUMPAD_PAGEDOWN, wxTRANSLATE("KP_NEXT") },
    { WXK_NUMPAD_PAGEDOWN, wxTRANSLATE("KP_PAGEDOWN") },
    { WXK_NUMPAD_END, wxTRANSLATE("KP_END") },
    { WXK_NUMPAD_BEGIN, wxTRANSLATE("KP_BEGIN") },
    { WXK_NUMPAD_INSERT, wxTRANSLATE("KP_INSERT") },
    { WXK_NUMPAD_DELETE, wxTRANSLATE("KP_DELETE") },
    { WXK_NUMPAD_EQUAL, wxTRANSLATE("KP_EQUAL") },
    { WXK_NUMPAD_MULTIPLY, wxTRANSLATE("KP_MULTIPLY") },
    { WXK_NUMPAD_ADD, wxTRANSLATE("KP_ADD") },
    { WXK_NUMPAD_SEPARATOR, wxTRANSLATE("KP_SEPARATOR") },
    { WXK_NUMPAD_SUBTRACT, wxTRANSLATE("KP_SUBTRACT") },
    { WXK_NUMPAD_DECIMAL, wxTRANSLATE("KP_DECIMAL") },
    { WXK_NUMPAD_DIVIDE, wxTRANSLATE("KP_DIVIDE") },
    { WXK_WINDOWS_LEFT, wxTRANSLATE("WINDOWS_LEFT") },
    { WXK_WINDOWS_RIGHT, wxTRANSLATE("WINDOWS_RIGHT") },
    { WXK_WINDOWS_MENU, wxTRANSLATE("WINDOWS_MENU") },
    { WXK_COMMAND, wxTRANSLATE("COMMAND") },
};

// return true if the 2 strings refer to the same accel
//
// as accels can be either translated or not, check for both possibilities and
// also compare case-insensitively as the key names case doesn't count
static inline bool CompareAccelString(const wxString& str, const wxChar *accel)
{
    return str.CmpNoCase(accel) == 0
#if wxUSE_INTL
            || str.CmpNoCase(wxGetTranslation(accel)) == 0
#endif
            ;
}

// return prefixCode+number if the string is of the form "<prefix><number>" and
// 0 if it isn't
//
// first and last parameter specify the valid domain for "number" part
static int
        IsNumberedAccelKey(const wxString& str,
                           const wxChar *prefix,
                           wxKeyCode prefixCode,
                           unsigned first,
                           unsigned last)
{
    const size_t lenPrefix = wxStrlen(prefix);
    if ( !CompareAccelString(str.Left(lenPrefix), prefix) )
        return 0;

    unsigned long num;
    if ( !str.Mid(lenPrefix).ToULong(&num) )
        return 0;

    if ( num < first || num > last )
    {
        // this must be a mistake, chances that this is a valid name of another
        // key are vanishingly small
        wxLogDebug(_T("Invalid key string \"%s\""), str.c_str());
        return 0;
    }

    return prefixCode + num - first;
}

/* static */
bool
wxAcceleratorEntry::ParseAccel(const wxString& text, int *flagsOut, int *keyOut)
{
    // the parser won't like trailing spaces
    wxString label = text;
    label.Trim(true);  // the initial \t must be preserved so don't strip leading whitespaces

    // check for accelerators: they are given after '\t'
    int posTab = label.Find(wxT('\t'));
    if ( posTab == wxNOT_FOUND )
    {
        return false;
    }

    // parse the accelerator string
    int accelFlags = wxACCEL_NORMAL;
    wxString current;
    for ( size_t n = (size_t)posTab + 1; n < label.length(); n++ )
    {
        if ( (label[n] == '+') || (label[n] == '-') )
        {
            // differentiate between a ctrl that will be translated to cmd on mac
            // and an explicit xctrl that will not be translated and remains a ctrl
            if ( CompareAccelString(current, wxTRANSLATE("ctrl")) )
                accelFlags |= wxACCEL_CMD;
            else if ( CompareAccelString(current, wxTRANSLATE("xctrl")) )
                accelFlags |= wxACCEL_CTRL;
            else if ( CompareAccelString(current, wxTRANSLATE("alt")) )
                accelFlags |= wxACCEL_ALT;
            else if ( CompareAccelString(current, wxTRANSLATE("shift")) )
                accelFlags |= wxACCEL_SHIFT;
            else // not a recognized modifier name
            {
                // we may have "Ctrl-+", for example, but we still want to
                // catch typos like "Crtl-A" so only give the warning if we
                // have something before the current '+' or '-', else take
                // it as a literal symbol
                if ( current.empty() )
                {
                    current += label[n];

                    // skip clearing it below
                    continue;
                }
                else
                {
                    wxLogDebug(wxT("Unknown accel modifier: '%s'"),
                               current.c_str());
                }
            }

            current.clear();
        }
        else // not special character
        {
            current += (wxChar) wxTolower(label[n]);
        }
    }

    int keyCode;
    const size_t len = current.length();
    switch ( len )
    {
        case 0:
            wxLogDebug(wxT("No accel key found, accel string ignored."));
            return false;

        case 1:
            // it's just a letter
            keyCode = current[0U];

            // if the key is used with any modifiers, make it an uppercase one
            // because Ctrl-A and Ctrl-a are the same; but keep it as is if it's
            // used alone as 'a' and 'A' are different
            if ( accelFlags != wxACCEL_NORMAL )
                keyCode = wxToupper(keyCode);
            break;

        default:
            keyCode = IsNumberedAccelKey(current, wxTRANSLATE("F"),
                                         WXK_F1, 1, 12);
            if ( !keyCode )
            {
                for ( size_t n = 0; n < WXSIZEOF(wxKeyNames); n++ )
                {
                    const wxKeyName& kn = wxKeyNames[n];
                    if ( CompareAccelString(current, kn.name) )
                    {
                        keyCode = kn.code;
                        break;
                    }
                }
            }

            if ( !keyCode )
                keyCode = IsNumberedAccelKey(current, wxTRANSLATE("KP_"),
                                             WXK_NUMPAD0, 0, 9);
            if ( !keyCode )
                keyCode = IsNumberedAccelKey(current, wxTRANSLATE("SPECIAL"),
                                             WXK_SPECIAL1, 1, 20);

            if ( !keyCode )
            {
                wxLogDebug(wxT("Unrecognized accel key '%s', accel string ignored."),
                           current.c_str());
                return false;
            }
    }


    wxASSERT_MSG( keyCode, _T("logic error: should have key code here") );

    if ( flagsOut )
        *flagsOut = accelFlags;
    if ( keyOut )
        *keyOut = keyCode;

    return true;
}

/* static */
wxAcceleratorEntry *wxAcceleratorEntry::Create(const wxString& str)
{
    int flags,
        keyCode;
    if ( !ParseAccel(str, &flags, &keyCode) )
        return NULL;

    return new wxAcceleratorEntry(flags, keyCode);
}

bool wxAcceleratorEntry::FromString(const wxString& str)
{
    return ParseAccel(str, &m_flags, &m_keyCode);
}

wxString wxAcceleratorEntry::ToString() const
{
    wxString text;

    int flags = GetFlags();
    if ( flags & wxACCEL_ALT )
        text += _("Alt-");
    if ( flags & wxACCEL_CMD )
        text += _("Ctrl-");
#ifdef __WXMAC__
    if ( flags & wxACCEL_CTRL )
        text += _("XCtrl-");
#endif
    if ( flags & wxACCEL_SHIFT )
        text += _("Shift-");

    const int code = GetKeyCode();

    if ( code >= WXK_F1 && code <= WXK_F12 )
        text << _("F") << code - WXK_F1 + 1;
    else if ( code >= WXK_NUMPAD0 && code <= WXK_NUMPAD9 )
        text << _("KP_") << code - WXK_NUMPAD0;
    else if ( code >= WXK_SPECIAL1 && code <= WXK_SPECIAL20 )
        text << _("SPECIAL") << code - WXK_SPECIAL1 + 1;
    else // check the named keys
    {
        size_t n;
        for ( n = 0; n < WXSIZEOF(wxKeyNames); n++ )
        {
            const wxKeyName& kn = wxKeyNames[n];
            if ( code == kn.code )
            {
                text << wxGetTranslation(kn.name);
                break;
            }
        }

        if ( n == WXSIZEOF(wxKeyNames) )
        {
            // must be a simple key
            if (
#if !wxUSE_UNICODE
                 isascii(code) &&
#endif // ANSI
                    wxIsalnum(code) )
            {
                text << (wxChar)code;
            }
            else
            {
                wxFAIL_MSG( wxT("unknown keyboard accelerator code") );
            }
        }
    }

    return text;
}

wxAcceleratorEntry *wxGetAccelFromString(const wxString& label)
{
    return wxAcceleratorEntry::Create(label);
}

#endif // wxUSE_ACCEL


// ----------------------------------------------------------------------------
// wxMenuItem
// ----------------------------------------------------------------------------

wxMenuItemBase::wxMenuItemBase(wxMenu *parentMenu,
                               int id,
                               const wxString& text,
                               const wxString& help,
                               wxItemKind kind,
                               wxMenu *subMenu)
{
    wxASSERT_MSG( parentMenu != NULL, wxT("menuitem should have a menu") );

    m_parentMenu  = parentMenu;
    m_subMenu     = subMenu;
    m_isEnabled   = true;
    m_isChecked   = false;
    m_id          = id;
    m_kind        = kind;
    if (m_id == wxID_ANY)
        m_id = wxNewId();
    if (m_id == wxID_SEPARATOR)
        m_kind = wxITEM_SEPARATOR;

    SetText(text);
    SetHelp(help);
}

wxMenuItemBase::~wxMenuItemBase()
{
    delete m_subMenu;
}

#if wxUSE_ACCEL

wxAcceleratorEntry *wxMenuItemBase::GetAccel() const
{
    return wxAcceleratorEntry::Create(GetText());
}

void wxMenuItemBase::SetAccel(wxAcceleratorEntry *accel)
{
    wxString text = m_text.BeforeFirst(wxT('\t'));
    if ( accel )
    {
        text += wxT('\t');
        text += accel->ToString();
    }

    SetText(text);
}

#endif // wxUSE_ACCEL

void wxMenuItemBase::SetText(const wxString& str)
{
    m_text = str;

    if ( m_text.empty() && !IsSeparator() )
    {
        wxASSERT_MSG( wxIsStockID(GetId()),
                      wxT("A non-stock menu item with an empty label?") );
        m_text = wxGetStockLabel(GetId(), wxSTOCK_WITH_ACCELERATOR |
                                          wxSTOCK_WITH_MNEMONIC);
    }
}

void wxMenuItemBase::SetHelp(const wxString& str)
{
    m_help = str;

    if ( m_help.empty() && !IsSeparator() && wxIsStockID(GetId()) )
    {
        // get a stock help string
        m_help = wxGetStockHelpString(GetId());
    }
}

wxString wxMenuItemBase::GetLabelText(const wxString& label)
{
    return GetLabelFromText(label);
}

bool wxMenuBase::ms_locked = true;

// ----------------------------------------------------------------------------
// wxMenu ctor and dtor
// ----------------------------------------------------------------------------

void wxMenuBase::Init(long style)
{
    m_menuBar = (wxMenuBar *)NULL;
    m_menuParent = (wxMenu *)NULL;

    m_invokingWindow = (wxWindow *)NULL;
    m_style = style;
    m_clientData = (void *)NULL;
    m_eventHandler = this;
}

wxMenuBase::~wxMenuBase()
{
    WX_CLEAR_LIST(wxMenuItemList, m_items);
}

// ----------------------------------------------------------------------------
// wxMenu item adding/removing
// ----------------------------------------------------------------------------

void wxMenuBase::AddSubMenu(wxMenu *submenu)
{
    wxCHECK_RET( submenu, _T("can't add a NULL submenu") );

    submenu->SetParent((wxMenu *)this);
}

wxMenuItem* wxMenuBase::DoAppend(wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Append()") );

    m_items.Append(item);
    item->SetMenu((wxMenu*)this);
    if ( item->IsSubMenu() )
    {
        AddSubMenu(item->GetSubMenu());
    }

    return item;
}

wxMenuItem* wxMenuBase::Insert(size_t pos, wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Insert") );

    if ( pos == GetMenuItemCount() )
    {
        return DoAppend(item);
    }
    else
    {
        wxCHECK_MSG( pos < GetMenuItemCount(), NULL,
                     wxT("invalid index in wxMenu::Insert") );

        return DoInsert(pos, item);
    }
}

wxMenuItem* wxMenuBase::DoInsert(size_t pos, wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Insert()") );

    wxMenuItemList::compatibility_iterator node = m_items.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("invalid index in wxMenu::Insert()") );

    m_items.Insert(node, item);
    item->SetMenu((wxMenu*)this);
    if ( item->IsSubMenu() )
    {
        AddSubMenu(item->GetSubMenu());
    }

    return item;
}

wxMenuItem *wxMenuBase::Remove(wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Remove") );

    return DoRemove(item);
}

wxMenuItem *wxMenuBase::DoRemove(wxMenuItem *item)
{
    wxMenuItemList::compatibility_iterator node = m_items.Find(item);

    // if we get here, the item is valid or one of Remove() functions is broken
    wxCHECK_MSG( node, NULL, wxT("bug in wxMenu::Remove logic") );

    // we detach the item, but we do delete the list node (i.e. don't call
    // DetachNode() here!)
    m_items.Erase(node);

    // item isn't attached to anything any more
    item->SetMenu((wxMenu *)NULL);
    wxMenu *submenu = item->GetSubMenu();
    if ( submenu )
    {
        submenu->SetParent((wxMenu *)NULL);
        if ( submenu->IsAttached() )
            submenu->Detach();
    }

    return item;
}

bool wxMenuBase::Delete(wxMenuItem *item)
{
    wxCHECK_MSG( item, false, wxT("invalid item in wxMenu::Delete") );

    return DoDelete(item);
}

bool wxMenuBase::DoDelete(wxMenuItem *item)
{
    wxMenuItem *item2 = DoRemove(item);
    wxCHECK_MSG( item2, false, wxT("failed to delete menu item") );

    // don't delete the submenu
    item2->SetSubMenu((wxMenu *)NULL);

    delete item2;

    return true;
}

bool wxMenuBase::Destroy(wxMenuItem *item)
{
    wxCHECK_MSG( item, false, wxT("invalid item in wxMenu::Destroy") );

    return DoDestroy(item);
}

bool wxMenuBase::DoDestroy(wxMenuItem *item)
{
    wxMenuItem *item2 = DoRemove(item);
    wxCHECK_MSG( item2, false, wxT("failed to delete menu item") );

    delete item2;

    return true;
}

// ----------------------------------------------------------------------------
// wxMenu searching for items
// ----------------------------------------------------------------------------

// Finds the item id matching the given string, wxNOT_FOUND if not found.
int wxMenuBase::FindItem(const wxString& text) const
{
    wxString label = wxMenuItem::GetLabelFromText(text);
    for ( wxMenuItemList::compatibility_iterator node = m_items.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxMenuItem *item = node->GetData();
        if ( item->IsSubMenu() )
        {
            int rc = item->GetSubMenu()->FindItem(label);
            if ( rc != wxNOT_FOUND )
                return rc;
        }

        // we execute this code for submenus as well to alllow finding them by
        // name just like the ordinary items
        if ( !item->IsSeparator() )
        {
            if ( item->GetLabel() == label )
                return item->GetId();
        }
    }

    return wxNOT_FOUND;
}

// recursive search for item by id
wxMenuItem *wxMenuBase::FindItem(int itemId, wxMenu **itemMenu) const
{
    if ( itemMenu )
        *itemMenu = NULL;

    wxMenuItem *item = NULL;
    for ( wxMenuItemList::compatibility_iterator node = m_items.GetFirst();
          node && !item;
          node = node->GetNext() )
    {
        item = node->GetData();

        if ( item->GetId() == itemId )
        {
            if ( itemMenu )
                *itemMenu = (wxMenu *)this;
        }
        else if ( item->IsSubMenu() )
        {
            item = item->GetSubMenu()->FindItem(itemId, itemMenu);
        }
        else
        {
            // don't exit the loop
            item = NULL;
        }
    }

    return item;
}

// non recursive search
wxMenuItem *wxMenuBase::FindChildItem(int id, size_t *ppos) const
{
    wxMenuItem *item = (wxMenuItem *)NULL;
    wxMenuItemList::compatibility_iterator node = GetMenuItems().GetFirst();

    size_t pos;
    for ( pos = 0; node; pos++ )
    {
        if ( node->GetData()->GetId() == id )
        {
            item = node->GetData();

            break;
        }

        node = node->GetNext();
    }

    if ( ppos )
    {
        *ppos = item ? pos : (size_t)wxNOT_FOUND;
    }

    return item;
}

// find by position
wxMenuItem* wxMenuBase::FindItemByPosition(size_t position) const
{
    wxCHECK_MSG( position < m_items.GetCount(), NULL,
                 _T("wxMenu::FindItemByPosition(): invalid menu index") );

    return m_items.Item( position )->GetData();
}

// ----------------------------------------------------------------------------
// wxMenu helpers used by derived classes
// ----------------------------------------------------------------------------

// Update a menu and all submenus recursively. source is the object that has
// the update event handlers defined for it. If NULL, the menu or associated
// window will be used.
void wxMenuBase::UpdateUI(wxEvtHandler* source)
{
    if (GetInvokingWindow())
    {
        // Don't update menus if the parent
        // frame is about to get deleted
        wxWindow *tlw = wxGetTopLevelParent( GetInvokingWindow() );
        if (tlw && wxPendingDelete.Member(tlw))
            return;
    }

    if ( !source && GetInvokingWindow() )
        source = GetInvokingWindow()->GetEventHandler();
    if ( !source )
        source = GetEventHandler();
    if ( !source )
        source = this;

    wxMenuItemList::compatibility_iterator  node = GetMenuItems().GetFirst();
    while ( node )
    {
        wxMenuItem* item = node->GetData();
        if ( !item->IsSeparator() )
        {
            wxWindowID id = item->GetId();
            wxUpdateUIEvent event(id);
            event.SetEventObject( source );

            if ( source->ProcessEvent(event) )
            {
                // if anything changed, update the changed attribute
                if (event.GetSetText())
                    SetLabel(id, event.GetText());
                if (event.GetSetChecked())
                    Check(id, event.GetChecked());
                if (event.GetSetEnabled())
                    Enable(id, event.GetEnabled());
            }

            // recurse to the submenus
            if ( item->GetSubMenu() )
                item->GetSubMenu()->UpdateUI(source);
        }
        //else: item is a separator (which doesn't process update UI events)

        node = node->GetNext();
    }
}

bool wxMenuBase::SendEvent(int id, int checked)
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, id);
    event.SetEventObject(this);
    event.SetInt(checked);

    bool processed = false;

    // Try the menu's event handler
    // if ( !processed )
    {
        wxEvtHandler *handler = GetEventHandler();
        if ( handler )
            processed = handler->ProcessEvent(event);
    }

    // Try the window the menu was popped up from (and up through the
    // hierarchy)
    if ( !processed )
    {
        const wxMenuBase *menu = this;
        while ( menu )
        {
            wxWindow *win = menu->GetInvokingWindow();
            if ( win )
            {
                processed = win->GetEventHandler()->ProcessEvent(event);
                break;
            }

            menu = menu->GetParent();
        }
    }

    return processed;
}

// ----------------------------------------------------------------------------
// wxMenu attaching/detaching to/from menu bar
// ----------------------------------------------------------------------------

wxMenuBar* wxMenuBase::GetMenuBar() const
{
    if(GetParent())
        return GetParent()->GetMenuBar();
    return m_menuBar;
}

void wxMenuBase::Attach(wxMenuBarBase *menubar)
{
    // use Detach() instead!
    wxASSERT_MSG( menubar, _T("menu can't be attached to NULL menubar") );

    // use IsAttached() to prevent this from happening
    wxASSERT_MSG( !m_menuBar, _T("attaching menu twice?") );

    m_menuBar = (wxMenuBar *)menubar;
}

void wxMenuBase::Detach()
{
    // use IsAttached() to prevent this from happening
    wxASSERT_MSG( m_menuBar, _T("detaching unattached menu?") );

    m_menuBar = NULL;
}

// ----------------------------------------------------------------------------
// wxMenu functions forwarded to wxMenuItem
// ----------------------------------------------------------------------------

void wxMenuBase::Enable( int id, bool enable )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::Enable: no such item") );

    item->Enable(enable);
}

bool wxMenuBase::IsEnabled( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenu::IsEnabled: no such item") );

    return item->IsEnabled();
}

void wxMenuBase::Check( int id, bool enable )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::Check: no such item") );

    item->Check(enable);
}

bool wxMenuBase::IsChecked( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenu::IsChecked: no such item") );

    return item->IsChecked();
}

void wxMenuBase::SetLabel( int id, const wxString &label )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::SetLabel: no such item") );

    item->SetText(label);
}

wxString wxMenuBase::GetLabel( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString, wxT("wxMenu::GetLabel: no such item") );

    return item->GetText();
}

void wxMenuBase::SetHelpString( int id, const wxString& helpString )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::SetHelpString: no such item") );

    item->SetHelp( helpString );
}

wxString wxMenuBase::GetHelpString( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString, wxT("wxMenu::GetHelpString: no such item") );

    return item->GetHelp();
}

// ----------------------------------------------------------------------------
// wxMenuBarBase ctor and dtor
// ----------------------------------------------------------------------------

wxMenuBarBase::wxMenuBarBase()
{
    // not attached yet
    m_menuBarFrame = NULL;
}

wxMenuBarBase::~wxMenuBarBase()
{
    WX_CLEAR_LIST(wxMenuList, m_menus);
}

// ----------------------------------------------------------------------------
// wxMenuBar item access: the base class versions manage m_menus list, the
// derived class should reflect the changes in the real menubar
// ----------------------------------------------------------------------------

wxMenu *wxMenuBarBase::GetMenu(size_t pos) const
{
    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::GetMenu()") );

    return node->GetData();
}

bool wxMenuBarBase::Append(wxMenu *menu, const wxString& WXUNUSED(title))
{
    wxCHECK_MSG( menu, false, wxT("can't append NULL menu") );

    m_menus.Append(menu);
    menu->Attach(this);

    return true;
}

bool wxMenuBarBase::Insert(size_t pos, wxMenu *menu,
                           const wxString& title)
{
    if ( pos == m_menus.GetCount() )
    {
        return wxMenuBarBase::Append(menu, title);
    }
    else // not at the end
    {
        wxCHECK_MSG( menu, false, wxT("can't insert NULL menu") );

        wxMenuList::compatibility_iterator node = m_menus.Item(pos);
        wxCHECK_MSG( node, false, wxT("bad index in wxMenuBar::Insert()") );

        m_menus.Insert(node, menu);
        menu->Attach(this);

        return true;
    }
}

wxMenu *wxMenuBarBase::Replace(size_t pos, wxMenu *menu,
                               const wxString& WXUNUSED(title))
{
    wxCHECK_MSG( menu, NULL, wxT("can't insert NULL menu") );

    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::Replace()") );

    wxMenu *menuOld = node->GetData();
    node->SetData(menu);

    menu->Attach(this);
    menuOld->Detach();

    return menuOld;
}

wxMenu *wxMenuBarBase::Remove(size_t pos)
{
    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::Remove()") );

    wxMenu *menu = node->GetData();
    m_menus.Erase(node);
    menu->Detach();

    return menu;
}

int wxMenuBarBase::FindMenu(const wxString& title) const
{
    wxString label = wxMenuItem::GetLabelFromText(title);

    size_t count = GetMenuCount();
    for ( size_t i = 0; i < count; i++ )
    {
        wxString title2 = GetLabelTop(i);
        if ( (title2 == title) ||
             (wxMenuItem::GetLabelFromText(title2) == label) )
        {
            // found
            return (int)i;
        }
    }

    return wxNOT_FOUND;

}

// ----------------------------------------------------------------------------
// wxMenuBar attaching/detaching to/from the frame
// ----------------------------------------------------------------------------

void wxMenuBarBase::Attach(wxFrame *frame)
{
    wxASSERT_MSG( !IsAttached(), wxT("menubar already attached!") );

    m_menuBarFrame = frame;
}

void wxMenuBarBase::Detach()
{
    wxASSERT_MSG( IsAttached(), wxT("detaching unattached menubar") );

    m_menuBarFrame = NULL;
}

// ----------------------------------------------------------------------------
// wxMenuBar searching for items
// ----------------------------------------------------------------------------

wxMenuItem *wxMenuBarBase::FindItem(int id, wxMenu **menu) const
{
    if ( menu )
        *menu = NULL;

    wxMenuItem *item = NULL;
    size_t count = GetMenuCount(), i;
    wxMenuList::const_iterator it;
    for ( i = 0, it = m_menus.begin(); !item && (i < count); i++, it++ )
    {
        item = (*it)->FindItem(id, menu);
    }

    return item;
}

int wxMenuBarBase::FindMenuItem(const wxString& menu, const wxString& item) const
{
    wxString label = wxMenuItem::GetLabelFromText(menu);

    int i = 0;
    wxMenuList::compatibility_iterator node;
    for ( node = m_menus.GetFirst(); node; node = node->GetNext(), i++ )
    {
        if ( label == wxMenuItem::GetLabelFromText(GetLabelTop(i)) )
            return node->GetData()->FindItem(item);
    }

    return wxNOT_FOUND;
}

// ---------------------------------------------------------------------------
// wxMenuBar functions forwarded to wxMenuItem
// ---------------------------------------------------------------------------

void wxMenuBarBase::Enable(int id, bool enable)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("attempt to enable an item which doesn't exist") );

    item->Enable(enable);
}

void wxMenuBarBase::Check(int id, bool check)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("attempt to check an item which doesn't exist") );
    wxCHECK_RET( item->IsCheckable(), wxT("attempt to check an uncheckable item") );

    item->Check(check);
}

bool wxMenuBarBase::IsChecked(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenuBar::IsChecked(): no such item") );

    return item->IsChecked();
}

bool wxMenuBarBase::IsEnabled(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenuBar::IsEnabled(): no such item") );

    return item->IsEnabled();
}

void wxMenuBarBase::SetLabel(int id, const wxString& label)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenuBar::SetLabel(): no such item") );

    item->SetText(label);
}

wxString wxMenuBarBase::GetLabel(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString,
                 wxT("wxMenuBar::GetLabel(): no such item") );

    return item->GetText();
}

void wxMenuBarBase::SetHelpString(int id, const wxString& helpString)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenuBar::SetHelpString(): no such item") );

    item->SetHelp(helpString);
}

wxString wxMenuBarBase::GetHelpString(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString,
                 wxT("wxMenuBar::GetHelpString(): no such item") );

    return item->GetHelp();
}

void wxMenuBarBase::UpdateMenus( void )
{
    wxEvtHandler* source;
    wxMenu* menu;
    int nCount = GetMenuCount();
    for (int n = 0; n < nCount; n++)
    {
        menu = GetMenu( n );
        if (menu != NULL)
        {
            source = menu->GetEventHandler();
            if (source != NULL)
                menu->UpdateUI( source );
        }
    }
}

// Get the text only, from the label
wxString wxMenuBarBase::GetMenuLabelText(size_t pos) const
{
    return wxMenuItem::GetLabelText(((wxMenuBar*)this)->GetMenuLabel(pos));
}


#endif // wxUSE_MENUS
