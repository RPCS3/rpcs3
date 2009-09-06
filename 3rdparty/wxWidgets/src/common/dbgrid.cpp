///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/dbgrid.cpp
// Purpose:     Displays a wxDbTable in a wxGrid.
// Author:      Roger Gammans, Paul Gammans
// Modified by:
// Created:
// RCS-ID:      $Id: dbgrid.cpp 43769 2006-12-03 18:20:28Z VZ $
// Copyright:   (c) 1999 The Computer Surgery (roger@computer-surgery.co.uk)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////
// Branched From : dbgrid.cpp,v 1.18 2000/12/19 13:00:58
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_ODBC && wxUSE_GRID

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
    #include "wx/dc.h"
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "wx/generic/gridctrl.h"
#include "wx/dbgrid.h"

// DLL options compatibility check:
WX_CHECK_BUILD_OPTIONS("wxDbGrid")


wxDbGridCellAttrProvider::wxDbGridCellAttrProvider()
{
    m_data=NULL;
    m_ColInfo=NULL;
}

wxDbGridCellAttrProvider::wxDbGridCellAttrProvider(wxDbTable *tab, wxDbGridColInfoBase* ColInfo)
{
    m_data=tab;
    m_ColInfo=ColInfo;
}

wxDbGridCellAttrProvider::~wxDbGridCellAttrProvider()
{
}

wxGridCellAttr *wxDbGridCellAttrProvider::GetAttr(int row, int col,
                                      wxGridCellAttr::wxAttrKind  kind) const
{
    wxGridCellAttr *attr = wxGridCellAttrProvider::GetAttr(row,col,kind);

    if (m_data && m_ColInfo && (m_data->GetNumberOfColumns() > m_ColInfo[col].DbCol))
    {
        //FIXME: this test could.
        //  ??::InsertPending == m_data->get_ModifiedStatus()
        //  and if InsertPending use colDef[].InsertAllowed
        if (!(m_data->GetColDefs()[(m_ColInfo[col].DbCol)].Updateable))
        {
            switch(kind)
            {
                case (wxGridCellAttr::Any):
                    if (!attr)
                    {
                        attr = new wxGridCellAttr;
                        // Store so we don't keep creating / deleting this...
                        wxDbGridCellAttrProvider * self = wxConstCast(this, wxDbGridCellAttrProvider) ;
                        attr->IncRef();
                        self->SetColAttr(attr, col);
                        attr->SetReadOnly();
                    }
                    else
                    {
                        //We now must check what we were returned. and do the right thing (tm)
                        wxGridCellAttr::wxAttrKind attrkind = attr->GetKind();
                        if ((attrkind == (wxGridCellAttr::Default)) || (attrkind == (wxGridCellAttr::Cell)) ||
                            (attrkind == (wxGridCellAttr::Col)))
                        {
                                wxGridCellAttr *attrtomerge = attr;
                                attr = new wxGridCellAttr;
                                attr->SetKind(wxGridCellAttr::Merged);
                                attr->MergeWith(attrtomerge);
                                attr->SetReadOnly();
                                attrtomerge->DecRef();
                        }
                        attr->SetReadOnly();
                    }
                break;
                case (wxGridCellAttr::Col):
                    //As we must have a Coll, and were setting Coll attributes
                    // we can based on wxdbTable's so just set RO if attr valid
                    if (!attr)
                    {
                        attr = new wxGridCellAttr;
                        wxDbGridCellAttrProvider * self = wxConstCast(this, wxDbGridCellAttrProvider) ;
                        attr->IncRef();
                        self->SetColAttr(attr, col);
                    }
                    attr->SetReadOnly();
                break;
                default:
                    //Dont add RO for...
                    //  wxGridCellAttr::Cell - Not required, will inherit on merge from row.
                    //  wxGridCellAttr::Row - If wxDbtable ever supports row locking could add
                    //                        support to make RO on a row basis also.
                    //  wxGridCellAttr::Default - Don't edit this ! or all cell with a attr will become readonly
                    //  wxGridCellAttr::Merged - This should never be asked for.
                break;
            }
        }

    }
    return attr;
}

void wxDbGridCellAttrProvider::AssignDbTable(wxDbTable *tab)
{
    m_data = tab;
}

