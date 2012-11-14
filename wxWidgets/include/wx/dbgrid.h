///////////////////////////////////////////////////////////////////////////////
// Name:        dbgrid.h
// Purpose:     Displays a wxDbTable in a wxGrid.
// Author:      Roger Gammans, Paul Gammans
// Modified by:
// Created:
// RCS-ID:      $Id: dbgrid.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1999 The Computer Surgery (roger@computer-surgery.co.uk)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////
// Branched From : dbgrid.h,v 1.19 2001/03/28 11:16:01
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_DBGRID_H_
#define _WX_GENERIC_DBGRID_H_

#if wxUSE_ODBC
#if wxUSE_GRID

#include "wx/log.h"
#include "wx/dbtable.h"
#include "wx/dynarray.h"
#include "wx/grid.h"
#include "wx/dbkeyg.h"

#define wxGRID_VALUE_DBAUTO     wxT("dbauto")

WX_DECLARE_USER_EXPORTED_OBJARRAY(GenericKey,keyarray,WXDLLIMPEXP_DBGRID);

static const int wxUSE_QUERY = -1;

class WXDLLIMPEXP_DBGRID wxDbGridColInfoBase
{
public:
    //Default ctor
    wxDbGridColInfoBase() { }
    wxDbGridColInfoBase(int colNo,
                        wxString type, wxString title) :
                        DbCol(colNo),
                        wxtypename(type),
                        Title(title)
                        { }
    //Copy Ctor
    wxDbGridColInfoBase(const wxDbGridColInfoBase& ref)
    {
        DbCol       = ref.DbCol;
        wxtypename  = ref.wxtypename;
        Title       = ref.Title;
    }
    //Empty destructor for member obj's
    ~wxDbGridColInfoBase() {}

    int        DbCol;
    wxString   wxtypename;
    wxString   Title;
};


class WXDLLIMPEXP_DBGRID wxDbGridColInfo
{
public:
    wxDbGridColInfo(int colNo,
                    wxString type,
                    wxString title,
                    wxDbGridColInfo *next) :
        m_data(colNo,type,title)
    {
        m_next=next;
    }

    //Empty List
    ~wxDbGridColInfo() { delete m_next; }

    //Recurse to find length.
    int Length() { return (m_next ? m_next->Length() +1 :  1); }

    // Adds a new column info (2 step creation)
    void AddColInfo (int colNo,
                    wxString type,
                    wxString title)
    {
        GetLast()->m_next = new wxDbGridColInfo (colNo, type, title, NULL);
    }

    // Searches last
    wxDbGridColInfo *GetLast() { return (m_next ? m_next->GetLast() : this); }


    protected:
    wxDbGridColInfoBase  m_data;
    wxDbGridColInfo     *m_next;

    friend class wxDbGridTableBase;
};


class WXDLLIMPEXP_DBGRID wxDbGridCellAttrProvider : public wxGridCellAttrProvider
{
public:
    wxDbGridCellAttrProvider();
    wxDbGridCellAttrProvider(wxDbTable *tab, wxDbGridColInfoBase* ColInfo);
    virtual ~wxDbGridCellAttrProvider();

    virtual wxGridCellAttr *GetAttr(int row, int col,
                                    wxGridCellAttr::wxAttrKind kind) const;
    virtual void AssignDbTable(wxDbTable *tab);
private:
    wxDbTable           *m_data;
    wxDbGridColInfoBase *m_ColInfo;
};


class WXDLLIMPEXP_DBGRID wxDbGridTableBase : public wxGridTableBase
{
public:
    wxDbGridTableBase(wxDbTable *tab, wxDbGridColInfo *ColInfo,
              int count = wxUSE_QUERY, bool takeOwnership = true);
    virtual ~wxDbGridTableBase();

    virtual int GetNumberRows()
    {
        wxLogDebug(wxT(" GetNumberRows() = %i"),m_rowtotal);
        return m_rowtotal;
    }
    virtual int GetNumberCols()
    {
        wxLogDebug(wxT(" GetNumberCols() = %i"),m_nocols);
        return m_nocols;
    }
    virtual bool     IsEmptyCell(int row, int col) ;
    virtual wxString GetValue(int row, int col) ;
    virtual void     SetValue(int row, int col, const wxString& value);
    virtual bool     CanHaveAttributes();
    virtual wxString GetTypeName(int row, int col);
    virtual bool     CanGetValueAs(int row, int col, const wxString& typeName);
    virtual bool     CanSetValueAs(int row, int col, const wxString& typeName);
    virtual long     GetValueAsLong(int row, int col);
    virtual double   GetValueAsDouble(int row, int col);
    virtual bool     GetValueAsBool(int row, int col);
    virtual void     SetValueAsLong(int row, int col, long value);
    virtual void     SetValueAsDouble(int row, int col, double value);
    virtual void     SetValueAsBool(int row, int col, bool value);
    virtual void    *GetValueAsCustom(int row, int col, const wxString& typeName);
    virtual void     SetValueAsCustom(int row, int col, const wxString& typeName, void* value);


    virtual wxString GetColLabelValue(int col);

    virtual bool     AssignDbTable(wxDbTable *tab, int count = wxUSE_QUERY, bool takeOwnership=true);
    virtual void     ValidateRow(int row);
    virtual bool     UpdateRow(int row) const
    {
        if (m_row != row)
            return true;
        else
            return Writeback();
    }

private:
    //Operates on the current row
    bool Writeback() const;

    typedef wxGridTableBase inherited;
    keyarray     m_keys;
    wxDbTable   *m_data;
    bool         m_dbowner;
    int          m_rowtotal;
    int          m_nocols;
    int          m_row;
    wxDbGridColInfoBase *m_ColInfo;
    bool         m_rowmodified;
};

#endif  // #if wxUSE_GRID
#endif  // #if wxUSE_ODBC

#endif  // _WX_GENERIC_DBGRID_H_
