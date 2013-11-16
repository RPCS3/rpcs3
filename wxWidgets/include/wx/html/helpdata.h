/////////////////////////////////////////////////////////////////////////////
// Name:        helpdata.h
// Purpose:     wxHtmlHelpData
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden and Vaclav Slavik
// RCS-ID:      $Id: helpdata.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Harm van der Heijden and Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPDATA_H_
#define _WX_HELPDATA_H_

#include "wx/defs.h"

#if wxUSE_HTML

#include "wx/object.h"
#include "wx/string.h"
#include "wx/filesys.h"
#include "wx/dynarray.h"
#include "wx/font.h"

class WXDLLIMPEXP_FWD_HTML wxHtmlHelpData;

//--------------------------------------------------------------------------------
// helper classes & structs
//--------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlBookRecord
{
public:
    wxHtmlBookRecord(const wxString& bookfile, const wxString& basepath,
                     const wxString& title, const wxString& start)
    {
        m_BookFile = bookfile;
        m_BasePath = basepath;
        m_Title = title;
        m_Start = start;
        // for debugging, give the contents index obvious default values
        m_ContentsStart = m_ContentsEnd = -1;
    }
    wxString GetBookFile() const { return m_BookFile; }
    wxString GetTitle() const { return m_Title; }
    wxString GetStart() const { return m_Start; }
    wxString GetBasePath() const { return m_BasePath; }
    /* SetContentsRange: store in the bookrecord where in the index/contents lists the
     * book's records are stored. This to facilitate searching in a specific book.
     * This code will have to be revised when loading/removing books becomes dynamic.
     * (as opposed to appending only)
     * Note that storing index range is pointless, because the index is alphab. sorted. */
    void SetContentsRange(int start, int end) { m_ContentsStart = start; m_ContentsEnd = end; }
    int GetContentsStart() const { return m_ContentsStart; }
    int GetContentsEnd() const { return m_ContentsEnd; }

    void SetTitle(const wxString& title) { m_Title = title; }
    void SetBasePath(const wxString& path) { m_BasePath = path; }
    void SetStart(const wxString& start) { m_Start = start; }

    // returns full filename of page (which is part of the book),
    // i.e. with book's basePath prepended. If page is already absolute
    // path, basePath is _not_ prepended.
    wxString GetFullPath(const wxString &page) const;

protected:
    wxString m_BookFile;
    wxString m_BasePath;
    wxString m_Title;
    wxString m_Start;
    int m_ContentsStart;
    int m_ContentsEnd;
};


WX_DECLARE_USER_EXPORTED_OBJARRAY(wxHtmlBookRecord, wxHtmlBookRecArray,
                                  WXDLLIMPEXP_HTML);

struct WXDLLIMPEXP_HTML wxHtmlHelpDataItem
{
    wxHtmlHelpDataItem() : level(0), parent(NULL), id(wxID_ANY), book(NULL) {}

    int level;
    wxHtmlHelpDataItem *parent;
    int id;
    wxString name;
    wxString page;
    wxHtmlBookRecord *book;

    // returns full filename of m_Page, i.e. with book's basePath prepended
    wxString GetFullPath() const { return book->GetFullPath(page); }

    // returns item indented with spaces if it has level>1:
    wxString GetIndentedName() const;
};

WX_DECLARE_USER_EXPORTED_OBJARRAY(wxHtmlHelpDataItem, wxHtmlHelpDataItems,
                                  WXDLLIMPEXP_HTML);

#if WXWIN_COMPATIBILITY_2_4
// old interface to contents and index:
struct wxHtmlContentsItem
{
    wxHtmlContentsItem();
    wxHtmlContentsItem(const wxHtmlHelpDataItem& d);
    wxHtmlContentsItem& operator=(const wxHtmlContentsItem& d);
    ~wxHtmlContentsItem();

    int m_Level;
    int m_ID;
    wxChar *m_Name;
    wxChar *m_Page;
    wxHtmlBookRecord *m_Book;

    // returns full filename of m_Page, i.e. with book's basePath prepended
    wxString GetFullPath() const { return m_Book->GetFullPath(m_Page); }

private:
    bool m_autofree;
};
#endif


//------------------------------------------------------------------------------
// wxHtmlSearchEngine
//                  This class takes input streams and scans them for occurence
//                  of keyword(s)
//------------------------------------------------------------------------------

class WXDLLIMPEXP_HTML wxHtmlSearchEngine : public wxObject
{
public:
    wxHtmlSearchEngine() : wxObject() {}
    virtual ~wxHtmlSearchEngine() {}

    // Sets the keyword we will be searching for
    virtual void LookFor(const wxString& keyword, bool case_sensitive, bool whole_words_only);