wxDbGridTableBase::wxDbGridTableBase(wxDbTable *tab, wxDbGridColInfo*  ColInfo,
                     int count, bool takeOwnership)  :
    m_keys(),
    m_data(tab),
    m_dbowner(takeOwnership),
    m_rowmodified(false)
{

    if (count == wxUSE_QUERY)
    {
        m_rowtotal = m_data ? m_data->Count() : 0;
    }
    else
    {
        m_rowtotal = count;
    }
//    m_keys.Size(m_rowtotal);
    m_row = -1;
    if (ColInfo)
    {
        m_nocols = ColInfo->Length();
        m_ColInfo = new wxDbGridColInfoBase[m_nocols];
        //Do Copy.
        wxDbGridColInfo *ptr = ColInfo;
        int i =0;
        while (ptr && i < m_nocols)
        {
            m_ColInfo[i] = ptr->m_data;
            ptr = ptr->m_next;
            i++;
        }
#ifdef __WXDEBUG__
        if (ptr)
        {
            wxLogDebug(wxT("NoCols over length after traversing %i items"),i);
        }
        if (i < m_nocols)
        {
            wxLogDebug(wxT("NoCols under length after traversing %i items"),i);
        }
#endif
    }
}

wxDbGridTableBase::~wxDbGridTableBase()
{
    wxDbGridCellAttrProvider *provider;

    //Can't check for update here as

    //FIXME: should i remove m_ColInfo and m_data from m_attrProvider if a wxDbGridAttrProvider
//    if ((provider = dynamic_cast<wxDbGridCellAttrProvider *>(GetAttrProvider())))
     // Using C casting for now until we can support dynamic_cast with wxWidgets
    provider = (wxDbGridCellAttrProvider *)(GetAttrProvider());
    if (provider)
    {
        provider->AssignDbTable(NULL);
    }
    delete [] m_ColInfo;

    Writeback();
    if (m_dbowner)
    {
        delete m_data;
    }
}

bool wxDbGridTableBase::CanHaveAttributes()
{
    if (!GetAttrProvider())
    {
        // use the default attr provider by default
        SetAttrProvider(new wxDbGridCellAttrProvider(m_data, m_ColInfo));
    }
    return true;
}


bool wxDbGridTableBase::AssignDbTable(wxDbTable *tab, int count, bool takeOwnership)
{
    wxDbGridCellAttrProvider *provider;

    //Remove Information from grid about old data
    if (GetView())
    {
        wxGrid *grid = GetView();
        grid->BeginBatch();
        grid->ClearSelection();
        if (grid->IsCellEditControlEnabled())
        {
            grid->DisableCellEditControl();
        }
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,0,m_rowtotal);
        grid->ProcessTableMessage(msg);
    }

    //reset our internals...
    Writeback();
    if (m_dbowner)
    {
        delete m_data;
    }
    m_keys.Empty();
    m_data = tab;
    //FIXME: Remove dynamic_cast before sumision to wxwin
//    if ((provider = dynamic_cast<wxDbGridCellAttrProvider *> (GetAttrProvider())))
     // Using C casting for now until we can support dynamic_cast with wxWidgets
    provider = (wxDbGridCellAttrProvider *)(GetAttrProvider());
    if (provider)
    {
        provider->AssignDbTable(m_data);
    }

    if (count == wxUSE_QUERY)
    {
        m_rowtotal = m_data ? m_data->Count() : 0;
    }
    else
    {
         m_rowtotal = count;
    }
    m_row = -1;

    //Add Information to grid about new data
    if (GetView())
    {
        wxGrid * grid = GetView();
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rowtotal);
        grid->ProcessTableMessage(msg);
        grid->EndBatch();
    }
    m_dbowner = takeOwnership;
    m_rowmodified = false;
    return true;
}

wxString wxDbGridTableBase::GetTypeName(int WXUNUSED(row), int col)
{
    if (GetNumberCols() > col)
    {
        if (m_ColInfo[col].wxtypename == wxGRID_VALUE_DBAUTO)
        {
            if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
            {
                wxFAIL_MSG (_T("You can not use wxGRID_VALUE_DBAUTO for virtual columns"));
            }
            switch(m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype)
            {
                case SQL_C_CHAR:
#ifdef SQL_C_WCHAR
                case SQL_C_WCHAR:
#endif
                    return wxGRID_VALUE_STRING;
                case SQL_C_SHORT:
                case SQL_C_SSHORT:
                    return wxGRID_VALUE_NUMBER;
                case SQL_C_USHORT:
                    return wxGRID_VALUE_NUMBER;
                case SQL_C_LONG:
                case SQL_C_SLONG:
                    return wxGRID_VALUE_NUMBER;
                case SQL_C_ULONG:
                    return wxGRID_VALUE_NUMBER;
                case SQL_C_FLOAT:
                    return wxGRID_VALUE_FLOAT;
                case SQL_C_DOUBLE:
                    return wxGRID_VALUE_FLOAT;
                case SQL_C_DATE:
                    return wxGRID_VALUE_DATETIME;
                case SQL_C_TIME:
                    return wxGRID_VALUE_DATETIME;
                case SQL_C_TIMESTAMP:
                    return wxGRID_VALUE_DATETIME;
                default:
                    return wxGRID_VALUE_STRING;
            }
        }
        else
        {
            return m_ColInfo[col].wxtypename;
        }
    }
    wxFAIL_MSG (_T("unknown column"));
    return wxString();
}

