///////////////////////////////////////////////////////////////////////////////
// Name:        wx/iconloc.h
// Purpose:     declaration of wxIconLocation class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.06.2003
// RCS-ID:      $Id: iconloc.h 27408 2004-05-23 20:53:33Z JS $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ICONLOC_H_
#define _WX_ICONLOC_H_

#include "wx/string.h"

// ----------------------------------------------------------------------------
// wxIconLocation: describes the location of an icon
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxIconLocationBase
{
public:
    // ctor takes the name of the file where the icon is
    wxEXPLICIT wxIconLocationBase(const wxString& filename = wxEmptyString)
        : m_filename(filename) { }

    // default copy ctor, assignment operator and dtor are ok


    // returns true if this object is valid/initialized
    bool IsOk() const { return !m_filename.empty(); }

    // set/get the icon file name
    void SetFileName(const wxString& filename) { m_filename = filename; }
    const wxString& GetFileName() const { return m_filename; }

private:
    wxString m_filename;
};

// under MSW the same file may contain several icons so we also store the
// index of the icon
#if defined(__WXMSW__)

class WXDLLIMPEXP_BASE wxIconLocation : public wxIconLocationBase
{
public:
    // ctor takes the name of the file where the icon is and the icons index in
    // the file
    wxEXPLICIT wxIconLocation(const wxString& file = wxEmptyString, int num = 0);

    // set/get the icon index
    void SetIndex(int num) { m_index = num; }
    int GetIndex() const { return m_index; }

private:
    int m_index;
};

inline
wxIconLocation::wxIconLocation(const wxString& file, int num)
              : wxIconLocationBase(file)
{
    SetIndex(num);
}

#else // !MSW

// must be a class because we forward declare it as class
class WXDLLIMPEXP_BASE wxIconLocation : public wxIconLocationBase
{
public:
    wxEXPLICIT wxIconLocation(const wxString& filename = wxEmptyString)
        : wxIconLocationBase(filename) { }
};

#endif // platform

#endif // _WX_ICONLOC_H_

