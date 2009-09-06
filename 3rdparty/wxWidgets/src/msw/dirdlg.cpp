/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/dirdlg.cpp
// Purpose:     wxDirDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dirdlg.cpp 39613 2006-06-07 11:44:19Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_DIRDLG

#if wxUSE_OLE && !defined(__GNUWIN32_OLD__) && (!defined(__WXWINCE__) || \
    (defined(__HANDHELDPC__) && (_WIN32_WCE >= 500)))

#include "wx/dirdlg.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/log.h"
    #include "wx/app.h"     // for GetComCtl32Version()
#endif

#include "wx/msw/private.h"
#include "wx/msw/wrapshl.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#ifndef BIF_NEWDIALOGSTYLE
    #define BIF_NEWDIALOGSTYLE 0x0040
#endif

#ifndef BIF_NONEWFOLDERBUTTON
    #define BIF_NONEWFOLDERBUTTON  0x0200
#endif

#ifndef BIF_EDITBOX
    #define BIF_EDITBOX 16
#endif

// ----------------------------------------------------------------------------
// wxWidgets macros
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxDirDialog, wxDialog)

// ----------------------------------------------------------------------------
// private functions prototypes
// ----------------------------------------------------------------------------

// the callback proc for the dir dlg
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp,
                                       LPARAM pData);


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDirDialog
// ----------------------------------------------------------------------------

wxDirDialog::wxDirDialog(wxWindow *parent,
                         const wxString& message,
                         const wxString& defaultPath,
                         long style,
                         const wxPoint& WXUNUSED(pos),
                         const wxSize& WXUNUSED(size),
                         const wxString& WXUNUSED(name))
{
    m_message = message;
    m_parent = parent;

    SetWindowStyle(style);
    SetPath(defaultPath);
}

void wxDirDialog::SetPath(const wxString& path)
{
    m_path = path;

    // SHBrowseForFolder doesn't like '/'s nor the trailing backslashes
    m_path.Replace(_T("/"), _T("\\"));
    if ( !m_path.empty() )
    {
        while ( *(m_path.end() - 1) == _T('\\') )
        {
            m_path.erase(m_path.length() - 1);
        }

        // but the root drive should have a trailing slash (again, this is just
        // the way the native dialog works)
        if ( *(m_path.end() - 1) == _T(':') )
        {
            m_path += _T('\\');
        }
    }
}

int wxDirDialog::ShowModal()
{
    wxWindow *parent = GetParent();

    BROWSEINFO bi;
    bi.hwndOwner      = parent ? GetHwndOf(parent) : NULL;
    bi.pidlRoot       = NULL;
    bi.pszDisplayName = NULL;
    // Please don't change this without checking it compiles
    // with eVC++ first.
#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
    bi.lpszTitle      = m_message.mb_str();
#else
    bi.lpszTitle      = m_message.c_str();
#endif
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
    bi.lpfn           = BrowseCallbackProc;
    bi.lParam         = (LPARAM)m_path.c_str();    // param for the callback

    static const int verComCtl32 = wxApp::GetComCtl32Version();

    // we always add the edit box (it doesn't hurt anybody, does it?) if it is
    // supported by the system
    if ( verComCtl32 >= 471 )
    {
        bi.ulFlags |= BIF_EDITBOX;
    }

    // to have the "New Folder" button we must use the "new" dialog style which
    // is also the only way to have a resizable dialog
    //
    // "new" style is only available in the version 5.0+ of comctl32.dll
    const bool needNewDir = !HasFlag(wxDD_DIR_MUST_EXIST);
    if ( (needNewDir || HasFlag(wxRESIZE_BORDER)) && (verComCtl32 >= 500) )
    {
        if (needNewDir)
        {
            bi.ulFlags |= BIF_NEWDIALOGSTYLE;
        }
        else
        {
            // Versions < 600 doesn't support BIF_NONEWFOLDERBUTTON
            // The only way to get rid of the Make New Folder button is use
            // the old dialog style which doesn't have the button thus we
            // simply don't set the New Dialog Style for such comctl versions.
            if (verComCtl32 >= 600)
            {
                bi.ulFlags |= BIF_NEWDIALOGSTYLE;
                bi.ulFlags |= BIF_NONEWFOLDERBUTTON;
            }
        }
    }

    // do show the dialog
    wxItemIdList pidl(SHBrowseForFolder(&bi));

    wxItemIdList::Free((LPITEMIDLIST)bi.pidlRoot);

    if ( !pidl )
    {
        // Cancel button pressed
        return wxID_CANCEL;
    }

    m_path = pidl.GetPath();

    // change current working directory if asked so
    if (HasFlag(wxDD_CHANGE_DIR))
        wxSetWorkingDirectory(m_path);

    return m_path.empty() ? wxID_CANCEL : wxID_OK;
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static int CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    switch(uMsg)
    {
#ifdef BFFM_SETSELECTION
        case BFFM_INITIALIZED:
            // sent immediately after initialisation and so we may set the
            // initial selection here
            //
            // wParam = TRUE => lParam is a string and not a PIDL
            ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
            break;
#endif // BFFM_SETSELECTION


        case BFFM_SELCHANGED:
            // note that this doesn't work with the new style UI (MSDN doesn't
            // say anything about it, but the comments in shlobj.h do!) but we
            // still execute this code in case it starts working again with the
            // "new new UI" (or would it be "NewUIEx" according to tradition?)
            {
                // Set the status window to the currently selected path.
                wxString strDir;
                if ( SHGetPathFromIDList((LPITEMIDLIST)lp,
                                         wxStringBuffer(strDir, MAX_PATH)) )
                {
                    // NB: this shouldn't be necessary with the new style box
                    //     (which is resizable), but as for now it doesn't work
                    //     anyhow (see the comment above) no harm in doing it

                    // need to truncate or it displays incorrectly
                    static const size_t maxChars = 37;
                    if ( strDir.length() > maxChars )
                    {
                        strDir = strDir.Right(maxChars);
                        strDir = wxString(wxT("...")) + strDir;
                    }

                    SendMessage(hwnd, BFFM_SETSTATUSTEXT,
                                0, (LPARAM)strDir.c_str());
                }
            }
            break;

        //case BFFM_VALIDATEFAILED: -- might be used to provide custom message
        //                             if the user types in invalid dir name
    }

    return 0;
}

#endif // compiler/platform on which the code here compiles

#endif // wxUSE_DIRDLG
