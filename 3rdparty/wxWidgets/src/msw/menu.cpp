/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/menu.cpp
// Purpose:     wxMenu, wxMenuBar, wxMenuItem
// Author:      Julian Smart
// Modified by: Vadim Zeitlin
// Created:     04/01/98
// RCS-ID:      $Id: menu.cpp 54478 2008-07-03 15:42:18Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#include "wx/menu.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/frame.h"
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#if wxUSE_OWNER_DRAWN
    #include "wx/ownerdrw.h"
#endif

#include "wx/msw/private.h"

#ifdef __WXWINCE__
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <ole2.h>
#include <shellapi.h>
#if (_WIN32_WCE < 400) && !defined(__HANDHELDPC__)
#include <aygshell.h>
#endif

#include "wx/msw/wince/missing.h"

#endif

// other standard headers
#include <string.h>

#if wxUSE_OWNER_DRAWN && defined(MIIM_BITMAP)
    #include "wx/dynlib.h"
#endif

#ifndef MNS_CHECKORBMP
    #define MNS_CHECKORBMP 0x04000000
#endif
#ifndef MIM_STYLE
    #define MIM_STYLE 0x00000010
#endif

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the (popup) menu title has this special id
static const int idMenuTitle = -3;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// make the given menu item default
static void SetDefaultMenuItem(HMENU WXUNUSED_IN_WINCE(hmenu),
                               UINT WXUNUSED_IN_WINCE(id))
{
#ifndef __WXWINCE__
    MENUITEMINFO mii;
    wxZeroMemory(mii);
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;
    mii.fState = MFS_DEFAULT;

    if ( !::SetMenuItemInfo(hmenu, id, FALSE, &mii) )
    {
        wxLogLastError(wxT("SetMenuItemInfo"));
    }
#endif
}

#ifdef __WXWINCE__
UINT GetMenuState(HMENU hMenu, UINT id, UINT flags)
{
    MENUITEMINFO info;
    wxZeroMemory(info);
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    // MF_BYCOMMAND is zero so test MF_BYPOSITION
    if ( !::GetMenuItemInfo(hMenu, id, flags & MF_BYPOSITION ? TRUE : FALSE , & info) )
        wxLogLastError(wxT("GetMenuItemInfo"));
    return info.fState;
}
#endif

// ============================================================================
// implementation
// ============================================================================

#include "wx/listimpl.cpp"

WX_DEFINE_LIST( wxMenuInfoList )

#if wxUSE_EXTENDED_RTTI

WX_DEFINE_FLAGS( wxMenuStyle )

wxBEGIN_FLAGS( wxMenuStyle )
    wxFLAGS_MEMBER(wxMENU_TEAROFF)
wxEND_FLAGS( wxMenuStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxMenu, wxEvtHandler,"wx/menu.h")

wxCOLLECTION_TYPE_INFO( wxMenuItem * , wxMenuItemList ) ;

template<> void wxCollectionToVariantArray( wxMenuItemList const &theList, wxxVariantArray &value)
{
    wxListCollectionToVariantArray<wxMenuItemList::compatibility_iterator>( theList , value ) ;
}

