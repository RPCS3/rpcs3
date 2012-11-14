///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/menuce.cpp
// Purpose:     Smartphone menus implementation
// Author:      Wlodzimierz ABX Skiba
// Modified by:
// Created:     28.05.2004
// RCS-ID:      $Id: menuce.cpp 40978 2006-09-03 12:23:04Z RR $
// Copyright:   (c) Wlodzimierz Skiba
// License:     wxWindows licence
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

#if defined(__SMARTPHONE__) && defined(__WXWINCE__)

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/toplevel.h"
    #include "wx/menu.h"
#endif //WX_PRECOMP

#include <windows.h>
#include <ole2.h>
#include <shellapi.h>
#include <aygshell.h>
#include <tpcshell.h>
#include <tpcuser.h>
#include "wx/msw/wince/missing.h"

#include "wx/msw/wince/resources.h"

#include "wx/stockitem.h"

wxTopLevelWindowMSW::ButtonMenu::ButtonMenu()
{
    m_id = wxID_ANY;
    m_label = wxEmptyString;
    m_menu = NULL;
    m_assigned = false;
}

wxTopLevelWindowMSW::ButtonMenu::~ButtonMenu()
{
    if(m_menu)
    {
        delete m_menu;
        m_menu = NULL;
    };
}

void wxTopLevelWindowMSW::SetLeftMenu(int id, const wxString& label, wxMenu *subMenu)
{
    m_LeftButton.SetButton(id, label, subMenu);
    ReloadAllButtons();
}

void wxTopLevelWindowMSW::SetRightMenu(int id, const wxString& label, wxMenu *subMenu)
{
    m_RightButton.SetButton(id, label, subMenu);
    ReloadAllButtons();
}

void wxTopLevelWindowMSW::ButtonMenu::SetButton(int id, const wxString& label, wxMenu *subMenu)
{
    m_assigned = true;
    m_id = id;
    if(label.empty() && wxIsStockID(id))
        m_label = wxGetStockLabel(id, wxSTOCK_NOFLAGS);
    else
        m_label = label;
    m_menu = subMenu;
}

wxMenu *wxTopLevelWindowMSW::ButtonMenu::DuplicateMenu(wxMenu *menu)
{
    // This is required in case of converting wxMenuBar to wxMenu in wxFrame::SetMenuBar.
    // All submenus has to be recreated because of new owner.

    wxMenu *duplication = new wxMenu;

    if (menu)
    {
        wxMenuItemList::compatibility_iterator node = menu->GetMenuItems().GetFirst();
        while (node)
        {
            wxMenuItem *item = node->GetData();
            if (item)
            {
                wxMenu *submenu = NULL;

                if(item->IsSubMenu())
                    submenu = DuplicateMenu( item->GetSubMenu() );
                else
                    submenu = NULL;

                wxMenuItem *new_item = wxMenuItem::New(duplication, item->GetId(), item->GetLabel(), item->GetHelp(), item->GetKind(), submenu);

                if( item->IsCheckable() )
                    new_item->Check(item->IsChecked());

                new_item->Enable( item->IsEnabled() );

                duplication->Append(new_item);
            }
            node = node->GetNext();
        }

    }

    return duplication;
}

void wxMenuToHMenu(wxMenu* in, HMENU hMenu)
{
    if(!in) return;

    wxChar buf[256];

    wxMenuItemList::compatibility_iterator node = in->GetMenuItems().GetFirst();
    while ( node )
    {
        wxMenuItem *item = node->GetData();

        UINT uFlags = 0;
        UINT uIDNewItem;
        LPCTSTR lpNewItem;

        if( item->IsSeparator() )
        {
            uFlags |= MF_SEPARATOR;
            uIDNewItem = (unsigned)wxID_ANY;
            lpNewItem = NULL;
        }
        else
        {
            // label
            uFlags |= MF_STRING;
            wxStrcpy(buf, item->GetLabel().c_str());
            lpNewItem = buf;

            // state
            uFlags |= ( item->IsEnabled() ? MF_ENABLED : MF_GRAYED );

            // checked
            uFlags |= ( item->IsChecked() ? MF_CHECKED : MF_UNCHECKED );

            if( item->IsSubMenu() )
            {
                uFlags |= MF_POPUP;
                HMENU hSubMenu = CreatePopupMenu();
                wxMenuToHMenu(item->GetSubMenu(), hSubMenu);
                uIDNewItem = (UINT) hSubMenu;
            }
            else
            {
                uIDNewItem = item->GetId();
            }
        }

        AppendMenu(hMenu, uFlags, uIDNewItem, lpNewItem);

        node = node->GetNext();
    }
}