bool wxDbGridTableBase::CanGetValueAs(int row, int col, const wxString& typeName)
{
    wxLogDebug(wxT("CanGetValueAs() on %i,%i"),row,col);
    //Is this needed? As it will be validated on GetValueAsXXXX
    ValidateRow(row);

    if (typeName == wxGRID_VALUE_STRING)
    {
        //FIXME ummm What about blob field etc.
        return true;
    }

    if (m_data->IsColNull((UWORD)m_ColInfo[col].DbCol))
    {
        return false;
    }

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        //If a virtual column then we can't find it's type. we have to
        // return false to get using wxVariant.
        return false;
    }
    int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;

    if (typeName == wxGRID_VALUE_DATETIME)
    {
        if ((sqltype == SQL_C_DATE) ||
            (sqltype == SQL_C_TIME) ||
            (sqltype == SQL_C_TIMESTAMP))
        {
            return true;
        }
        return false;
    }
    if (typeName == wxGRID_VALUE_NUMBER)
    {
        if ((sqltype == SQL_C_SSHORT) ||
            (sqltype == SQL_C_USHORT) ||
            (sqltype == SQL_C_SLONG)  ||
            (sqltype == SQL_C_ULONG))
        {
            return true;
        }
        return false;
    }
    if (typeName == wxGRID_VALUE_FLOAT)
    {
        if ((sqltype == SQL_C_SSHORT) ||
            (sqltype == SQL_C_USHORT) ||
            (sqltype == SQL_C_SLONG)  ||
            (sqltype == SQL_C_ULONG)  ||
            (sqltype == SQL_C_FLOAT)  ||
            (sqltype == SQL_C_DOUBLE))
        {
            return true;
        }
        return false;
    }
    return false;
}

bool wxDbGridTableBase::CanSetValueAs(int WXUNUSED(row), int col, const wxString& typeName)
{
    if (typeName == wxGRID_VALUE_STRING)
    {
        //FIXME ummm What about blob field etc.
        return true;
    }

    if (!(m_data->GetColDefs()[(m_ColInfo[col].DbCol)].Updateable))
    {
        return false;
    }

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        //If a virtual column then we can't find it's type. we have to faulse to
        //get using wxVairent.
        return false;
    }

    int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;
    if (typeName == wxGRID_VALUE_DATETIME)
    {
        if ((sqltype == SQL_C_DATE) ||
            (sqltype == SQL_C_TIME) ||
            (sqltype == SQL_C_TIMESTAMP))
        {
            return true;
        }
        return false;
    }
    if (typeName == wxGRID_VALUE_NUMBER)
    {
        if ((sqltype == SQL_C_SSHORT) ||
            (sqltype == SQL_C_USHORT) ||
            (sqltype == SQL_C_SLONG)  ||
            (sqltype == SQL_C_ULONG))
        {
            return true;
        }
        return false;
    }
    if (typeName == wxGRID_VALUE_FLOAT)
    {
        if ((sqltype == SQL_C_SSHORT) ||
            (sqltype == SQL_C_USHORT) ||
            (sqltype == SQL_C_SLONG)  ||
            (sqltype == SQL_C_ULONG)  ||
            (sqltype == SQL_C_FLOAT)  ||
            (sqltype == SQL_C_DOUBLE))
        {
            return true;
        }
        return false;
    }
    return false;
}

long wxDbGridTableBase::GetValueAsLong(int row, int col)
{
    ValidateRow(row);

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        wxFAIL_MSG (_T("You can not use GetValueAsLong for virtual columns"));
        return 0;
    }
    int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;
    if ((sqltype == SQL_C_SSHORT) ||
        (sqltype == SQL_C_USHORT) ||
        (sqltype == SQL_C_SLONG) ||
        (sqltype == SQL_C_ULONG))
    {
        wxVariant val = m_data->GetColumn(m_ColInfo[col].DbCol);
        return val.GetLong();
    }
    wxFAIL_MSG (_T("unknown column, "));
    return 0;
}