wxBEGIN_PROPERTIES_TABLE(wxMenu)
    wxEVENT_PROPERTY( Select , wxEVT_COMMAND_MENU_SELECTED , wxCommandEvent)
    wxPROPERTY( Title, wxString , SetTitle, GetTitle, wxString(), 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxREADONLY_PROPERTY_FLAGS( MenuStyle , wxMenuStyle , long , GetStyle , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
    wxPROPERTY_COLLECTION( MenuItems , wxMenuItemList , wxMenuItem* , Append , GetMenuItems , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxMenu)
wxEND_HANDLERS_TABLE()

wxDIRECT_CONSTRUCTOR_2( wxMenu , wxString , Title , long , MenuStyle  )

WX_DEFINE_FLAGS( wxMenuBarStyle )

wxBEGIN_FLAGS( wxMenuBarStyle )
    wxFLAGS_MEMBER(wxMB_DOCKABLE)
wxEND_FLAGS( wxMenuBarStyle )

// the negative id would lead the window (its superclass !) to vetoe streaming out otherwise
bool wxMenuBarStreamingCallback( const wxObject *WXUNUSED(object), wxWriter * , wxPersister * , wxxVariantArray & )
{
    return true ;
}

IMPLEMENT_DYNAMIC_CLASS_XTI_CALLBACK(wxMenuBar, wxWindow ,"wx/menu.h",wxMenuBarStreamingCallback)

IMPLEMENT_DYNAMIC_CLASS_XTI(wxMenuInfo, wxObject , "wx/menu.h" )

wxBEGIN_PROPERTIES_TABLE(wxMenuInfo)
    wxREADONLY_PROPERTY( Menu , wxMenu* , GetMenu , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxREADONLY_PROPERTY( Title , wxString , GetTitle , wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxMenuInfo)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_2( wxMenuInfo , wxMenu* , Menu , wxString , Title )

wxCOLLECTION_TYPE_INFO( wxMenuInfo * , wxMenuInfoList ) ;

template<> void wxCollectionToVariantArray( wxMenuInfoList const &theList, wxxVariantArray &value)
{
    wxListCollectionToVariantArray<wxMenuInfoList::compatibility_iterator>( theList , value ) ;
}

wxBEGIN_PROPERTIES_TABLE(wxMenuBar)
    wxPROPERTY_COLLECTION( MenuInfos , wxMenuInfoList , wxMenuInfo* , Append , GetMenuInfos , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxMenuBar)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_DUMMY( wxMenuBar )

#else
IMPLEMENT_DYNAMIC_CLASS(wxMenu, wxEvtHandler)
IMPLEMENT_DYNAMIC_CLASS(wxMenuBar, wxWindow)
IMPLEMENT_DYNAMIC_CLASS(wxMenuInfo, wxObject)
#endif

const wxMenuInfoList& wxMenuBar::GetMenuInfos() const
{
    wxMenuInfoList* list = const_cast< wxMenuInfoList* >( &m_menuInfos ) ;
    WX_CLEAR_LIST( wxMenuInfoList , *list ) ;
    for( size_t i = 0 ; i < GetMenuCount() ; ++i )
    {
        wxMenuInfo* info = new wxMenuInfo() ;
        info->Create( const_cast<wxMenuBar*>(this)->GetMenu(i) , GetLabelTop(i) ) ;
        list->Append( info ) ;
    }
    return m_menuInfos ;
}

// ---------------------------------------------------------------------------
// wxMenu construction, adding and removing menu items
// ---------------------------------------------------------------------------

// Construct a menu with optional title (then use append)
void wxMenu::Init()
{
    m_doBreak = false;
    m_startRadioGroup = -1;

    // create the menu
    m_hMenu = (WXHMENU)CreatePopupMenu();
    if ( !m_hMenu )
    {
        wxLogLastError(wxT("CreatePopupMenu"));
    }

    // if we have a title, insert it in the beginning of the menu
    if ( !m_title.empty() )
    {
        Append(idMenuTitle, m_title);
        AppendSeparator();
    }
}

// The wxWindow destructor will take care of deleting the submenus.
wxMenu::~wxMenu()
{
    // we should free Windows resources only if Windows doesn't do it for us
    // which happens if we're attached to a menubar or a submenu of another
    // menu
    if ( !IsAttached() && !GetParent() )
    {
        if ( !::DestroyMenu(GetHmenu()) )
        {
            wxLogLastError(wxT("DestroyMenu"));
        }
    }

#if wxUSE_ACCEL
    // delete accels
    WX_CLEAR_ARRAY(m_accels);
#endif // wxUSE_ACCEL
}

void wxMenu::Break()
{
    // this will take effect during the next call to Append()
    m_doBreak = true;
}

void wxMenu::Attach(wxMenuBarBase *menubar)
{
    wxMenuBase::Attach(menubar);

    EndRadioGroup();
}

#if wxUSE_ACCEL

int wxMenu::FindAccel(int id) const
{
    size_t n, count = m_accels.GetCount();
    for ( n = 0; n < count; n++ )
    {
        if ( m_accels[n]->m_command == id )
            return n;
    }

    return wxNOT_FOUND;
}

void wxMenu::UpdateAccel(wxMenuItem *item)
{
    if ( item->IsSubMenu() )
    {
        wxMenu *submenu = item->GetSubMenu();
        wxMenuItemList::compatibility_iterator node = submenu->GetMenuItems().GetFirst();
        while ( node )
        {
            UpdateAccel(node->GetData());

            node = node->GetNext();
        }
    }
    else if ( !item->IsSeparator() )
    {
        // recurse upwards: we should only modify m_accels of the top level
        // menus, not of the submenus as wxMenuBar doesn't look at them
        // (alternative and arguable cleaner solution would be to recurse
        // downwards in GetAccelCount() and CopyAccels())
        if ( GetParent() )
        {
            GetParent()->UpdateAccel(item);
            return;
        }

        // find the (new) accel for this item
        wxAcceleratorEntry *accel = wxAcceleratorEntry::Create(item->GetText());
        if ( accel )
            accel->m_command = item->GetId();

        // find the old one
        int n = FindAccel(item->GetId());
        if ( n == wxNOT_FOUND )
        {
            // no old, add new if any
            if ( accel )
                m_accels.Add(accel);
            else
                return;     // skipping RebuildAccelTable() below
        }
        else
        {
            // replace old with new or just remove the old one if no new
            delete m_accels[n];
            if ( accel )
                m_accels[n] = accel;
            else
                m_accels.RemoveAt(n);
        }

        if ( IsAttached() )
        {
            GetMenuBar()->RebuildAccelTable();
        }
    }
    //else: it is a separator, they can't have accels, nothing to do
}

#endif // wxUSE_ACCEL

// append a new item or submenu to the menu
bool wxMenu::DoInsertOrAppend(wxMenuItem *pItem, size_t pos)
{
#if wxUSE_ACCEL
    UpdateAccel(pItem);
#endif // wxUSE_ACCEL

    // we should support disabling the item even prior to adding it to the menu
    UINT flags = pItem->IsEnabled() ? MF_ENABLED : MF_GRAYED;

    // if "Break" has just been called, insert a menu break before this item
    // (and don't forget to reset the flag)
    if ( m_doBreak ) {
        flags |= MF_MENUBREAK;
        m_doBreak = false;
    }

    if ( pItem->IsSeparator() ) {
        flags |= MF_SEPARATOR;
    }

    // id is the numeric id for normal menu items and HMENU for submenus as
    // required by ::AppendMenu() API
    UINT id;
    wxMenu *submenu = pItem->GetSubMenu();
    if ( submenu != NULL ) {
        wxASSERT_MSG( submenu->GetHMenu(), wxT("invalid submenu") );

        submenu->SetParent(this);

        id = (UINT)submenu->GetHMenu();

        flags |= MF_POPUP;
    }
    else {
        id = pItem->GetId();
    }


    // prepare to insert the item in the menu
    wxString itemText = pItem->GetText();
    LPCTSTR pData = NULL;
    if ( pos == (size_t)-1 )
    {
        // append at the end (note that the item is already appended to
        // internal data structures)
        pos = GetMenuItemCount() - 1;
    }

    // adjust position to account for the title, if any
    if ( !m_title.empty() )
        pos += 2; // for the title itself and its separator

    BOOL ok = false;

#if wxUSE_OWNER_DRAWN
    // Currently, mixing owner-drawn and non-owner-drawn items results in
    // inconsistent margins, so we force this to be owner-drawn if any other
    // items already are. Later we might want to use a boolean in the wxMenu
    // to avoid search. Also we might make this fix unnecessary by getting the correct
    // margin using NONCLIENTMETRICS.
    if ( !pItem->IsOwnerDrawn() && !pItem->IsSeparator() )
    {
        // Check if any other items are ownerdrawn, and make ownerdrawn if so
        wxMenuItemList::compatibility_iterator node = GetMenuItems().GetFirst();
        while (node)
        {
            if (node->GetData()->IsOwnerDrawn())
            {
                pItem->SetOwnerDrawn(true);
                break;
            }
            node = node->GetNext();
        }
    }
#endif

    // check if we have something more than a simple text item
#if wxUSE_OWNER_DRAWN
    if ( pItem->IsOwnerDrawn() )
    {
        // is the item owner-drawn just because of the [checked] bitmap?
        if ( (pItem->GetBitmap(false).Ok() || pItem->GetBitmap(true).Ok()) &&
                !pItem->GetTextColour().Ok() &&
                    !pItem->GetBackgroundColour().Ok() &&
                        !pItem->GetFont().Ok() )
        {
            // try to use InsertMenuItem() as it's guaranteed to look correct
            // while our owner-drawn code is not

            // first compile-time check
            // MIIM_BITMAP only works under Win98/2000+
#if defined(MIIM_BITMAP) && (_WIN32_WINNT >= 0x0500)
            WinStruct<wxMENUITEMINFO_> mii;
            if ( wxGetWinVersion() >= wxWinVersion_98 )
            {
                mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
                if ( pItem->IsCheckable() )
                {
                    // need to set checked/unchecked bitmaps as otherwise our
                    // MSWOnDrawItem() item is not called
                    mii.fMask |= MIIM_CHECKMARKS;
                }

                mii.cch = itemText.length();
                mii.dwTypeData = wx_const_cast(wxChar *, itemText.c_str());

                if (flags & MF_POPUP)
                {
                    mii.fMask |= MIIM_SUBMENU;
                    mii.hSubMenu = (HMENU)pItem->GetSubMenu()->GetHMenu();
                }
                else
                {
                    mii.fMask |= MIIM_ID;
                    mii.wID = id;
                }

                // we can't pass HBITMAP directly as hbmpItem for 2 reasons:
                //  1. we can't draw it with transparency then (this is not
                //     very important now but would be with themed menu bg)
                //  2. worse, Windows inverts the bitmap for the selected
                //     item and this looks downright ugly
                //
                // so instead draw it ourselves in MSWOnDrawItem()
                mii.dwItemData = wx_reinterpret_cast(ULONG_PTR, pItem);
                if ( pItem->IsCheckable() )
                {
                    mii.hbmpChecked =
                    mii.hbmpUnchecked = HBMMENU_CALLBACK;
                }
                mii.hbmpItem = HBMMENU_CALLBACK;

                ok = ::InsertMenuItem(GetHmenu(), pos, TRUE /* by pos */,
                                      (MENUITEMINFO*)&mii);
                if ( !ok )
                {
                    wxLogLastError(wxT("InsertMenuItem()"));
                }
                else // InsertMenuItem() ok
                {
                    // we need to remove the extra indent which is reserved for
                    // the checkboxes by default as it looks ugly unless check
                    // boxes are used together with bitmaps and this is not the
                    // case in wx API
                    WinStruct<MENUINFO> mi;

                    // don't call SetMenuInfo() directly, this would prevent
                    // the app from starting up under Windows 95/NT 4
                    typedef BOOL (WINAPI *SetMenuInfo_t)(HMENU, MENUINFO *);

                    wxDynamicLibrary dllUser(_T("user32"));
                    wxDYNLIB_FUNCTION(SetMenuInfo_t, SetMenuInfo, dllUser);
                    if ( pfnSetMenuInfo )
                    {
                        mi.fMask = MIM_STYLE;
                        mi.dwStyle = MNS_CHECKORBMP;
                        if ( !(*pfnSetMenuInfo)(GetHmenu(), &mi) )
                            wxLogLastError(_T("SetMenuInfo(MNS_NOCHECK)"));
                    }

                    // tell the item that it's not really owner-drawn but only
                    // needs to draw its bitmap, the rest is done by Windows
                    pItem->ResetOwnerDrawn();
                }
            }
#endif // defined(MIIM_BITMAP) && (_WIN32_WINNT >= 0x0500)
        }

        if ( !ok )
        {
            // item draws itself, pass pointer to it in data parameter
            flags |= MF_OWNERDRAW;
            pData = (LPCTSTR)pItem;
        }
    }
    else
#endif // wxUSE_OWNER_DRAWN
    {
        // item is just a normal string (passed in data parameter)
        flags |= MF_STRING;

#ifdef __WXWINCE__
        itemText = wxMenuItem::GetLabelFromText(itemText);
#endif

        pData = (wxChar*)itemText.c_str();
    }

    // item might have already been inserted by InsertMenuItem() above
    if ( !ok )
    {
        if ( !::InsertMenu(GetHmenu(), pos, flags | MF_BYPOSITION, id, pData) )
        {
            wxLogLastError(wxT("InsertMenu[Item]()"));

            return false;
        }
    }


    // if we just appended the title, highlight it
    if ( (int)id == idMenuTitle )
    {
        // visually select the menu title
        SetDefaultMenuItem(GetHmenu(), id);
    }

    // if we're already attached to the menubar, we must update it
    if ( IsAttached() && GetMenuBar()->IsAttached() )
    {
        GetMenuBar()->Refresh();
    }

    return true;
}

void wxMenu::EndRadioGroup()
{
    // we're not inside a radio group any longer
    m_startRadioGroup = -1;
}

wxMenuItem* wxMenu::DoAppend(wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, _T("NULL item in wxMenu::DoAppend") );

    bool check = false;

    if ( item->GetKind() == wxITEM_RADIO )
    {
        int count = GetMenuItemCount();

        if ( m_startRadioGroup == -1 )
        {
            // start a new radio group
            m_startRadioGroup = count;

            // for now it has just one element
            item->SetAsRadioGroupStart();
            item->SetRadioGroupEnd(m_startRadioGroup);

            // ensure that we have a checked item in the radio group
            check = true;
        }
        else // extend the current radio group
        {
            // we need to update its end item
            item->SetRadioGroupStart(m_startRadioGroup);
            wxMenuItemList::compatibility_iterator node = GetMenuItems().Item(m_startRadioGroup);

            if ( node )
            {
                node->GetData()->SetRadioGroupEnd(count);
            }
            else
            {
                wxFAIL_MSG( _T("where is the radio group start item?") );
            }
        }
    }
    else // not a radio item
    {
        EndRadioGroup();
    }

    if ( !wxMenuBase::DoAppend(item) || !DoInsertOrAppend(item) )
    {
        return NULL;
    }

    if ( check )
    {
        // check the item initially
        item->Check(true);
    }

    return item;
}

wxMenuItem* wxMenu::DoInsert(size_t pos, wxMenuItem *item)
{
    if (wxMenuBase::DoInsert(pos, item) && DoInsertOrAppend(item, pos))
        return item;
    else
        return NULL;
}

wxMenuItem *wxMenu::DoRemove(wxMenuItem *item)
{
    // we need to find the item's position in the child list
    size_t pos;
    wxMenuItemList::compatibility_iterator node = GetMenuItems().GetFirst();
    for ( pos = 0; node; pos++ )
    {
        if ( node->GetData() == item )
            break;

        node = node->GetNext();
    }

    // DoRemove() (unlike Remove) can only be called for an existing item!
    wxCHECK_MSG( node, NULL, wxT("bug in wxMenu::Remove logic") );

#if wxUSE_ACCEL
    // remove the corresponding accel from the accel table
    int n = FindAccel(item->GetId());
    if ( n != wxNOT_FOUND )
    {
        delete m_accels[n];

        m_accels.RemoveAt(n);
    }
    //else: this item doesn't have an accel, nothing to do
#endif // wxUSE_ACCEL

    // remove the item from the menu
    if ( !::RemoveMenu(GetHmenu(), (UINT)pos, MF_BYPOSITION) )
    {
        wxLogLastError(wxT("RemoveMenu"));
    }

    if ( IsAttached() && GetMenuBar()->IsAttached() )
    {
        // otherwise, the change won't be visible
        GetMenuBar()->Refresh();
    }

    // and from internal data structures
    return wxMenuBase::DoRemove(item);
}

// ---------------------------------------------------------------------------
// accelerator helpers
// ---------------------------------------------------------------------------

#if wxUSE_ACCEL

// create the wxAcceleratorEntries for our accels and put them into the provided
// array - return the number of accels we have
size_t wxMenu::CopyAccels(wxAcceleratorEntry *accels) const
{
    size_t count = GetAccelCount();
    for ( size_t n = 0; n < count; n++ )
    {
        *accels++ = *m_accels[n];
    }

    return count;
}

#endif // wxUSE_ACCEL

// ---------------------------------------------------------------------------
// set wxMenu title
// ---------------------------------------------------------------------------

void wxMenu::SetTitle(const wxString& label)
{
    bool hasNoTitle = m_title.empty();
    m_title = label;

    HMENU hMenu = GetHmenu();

    if ( hasNoTitle )
    {
        if ( !label.empty() )
        {
            if ( !::InsertMenu(hMenu, 0u, MF_BYPOSITION | MF_STRING,
                               (unsigned)idMenuTitle, m_title) ||
                 !::InsertMenu(hMenu, 1u, MF_BYPOSITION, (unsigned)-1, NULL) )
            {
                wxLogLastError(wxT("InsertMenu"));
            }
        }
    }
    else
    {
        if ( label.empty() )
        {
            // remove the title and the separator after it
            if ( !RemoveMenu(hMenu, 0, MF_BYPOSITION) ||
                 !RemoveMenu(hMenu, 0, MF_BYPOSITION) )
            {
                wxLogLastError(wxT("RemoveMenu"));
            }
        }
        else
        {
            // modify the title
#ifdef __WXWINCE__
            MENUITEMINFO info;
            wxZeroMemory(info);
            info.cbSize = sizeof(info);
            info.fMask = MIIM_TYPE;
            info.fType = MFT_STRING;
            info.cch = m_title.length();
            info.dwTypeData = (LPTSTR) m_title.c_str();
            if ( !SetMenuItemInfo(hMenu, 0, TRUE, & info) )
            {
                wxLogLastError(wxT("SetMenuItemInfo"));
            }
#else
            if ( !ModifyMenu(hMenu, 0u,
                             MF_BYPOSITION | MF_STRING,
                             (unsigned)idMenuTitle, m_title) )
            {
                wxLogLastError(wxT("ModifyMenu"));
            }
#endif
        }
    }

#ifdef __WIN32__
    // put the title string in bold face
    if ( !m_title.empty() )
    {
        SetDefaultMenuItem(GetHmenu(), (UINT)idMenuTitle);
    }
#endif // Win32
}

// ---------------------------------------------------------------------------
// event processing
// ---------------------------------------------------------------------------

bool wxMenu::MSWCommand(WXUINT WXUNUSED(param), WXWORD id)
{
    // ignore commands from the menu title
    if ( id != (WXWORD)idMenuTitle )
    {
        // update the check item when it's clicked
        wxMenuItem * const item = FindItem(id);
        if ( item && item->IsCheckable() )
            item->Toggle();

        // get the status of the menu item: note that it has been just changed
        // by Toggle() above so here we already get the new state of the item
        UINT menuState = ::GetMenuState(GetHmenu(), id, MF_BYCOMMAND);
        SendEvent(id, menuState & MF_CHECKED);
    }

    return true;
}

// ---------------------------------------------------------------------------
// other
// ---------------------------------------------------------------------------

wxWindow *wxMenu::GetWindow() const
{
    if ( m_invokingWindow != NULL )
        return m_invokingWindow;
    else if ( GetMenuBar() != NULL)
        return GetMenuBar()->GetFrame();

    return NULL;
}

// ---------------------------------------------------------------------------
// Menu Bar
// ---------------------------------------------------------------------------

void wxMenuBar::Init()
{
    m_eventHandler = this;
    m_hMenu = 0;
#if wxUSE_TOOLBAR && defined(__WXWINCE__)
    m_toolBar = NULL;
#endif
    // Not using a combined wxToolBar/wxMenuBar? then use
    // a commandbar in WinCE .NET just to implement the
    // menubar.
#if defined(WINCE_WITH_COMMANDBAR)
    m_commandBar = NULL;
    m_adornmentsAdded = false;
#endif
}

wxMenuBar::wxMenuBar()
{
    Init();
}

wxMenuBar::wxMenuBar( long WXUNUSED(style) )
{
    Init();
}

wxMenuBar::wxMenuBar(size_t count, wxMenu *menus[], const wxString titles[], long WXUNUSED(style))
{
    Init();

    m_titles.Alloc(count);

    for ( size_t i = 0; i < count; i++ )
    {
        m_menus.Append(menus[i]);
        m_titles.Add(titles[i]);

        menus[i]->Attach(this);
    }
}

wxMenuBar::~wxMenuBar()
{
    // In Windows CE (not .NET), the menubar is always associated
    // with a toolbar, which destroys the menu implicitly.
#if defined(WINCE_WITHOUT_COMMANDBAR) && defined(__POCKETPC__)
    if (GetToolBar())
    {
        wxToolMenuBar* toolMenuBar = wxDynamicCast(GetToolBar(), wxToolMenuBar);
        if (toolMenuBar)
            toolMenuBar->SetMenuBar(NULL);
    }
#else
    // we should free Windows resources only if Windows doesn't do it for us
    // which happens if we're attached to a frame
    if (m_hMenu && !IsAttached())
    {
#if defined(WINCE_WITH_COMMANDBAR)
        ::DestroyWindow((HWND) m_commandBar);
        m_commandBar = (WXHWND) NULL;
#else
        ::DestroyMenu((HMENU)m_hMenu);
#endif
        m_hMenu = (WXHMENU)NULL;
    }
#endif
}

// ---------------------------------------------------------------------------
// wxMenuBar helpers
// ---------------------------------------------------------------------------

void wxMenuBar::Refresh()
{
    if ( IsFrozen() )
        return;

    wxCHECK_RET( IsAttached(), wxT("can't refresh unattached menubar") );

#if defined(WINCE_WITHOUT_COMMANDBAR)
    if (GetToolBar())
    {
        CommandBar_DrawMenuBar((HWND) GetToolBar()->GetHWND(), 0);
    }
#elif defined(WINCE_WITH_COMMANDBAR)
    if (m_commandBar)
        DrawMenuBar((HWND) m_commandBar);
#else
    DrawMenuBar(GetHwndOf(GetFrame()));
#endif
}

WXHMENU wxMenuBar::Create()
{
    // Note: this doesn't work at all on Smartphone,
    // since you have to use resources.
    // We'll have to find another way to add a menu
    // by changing/adding menu items to an existing menu.
#if defined(WINCE_WITHOUT_COMMANDBAR)
    if ( m_hMenu != 0 )
        return m_hMenu;

    if (!GetToolBar())
        return 0;

    HWND hCommandBar = (HWND) GetToolBar()->GetHWND();
    HMENU hMenu = (HMENU)::SendMessage(hCommandBar, SHCMBM_GETMENU, (WPARAM)0, (LPARAM)0);

    // hMenu may be zero on Windows Mobile 5. So add the menus anyway.
    if (1) // (hMenu)
    {
        TBBUTTON tbButton;
        memset(&tbButton, 0, sizeof(TBBUTTON));
        tbButton.iBitmap = I_IMAGENONE;
        tbButton.fsState = TBSTATE_ENABLED;
        tbButton.fsStyle = TBSTYLE_DROPDOWN | TBSTYLE_NO_DROPDOWN_ARROW | TBSTYLE_AUTOSIZE;

        size_t i;
        for (i = 0; i < GetMenuCount(); i++)
        {
            HMENU hPopupMenu = (HMENU) GetMenu(i)->GetHMenu() ;
            tbButton.dwData = (DWORD)hPopupMenu;
            wxString label = wxStripMenuCodes(GetLabelTop(i));
            tbButton.iString = (int) label.c_str();

            int position = i;

            tbButton.idCommand = NewControlId();
            if (!::SendMessage(hCommandBar, TB_INSERTBUTTON, position, (LPARAM)&tbButton))
            {
                wxLogLastError(wxT("TB_INSERTBUTTON"));
            }
        }
    }
    m_hMenu = (WXHMENU) hMenu;
    return m_hMenu;
#else
    if ( m_hMenu != 0 )
        return m_hMenu;

    m_hMenu = (WXHMENU)::CreateMenu();

    if ( !m_hMenu )
    {
        wxLogLastError(wxT("CreateMenu"));
    }
    else
    {
        size_t count = GetMenuCount(), i;
        wxMenuList::iterator it;
        for ( i = 0, it = m_menus.begin(); i < count; i++, it++ )
        {
            if ( !::AppendMenu((HMENU)m_hMenu, MF_POPUP | MF_STRING,
                               (UINT)(*it)->GetHMenu(),
                               m_titles[i]) )
            {
                wxLogLastError(wxT("AppendMenu"));
            }
        }
    }

    return m_hMenu;
#endif
}

int wxMenuBar::MSWPositionForWxMenu(wxMenu *menu, int wxpos)
{
    wxASSERT(menu);
    wxASSERT(menu->GetHMenu());
    wxASSERT(m_hMenu);

#if defined(__WXWINCE__)
    int totalMSWItems = GetMenuCount();
#else
    int totalMSWItems = GetMenuItemCount((HMENU)m_hMenu);
#endif

    int i; // For old C++ compatibility
    for(i=wxpos; i<totalMSWItems; i++)
    {
        if(GetSubMenu((HMENU)m_hMenu,i)==(HMENU)menu->GetHMenu())
            return i;
    }
    for(i=0; i<wxpos; i++)
    {
        if(GetSubMenu((HMENU)m_hMenu,i)==(HMENU)menu->GetHMenu())
            return i;
    }
    wxFAIL;
    return -1;
}

// ---------------------------------------------------------------------------
// wxMenuBar functions to work with the top level submenus
// ---------------------------------------------------------------------------

// NB: we don't support owner drawn top level items for now, if we do these
//     functions would have to be changed to use wxMenuItem as well

void wxMenuBar::EnableTop(size_t pos, bool enable)
{
    wxCHECK_RET( IsAttached(), wxT("doesn't work with unattached menubars") );
    wxCHECK_RET( pos < GetMenuCount(), wxT("invalid menu index") );

    int flag = enable ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem((HMENU)m_hMenu, MSWPositionForWxMenu(GetMenu(pos),pos), MF_BYPOSITION | flag);

    Refresh();
}

void wxMenuBar::SetLabelTop(size_t pos, const wxString& label)
{
    wxCHECK_RET( pos < GetMenuCount(), wxT("invalid menu index") );

    m_titles[pos] = label;

    if ( !IsAttached() )
    {
        return;
    }
    //else: have to modify the existing menu

    int mswpos = MSWPositionForWxMenu(GetMenu(pos),pos);

    UINT id;
    UINT flagsOld = ::GetMenuState((HMENU)m_hMenu, mswpos, MF_BYPOSITION);
    if ( flagsOld == 0xFFFFFFFF )
    {
        wxLogLastError(wxT("GetMenuState"));

        return;
    }

    if ( flagsOld & MF_POPUP )
    {
        // HIBYTE contains the number of items in the submenu in this case
        flagsOld &= 0xff;
        id = (UINT)::GetSubMenu((HMENU)m_hMenu, mswpos);
    }
    else
    {
        id = pos;
    }

#ifdef __WXWINCE__
    MENUITEMINFO info;
    wxZeroMemory(info);
    info.cbSize = sizeof(info);
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    info.cch = label.length();
    info.dwTypeData = (LPTSTR) label.c_str();
    if ( !SetMenuItemInfo(GetHmenu(), id, TRUE, & info) )
    {
        wxLogLastError(wxT("SetMenuItemInfo"));
    }

#else
    if ( ::ModifyMenu(GetHmenu(), mswpos, MF_BYPOSITION | MF_STRING | flagsOld,
        id, label) == (int)0xFFFFFFFF )
    {
        wxLogLastError(wxT("ModifyMenu"));
    }
#endif

    Refresh();
}

wxString wxMenuBar::GetLabelTop(size_t pos) const
{
    wxCHECK_MSG( pos < GetMenuCount(), wxEmptyString,
                 wxT("invalid menu index in wxMenuBar::GetLabelTop") );

    return wxMenuItem::GetLabelFromText(m_titles[pos]);
}

// Gets the original label at the top-level of the menubar
wxString wxMenuBar::GetMenuLabel(size_t pos) const
{
    wxCHECK_MSG( pos < GetMenuCount(), wxEmptyString,
                 wxT("invalid menu index in wxMenuBar::GetMenuLabel") );

    return m_titles[pos];
}

// ---------------------------------------------------------------------------
// wxMenuBar construction
// ---------------------------------------------------------------------------

wxMenu *wxMenuBar::Replace(size_t pos, wxMenu *menu, const wxString& title)
{
    wxMenu *menuOld = wxMenuBarBase::Replace(pos, menu, title);
    if ( !menuOld )
        return NULL;

    m_titles[pos] = title;

#if defined(WINCE_WITHOUT_COMMANDBAR)
    if (IsAttached())
#else
    if (GetHmenu())
#endif
    {
        int mswpos = MSWPositionForWxMenu(menuOld,pos);

        // can't use ModifyMenu() because it deletes the submenu it replaces
        if ( !::RemoveMenu(GetHmenu(), (UINT)mswpos, MF_BYPOSITION) )
        {
            wxLogLastError(wxT("RemoveMenu"));
        }

        if ( !::InsertMenu(GetHmenu(), (UINT)mswpos,
                           MF_BYPOSITION | MF_POPUP | MF_STRING,
                           (UINT)GetHmenuOf(menu), title) )
        {
            wxLogLastError(wxT("InsertMenu"));
        }

#if wxUSE_ACCEL
        if ( menuOld->HasAccels() || menu->HasAccels() )
        {
            // need to rebuild accell table
            RebuildAccelTable();
        }
#endif // wxUSE_ACCEL

        if (IsAttached())
            Refresh();
    }

    return menuOld;
}

bool wxMenuBar::Insert(size_t pos, wxMenu *menu, const wxString& title)
{
    // Find out which MSW item before which we'll be inserting before
    // wxMenuBarBase::Insert is called and GetMenu(pos) is the new menu.
    // If IsAttached() is false this won't be used anyway
    bool isAttached =
#if defined(WINCE_WITHOUT_COMMANDBAR)
        IsAttached();
#else
        (GetHmenu() != 0);
#endif

    int mswpos = (!isAttached || (pos == m_menus.GetCount()))
        ?   -1 // append the menu
        :   MSWPositionForWxMenu(GetMenu(pos),pos);

    if ( !wxMenuBarBase::Insert(pos, menu, title) )
        return false;

    m_titles.Insert(title, pos);

    if ( isAttached )
    {
#if defined(WINCE_WITHOUT_COMMANDBAR)
        if (!GetToolBar())
            return false;
        TBBUTTON tbButton;
        memset(&tbButton, 0, sizeof(TBBUTTON));
        tbButton.iBitmap = I_IMAGENONE;
        tbButton.fsState = TBSTATE_ENABLED;
        tbButton.fsStyle = TBSTYLE_DROPDOWN | TBSTYLE_NO_DROPDOWN_ARROW | TBSTYLE_AUTOSIZE;

        HMENU hPopupMenu = (HMENU) menu->GetHMenu() ;
        tbButton.dwData = (DWORD)hPopupMenu;
        wxString label = wxStripMenuCodes(title);
        tbButton.iString = (int) label.c_str();

        tbButton.idCommand = NewControlId();
        if (!::SendMessage((HWND) GetToolBar()->GetHWND(), TB_INSERTBUTTON, pos, (LPARAM)&tbButton))
        {
            wxLogLastError(wxT("TB_INSERTBUTTON"));
            return false;
        }
        wxUnusedVar(mswpos);
#else
        if ( !::InsertMenu(GetHmenu(), mswpos,
                           MF_BYPOSITION | MF_POPUP | MF_STRING,
                           (UINT)GetHmenuOf(menu), title) )
        {
            wxLogLastError(wxT("InsertMenu"));
        }
#endif
#if wxUSE_ACCEL
        if ( menu->HasAccels() )
        {
            // need to rebuild accell table
            RebuildAccelTable();
        }
#endif // wxUSE_ACCEL

        if (IsAttached())
            Refresh();
    }

    return true;
}

bool wxMenuBar::Append(wxMenu *menu, const wxString& title)
{
    WXHMENU submenu = menu ? menu->GetHMenu() : 0;
    wxCHECK_MSG( submenu, false, wxT("can't append invalid menu to menubar") );

    if ( !wxMenuBarBase::Append(menu, title) )
        return false;

    m_titles.Add(title);

#if defined(WINCE_WITHOUT_COMMANDBAR)
    if (IsAttached())
#else
    if (GetHmenu())
#endif
    {
#if defined(WINCE_WITHOUT_COMMANDBAR)
        if (!GetToolBar())
            return false;
        TBBUTTON tbButton;
        memset(&tbButton, 0, sizeof(TBBUTTON));
        tbButton.iBitmap = I_IMAGENONE;
        tbButton.fsState = TBSTATE_ENABLED;
        tbButton.fsStyle = TBSTYLE_DROPDOWN | TBSTYLE_NO_DROPDOWN_ARROW | TBSTYLE_AUTOSIZE;

        size_t pos = GetMenuCount();
        HMENU hPopupMenu = (HMENU) menu->GetHMenu() ;
        tbButton.dwData = (DWORD)hPopupMenu;
        wxString label = wxStripMenuCodes(title);
        tbButton.iString = (int) label.c_str();

        tbButton.idCommand = NewControlId();
        if (!::SendMessage((HWND) GetToolBar()->GetHWND(), TB_INSERTBUTTON, pos, (LPARAM)&tbButton))
        {
            wxLogLastError(wxT("TB_INSERTBUTTON"));
            return false;
        }
#else
        if ( !::AppendMenu(GetHmenu(), MF_POPUP | MF_STRING,
                           (UINT)submenu, title) )
        {
            wxLogLastError(wxT("AppendMenu"));
        }
#endif

#if wxUSE_ACCEL
        if ( menu->HasAccels() )
        {
            // need to rebuild accelerator table
            RebuildAccelTable();
        }
#endif // wxUSE_ACCEL

        if (IsAttached())
            Refresh();
    }

    return true;
}

wxMenu *wxMenuBar::Remove(size_t pos)
{
    wxMenu *menu = wxMenuBarBase::Remove(pos);
    if ( !menu )
        return NULL;

#if defined(WINCE_WITHOUT_COMMANDBAR)
    if (IsAttached())
#else
    if (GetHmenu())
#endif
    {
#if defined(WINCE_WITHOUT_COMMANDBAR)
        if (GetToolBar())
        {
            if (!::SendMessage((HWND) GetToolBar()->GetHWND(), TB_DELETEBUTTON, (UINT) pos, (LPARAM) 0))
            {
                wxLogLastError(wxT("TB_DELETEBUTTON"));
            }
        }
#else
        if ( !::RemoveMenu(GetHmenu(), (UINT)MSWPositionForWxMenu(menu,pos), MF_BYPOSITION) )
        {
            wxLogLastError(wxT("RemoveMenu"));
        }
#endif

#if wxUSE_ACCEL
        if ( menu->HasAccels() )
        {
            // need to rebuild accell table
            RebuildAccelTable();
        }
#endif // wxUSE_ACCEL

        if (IsAttached())
            Refresh();
    }

    m_titles.RemoveAt(pos);

    return menu;
}

#if wxUSE_ACCEL

void wxMenuBar::RebuildAccelTable()
{
    // merge the accelerators of all menus into one accel table
    size_t nAccelCount = 0;
    size_t i, count = GetMenuCount();
    wxMenuList::iterator it;
    for ( i = 0, it = m_menus.begin(); i < count; i++, it++ )
    {
        nAccelCount += (*it)->GetAccelCount();
    }

    if ( nAccelCount )
    {
        wxAcceleratorEntry *accelEntries = new wxAcceleratorEntry[nAccelCount];

        nAccelCount = 0;
        for ( i = 0, it = m_menus.begin(); i < count; i++, it++ )
        {
            nAccelCount += (*it)->CopyAccels(&accelEntries[nAccelCount]);
        }

        m_accelTable = wxAcceleratorTable(nAccelCount, accelEntries);

        delete [] accelEntries;
    }
}

#endif // wxUSE_ACCEL

void wxMenuBar::Attach(wxFrame *frame)
{
    wxMenuBarBase::Attach(frame);

#if defined(WINCE_WITH_COMMANDBAR)
    if (!m_hMenu)
        this->Create();
    if (!m_commandBar)
        m_commandBar = (WXHWND) CommandBar_Create(wxGetInstance(), (HWND) frame->GetHWND(), NewControlId());
    if (m_commandBar)
    {
        if (m_hMenu)
        {
            if (!CommandBar_InsertMenubarEx((HWND) m_commandBar, NULL, (LPTSTR) m_hMenu, 0))
            {
                wxLogLastError(wxT("CommandBar_InsertMenubarEx"));
            }
        }
    }
#endif

#if wxUSE_ACCEL
    RebuildAccelTable();
#endif // wxUSE_ACCEL
}

#if defined(WINCE_WITH_COMMANDBAR)
bool wxMenuBar::AddAdornments(long style)
{
    if (m_adornmentsAdded || !m_commandBar)
        return false;

    if (style & wxCLOSE_BOX)
    {
        if (!CommandBar_AddAdornments((HWND) m_commandBar, 0, 0))
            wxLogLastError(wxT("CommandBar_AddAdornments"));
        else
            return true;
    }
    return false;
}
#endif

void wxMenuBar::Detach()
{
    wxMenuBarBase::Detach();
}

#endif // wxUSE_MENUS
