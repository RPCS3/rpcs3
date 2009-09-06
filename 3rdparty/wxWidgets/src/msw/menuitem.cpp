///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/menuitem.cpp
// Purpose:     wxMenuItem implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.11.97
// RCS-ID:      $Id: menuitem.cpp 41021 2006-09-05 21:00:55Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_MENUS

#include "wx/menuitem.h"
#include "wx/stockitem.h"

#ifndef WX_PRECOMP
    #include "wx/font.h"
    #include "wx/bitmap.h"
    #include "wx/settings.h"
    #include "wx/window.h"
    #include "wx/accel.h"
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/menu.h"
#endif

#if wxUSE_ACCEL
    #include "wx/accel.h"
#endif // wxUSE_ACCEL

#include "wx/msw/private.h"

#ifdef __WXWINCE__
// Implemented in menu.cpp
UINT GetMenuState(HMENU hMenu, UINT id, UINT flags) ;
#endif

// ---------------------------------------------------------------------------
// macro
// ---------------------------------------------------------------------------

// hide the ugly cast
#define GetHMenuOf(menu)    ((HMENU)menu->GetHMenu())

// conditional compilation
#if wxUSE_OWNER_DRAWN
    #define OWNER_DRAWN_ONLY( code ) if ( IsOwnerDrawn() ) code
#else // !wxUSE_OWNER_DRAWN
    #define OWNER_DRAWN_ONLY( code )
#endif // wxUSE_OWNER_DRAWN/!wxUSE_OWNER_DRAWN

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// dynamic classes implementation
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI

bool wxMenuItemStreamingCallback( const wxObject *object, wxWriter * , wxPersister * , wxxVariantArray & )
{
    const wxMenuItem * mitem = dynamic_cast<const wxMenuItem*>(object) ;
    if ( mitem->GetMenu() && !mitem->GetMenu()->GetTitle().empty() )
    {
        // we don't stream out the first two items for menus with a title, they will be reconstructed
        if ( mitem->GetMenu()->FindItemByPosition(0) == mitem || mitem->GetMenu()->FindItemByPosition(1) == mitem )
            return false ;
    }
    return true ;
}

wxBEGIN_ENUM( wxItemKind )
    wxENUM_MEMBER( wxITEM_SEPARATOR )
    wxENUM_MEMBER( wxITEM_NORMAL )
    wxENUM_MEMBER( wxITEM_CHECK )
    wxENUM_MEMBER( wxITEM_RADIO )
wxEND_ENUM( wxItemKind )

IMPLEMENT_DYNAMIC_CLASS_XTI_CALLBACK(wxMenuItem, wxObject,"wx/menuitem.h",wxMenuItemStreamingCallback)

