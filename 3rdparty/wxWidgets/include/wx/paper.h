/////////////////////////////////////////////////////////////////////////////
// Name:        paper.h
// Purpose:     Paper database types and classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: paper.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PAPERH__
#define _WX_PAPERH__

#include "wx/defs.h"
#include "wx/event.h"
#include "wx/cmndata.h"
#include "wx/intl.h"
#include "wx/hashmap.h"

/*
 * Paper type: see defs.h for wxPaperSize enum.
 * A wxPrintPaperType can have an id and a name, or just a name and wxPAPER_NONE,
 * so you can add further paper types without needing new ids.
 */

#ifdef __WXMSW__
#define WXADDPAPER(paperId, platformId, name, w, h) AddPaperType(paperId, platformId, name, w, h)
#else
#define WXADDPAPER(paperId, platformId, name, w, h) AddPaperType(paperId, 0, name, w, h)
#endif

class WXDLLEXPORT wxPrintPaperType: public wxObject
{
public:
    wxPrintPaperType();

    // platformId is a platform-specific id, such as in Windows, DMPAPER_...
    wxPrintPaperType(wxPaperSize paperId, int platformId, const wxString& name, int w, int h);

    inline wxString GetName() const { return wxGetTranslation(m_paperName); }
    inline wxPaperSize GetId() const { return m_paperId; }
    inline int GetPlatformId() const { return m_platformId; }

    // Get width and height in tenths of a millimetre
    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }

    // Get size in tenths of a millimetre
    inline wxSize GetSize() const { return wxSize(m_width, m_height); }

    // Get size in a millimetres
    inline wxSize GetSizeMM() const { return wxSize(m_width/10, m_height/10); }

    // Get width and height in device units (1/72th of an inch)
    wxSize GetSizeDeviceUnits() const ;

public:
    wxPaperSize m_paperId;
    int         m_platformId;
    int         m_width;  // In tenths of a millimetre
    int         m_height; // In tenths of a millimetre
    wxString    m_paperName;

private:
    DECLARE_DYNAMIC_CLASS(wxPrintPaperType)
};

WX_DECLARE_STRING_HASH_MAP(wxPrintPaperType*, wxStringToPrintPaperTypeHashMap);

class WXDLLIMPEXP_FWD_CORE wxPrintPaperTypeList;

class WXDLLEXPORT wxPrintPaperDatabase
{
public:
    wxPrintPaperDatabase();
    ~wxPrintPaperDatabase();

    void CreateDatabase();
    void ClearDatabase();

    void AddPaperType(wxPaperSize paperId, const wxString& name, int w, int h);
    void AddPaperType(wxPaperSize paperId, int platformId, const wxString& name, int w, int h);

    // Find by name
    wxPrintPaperType *FindPaperType(const wxString& name);

    // Find by size id
    wxPrintPaperType *FindPaperType(wxPaperSize id);

    // Find by platform id
    wxPrintPaperType *FindPaperTypeByPlatformId(int id);

    // Find by size
    wxPrintPaperType *FindPaperType(const wxSize& size);

    // Convert name to size id
    wxPaperSize ConvertNameToId(const wxString& name);

    // Convert size id to name
    wxString ConvertIdToName(wxPaperSize paperId);

    // Get the paper size
    wxSize GetSize(wxPaperSize paperId);

    // Get the paper size
    wxPaperSize GetSize(const wxSize& size);

    //
    wxPrintPaperType* Item(size_t index) const;
    size_t GetCount() const;
private:
    wxStringToPrintPaperTypeHashMap* m_map;
    wxPrintPaperTypeList* m_list;
    // DECLARE_DYNAMIC_CLASS(wxPrintPaperDatabase)
};

extern WXDLLEXPORT_DATA(wxPrintPaperDatabase*) wxThePrintPaperDatabase;


#endif
    // _WX_PAPERH__