void wxTopLevelWindowMSW::ReloadButton(ButtonMenu& button, UINT menuID)
{
    TBBUTTONINFO  button_info;
    wxChar        buf[256];

    // set button name
    memset (&button_info, 0, sizeof (TBBUTTONINFO));
    button_info.cbSize = sizeof(TBBUTTONINFO);
    button_info.dwMask = TBIF_TEXT | TBIF_STATE;
    button_info.fsState = TBSTATE_ENABLED;
    wxStrcpy(buf, button.GetLabel().c_str());
    button_info.pszText = buf;
    ::SendMessage(m_MenuBarHWND, TB_SETBUTTONINFO, menuID, (LPARAM) &button_info);

    if(button.IsMenu())
    {
        HMENU hPopupMenu = (HMENU) ::SendMessage(m_MenuBarHWND, SHCMBM_GETSUBMENU, 0, menuID);
        RemoveMenu(hPopupMenu, 0, MF_BYPOSITION);
        wxMenuToHMenu(button.GetMenu(), hPopupMenu);
    }
}

void wxTopLevelWindowMSW::ReloadAllButtons()
{
    // first reaload only after initialization of both buttons
    // it should is done at the end of Create() of wxTLW
    if(!m_LeftButton.IsAssigned() || !m_RightButton.IsAssigned())
        return;

    SHMENUBARINFO menu_bar;
    wxString      label;

    memset (&menu_bar,    0, sizeof (SHMENUBARINFO));
    menu_bar.cbSize     = sizeof (SHMENUBARINFO);
    menu_bar.hwndParent = (HWND) GetHWND();

    if(m_LeftButton.IsMenu() && m_RightButton.IsMenu())
        menu_bar.nToolBarId = IDR_MENUBAR_BOTH_MENUS;
    else if(m_LeftButton.IsMenu())
        menu_bar.nToolBarId = IDR_MENUBAR_LEFT_MENU;
    else if(m_RightButton.IsMenu())
        menu_bar.nToolBarId = IDR_MENUBAR_RIGHT_MENU;
    else
        menu_bar.nToolBarId = IDR_MENUBAR_ONE_BUTTON;

    menu_bar.hInstRes = wxGetInstance();

    if (!SHCreateMenuBar(&menu_bar))
    {
        wxFAIL_MSG( _T("SHCreateMenuBar failed") );
        return;
    }

    HWND prev_MenuBar = m_MenuBarHWND;
    m_MenuBarHWND = menu_bar.hwndMB;

    ReloadButton(m_LeftButton, IDM_LEFT);
    ReloadButton(m_RightButton, IDM_RIGHT);

    // hide previous and show new menubar
    if ( prev_MenuBar )
        ::ShowWindow( prev_MenuBar, SW_HIDE );
    ::ShowWindow( m_MenuBarHWND, SW_SHOW );

    // Setup backspace key handling
    SendMessage(m_MenuBarHWND, SHCMBM_OVERRIDEKEY, VK_TBACK,
                MAKELPARAM( SHMBOF_NODEFAULT | SHMBOF_NOTIFY,
                            SHMBOF_NODEFAULT | SHMBOF_NOTIFY ));
}

bool wxTopLevelWindowMSW::HandleCommand(WXWORD id, WXWORD WXUNUSED(cmd), WXHWND WXUNUSED(control))
{
    // handle here commands from Smartphone menu bar
    if ( id == IDM_LEFT || id == IDM_RIGHT )
    {
        int menuId = id == IDM_LEFT ? m_LeftButton.GetId() : m_RightButton.GetId() ;
        wxCommandEvent commandEvent(wxEVT_COMMAND_MENU_SELECTED, menuId);
        commandEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(commandEvent);
        return true;
    }
    return false;
}

bool wxTopLevelWindowMSW::MSWShouldPreProcessMessage(WXMSG* pMsg)
{
    MSG *msg = (MSG *)pMsg;

    // Process back key to be like backspace.
    if (msg->message == WM_HOTKEY)
    {
        if (HIWORD(msg->lParam) == VK_TBACK)
            SHSendBackToFocusWindow(msg->message, msg->wParam, msg->lParam);
    }

    return wxTopLevelWindowBase::MSWShouldPreProcessMessage(pMsg);
}

#endif // __SMARTPHONE__ && __WXWINCE__