wxBEGIN_PROPERTIES_TABLE(wxMenuItem)
    wxPROPERTY( Parent,wxMenu*, SetMenu, GetMenu, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Id,int, SetId, GetId, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Text, wxString , SetText, GetText, wxString(), 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Help, wxString , SetHelp, GetHelp, wxString(), 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxREADONLY_PROPERTY( Kind, wxItemKind , GetKind , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( SubMenu,wxMenu*, SetSubMenu, GetSubMenu, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Enabled , bool , Enable , IsEnabled , wxxVariant((bool)true) , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Checked , bool , Check , IsChecked , wxxVariant((bool)false) , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Checkable , bool , SetCheckable , IsCheckable , wxxVariant((bool)false) , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxMenuItem)
wxEND_HANDLERS_TABLE()

wxDIRECT_CONSTRUCTOR_6( wxMenuItem , wxMenu* , Parent , int , Id , wxString , Text , wxString , Help , wxItemKind , Kind , wxMenu* , SubMenu  )
#else
IMPLEMENT_DYNAMIC_CLASS(wxMenuItem, wxObject)
#endif

// ----------------------------------------------------------------------------
// wxMenuItem
// ----------------------------------------------------------------------------

// ctor & dtor
// -----------

wxMenuItem::wxMenuItem(wxMenu *pParentMenu,
                       int id,
                       const wxString& text,
                       const wxString& strHelp,
                       wxItemKind kind,
                       wxMenu *pSubMenu)
          : wxMenuItemBase(pParentMenu, id, text, strHelp, kind, pSubMenu)
#if wxUSE_OWNER_DRAWN
            , wxOwnerDrawn(text, kind == wxITEM_CHECK, true)
#endif // owner drawn
{
    Init();
}

wxMenuItem::wxMenuItem(wxMenu *parentMenu,
                       int id,
                       const wxString& text,
                       const wxString& help,
                       bool isCheckable,
                       wxMenu *subMenu)
          : wxMenuItemBase(parentMenu, id, text, help,
                           isCheckable ? wxITEM_CHECK : wxITEM_NORMAL, subMenu)
#if wxUSE_OWNER_DRAWN
           , wxOwnerDrawn(text, isCheckable, true)
#endif // owner drawn
{
    Init();
}

void wxMenuItem::Init()
{
    m_radioGroup.start = -1;
    m_isRadioGroupStart = false;

#if  wxUSE_OWNER_DRAWN

    // when the color is not valid, wxOwnerDraw takes the default ones.
    // If we set the colors here and they are changed by the user during
    // the execution, then the colors are not updated until the application
    // is restarted and our menus look bad
    SetTextColour(wxNullColour);
    SetBackgroundColour(wxNullColour);

    // setting default colors switched ownerdraw on: switch it off again
    ResetOwnerDrawn();

    //  switch ownerdraw back on if using a non default margin
    if ( GetId() != wxID_SEPARATOR )
        SetMarginWidth(GetMarginWidth());

    // tell the owner drawing code to show the accel string as well
    SetAccelString(m_text.AfterFirst(_T('\t')));
#endif // wxUSE_OWNER_DRAWN
}

wxMenuItem::~wxMenuItem()
{
}

// misc
// ----

// return the id for calling Win32 API functions
int wxMenuItem::GetRealId() const
{
    return m_subMenu ? (int)m_subMenu->GetHMenu() : GetId();
}

// get item state
// --------------

bool wxMenuItem::IsChecked() const
{
    // fix that RTTI is always getting the correct state (separators cannot be checked, but the call below
    // returns true
    if ( GetId() == wxID_SEPARATOR )
        return false ;

    int flag = ::GetMenuState(GetHMenuOf(m_parentMenu), GetId(), MF_BYCOMMAND);

    return (flag & MF_CHECKED) != 0;
}

/* static */
wxString wxMenuItemBase::GetLabelFromText(const wxString& text)
{
    return wxStripMenuCodes(text);
}

// radio group stuff
// -----------------

void wxMenuItem::SetAsRadioGroupStart()
{
    m_isRadioGroupStart = true;
}

void wxMenuItem::SetRadioGroupStart(int start)
{
    wxASSERT_MSG( !m_isRadioGroupStart,
                  _T("should only be called for the next radio items") );

    m_radioGroup.start = start;
}

void wxMenuItem::SetRadioGroupEnd(int end)
{
    wxASSERT_MSG( m_isRadioGroupStart,
                  _T("should only be called for the first radio item") );

    m_radioGroup.end = end;
}

// change item state
// -----------------

void wxMenuItem::Enable(bool enable)
{
    if ( m_isEnabled == enable )
        return;

    long rc = EnableMenuItem(GetHMenuOf(m_parentMenu),
                             GetRealId(),
                             MF_BYCOMMAND |
                             (enable ? MF_ENABLED : MF_GRAYED));

    if ( rc == -1 ) {
        wxLogLastError(wxT("EnableMenuItem"));
    }

    wxMenuItemBase::Enable(enable);
}

void wxMenuItem::Check(bool check)
{
    wxCHECK_RET( IsCheckable(), wxT("only checkable items may be checked") );

    if ( m_isChecked == check )
        return;

    int flags = check ? MF_CHECKED : MF_UNCHECKED;
    HMENU hmenu = GetHMenuOf(m_parentMenu);

    if ( GetKind() == wxITEM_RADIO )
    {
        // it doesn't make sense to uncheck a radio item - what would this do?
        if ( !check )
            return;

        // get the index of this item in the menu
        const wxMenuItemList& items = m_parentMenu->GetMenuItems();
        int pos = items.IndexOf(this);
        wxCHECK_RET( pos != wxNOT_FOUND,
                     _T("menuitem not found in the menu items list?") );

        // get the radio group range
        int start,
            end;

        if ( m_isRadioGroupStart )
        {
            // we already have all information we need
            start = pos;
            end = m_radioGroup.end;
        }
        else // next radio group item
        {
            // get the radio group end from the start item
            start = m_radioGroup.start;
            end = items.Item(start)->GetData()->m_radioGroup.end;
        }

#ifdef __WIN32__
        // calling CheckMenuRadioItem() with such parameters hangs my system
        // (NT4 SP6) and I suspect this could happen to the others as well - so
        // don't do it!
        wxCHECK_RET( start != -1 && end != -1,
                     _T("invalid ::CheckMenuRadioItem() parameter(s)") );

        if ( !::CheckMenuRadioItem(hmenu,
                                   start,   // the first radio group item
                                   end,     // the last one
                                   pos,     // the one to check
                                   MF_BYPOSITION) )
        {
            wxLogLastError(_T("CheckMenuRadioItem"));
        }
#endif // __WIN32__

        // also uncheck all the other items in this radio group
        wxMenuItemList::compatibility_iterator node = items.Item(start);
        for ( int n = start; n <= end && node; n++ )
        {
            if ( n != pos )
            {
                node->GetData()->m_isChecked = false;
            }

            node = node->GetNext();
        }
    }
    else // check item
    {
        if ( ::CheckMenuItem(hmenu,
                             GetRealId(),
                             MF_BYCOMMAND | flags) == (DWORD)-1 )
        {
            wxFAIL_MSG( _T("CheckMenuItem() failed, item not in the menu?") );
        }
    }

    wxMenuItemBase::Check(check);
}

void wxMenuItem::SetText(const wxString& txt)
{
    wxString text = txt;

    // don't do anything if label didn't change
    if ( m_text == txt )
        return;

    // wxMenuItemBase will do stock ID checks
    wxMenuItemBase::SetText(text);

    // m_text could now be different from 'text' if we are a stock menu item,
    // so use only m_text below

    OWNER_DRAWN_ONLY( wxOwnerDrawn::SetName(m_text) );
#if wxUSE_OWNER_DRAWN
    // tell the owner drawing code to to show the accel string as well
    SetAccelString(m_text.AfterFirst(_T('\t')));
#endif

    HMENU hMenu = GetHMenuOf(m_parentMenu);
    wxCHECK_RET( hMenu, wxT("menuitem without menu") );

#if wxUSE_ACCEL
    m_parentMenu->UpdateAccel(this);
#endif // wxUSE_ACCEL

    UINT id = GetRealId();
    UINT flagsOld = ::GetMenuState(hMenu, id, MF_BYCOMMAND);
    if ( flagsOld == 0xFFFFFFFF )
    {
        // It's not an error, it means that the menu item doesn't exist
        //wxLogLastError(wxT("GetMenuState"));
    }
    else
    {
        if ( IsSubMenu() )
        {
            // high byte contains the number of items in a submenu for submenus
            flagsOld &= 0xFF;
            flagsOld |= MF_POPUP;
        }

        LPCTSTR data;

#if wxUSE_OWNER_DRAWN
        if ( IsOwnerDrawn() )
        {
            flagsOld |= MF_OWNERDRAW;
            data = (LPCTSTR)this;
        }
        else
#endif  //owner drawn
        {
            flagsOld |= MF_STRING;
            data = (wxChar*) m_text.c_str();
        }

#ifdef __WXWINCE__
        // FIXME: complete this, applying the old
        // flags.
        // However, the WinCE doc for SetMenuItemInfo
        // says that you can't use it to set the menu
        // item state; only data, id and type.
        MENUITEMINFO info;
        wxZeroMemory(info);
        info.cbSize = sizeof(info);
        info.fMask = MIIM_TYPE;
        info.fType = MFT_STRING;
        info.cch = m_text.length();
        info.dwTypeData = (LPTSTR) data ;
        if ( !::SetMenuItemInfo(hMenu, id, FALSE, & info) )
        {
            wxLogLastError(wxT("SetMenuItemInfo"));
        }
#else
        if ( ::ModifyMenu(hMenu, id,
                          MF_BYCOMMAND | flagsOld,
                          id, data) == (int)0xFFFFFFFF )
        {
            wxLogLastError(wxT("ModifyMenu"));
        }
#endif
    }
}

void wxMenuItem::SetCheckable(bool checkable)
{
    wxMenuItemBase::SetCheckable(checkable);
    OWNER_DRAWN_ONLY( wxOwnerDrawn::SetCheckable(checkable) );
}

// ----------------------------------------------------------------------------
// wxMenuItemBase
// ----------------------------------------------------------------------------

wxMenuItem *wxMenuItemBase::New(wxMenu *parentMenu,
                                int id,
                                const wxString& name,
                                const wxString& help,
                                wxItemKind kind,
                                wxMenu *subMenu)
{
    return new wxMenuItem(parentMenu, id, name, help, kind, subMenu);
}

#endif // wxUSE_MENUS