double wxDbGridTableBase::GetValueAsDouble(int row, int col)
{
    wxLogDebug(wxT("GetValueAsDouble() on %i,%i"),row,col);
    ValidateRow(row);

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        wxFAIL_MSG (_T("You can not use GetValueAsDouble for virtual columns"));
        return 0.0;
    }
    int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;
    if ((sqltype == SQL_C_SSHORT) ||
        (sqltype == SQL_C_USHORT) ||
        (sqltype == SQL_C_SLONG) ||
        (sqltype == SQL_C_ULONG) ||
        (sqltype == SQL_C_FLOAT) ||
        (sqltype == SQL_C_DOUBLE))
    {
        wxVariant val = m_data->GetColumn(m_ColInfo[col].DbCol);
        return val.GetDouble();
    }
    wxFAIL_MSG (_T("unknown column"));
    return 0.0;
}

bool wxDbGridTableBase::GetValueAsBool(int row, int col)
{
    wxLogDebug(wxT("GetValueAsBool() on %i,%i"),row,col);
    ValidateRow(row);

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        wxFAIL_MSG (_T("You can not use GetValueAsBool for virtual columns"));
        return 0;
    }
    int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;
    if ((sqltype == SQL_C_SSHORT) ||
        (sqltype == SQL_C_USHORT) ||
        (sqltype == SQL_C_SLONG) ||
        (sqltype == SQL_C_ULONG))
    {
        wxVariant val = m_data->GetColumn(m_ColInfo[col].DbCol);
        return val.GetBool();
    }
    wxFAIL_MSG (_T("unknown column, "));
    return 0;
}

void* wxDbGridTableBase::GetValueAsCustom(int row, int col, const wxString& typeName)
{
    wxLogDebug(wxT("GetValueAsCustom() on %i,%i"),row,col);
    ValidateRow(row);

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        wxFAIL_MSG (_T("You can not use GetValueAsCustom for virtual columns"));
        return NULL;
    }
    if (m_data->IsColNull((UWORD)m_ColInfo[col].DbCol))
        return NULL;

    if (typeName == wxGRID_VALUE_DATETIME)
    {
        wxDbColDef *pColDefs = m_data->GetColDefs();
        int sqltype = pColDefs[(m_ColInfo[col].DbCol)].SqlCtype;

        if ((sqltype == SQL_C_DATE) ||
            (sqltype == SQL_C_TIME) ||
            (sqltype == SQL_C_TIMESTAMP))
        {
            wxVariant val = m_data->GetColumn(m_ColInfo[col].DbCol);
            return new wxDateTime(val.GetDateTime());
        }
    }
    wxFAIL_MSG (_T("unknown column data type "));
    return NULL;
}


void wxDbGridTableBase::SetValueAsCustom(int row, int col, const wxString& typeName, void* value)
{
    wxLogDebug(wxT("SetValueAsCustom() on %i,%i"),row,col);
    ValidateRow(row);

    if (m_data->GetNumberOfColumns() <= m_ColInfo[col].DbCol)
    {
        wxFAIL_MSG (_T("You can not use SetValueAsCustom for virtual columns"));
        return;
    }

    if (typeName == wxGRID_VALUE_DATETIME)
    {
        int sqltype = m_data->GetColDefs()[(m_ColInfo[col].DbCol)].SqlCtype;
        if ((sqltype == SQL_C_DATE) ||
            (sqltype == SQL_C_TIME) ||
            (sqltype == SQL_C_TIMESTAMP))
        {
            //FIXME: you can't dynamic_cast from (void *)
            //wxDateTime *date = wxDynamicCast(value, wxDateTime);
            wxDateTime *date = (wxDateTime *)value;
            if (!date)
            {
                wxFAIL_MSG (_T("Failed to convert data"));
                return;
            }
            wxVariant val(date);
            m_rowmodified = true;
            m_data->SetColumn(m_ColInfo[col].DbCol,val);
        }
    }
    wxFAIL_MSG (_T("unknown column data type"));
    return ;
}


wxString wxDbGridTableBase::GetColLabelValue(int col)
{
    if (GetNumberCols() > col)
    {
        return m_ColInfo[col].Title;
    }
    wxFAIL_MSG (_T("unknown column"));
    return wxString();
}

