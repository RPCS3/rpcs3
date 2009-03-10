/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/filedlg.cpp
// Purpose:     wxFileDialog
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: filedlg.cpp 43833 2006-12-06 17:17:37Z VZ $
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

#if wxUSE_FILEDLG && !(defined(__SMARTPHONE__) && defined(__WXWINCE__))

#include "wx/filedlg.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcdlg.h"
    #include "wx/msw/missing.h"
    #include "wx/utils.h"
    #include "wx/msgdlg.h"
    #include "wx/filefn.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/math.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "wx/filename.h"
#include "wx/tokenzr.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#ifdef __WIN32__
# define wxMAXPATH   65534
#else
# define wxMAXPATH   1024
#endif

# define wxMAXFILE   1024

# define wxMAXEXT    5

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// standard dialog size
static wxRect gs_rectDialog(0, 0, 428, 266);

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_CLASS(wxFileDialog, wxFileDialogBase)

// ----------------------------------------------------------------------------
// hook function for moving the dialog
// ----------------------------------------------------------------------------

UINT_PTR APIENTRY
wxFileDialogHookFunction(HWND      hDlg,
                         UINT      iMsg,
                         WPARAM    WXUNUSED(wParam),
                         LPARAM    lParam)
{
    switch ( iMsg )
    {
        case WM_NOTIFY:
            {
                OFNOTIFY *pNotifyCode = wx_reinterpret_cast(OFNOTIFY *, lParam);
                if ( pNotifyCode->hdr.code == CDN_INITDONE )
                {
                    // note that we need to move the parent window: hDlg is a
                    // child of it when OFN_EXPLORER is used
                    ::SetWindowPos
                      (
                        ::GetParent(hDlg),
                        HWND_TOP,
                        gs_rectDialog.x, gs_rectDialog.y,
                        0, 0,
                        SWP_NOZORDER | SWP_NOSIZE
                      );
                 }
            }
            break;

        case WM_DESTROY:
            // reuse the position used for the dialog the next time by default
            //
            // NB: at least under Windows 2003 this is useless as after the
            //     first time it's shown the dialog always remembers its size
            //     and position itself and ignores any later SetWindowPos calls
            wxCopyRECTToRect(wxGetWindowRect(::GetParent(hDlg)), gs_rectDialog);
            break;
    }

    // do the default processing
    return 0;
}

// ----------------------------------------------------------------------------
// wxFileDialog
// ----------------------------------------------------------------------------

wxFileDialog::wxFileDialog(wxWindow *parent,
                           const wxString& message,
                           const wxString& defaultDir,
                           const wxString& defaultFileName,
                           const wxString& wildCard,
                           long style,
                           const wxPoint& pos,
                           const wxSize& sz,
                           const wxString& name)
            : wxFileDialogBase(parent, message, defaultDir, defaultFileName,
                               wildCard, style, pos, sz, name)

{
    // NB: all style checks are done by wxFileDialogBase::Create

    m_bMovedWindow = false;

    // Must set to zero, otherwise the wx routines won't size the window
    // the second time you call the file dialog, because it thinks it is
    // already at the requested size.. (when centering)
    gs_rectDialog.x =
    gs_rectDialog.y = 0;

}
void wxFileDialog::GetPaths(wxArrayString& paths) const
{
    paths.Empty();

    wxString dir(m_dir);
    if ( m_dir.Last() != _T('\\') )
        dir += _T('\\');

    size_t count = m_fileNames.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        if (wxFileName(m_fileNames[n]).IsAbsolute())
            paths.Add(m_fileNames[n]);
        else
            paths.Add(dir + m_fileNames[n]);
    }
}

void wxFileDialog::GetFilenames(wxArrayString& files) const
{
    files = m_fileNames;
}

void wxFileDialog::SetPath(const wxString& path)
{
    wxString ext;
    wxSplitPath(path, &m_dir, &m_fileName, &ext);
    if ( !ext.empty() )
        m_fileName << _T('.') << ext;
}

void wxFileDialog::DoGetPosition(int *x, int *y) const
{
    if ( x )
        *x = gs_rectDialog.x;
    if ( y )
        *y = gs_rectDialog.y;
}


void wxFileDialog::DoGetSize(int *width, int *height) const
{
    if ( width )
        *width = gs_rectDialog.width;
    if ( height )
        *height = gs_rectDialog.height;
}

