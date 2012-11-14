/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/fldlgcmn.cpp
// Purpose:     wxFileDialog common functions
// Author:      John Labenski
// Modified by:
// Created:     14.06.03 (extracted from src/*/filedlg.cpp)
// RCS-ID:      $Id: fldlgcmn.cpp 66916 2011-02-16 21:48:55Z JS $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_FILEDLG

#include "wx/filedlg.h"
#include "wx/dirdlg.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/window.h"
#endif // WX_PRECOMP

//----------------------------------------------------------------------------
// wxFileDialogBase
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxFileDialogBase, wxDialog)

void wxFileDialogBase::Init()
{
    m_filterIndex =
    m_windowStyle = 0;
}

bool wxFileDialogBase::Create(wxWindow *parent,
                              const wxString& message,
                              const wxString& defaultDir,
                              const wxString& defaultFile,
                              const wxString& wildCard,
                              long style,
                              const wxPoint& WXUNUSED(pos),
                              const wxSize& WXUNUSED(sz),
                              const wxString& WXUNUSED(name))
{
    m_message = message;
    m_dir = defaultDir;
    m_fileName = defaultFile;
    m_wildCard = wildCard;

    m_parent = parent;
    m_windowStyle = style;
    m_filterIndex = 0;

    if (!HasFdFlag(wxFD_OPEN) && !HasFdFlag(wxFD_SAVE))
        m_windowStyle |= wxFD_OPEN;     // wxFD_OPEN is the default

    // check that the styles are not contradictory
    wxASSERT_MSG( !(HasFdFlag(wxFD_SAVE) && HasFdFlag(wxFD_OPEN)),
                  _T("can't specify both wxFD_SAVE and wxFD_OPEN at once") );

    wxASSERT_MSG( !HasFdFlag(wxFD_SAVE) ||
                    (!HasFdFlag(wxFD_MULTIPLE) && !HasFdFlag(wxFD_FILE_MUST_EXIST)),
                   _T("wxFD_MULTIPLE or wxFD_FILE_MUST_EXIST can't be used with wxFD_SAVE" ) );

    wxASSERT_MSG( !HasFdFlag(wxFD_OPEN) || !HasFdFlag(wxFD_OVERWRITE_PROMPT),
                  _T("wxFD_OVERWRITE_PROMPT can't be used with wxFD_OPEN") );

    if ( wildCard.empty() || wildCard == wxFileSelectorDefaultWildcardStr )
    {
        m_wildCard = wxString::Format(_("All files (%s)|%s"),
                                      wxFileSelectorDefaultWildcardStr,
                                      wxFileSelectorDefaultWildcardStr);
    }
    else // have wild card
    {
        // convert m_wildCard from "*.bar" to "bar files (*.bar)|*.bar"
        if ( m_wildCard.Find(wxT('|')) == wxNOT_FOUND )
        {
            wxString::size_type nDot = m_wildCard.find(_T("*."));
            if ( nDot != wxString::npos )
                nDot++;
            else
                nDot = 0;

            m_wildCard = wxString::Format
                         (
                            _("%s files (%s)|%s"),
                            wildCard.c_str() + nDot,
                            wildCard.c_str(),
                            wildCard.c_str()
                         );
        }
    }

    return true;
}

#if WXWIN_COMPATIBILITY_2_4
// Parses the filterStr, returning the number of filters.
// Returns 0 if none or if there's a problem.
// filterStr is in the form: "All files (*.*)|*.*|JPEG Files (*.jpeg)|*.jpg"
int wxFileDialogBase::ParseWildcard(const wxString& filterStr,
                                    wxArrayString& descriptions,
                                    wxArrayString& filters)
{
    return ::wxParseCommonDialogsFilter(filterStr, descriptions, filters);
}
#endif // WXWIN_COMPATIBILITY_2_4

#if WXWIN_COMPATIBILITY_2_6
long wxFileDialogBase::GetStyle() const
{
    return GetWindowStyle();
}

void wxFileDialogBase::SetStyle(long style)
{
    SetWindowStyle(style);
}
#endif // WXWIN_COMPATIBILITY_2_6


wxString wxFileDialogBase::AppendExtension(const wxString &filePath,
                                           const wxString &extensionList)
{
    // strip off path, to avoid problems with "path.bar/foo"
    wxString fileName = filePath.AfterLast(wxFILE_SEP_PATH);

    // if fileName is of form "foo.bar" it's ok, return it
    int idx_dot = fileName.Find(wxT('.'), true);
    if ((idx_dot != wxNOT_FOUND) && (idx_dot < (int)fileName.length() - 1))
        return filePath;

    // get the first extension from extensionList, or all of it
    wxString ext = extensionList.BeforeFirst(wxT(';'));

    // if ext == "foo" or "foo." there's no extension
    int idx_ext_dot = ext.Find(wxT('.'), true);
    if ((idx_ext_dot == wxNOT_FOUND) || (idx_ext_dot == (int)ext.length() - 1))
        return filePath;
    else
        ext = ext.AfterLast(wxT('.'));

    // if ext == "*" or "bar*" or "b?r" or " " then its not valid
    if ((ext.Find(wxT('*')) != wxNOT_FOUND) ||
        (ext.Find(wxT('?')) != wxNOT_FOUND) ||
        (ext.Strip(wxString::both).empty()))
        return filePath;

    // if fileName doesn't have a '.' then add one
    if (filePath.Last() != wxT('.'))
        ext = wxT(".") + ext;

    return filePath + ext;
}