bool wxDbGridTableBase::IsEmptyCell(int row, int col)
{
    wxLogDebug(wxT("IsEmtpyCell on %i,%i"),row,col);

    ValidateRow(row);
    return m_data->IsColNull((UWORD)m_ColInfo[col].DbCol);
}


wxString wxDbGridTableBase::GetValue(int row, int col)
{
    wxLogDebug(wxT("GetValue() on %i,%i"),row,col);

    ValidateRow(row);
    wxVariant val = m_data->GetColumn(m_ColInfo[col].DbCol);
    wxLogDebug(wxT("\tReturning \"%s\"\n"),val.GetString().c_str());

    return val.GetString();
}


void wxDbGridTableBase::SetValue(int row, int col,const wxString& value)
{
    wxLogDebug(wxT("SetValue() on %i,%i"),row,col);

    ValidateRow(row);
    wxVariant val(value);

    m_rowmodified = true;
    m_data->SetColumn(m_ColInfo[col].DbCol,val);
}


void wxDbGridTableBase::SetValueAsLong(int row, int col, long value)
{
    wxLogDebug(wxT("SetValueAsLong() on %i,%i"),row,col);

    ValidateRow(row);
    wxVariant val(value);

    m_rowmodified = true;
    m_data->SetColumn(m_ColInfo[col].DbCol,val);
}


void wxDbGridTableBase::SetValueAsDouble(int row, int col, double value)
{
    wxLogDebug(wxT("SetValueAsDouble() on %i,%i"),row,col);

    ValidateRow(row);
    wxVariant val(value);

    m_rowmodified = true;
    m_data->SetColumn(m_ColInfo[col].DbCol,val);

}


void wxDbGridTableBase::SetValueAsBool(int row, int col, bool value)
{
    wxLogDebug(wxT("SetValueAsBool() on %i,%i"),row,col);

    ValidateRow(row);
    wxVariant val(value);

    m_rowmodified = true;
    m_data->SetColumn(m_ColInfo[col].DbCol,val);
}


void wxDbGridTableBase::ValidateRow(int row)
{
    wxLogDebug(wxT("ValidateRow(%i) currently on row (%i). Array count = %lu"),
               row, m_row, (unsigned long)m_keys.GetCount());

    if (row == m_row)
         return;
    Writeback();

    //We add to row as Count is unsigned!
    if ((unsigned)(row+1) > m_keys.GetCount())
    {
        wxLogDebug(wxT("\trow key unknown"));
        // Extend Array, iterate through data filling with keys
        m_data->SetRowMode(wxDbTable::WX_ROW_MODE_QUERY);
        int trow;
        for (trow = m_keys.GetCount(); trow <= row; trow++)
        {
            wxLogDebug(wxT("Fetching row %i.."), trow);
            bool ret = m_data->GetNext();

            wxLogDebug(wxT(" ...success=(%i)"),ret);
            GenericKey k = m_data->GetKey();
            m_keys.Add(k);
        }
        m_row = row;
    }
    else
    {
        wxLogDebug(wxT("\trow key known centering data"));
        GenericKey k = m_keys.Item(row);
        m_data->SetRowMode(wxDbTable::WX_ROW_MODE_INDIVIDUAL);
        m_data->ClearMemberVars();
        m_data->SetKey(k);
        if (!m_data->QueryOnKeyFields())
        {
            wxDbLogExtendedErrorMsg(_T("ODBC error during Query()\n\n"), m_data->GetDb(),__TFILE__,__LINE__);
        }

        m_data->GetNext();

        m_row = row;
    }
    m_rowmodified = false;
}

bool wxDbGridTableBase::Writeback() const
{
    if (!m_rowmodified)
    {
        return true;
    }

    bool result=true;
    wxLogDebug(wxT("\trow key unknown"));

// FIXME: this code requires dbtable support for record status
#if 0
    switch (m_data->get_ModifiedStatus())
    {
        case wxDbTable::UpdatePending:
            result = m_data->Update();
           break;
        case wxDbTable::InsertPending:
            result = (m_data->Insert() == SQL_SUCCESS);
        break;
        default:
            //Nothing
        break;
    }
#else
    wxLogDebug(wxT("WARNING : Row writeback not implemented "));
#endif
    return result;
}

#include "wx/arrimpl.cpp"

WX_DEFINE_EXPORTED_OBJARRAY(keyarray)

#endif  // wxUSE_GRID && wxUSE_ODBC