void wxFileDialog::DoMoveWindow(int x, int y, int WXUNUSED(w), int WXUNUSED(h))
{
    m_bMovedWindow = true;

    gs_rectDialog.x = x;
    gs_rectDialog.y = y;

    // size of the dialog can't be changed because the controls are not laid
    // out correctly then
}

// helper used below in ShowModal(): style is used to determine whether to show
// the "Save file" dialog (if it contains wxFD_SAVE bit) or "Open file" one;
// returns true on success or false on failure in which case err is filled with
// the CDERR_XXX constant
static bool DoShowCommFileDialog(OPENFILENAME *of, long style, DWORD *err)
{
    if ( style & wxFD_SAVE ? GetSaveFileName(of) : GetOpenFileName(of) )
        return true;

    if ( err )
    {
#ifdef __WXWINCE__
        // according to MSDN, CommDlgExtendedError() should work under CE as
        // well but apparently in practice it doesn't (anybody has more
        // details?)
        *err = GetLastError();
#else
        *err = CommDlgExtendedError();
#endif
    }

    return false;
}

// We want to use OPENFILENAME struct version 5 (Windows 2000/XP) but we don't
// know if the OPENFILENAME declared in the currently used headers is a V5 or
// V4 (smaller) one so we try to manually extend the struct in case it is the
// old one.
//
// We don't do this on Windows CE nor under Win64, however, as there are no
// compilers with old headers for these architectures
#if defined(__WXWINCE__) || defined(__WIN64__)
    typedef OPENFILENAME wxOPENFILENAME;

    static const DWORD gs_ofStructSize = sizeof(OPENFILENAME);
#else // !__WXWINCE__ || __WIN64__
    #define wxTRY_SMALLER_OPENFILENAME

    struct wxOPENFILENAME : public OPENFILENAME
    {
        // fields added in Windows 2000/XP comdlg32.dll version
        void *pVoid;
        DWORD dw1;
        DWORD dw2;
    };

    // hardcoded sizeof(OPENFILENAME) in the Platform SDK: we have to do it
    // because sizeof(OPENFILENAME) in the headers we use when compiling the
    // library could be less if _WIN32_WINNT is not >= 0x500
    static const DWORD wxOPENFILENAME_V5_SIZE = 88;

    // this is hardcoded sizeof(OPENFILENAME_NT4) from Platform SDK
    static const DWORD wxOPENFILENAME_V4_SIZE = 76;

    // always try the new one first
    static DWORD gs_ofStructSize = wxOPENFILENAME_V5_SIZE;
#endif // __WXWINCE__ || __WIN64__/!...

