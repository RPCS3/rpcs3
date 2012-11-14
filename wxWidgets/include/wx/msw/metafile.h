/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/metafile.h
// Purpose:     wxMetaFile, wxMetaFileDC and wxMetaFileDataObject classes
// Author:      Julian Smart
// Modified by: VZ 07.01.00: implemented wxMetaFileDataObject
// Created:     01/02/97
// RCS-ID:      $Id: metafile.h 46103 2007-05-18 15:14:44Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_METAFIILE_H_
#define _WX_METAFIILE_H_

#include "wx/dc.h"
#include "wx/gdiobj.h"

#if wxUSE_DRAG_AND_DROP
    #include "wx/dataobj.h"
#endif

// ----------------------------------------------------------------------------
// Metafile and metafile device context classes
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMetafile;

class WXDLLEXPORT wxMetafileRefData: public wxGDIRefData
{
    friend class WXDLLEXPORT wxMetafile;
public:
    wxMetafileRefData();
    virtual ~wxMetafileRefData();

public:
    WXHANDLE m_metafile;
    int m_windowsMappingMode;
    int m_width, m_height;
};

#define M_METAFILEDATA ((wxMetafileRefData *)m_refData)

class WXDLLEXPORT wxMetafile: public wxGDIObject
{
public:
    wxMetafile(const wxString& file = wxEmptyString);
    virtual ~wxMetafile();

    // After this is called, the metafile cannot be used for anything
    // since it is now owned by the clipboard.
    virtual bool SetClipboard(int width = 0, int height = 0);

    virtual bool Play(wxDC *dc);
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return (M_METAFILEDATA && (M_METAFILEDATA->m_metafile != 0)); };

    // set/get the size of metafile for clipboard operations
    wxSize GetSize() const { return wxSize(GetWidth(), GetHeight()); }
    int GetWidth() const { return M_METAFILEDATA->m_width; }
    int GetHeight() const { return M_METAFILEDATA->m_height; }

    void SetWidth(int width) { M_METAFILEDATA->m_width = width; }
    void SetHeight(int height) { M_METAFILEDATA->m_height = height; }

    // Implementation
    WXHANDLE GetHMETAFILE() const { return M_METAFILEDATA->m_metafile; }
    void SetHMETAFILE(WXHANDLE mf) ;
    int GetWindowsMappingMode() const { return M_METAFILEDATA->m_windowsMappingMode; }
    void SetWindowsMappingMode(int mm);

private:
    DECLARE_DYNAMIC_CLASS(wxMetafile)
};

class WXDLLEXPORT wxMetafileDC: public wxDC
{
public:
    // Don't supply origin and extent
    // Supply them to wxMakeMetaFilePlaceable instead.
    wxMetafileDC(const wxString& file = wxEmptyString);

    // Supply origin and extent (recommended).
    // Then don't need to supply them to wxMakeMetaFilePlaceable.
    wxMetafileDC(const wxString& file, int xext, int yext, int xorg, int yorg);

    virtual ~wxMetafileDC();

    // Should be called at end of drawing
    virtual wxMetafile *Close();
    virtual void SetMapMode(int mode);

#if wxABI_VERSION >= 20805
    virtual void DoGetTextExtent(const wxString& string,
                                 wxCoord *x, wxCoord *y,
                                 wxCoord *descent = NULL,
                                 wxCoord *externalLeading = NULL,
                                 const wxFont *theFont = NULL) const;
#endif // wx ABI 2.8.5+

    // this method shouldn't have been defined here (DoGetTextExtent() is the
    // correct one) but keep it to avoid breaking binary backwards
    // compatibility
    virtual void GetTextExtent(const wxString& string, long *x, long *y,
            long *descent = NULL, long *externalLeading = NULL,
            wxFont *theFont = NULL, bool use16bit = false) const;

    // Implementation
    wxMetafile *GetMetaFile() const { return m_metaFile; }
    void SetMetaFile(wxMetafile *mf) { m_metaFile = mf; }
    int GetWindowsMappingMode() const { return m_windowsMappingMode; }
    void SetWindowsMappingMode(int mm) { m_windowsMappingMode = mm; }

protected:
    virtual void DoGetSize(int *width, int *height) const;

    int           m_windowsMappingMode;
    wxMetafile*   m_metaFile;

private:
    DECLARE_DYNAMIC_CLASS(wxMetafileDC)
};

/*
 * Pass filename of existing non-placeable metafile, and bounding box.
 * Adds a placeable metafile header, sets the mapping mode to anisotropic,
 * and sets the window origin and extent to mimic the wxMM_TEXT mapping mode.
 *
 */

// No origin or extent
bool WXDLLEXPORT wxMakeMetafilePlaceable(const wxString& filename, float scale = 1.0);

// Optional origin and extent
bool WXDLLEXPORT wxMakeMetaFilePlaceable(const wxString& filename, int x1, int y1, int x2, int y2, float scale = 1.0, bool useOriginAndExtent = true);

// ----------------------------------------------------------------------------
// wxMetafileDataObject is a specialization of wxDataObject for metafile data
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

class WXDLLEXPORT wxMetafileDataObject : public wxDataObjectSimple
{
public:
    // ctors
    wxMetafileDataObject() : wxDataObjectSimple(wxDF_METAFILE)
        { }
    wxMetafileDataObject(const wxMetafile& metafile)
        : wxDataObjectSimple(wxDF_METAFILE), m_metafile(metafile) { }

    // virtual functions which you may override if you want to provide data on
    // demand only - otherwise, the trivial default versions will be used
    virtual void SetMetafile(const wxMetafile& metafile)
        { m_metafile = metafile; }
    virtual wxMetafile GetMetafile() const
        { return m_metafile; }

    // implement base class pure virtuals
    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);

protected:
    wxMetafile m_metafile;
};

#endif // wxUSE_DRAG_AND_DROP

#endif
    // _WX_METAFIILE_H_

