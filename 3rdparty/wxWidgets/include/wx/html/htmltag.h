/////////////////////////////////////////////////////////////////////////////
// Name:        htmltag.h
// Purpose:     wxHtmlTag class (represents single tag)
// Author:      Vaclav Slavik
// RCS-ID:      $Id: htmltag.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HTMLTAG_H_
#define _WX_HTMLTAG_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/object.h"
#include "wx/arrstr.h"

class WXDLLIMPEXP_FWD_CORE wxColour;
class WXDLLIMPEXP_FWD_HTML wxHtmlEntitiesParser;

//-----------------------------------------------------------------------------
// wxHtmlTagsCache
//          - internal wxHTML class, do not use!
//-----------------------------------------------------------------------------

struct wxHtmlCacheItem;

class WXDLLIMPEXP_HTML wxHtmlTagsCache : public wxObject
{
    DECLARE_DYNAMIC_CLASS(wxHtmlTagsCache)

private:
    wxHtmlCacheItem *m_Cache;
    int m_CacheSize;
    int m_CachePos;

public:
    wxHtmlTagsCache() : wxObject() {m_CacheSize = 0; m_Cache = NULL;}
    wxHtmlTagsCache(const wxString& source);
    virtual ~wxHtmlTagsCache() {free(m_Cache);}

    // Finds parameters for tag starting at at and fills the variables
    void QueryTag(int at, int* end1, int* end2);

    DECLARE_NO_COPY_CLASS(wxHtmlTagsCache)
};


//--------------------------------------------------------------------------------
// wxHtmlTag
//                  This represents single tag. It is used as internal structure
//                  by wxHtmlParser.
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlTag : public wxObject
{
    DECLARE_CLASS(wxHtmlTag)

protected:
    // constructs wxHtmlTag object based on HTML tag.
    // The tag begins (with '<' character) at position pos in source
    // end_pos is position where parsing ends (usually end of document)
    wxHtmlTag(wxHtmlTag *parent,
              const wxString& source, int pos, int end_pos,
              wxHtmlTagsCache *cache,
              wxHtmlEntitiesParser *entParser);
    friend class wxHtmlParser;
public:
    virtual ~wxHtmlTag();

    wxHtmlTag *GetParent() const {return m_Parent;}
    wxHtmlTag *GetFirstSibling() const;
    wxHtmlTag *GetLastSibling() const;
    wxHtmlTag *GetChildren() const { return m_FirstChild; }
    wxHtmlTag *GetPreviousSibling() const { return m_Prev; }
    wxHtmlTag *GetNextSibling() const {return m_Next; }
    // Return next tag, as if tree had been flattened
    wxHtmlTag *GetNextTag() const;

    // Returns tag's name in uppercase.
    inline wxString GetName() const {return m_Name;}

    // Returns true if the tag has given parameter. Parameter
    // should always be in uppercase.
    // Example : <IMG SRC="test.jpg"> HasParam("SRC") returns true
    bool HasParam(const wxString& par) const;

    // Returns value of the param. Value is in uppercase unless it is
    // enclosed with "
    // Example : <P align=right> GetParam("ALIGN") returns (RIGHT)
    //           <P IMG SRC="WhaT.jpg"> GetParam("SRC") returns (WhaT.jpg)
    //                           (or ("WhaT.jpg") if with_commas == true)
    wxString GetParam(const wxString& par, bool with_commas = false) const;

    // Convenience functions:
    bool GetParamAsColour(const wxString& par, wxColour *clr) const;
    bool GetParamAsInt(const wxString& par, int *clr) const;

    // Scans param like scanf() functions family does.
    // Example : ScanParam("COLOR", "\"#%X\"", &clr);
    // This is always with with_commas=false
    // Returns number of scanned values
    // (like sscanf() does)
    // NOTE: unlike scanf family, this function only accepts
    //       *one* parameter !
    int ScanParam(const wxString& par, const wxChar *format, void *param) const;

    // Returns string containing all params.
    wxString GetAllParams() const;

    // return true if this there is matching ending tag
    inline bool HasEnding() const {return m_End1 >= 0;}

    // returns beginning position of _internal_ block of text
    // See explanation (returned value is marked with *):
    // bla bla bla <MYTAG>* bla bla intenal text</MYTAG> bla bla
    inline int GetBeginPos() const {return m_Begin;}
    // returns ending position of _internal_ block of text.
    // bla bla bla <MYTAG> bla bla intenal text*</MYTAG> bla bla
    inline int GetEndPos1() const {return m_End1;}
    // returns end position 2 :
    // bla bla bla <MYTAG> bla bla internal text</MYTAG>* bla bla
    inline int GetEndPos2() const {return m_End2;}

private:
    wxString m_Name;
    int m_Begin, m_End1, m_End2;
    wxArrayString m_ParamNames, m_ParamValues;

    // DOM tree relations:
    wxHtmlTag *m_Next;
    wxHtmlTag *m_Prev;
    wxHtmlTag *m_FirstChild, *m_LastChild;
    wxHtmlTag *m_Parent;

    DECLARE_NO_COPY_CLASS(wxHtmlTag)
};





#endif

#endif // _WX_HTMLTAG_H_