int wxFileDialog::ShowModal()
{
    HWND hWnd = 0;
    if (m_parent) hWnd = (HWND) m_parent->GetHWND();
    if (!hWnd && wxTheApp->GetTopWindow())
        hWnd = (HWND) wxTheApp->GetTopWindow()->GetHWND();

    static wxChar fileNameBuffer [ wxMAXPATH ];           // the file-name
    wxChar        titleBuffer    [ wxMAXFILE+1+wxMAXEXT ];  // the file-name, without path

    *fileNameBuffer = wxT('\0');
    *titleBuffer    = wxT('\0');

#if WXWIN_COMPATIBILITY_2_4
    long msw_flags = 0;
    if ( HasFdFlag(wxHIDE_READONLY) || HasFdFlag(wxFD_SAVE) )
        msw_flags |= OFN_HIDEREADONLY;
#else
    long msw_flags = OFN_HIDEREADONLY;
#endif

    if ( HasFdFlag(wxFD_FILE_MUST_EXIST) )
        msw_flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    /*
        If the window has been moved the programmer is probably
        trying to center or position it.  Thus we set the callback
        or hook function so that we can actually adjust the position.
        Without moving or centering the dlg, it will just stay
        in the upper left of the frame, it does not center
        automatically.
    */
    if (m_bMovedWindow) // we need these flags.
    {
        msw_flags |= OFN_EXPLORER|OFN_ENABLEHOOK;
#ifndef __WXWINCE__
        msw_flags |= OFN_ENABLESIZING;
#endif
    }

    if ( HasFdFlag(wxFD_MULTIPLE) )
    {
        // OFN_EXPLORER must always be specified with OFN_ALLOWMULTISELECT
        msw_flags |= OFN_EXPLORER | OFN_ALLOWMULTISELECT;
    }

    // if wxFD_CHANGE_DIR flag is not given we shouldn't change the CWD which the
    // standard dialog does by default (notice that under NT it does it anyhow,
    // OFN_NOCHANGEDIR or not, see below)
    if ( !HasFdFlag(wxFD_CHANGE_DIR) )
    {
        msw_flags |= OFN_NOCHANGEDIR;
    }

    if ( HasFdFlag(wxFD_OVERWRITE_PROMPT) )
    {
        msw_flags |= OFN_OVERWRITEPROMPT;
    }

    wxOPENFILENAME of;
    wxZeroMemory(of);

    of.lStructSize       = gs_ofStructSize;
    of.hwndOwner         = hWnd;
    of.lpstrTitle        = WXSTRINGCAST m_message;
    of.lpstrFileTitle    = titleBuffer;
    of.nMaxFileTitle     = wxMAXFILE + 1 + wxMAXEXT;

    // Convert forward slashes to backslashes (file selector doesn't like
    // forward slashes) and also squeeze multiple consecutive slashes into one
    // as it doesn't like two backslashes in a row neither

    wxString  dir;
    size_t    i, len = m_dir.length();
    dir.reserve(len);
    for ( i = 0; i < len; i++ )
    {
        wxChar ch = m_dir[i];
        switch ( ch )
        {
            case _T('/'):
                // convert to backslash
                ch = _T('\\');

                // fall through

            case _T('\\'):
                while ( i < len - 1 )
                {
                    wxChar chNext = m_dir[i + 1];
                    if ( chNext != _T('\\') && chNext != _T('/') )
                        break;

                    // ignore the next one, unless it is at the start of a UNC path
                    if (i > 0)
                        i++;
                    else
                        break;
                }
                // fall through

            default:
                // normal char
                dir += ch;
        }
    }

    of.lpstrInitialDir   = dir.c_str();

    of.Flags             = msw_flags;
    of.lpfnHook          = wxFileDialogHookFunction;

    wxArrayString wildDescriptions, wildFilters;

    size_t items = wxParseCommonDialogsFilter(m_wildCard, wildDescriptions, wildFilters);

    wxASSERT_MSG( items > 0 , _T("empty wildcard list") );

    wxString filterBuffer;

    for (i = 0; i < items ; i++)
    {
        filterBuffer += wildDescriptions[i];
        filterBuffer += wxT("|");
        filterBuffer += wildFilters[i];
        filterBuffer += wxT("|");
    }

    // Replace | with \0
    for (i = 0; i < filterBuffer.length(); i++ ) {
        if ( filterBuffer.GetChar(i) == wxT('|') ) {
            filterBuffer[i] = wxT('\0');
        }
    }

    of.lpstrFilter  = (LPTSTR)filterBuffer.c_str();
    of.nFilterIndex = m_filterIndex + 1;

    //=== Setting defaultFileName >>=========================================

    wxStrncpy( fileNameBuffer, (const wxChar *)m_fileName, wxMAXPATH-1 );
    fileNameBuffer[ wxMAXPATH-1 ] = wxT('\0');

    of.lpstrFile = fileNameBuffer;  // holds returned filename
    of.nMaxFile  = wxMAXPATH;

    // we must set the default extension because otherwise Windows would check
    // for the existing of a wrong file with wxFD_OVERWRITE_PROMPT (i.e. if the
    // user types "foo" and the default extension is ".bar" we should force it
    // to check for "foo.bar" existence and not "foo")
    wxString defextBuffer; // we need it to be alive until GetSaveFileName()!
    if (HasFdFlag(wxFD_SAVE))
    {
        const wxChar* extension = filterBuffer;
        int maxFilter = (int)(of.nFilterIndex*2L) - 1;

        for( int i = 0; i < maxFilter; i++ )           // get extension
            extension = extension + wxStrlen( extension ) + 1;

        // use dummy name a to avoid assert in AppendExtension
        defextBuffer = AppendExtension(wxT("a"), extension);
        if (defextBuffer.StartsWith(wxT("a.")))
        {
            defextBuffer = defextBuffer.Mid(2); // remove "a."
            of.lpstrDefExt = defextBuffer.c_str();
        }
    }

    // store off before the standard windows dialog can possibly change it
    const wxString cwdOrig = wxGetCwd();

    //== Execute FileDialog >>=================================================

    DWORD errCode;
    bool success = DoShowCommFileDialog(&of, m_windowStyle, &errCode);

#ifdef wxTRY_SMALLER_OPENFILENAME
    // the system might be too old to support the new version file dialog
    // boxes, try with the old size
    if ( !success && errCode == CDERR_STRUCTSIZE &&
            of.lStructSize != wxOPENFILENAME_V4_SIZE )
    {
        of.lStructSize = wxOPENFILENAME_V4_SIZE;

        success = DoShowCommFileDialog(&of, m_windowStyle, &errCode);

        if ( success || !errCode )
        {
            // use this struct size for subsequent dialogs
            gs_ofStructSize = of.lStructSize;
        }
    }
#endif // wxTRY_SMALLER_OPENFILENAME

    if ( success )
    {
        // GetOpenFileName will always change the current working directory on
        // (according to MSDN) "Windows NT 4.0/2000/XP" because the flag
        // OFN_NOCHANGEDIR has no effect.  If the user did not specify
        // wxFD_CHANGE_DIR let's restore the current working directory to what it
        // was before the dialog was shown.
        if ( msw_flags & OFN_NOCHANGEDIR )
        {
            wxSetWorkingDirectory(cwdOrig);
        }

        m_fileNames.Empty();

        if ( ( HasFdFlag(wxFD_MULTIPLE) ) &&
#if defined(OFN_EXPLORER)
             ( fileNameBuffer[of.nFileOffset-1] == wxT('\0') )
#else
             ( fileNameBuffer[of.nFileOffset-1] == wxT(' ') )
#endif // OFN_EXPLORER
           )
        {
#if defined(OFN_EXPLORER)
            m_dir = fileNameBuffer;
            i = of.nFileOffset;
            m_fileName = &fileNameBuffer[i];
            m_fileNames.Add(m_fileName);
            i += m_fileName.length() + 1;

            while (fileNameBuffer[i] != wxT('\0'))
            {
                m_fileNames.Add(&fileNameBuffer[i]);
                i += wxStrlen(&fileNameBuffer[i]) + 1;
            }
#else
            wxStringTokenizer toke(fileNameBuffer, _T(" \t\r\n"));
            m_dir = toke.GetNextToken();
            m_fileName = toke.GetNextToken();
            m_fileNames.Add(m_fileName);

            while (toke.HasMoreTokens())
                m_fileNames.Add(toke.GetNextToken());
#endif // OFN_EXPLORER

            wxString dir(m_dir);
            if ( m_dir.Last() != _T('\\') )
                dir += _T('\\');

            m_path = dir + m_fileName;
            m_filterIndex = (int)of.nFilterIndex - 1;
        }
        else
        {
            //=== Adding the correct extension >>=================================

            m_filterIndex = (int)of.nFilterIndex - 1;

            if ( !of.nFileExtension ||
                 (of.nFileExtension && fileNameBuffer[of.nFileExtension] == wxT('\0')) )
            {
                // User has typed a filename without an extension:
                const wxChar* extension = filterBuffer;
                int   maxFilter = (int)(of.nFilterIndex*2L) - 1;

                for( int i = 0; i < maxFilter; i++ )           // get extension
                    extension = extension + wxStrlen( extension ) + 1;

                m_fileName = AppendExtension(fileNameBuffer, extension);
                wxStrncpy(fileNameBuffer, m_fileName.c_str(), wxMin(m_fileName.length(), wxMAXPATH-1));
                fileNameBuffer[wxMin(m_fileName.length(), wxMAXPATH-1)] = wxT('\0');
            }

            m_path = fileNameBuffer;
            m_fileName = wxFileNameFromPath(fileNameBuffer);
            m_fileNames.Add(m_fileName);
            m_dir = wxPathOnly(fileNameBuffer);
        }
    }
#ifdef __WXDEBUG__
    else
    {
        // common dialog failed - why?
        if ( errCode != 0 )
        {
            // this msg is only for developers so don't translate it
            wxLogError(wxT("Common dialog failed with error code %0lx."),
                       errCode);
        }
        //else: it was just cancelled
    }
#endif // __WXDEBUG__

    return success ? wxID_OK : wxID_CANCEL;

}

#endif // wxUSE_FILEDLG && !(__SMARTPHONE__ && __WXWINCE__)