//----------------------------------------------------------------------------
// wxFileDialog convenience functions
//----------------------------------------------------------------------------

wxString wxFileSelector(const wxChar *title,
                               const wxChar *defaultDir,
                               const wxChar *defaultFileName,
                               const wxChar *defaultExtension,
                               const wxChar *filter,
                               int flags,
                               wxWindow *parent,
                               int x, int y)
{
    // The defaultExtension, if non-NULL, is
    // appended to the filename if the user fails to type an extension. The new
    // implementation (taken from wxFileSelectorEx) appends the extension
    // automatically, by looking at the filter specification. In fact this
    // should be better than the native Microsoft implementation because
    // Windows only allows *one* default extension, whereas here we do the
    // right thing depending on the filter the user has chosen.

    // If there's a default extension specified but no filter, we create a
    // suitable filter.

    wxString filter2;
    if ( !wxIsEmpty(defaultExtension) && wxIsEmpty(filter) )
        filter2 = wxString(wxT("*.")) + defaultExtension;
    else if ( !wxIsEmpty(filter) )
        filter2 = filter;

    wxString defaultDirString;
    if (!wxIsEmpty(defaultDir))
        defaultDirString = defaultDir;

    wxString defaultFilenameString;
    if (!wxIsEmpty(defaultFileName))
        defaultFilenameString = defaultFileName;

    wxFileDialog fileDialog(parent, title, defaultDirString,
                            defaultFilenameString, filter2,
                            flags, wxPoint(x, y));

   // if filter is of form "All files (*)|*|..." set correct filter index
    if((wxStrlen(defaultExtension) != 0) && (filter2.Find(wxT('|')) != wxNOT_FOUND))
    {
        int filterIndex = 0;

        wxArrayString descriptions, filters;
        // don't care about errors, handled already by wxFileDialog
        (void)wxParseCommonDialogsFilter(filter2, descriptions, filters);
        for (size_t n=0; n<filters.GetCount(); n++)
        {
            if (filters[n].Contains(defaultExtension))
            {
                filterIndex = n;
                        break;
            }
        }

        if (filterIndex > 0)
            fileDialog.SetFilterIndex(filterIndex);
    }

    wxString filename;
    if ( fileDialog.ShowModal() == wxID_OK )
    {
        filename = fileDialog.GetPath();
    }

    return filename;
}

//----------------------------------------------------------------------------
// wxFileSelectorEx
//----------------------------------------------------------------------------

wxString wxFileSelectorEx(const wxChar *title,
                          const wxChar *defaultDir,
                          const wxChar *defaultFileName,
                          int* defaultFilterIndex,
                          const wxChar *filter,
                          int       flags,
                          wxWindow* parent,
                          int       x,
                          int       y)

{
    wxFileDialog fileDialog(parent,
                            !wxIsEmpty(title) ? title : wxEmptyString,
                            !wxIsEmpty(defaultDir) ? defaultDir : wxEmptyString,
                            !wxIsEmpty(defaultFileName) ? defaultFileName : wxEmptyString,
                            !wxIsEmpty(filter) ? filter : wxEmptyString,
                            flags, wxPoint(x, y));

    wxString filename;
    if ( fileDialog.ShowModal() == wxID_OK )
    {
        if ( defaultFilterIndex )
            *defaultFilterIndex = fileDialog.GetFilterIndex();

        filename = fileDialog.GetPath();
    }

    return filename;
}

//----------------------------------------------------------------------------
// wxDefaultFileSelector - Generic load/save dialog (for internal use only)
//----------------------------------------------------------------------------

static wxString wxDefaultFileSelector(bool load,
                                      const wxChar *what,
                                      const wxChar *extension,
                                      const wxChar *default_name,
                                      wxWindow *parent)
{
    wxString prompt;
    wxString str;
    if (load)
        str = _("Load %s file");
    else
        str = _("Save %s file");
    prompt.Printf(str, what);

    wxString wild;
    const wxChar *ext = extension;
    if ( !wxIsEmpty(ext) )
    {
        if ( *ext == wxT('.') )
            ext++;

        wild.Printf(wxT("*.%s"), ext);
    }
    else // no extension specified
    {
        wild = wxFileSelectorDefaultWildcardStr;
    }

    return wxFileSelector(prompt, NULL, default_name, ext, wild,
                          load ? (wxFD_OPEN | wxFD_FILE_MUST_EXIST) : wxFD_SAVE, parent);
}

//----------------------------------------------------------------------------
// wxLoadFileSelector
//----------------------------------------------------------------------------

WXDLLEXPORT wxString wxLoadFileSelector(const wxChar *what,
                                        const wxChar *extension,
                                        const wxChar *default_name,
                                        wxWindow *parent)
{
    return wxDefaultFileSelector(true, what, extension, default_name, parent);
}

//----------------------------------------------------------------------------
// wxSaveFileSelector
//----------------------------------------------------------------------------

WXDLLEXPORT wxString wxSaveFileSelector(const wxChar *what,
                                        const wxChar *extension,
                                        const wxChar *default_name,
                                        wxWindow *parent)
{
    return wxDefaultFileSelector(false, what, extension, default_name, parent);
}


//----------------------------------------------------------------------------
// wxDirDialogBase
//----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_6
long wxDirDialogBase::GetStyle() const
{
    return GetWindowStyle();
}

void wxDirDialogBase::SetStyle(long style)
{
    SetWindowStyle(style);
}
#endif // WXWIN_COMPATIBILITY_2_6


#endif // wxUSE_FILEDLG