    // Scans the stream for the keyword.
    // Returns true if the stream contains keyword, fALSE otherwise
    virtual bool Scan(const wxFSFile& file);

private:
    wxString m_Keyword;
    bool m_CaseSensitive;
    bool m_WholeWords;

    DECLARE_NO_COPY_CLASS(wxHtmlSearchEngine)
};


// State information of a search action. I'd have preferred to make this a
// nested class inside wxHtmlHelpData, but that's against coding standards :-(
// Never construct this class yourself, obtain a copy from
// wxHtmlHelpData::PrepareKeywordSearch(const wxString& key)
class WXDLLIMPEXP_HTML wxHtmlSearchStatus
{
public:
    // constructor; supply wxHtmlHelpData ptr, the keyword and (optionally) the
    // title of the book to search. By default, all books are searched.
    wxHtmlSearchStatus(wxHtmlHelpData* base, const wxString& keyword,
                       bool case_sensitive, bool whole_words_only,
                       const wxString& book = wxEmptyString);
    bool Search();  // do the next iteration
    bool IsActive() { return m_Active; }
    int GetCurIndex() { return m_CurIndex; }
    int GetMaxIndex() { return m_MaxIndex; }
    const wxString& GetName() { return m_Name; }

    const wxHtmlHelpDataItem *GetCurItem() const { return m_CurItem; }
#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( wxHtmlContentsItem* GetContentsItem() );
#endif

private:
    wxHtmlHelpData* m_Data;
    wxHtmlSearchEngine m_Engine;
    wxString m_Keyword, m_Name;
    wxString m_LastPage;
    wxHtmlHelpDataItem* m_CurItem;
    bool m_Active;   // search is not finished
    int m_CurIndex;  // where we are now
    int m_MaxIndex;  // number of files we search
    // For progress bar: 100*curindex/maxindex = % complete

    DECLARE_NO_COPY_CLASS(wxHtmlSearchStatus)
};

class WXDLLIMPEXP_HTML wxHtmlHelpData : public wxObject
{
    DECLARE_DYNAMIC_CLASS(wxHtmlHelpData)
    friend class wxHtmlSearchStatus;

public:
    wxHtmlHelpData();
    virtual ~wxHtmlHelpData();

    // Sets directory where temporary files are stored.
    // These temp files are index & contents file in binary (much faster to read)
    // form. These files are NOT deleted on program's exit.
    void SetTempDir(const wxString& path);

    // Adds new book. 'book' is location of .htb file (stands for "html book").
    // See documentation for details on its format.
    // Returns success.
    bool AddBook(const wxString& book);
    bool AddBookParam(const wxFSFile& bookfile,
                      wxFontEncoding encoding,
                      const wxString& title, const wxString& contfile,
                      const wxString& indexfile = wxEmptyString,
                      const wxString& deftopic = wxEmptyString,
                      const wxString& path = wxEmptyString);

    // Some accessing stuff:

    // returns URL of page on basis of (file)name
    wxString FindPageByName(const wxString& page);
    // returns URL of page on basis of MS id
    wxString FindPageById(int id);

    const wxHtmlBookRecArray& GetBookRecArray() const { return m_bookRecords; }

    const wxHtmlHelpDataItems& GetContentsArray() const { return m_contents; }
    const wxHtmlHelpDataItems& GetIndexArray() const { return m_index; }

#if WXWIN_COMPATIBILITY_2_4
    // deprecated interface, new interface is arrays-based (see above)
    wxDEPRECATED( wxHtmlContentsItem* GetContents() );
    wxDEPRECATED( int GetContentsCnt() );
    wxDEPRECATED( wxHtmlContentsItem* GetIndex() );
    wxDEPRECATED( int GetIndexCnt() );
#endif

protected:
    wxString m_tempPath;

    // each book has one record in this array:
    wxHtmlBookRecArray m_bookRecords;

    wxHtmlHelpDataItems m_contents; // list of all available books and pages
    wxHtmlHelpDataItems m_index; // list of index itesm

#if WXWIN_COMPATIBILITY_2_4
    // deprecated data structures, set only if GetContents(), GetIndex()
    // called
    wxHtmlContentsItem* m_cacheContents;
    wxHtmlContentsItem* m_cacheIndex;
private:
    void CleanCompatibilityData();
#endif

protected:
    // Imports .hhp files (MS HTML Help Workshop)
    bool LoadMSProject(wxHtmlBookRecord *book, wxFileSystem& fsys,
                       const wxString& indexfile, const wxString& contentsfile);
    // Reads binary book
    bool LoadCachedBook(wxHtmlBookRecord *book, wxInputStream *f);
    // Writes binary book
    bool SaveCachedBook(wxHtmlBookRecord *book, wxOutputStream *f);

    DECLARE_NO_COPY_CLASS(wxHtmlHelpData)
};

#endif

#endif
