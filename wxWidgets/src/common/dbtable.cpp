///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/dbtable.cpp
// Purpose:     Implementation of the wxDbTable class.
// Author:      Doug Card
// Modified by: George Tasker
//              Bart Jourquin
//              Mark Johnson
// Created:     9.96
// RCS-ID:      $Id: dbtable.cpp 48685 2007-09-14 19:02:28Z VZ $
// Copyright:   (c) 1996 Remstar International, Inc.
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include  "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_ODBC

#ifndef WX_PRECOMP
    #include "wx/object.h"
    #include "wx/list.h"
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/log.h"
#endif

#ifdef DBDEBUG_CONSOLE
#if wxUSE_IOSTREAMH
    #include <iostream.h>
#else
    #include <iostream>
#endif
    #include "wx/ioswrap.h"
#endif

#include "wx/filefn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wx/dbtable.h"

#ifdef __UNIX__
// The HPUX preprocessor lines below were commented out on 8/20/97
// because macros.h currently redefines DEBUG and is unneeded.
// #  ifdef HPUX
// #    include <macros.h>
// #  endif
#  ifdef LINUX
#    include <sys/minmax.h>
#  endif
#endif

ULONG lastTableID = 0;


#ifdef __WXDEBUG__
    #include "wx/thread.h"

    wxList TablesInUse;
#if wxUSE_THREADS
    wxCriticalSection csTablesInUse;
#endif // wxUSE_THREADS
#endif


void csstrncpyt(wxChar *target, const wxChar *source, int n)
{
    while ( (*target++ = *source++) != '\0' && --n != 0 )
        ;

    *target = '\0';
}



/********** wxDbColDef::wxDbColDef() Constructor **********/
wxDbColDef::wxDbColDef()
{
    Initialize();
}  // Constructor


bool wxDbColDef::Initialize()
{
    ColName[0]      = 0;
    DbDataType      = DB_DATA_TYPE_INTEGER;
    SqlCtype        = SQL_C_LONG;
    PtrDataObj      = NULL;
    SzDataObj       = 0;
    KeyField        = false;
    Updateable      = false;
    InsertAllowed   = false;
    DerivedCol      = false;
    CbValue         = 0;
    Null            = false;

    return true;
}  // wxDbColDef::Initialize()


/********** wxDbTable::wxDbTable() Constructor **********/
wxDbTable::wxDbTable(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
                    const wxString &qryTblName, bool qryOnly, const wxString &tblPath)
{
    if (!initialize(pwxDb, tblName, numColumns, qryTblName, qryOnly, tblPath))
        cleanup();
}  // wxDbTable::wxDbTable()


/***** DEPRECATED: use wxDbTable::wxDbTable() format above *****/
#if WXWIN_COMPATIBILITY_2_4
wxDbTable::wxDbTable(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
                    const wxChar *qryTblName, bool qryOnly, const wxString &tblPath)
{
    wxString tempQryTblName;
    tempQryTblName = qryTblName;
    if (!initialize(pwxDb, tblName, numColumns, tempQryTblName, qryOnly, tblPath))
        cleanup();
}  // wxDbTable::wxDbTable()
#endif // WXWIN_COMPATIBILITY_2_4


/********** wxDbTable::~wxDbTable() **********/
wxDbTable::~wxDbTable()
{
    this->cleanup();
}  // wxDbTable::~wxDbTable()


bool wxDbTable::initialize(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
                    const wxString &qryTblName, bool qryOnly, const wxString &tblPath)
{
    // Initializing member variables
    pDb                 = pwxDb;                    // Pointer to the wxDb object
    henv                = 0;
    hdbc                = 0;
    hstmt               = 0;
    m_hstmtGridQuery               = 0;
    hstmtDefault        = 0;                        // Initialized below
    hstmtCount          = 0;                        // Initialized first time it is needed
    hstmtInsert         = 0;
    hstmtDelete         = 0;
    hstmtUpdate         = 0;
    hstmtInternal       = 0;
    colDefs             = 0;
    tableID             = 0;
    m_numCols           = numColumns;               // Number of columns in the table
    where.Empty();                                  // Where clause
    orderBy.Empty();                                // Order By clause
    from.Empty();                                   // From clause
    selectForUpdate     = false;                    // SELECT ... FOR UPDATE; Indicates whether to include the FOR UPDATE phrase
    queryOnly           = qryOnly;
    insertable          = true;
    tablePath.Empty();
    tableName.Empty();
    queryTableName.Empty();

    wxASSERT(tblName.length());
    wxASSERT(pDb);

    if (!pDb)
        return false;

    tableName = tblName;                        // Table Name
    if ((pDb->Dbms() == dbmsORACLE) ||
        (pDb->Dbms() == dbmsFIREBIRD) ||
        (pDb->Dbms() == dbmsINTERBASE))
        tableName = tableName.Upper();

    if (tblPath.length())
        tablePath = tblPath;                    // Table Path - used for dBase files
    else
        tablePath.Empty();

    if (qryTblName.length())                    // Name of the table/view to query
        queryTableName = qryTblName;
    else
        queryTableName = tblName;

    if ((pDb->Dbms() == dbmsORACLE) ||
        (pDb->Dbms() == dbmsFIREBIRD) ||
        (pDb->Dbms() == dbmsINTERBASE))
        queryTableName = queryTableName.Upper();

    pDb->incrementTableCount();

    wxString s;
    tableID = ++lastTableID;
    s.Printf(wxT("wxDbTable constructor (%-20s) tableID:[%6lu] pDb:[%p]"),
             tblName.c_str(), tableID, wx_static_cast(void*, pDb));

#ifdef __WXDEBUG__
    wxTablesInUse *tableInUse;
    tableInUse            = new wxTablesInUse();
    tableInUse->tableName = tblName;
    tableInUse->tableID   = tableID;
    tableInUse->pDb       = pDb;
    {
#if wxUSE_THREADS
        wxCriticalSectionLocker lock(csTablesInUse);
#endif // wxUSE_THREADS
        TablesInUse.Append(tableInUse);
    }
#endif

    pDb->WriteSqlLog(s);

    // Grab the HENV and HDBC from the wxDb object
    henv = pDb->GetHENV();
    hdbc = pDb->GetHDBC();

    // Allocate space for column definitions
    if (m_numCols)
        colDefs = new wxDbColDef[m_numCols];  // Points to the first column definition

    // Allocate statement handles for the table
    if (!queryOnly)
    {
        // Allocate a separate statement handle for performing inserts
        if (SQLAllocStmt(hdbc, &hstmtInsert) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc);
        // Allocate a separate statement handle for performing deletes
        if (SQLAllocStmt(hdbc, &hstmtDelete) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc);
        // Allocate a separate statement handle for performing updates
        if (SQLAllocStmt(hdbc, &hstmtUpdate) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc);
    }
    // Allocate a separate statement handle for internal use
    if (SQLAllocStmt(hdbc, &hstmtInternal) != SQL_SUCCESS)
        pDb->DispAllErrors(henv, hdbc);

    // Set the cursor type for the statement handles
    cursorType = SQL_CURSOR_STATIC;

    if (SQLSetStmtOption(hstmtInternal, SQL_CURSOR_TYPE, cursorType) != SQL_SUCCESS)
    {
        // Check to see if cursor type is supported
        pDb->GetNextError(henv, hdbc, hstmtInternal);
        if (! wxStrcmp(pDb->sqlState, wxT("01S02")))  // Option Value Changed
        {
            // Datasource does not support static cursors.  Driver
            // will substitute a cursor type.  Call SQLGetStmtOption()
            // to determine which cursor type was selected.
            if (SQLGetStmtOption(hstmtInternal, SQL_CURSOR_TYPE, &cursorType) != SQL_SUCCESS)
                pDb->DispAllErrors(henv, hdbc, hstmtInternal);
#ifdef DBDEBUG_CONSOLE
            cout << wxT("Static cursor changed to: ");
            switch(cursorType)
            {
            case SQL_CURSOR_FORWARD_ONLY:
                cout << wxT("Forward Only");
                break;
            case SQL_CURSOR_STATIC:
                cout << wxT("Static");
                break;
            case SQL_CURSOR_KEYSET_DRIVEN:
                cout << wxT("Keyset Driven");
                break;
            case SQL_CURSOR_DYNAMIC:
                cout << wxT("Dynamic");
                break;
            }
            cout << endl << endl;
#endif
            // BJO20000425
            if (pDb->FwdOnlyCursors() && cursorType != SQL_CURSOR_FORWARD_ONLY)
            {
                // Force the use of a forward only cursor...
                cursorType = SQL_CURSOR_FORWARD_ONLY;
                if (SQLSetStmtOption(hstmtInternal, SQL_CURSOR_TYPE, cursorType) != SQL_SUCCESS)
                {
                    // Should never happen
                    pDb->GetNextError(henv, hdbc, hstmtInternal);
                    return false;
                }
            }
        }
        else
        {
            pDb->DispNextError();
            pDb->DispAllErrors(henv, hdbc, hstmtInternal);
        }
    }
#ifdef DBDEBUG_CONSOLE
    else
        cout << wxT("Cursor Type set to STATIC") << endl << endl;
#endif

    if (!queryOnly)
    {
        // Set the cursor type for the INSERT statement handle
        if (SQLSetStmtOption(hstmtInsert, SQL_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc, hstmtInsert);
        // Set the cursor type for the DELETE statement handle
        if (SQLSetStmtOption(hstmtDelete, SQL_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc, hstmtDelete);
        // Set the cursor type for the UPDATE statement handle
        if (SQLSetStmtOption(hstmtUpdate, SQL_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc, hstmtUpdate);
    }

    // Make the default cursor the active cursor
    hstmtDefault = GetNewCursor(false,false);
    wxASSERT(hstmtDefault);
    hstmt = *hstmtDefault;

    return true;

}  // wxDbTable::initialize()


void wxDbTable::cleanup()
{
    wxString s;
    if (pDb)
    {
        s.Printf(wxT("wxDbTable destructor (%-20s) tableID:[%6lu] pDb:[%p]"),
                 tableName.c_str(), tableID, wx_static_cast(void*, pDb));
        pDb->WriteSqlLog(s);
    }

#ifdef __WXDEBUG__
    if (tableID)
    {
        bool found = false;

        wxList::compatibility_iterator pNode;
        {
#if wxUSE_THREADS
            wxCriticalSectionLocker lock(csTablesInUse);
#endif // wxUSE_THREADS
            pNode = TablesInUse.GetFirst();
            while (!found && pNode)
            {
                if (((wxTablesInUse *)pNode->GetData())->tableID == tableID)
                {
                    found = true;
                    delete (wxTablesInUse *)pNode->GetData();
                    TablesInUse.Erase(pNode);
                }
                else
                    pNode = pNode->GetNext();
            }
        }
        if (!found)
        {
            wxString msg;
            msg.Printf(wxT("Unable to find the tableID in the linked\nlist of tables in use.\n\n%s"),s.c_str());
            wxLogDebug (msg,wxT("NOTICE..."));
        }
    }
#endif

    // Decrement the wxDb table count
    if (pDb)
        pDb->decrementTableCount();

    // Delete memory allocated for column definitions
    if (colDefs)
        delete [] colDefs;

    // Free statement handles
    if (!queryOnly)
    {
        if (hstmtInsert)
        {
/*
ODBC 3.0 says to use this form
            if (SQLFreeHandle(*hstmtDel, SQL_DROP) != SQL_SUCCESS)
*/
            if (SQLFreeStmt(hstmtInsert, SQL_DROP) != SQL_SUCCESS)
                pDb->DispAllErrors(henv, hdbc);
        }

        if (hstmtDelete)
        {
/*
ODBC 3.0 says to use this form
            if (SQLFreeHandle(*hstmtDel, SQL_DROP) != SQL_SUCCESS)
*/
            if (SQLFreeStmt(hstmtDelete, SQL_DROP) != SQL_SUCCESS)
                pDb->DispAllErrors(henv, hdbc);
        }

        if (hstmtUpdate)
        {
/*
ODBC 3.0 says to use this form
            if (SQLFreeHandle(*hstmtDel, SQL_DROP) != SQL_SUCCESS)
*/
            if (SQLFreeStmt(hstmtUpdate, SQL_DROP) != SQL_SUCCESS)
                pDb->DispAllErrors(henv, hdbc);
        }
    }

    if (hstmtInternal)
    {
        if (SQLFreeStmt(hstmtInternal, SQL_DROP) != SQL_SUCCESS)
            pDb->DispAllErrors(henv, hdbc);
    }

    // Delete dynamically allocated cursors
    if (hstmtDefault)
        DeleteCursor(hstmtDefault);

    if (hstmtCount)
        DeleteCursor(hstmtCount);

    if (m_hstmtGridQuery)
        DeleteCursor(m_hstmtGridQuery);

}  // wxDbTable::cleanup()


/***************************** PRIVATE FUNCTIONS *****************************/


void wxDbTable::setCbValueForColumn(int columnIndex)
{
    switch(colDefs[columnIndex].DbDataType)
    {
        case DB_DATA_TYPE_VARCHAR:
        case DB_DATA_TYPE_MEMO:
            if (colDefs[columnIndex].Null)
                colDefs[columnIndex].CbValue = SQL_NULL_DATA;
            else
                colDefs[columnIndex].CbValue = SQL_NTS;
            break;
        case DB_DATA_TYPE_INTEGER:
            if (colDefs[columnIndex].Null)
                colDefs[columnIndex].CbValue = SQL_NULL_DATA;
            else
                colDefs[columnIndex].CbValue = 0;
            break;
        case DB_DATA_TYPE_FLOAT:
            if (colDefs[columnIndex].Null)
                colDefs[columnIndex].CbValue = SQL_NULL_DATA;
            else
                colDefs[columnIndex].CbValue = 0;
            break;
        case DB_DATA_TYPE_DATE:
            if (colDefs[columnIndex].Null)
                colDefs[columnIndex].CbValue = SQL_NULL_DATA;
            else
                colDefs[columnIndex].CbValue = 0;
            break;
        case DB_DATA_TYPE_BLOB:
            if (colDefs[columnIndex].Null)
                colDefs[columnIndex].CbValue = SQL_NULL_DATA;
            else
                if (colDefs[columnIndex].SqlCtype == SQL_C_WXCHAR)
                    colDefs[columnIndex].CbValue = SQL_NTS;
                else
                    colDefs[columnIndex].CbValue = SQL_LEN_DATA_AT_EXEC(colDefs[columnIndex].SzDataObj);
            break;
    }
}

/********** wxDbTable::bindParams() **********/
bool wxDbTable::bindParams(bool forUpdate)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    SWORD   fSqlType    = 0;
    SDWORD  precision   = 0;
    SWORD   scale       = 0;

    // Bind each column of the table that should be bound
    // to a parameter marker
    int i;
    UWORD colNumber;

    for (i=0, colNumber=1; i < m_numCols; i++)
    {
        if (forUpdate)
        {
            if (!colDefs[i].Updateable)
                continue;
        }
        else
        {
            if (!colDefs[i].InsertAllowed)
                continue;
        }

        switch(colDefs[i].DbDataType)
        {
            case DB_DATA_TYPE_VARCHAR:
                fSqlType = pDb->GetTypeInfVarchar().FsqlType;
                precision = colDefs[i].SzDataObj;
                scale = 0;
                break;
            case DB_DATA_TYPE_MEMO:
                fSqlType = pDb->GetTypeInfMemo().FsqlType;
                precision = colDefs[i].SzDataObj;
                scale = 0;
                break;
            case DB_DATA_TYPE_INTEGER:
                fSqlType = pDb->GetTypeInfInteger().FsqlType;
                precision = pDb->GetTypeInfInteger().Precision;
                scale = 0;
                break;
            case DB_DATA_TYPE_FLOAT:
                fSqlType = pDb->GetTypeInfFloat().FsqlType;
                precision = pDb->GetTypeInfFloat().Precision;
                scale = pDb->GetTypeInfFloat().MaximumScale;
                // SQL Sybase Anywhere v5.5 returned a negative number for the
                // MaxScale.  This caused ODBC to kick out an error on ibscale.
                // I check for this here and set the scale = precision.
                //if (scale < 0)
                // scale = (short) precision;
                break;
            case DB_DATA_TYPE_DATE:
                fSqlType = pDb->GetTypeInfDate().FsqlType;
                precision = pDb->GetTypeInfDate().Precision;
                scale = 0;
                break;
            case DB_DATA_TYPE_BLOB:
                fSqlType = pDb->GetTypeInfBlob().FsqlType;
                precision = colDefs[i].SzDataObj;
                scale = 0;
                break;
        }

        setCbValueForColumn(i);

        if (forUpdate)
        {
            if (SQLBindParameter(hstmtUpdate, colNumber++, SQL_PARAM_INPUT, colDefs[i].SqlCtype,
                                 fSqlType, precision, scale, (UCHAR*) colDefs[i].PtrDataObj,
                                 precision+1, &colDefs[i].CbValue) != SQL_SUCCESS)
            {
                return(pDb->DispAllErrors(henv, hdbc, hstmtUpdate));
            }
        }
        else
        {
            if (SQLBindParameter(hstmtInsert, colNumber++, SQL_PARAM_INPUT, colDefs[i].SqlCtype,
                                 fSqlType, precision, scale, (UCHAR*) colDefs[i].PtrDataObj,
                                 precision+1, &colDefs[i].CbValue) != SQL_SUCCESS)
            {
                return(pDb->DispAllErrors(henv, hdbc, hstmtInsert));
            }
        }
    }

    // Completed successfully
    return true;

}  // wxDbTable::bindParams()


/********** wxDbTable::bindInsertParams() **********/
bool wxDbTable::bindInsertParams(void)
{
    return bindParams(false);
}  // wxDbTable::bindInsertParams()


/********** wxDbTable::bindUpdateParams() **********/
bool wxDbTable::bindUpdateParams(void)
{
    return bindParams(true);
}  // wxDbTable::bindUpdateParams()


/********** wxDbTable::bindCols() **********/
bool wxDbTable::bindCols(HSTMT cursor)
{
    // Bind each column of the table to a memory address for fetching data
    UWORD i;
    for (i = 0; i < m_numCols; i++)
    {
        if (SQLBindCol(cursor, (UWORD)(i+1), colDefs[i].SqlCtype, (UCHAR*) colDefs[i].PtrDataObj,
                       colDefs[i].SzDataObj, &colDefs[i].CbValue ) != SQL_SUCCESS)
          return (pDb->DispAllErrors(henv, hdbc, cursor));
    }

    // Completed successfully
    return true;
}  // wxDbTable::bindCols()


/********** wxDbTable::getRec() **********/
bool wxDbTable::getRec(UWORD fetchType)
{
    RETCODE retcode;

    if (!pDb->FwdOnlyCursors())
    {
        // Fetch the NEXT, PREV, FIRST or LAST record, depending on fetchType
        SQLULEN cRowsFetched;
        UWORD   rowStatus;

        retcode = SQLExtendedFetch(hstmt, fetchType, 0, &cRowsFetched, &rowStatus);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        {
            if (retcode == SQL_NO_DATA_FOUND)
                return false;
            else
                return(pDb->DispAllErrors(henv, hdbc, hstmt));
        }
        else
        {
            // Set the Null member variable to indicate the Null state
            // of each column just read in.
            int i;
            for (i = 0; i < m_numCols; i++)
                colDefs[i].Null = (colDefs[i].CbValue == SQL_NULL_DATA);
        }
    }
    else
    {
        // Fetch the next record from the record set
        retcode = SQLFetch(hstmt);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        {
            if (retcode == SQL_NO_DATA_FOUND)
                return false;
            else
                return(pDb->DispAllErrors(henv, hdbc, hstmt));
        }
        else
        {
            // Set the Null member variable to indicate the Null state
            // of each column just read in.
            int i;
            for (i = 0; i < m_numCols; i++)
                colDefs[i].Null = (colDefs[i].CbValue == SQL_NULL_DATA);
        }
    }

    // Completed successfully
    return true;

}  // wxDbTable::getRec()


/********** wxDbTable::execDelete() **********/
bool wxDbTable::execDelete(const wxString &pSqlStmt)
{
    RETCODE retcode;

    // Execute the DELETE statement
    retcode = SQLExecDirect(hstmtDelete, (SQLTCHAR FAR *) pSqlStmt.c_str(), SQL_NTS);

    if (retcode == SQL_SUCCESS ||
        retcode == SQL_NO_DATA_FOUND ||
        retcode == SQL_SUCCESS_WITH_INFO)
    {
        // Record deleted successfully
        return true;
    }

    // Problem deleting record
    return(pDb->DispAllErrors(henv, hdbc, hstmtDelete));

}  // wxDbTable::execDelete()


/********** wxDbTable::execUpdate() **********/
bool wxDbTable::execUpdate(const wxString &pSqlStmt)
{
    RETCODE retcode;

    // Execute the UPDATE statement
    retcode = SQLExecDirect(hstmtUpdate, (SQLTCHAR FAR *) pSqlStmt.c_str(), SQL_NTS);

    if (retcode == SQL_SUCCESS ||
        retcode == SQL_NO_DATA_FOUND ||
        retcode == SQL_SUCCESS_WITH_INFO)
    {
        // Record updated successfully
        return true;
    }
    else if (retcode == SQL_NEED_DATA)
    {
        PTR pParmID;
        retcode = SQLParamData(hstmtUpdate, &pParmID);
        while (retcode == SQL_NEED_DATA)
        {
            // Find the parameter
            int i;
            for (i=0; i < m_numCols; i++)
            {
                if (colDefs[i].PtrDataObj == pParmID)
                {
                    // We found it.  Store the parameter.
                    retcode = SQLPutData(hstmtUpdate, pParmID, colDefs[i].SzDataObj);
                    if (retcode != SQL_SUCCESS)
                    {
                        pDb->DispNextError();
                        return pDb->DispAllErrors(henv, hdbc, hstmtUpdate);
                    }
                    break;
                }
            }
            retcode = SQLParamData(hstmtUpdate, &pParmID);
        }
        if (retcode == SQL_SUCCESS ||
            retcode == SQL_NO_DATA_FOUND ||
            retcode == SQL_SUCCESS_WITH_INFO)
        {
            // Record updated successfully
            return true;
        }
    }

    // Problem updating record
    return(pDb->DispAllErrors(henv, hdbc, hstmtUpdate));

}  // wxDbTable::execUpdate()


/********** wxDbTable::query() **********/
bool wxDbTable::query(int queryType, bool forUpdate, bool distinct, const wxString &pSqlStmt)
{
    wxString sqlStmt;

    if (forUpdate)
        // The user may wish to select for update, but the DBMS may not be capable
        selectForUpdate = CanSelectForUpdate();
    else
        selectForUpdate = false;

    // Set the SQL SELECT string
    if (queryType != DB_SELECT_STATEMENT)               // A select statement was not passed in,
    {                                                   // so generate a select statement.
        BuildSelectStmt(sqlStmt, queryType, distinct);
        pDb->WriteSqlLog(sqlStmt);
    }

    // Make sure the cursor is closed first
    if (!CloseCursor(hstmt))
        return false;

    // Execute the SQL SELECT statement
    int retcode;
    retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) (queryType == DB_SELECT_STATEMENT ? pSqlStmt.c_str() : sqlStmt.c_str()), SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        return(pDb->DispAllErrors(henv, hdbc, hstmt));

    // Completed successfully
    return true;

}  // wxDbTable::query()


/***************************** PUBLIC FUNCTIONS *****************************/


/********** wxDbTable::Open() **********/
bool wxDbTable::Open(bool checkPrivileges, bool checkTableExists)
{
    if (!pDb)
        return false;

    int i;
    wxString sqlStmt;
    wxString s;

    // Calculate the maximum size of the concatenated
    // keys for use with wxDbGrid
    m_keysize = 0;
    for (i=0; i < m_numCols; i++)
    {
        if (colDefs[i].KeyField)
        {
            m_keysize += colDefs[i].SzDataObj;
        }
    }

    s.Empty();

    bool exists = true;
    if (checkTableExists)
    {
        if (pDb->Dbms() == dbmsPOSTGRES)
            exists = pDb->TableExists(tableName, NULL, tablePath);
        else
            exists = pDb->TableExists(tableName, pDb->GetUsername(), tablePath);
    }

    // Verify that the table exists in the database
    if (!exists)
    {
        s = wxT("Table/view does not exist in the database");
        if ( *(pDb->dbInf.accessibleTables) == wxT('Y'))
            s += wxT(", or you have no permissions.\n");
        else
            s += wxT(".\n");
    }
    else if (checkPrivileges)
    {
        // Verify the user has rights to access the table.
        bool hasPrivs wxDUMMY_INITIALIZE(true);

        if (pDb->Dbms() == dbmsPOSTGRES)
            hasPrivs = pDb->TablePrivileges(tableName, wxT("SELECT"), pDb->GetUsername(), NULL, tablePath);
        else
            hasPrivs = pDb->TablePrivileges(tableName, wxT("SELECT"), pDb->GetUsername(), pDb->GetUsername(), tablePath);

        if (!hasPrivs)
            s = wxT("Connecting user does not have sufficient privileges to access this table.\n");
    }

    if (!s.empty())
    {
        wxString p;

        if (!tablePath.empty())
            p.Printf(wxT("Error opening '%s/%s'.\n"),tablePath.c_str(),tableName.c_str());
        else
            p.Printf(wxT("Error opening '%s'.\n"), tableName.c_str());

        p += s;
        pDb->LogError(p.GetData());

        return false;
    }

    // Bind the member variables for field exchange between
    // the wxDbTable object and the ODBC record.
    if (!queryOnly)
    {
        if (!bindInsertParams())                    // Inserts
            return false;

        if (!bindUpdateParams())                    // Updates
            return false;
    }

    if (!bindCols(*hstmtDefault))                   // Selects
        return false;

    if (!bindCols(hstmtInternal))                   // Internal use only
        return false;

     /*
     * Do NOT bind the hstmtCount cursor!!!
     */

    // Build an insert statement using parameter markers
    if (!queryOnly && m_numCols > 0)
    {
        bool needComma = false;
        sqlStmt.Printf(wxT("INSERT INTO %s ("),
                       pDb->SQLTableName(tableName.c_str()).c_str());
        for (i = 0; i < m_numCols; i++)
        {
            if (! colDefs[i].InsertAllowed)
                continue;
            if (needComma)
                sqlStmt += wxT(",");
            sqlStmt += pDb->SQLColumnName(colDefs[i].ColName);
            needComma = true;
        }
        needComma = false;
        sqlStmt += wxT(") VALUES (");

        int insertableCount = 0;

        for (i = 0; i < m_numCols; i++)
        {
            if (! colDefs[i].InsertAllowed)
                continue;
            if (needComma)
                sqlStmt += wxT(",");
            sqlStmt += wxT("?");
            needComma = true;
            insertableCount++;
        }
        sqlStmt += wxT(")");

        // Prepare the insert statement for execution
        if (insertableCount)
        {
            if (SQLPrepare(hstmtInsert, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS) != SQL_SUCCESS)
                return(pDb->DispAllErrors(henv, hdbc, hstmtInsert));
        }
        else
            insertable = false;
    }

    // Completed successfully
    return true;

}  // wxDbTable::Open()


/********** wxDbTable::Query() **********/
bool wxDbTable::Query(bool forUpdate, bool distinct)
{

    return(query(DB_SELECT_WHERE, forUpdate, distinct));

}  // wxDbTable::Query()


/********** wxDbTable::QueryBySqlStmt() **********/
bool wxDbTable::QueryBySqlStmt(const wxString &pSqlStmt)
{
    pDb->WriteSqlLog(pSqlStmt);

    return(query(DB_SELECT_STATEMENT, false, false, pSqlStmt));

}  // wxDbTable::QueryBySqlStmt()


/********** wxDbTable::QueryMatching() **********/
bool wxDbTable::QueryMatching(bool forUpdate, bool distinct)
{

    return(query(DB_SELECT_MATCHING, forUpdate, distinct));

}  // wxDbTable::QueryMatching()


/********** wxDbTable::QueryOnKeyFields() **********/
bool wxDbTable::QueryOnKeyFields(bool forUpdate, bool distinct)
{

    return(query(DB_SELECT_KEYFIELDS, forUpdate, distinct));

}  // wxDbTable::QueryOnKeyFields()


/********** wxDbTable::GetPrev() **********/
bool wxDbTable::GetPrev(void)
{
    if (pDb->FwdOnlyCursors())
    {
        wxFAIL_MSG(wxT("GetPrev()::Backward scrolling cursors are not enabled for this instance of wxDbTable"));
        return false;
    }
    else
        return(getRec(SQL_FETCH_PRIOR));

}  // wxDbTable::GetPrev()


/********** wxDbTable::operator-- **********/
bool wxDbTable::operator--(int)
{
    if (pDb->FwdOnlyCursors())
    {
        wxFAIL_MSG(wxT("operator--:Backward scrolling cursors are not enabled for this instance of wxDbTable"));
        return false;
    }
    else
        return(getRec(SQL_FETCH_PRIOR));

}  // wxDbTable::operator--


/********** wxDbTable::GetFirst() **********/
bool wxDbTable::GetFirst(void)
{
    if (pDb->FwdOnlyCursors())
    {
        wxFAIL_MSG(wxT("GetFirst():Backward scrolling cursors are not enabled for this instance of wxDbTable"));
        return false;
    }
    else
        return(getRec(SQL_FETCH_FIRST));

}  // wxDbTable::GetFirst()


/********** wxDbTable::GetLast() **********/
bool wxDbTable::GetLast(void)
{
    if (pDb->FwdOnlyCursors())
    {
        wxFAIL_MSG(wxT("GetLast()::Backward scrolling cursors are not enabled for this instance of wxDbTable"));
        return false;
    }
    else
        return(getRec(SQL_FETCH_LAST));

}  // wxDbTable::GetLast()


/********** wxDbTable::BuildDeleteStmt() **********/
void wxDbTable::BuildDeleteStmt(wxString &pSqlStmt, int typeOfDel, const wxString &pWhereClause)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return;

    wxString whereClause;

    whereClause.Empty();

    // Handle the case of DeleteWhere() and the where clause is blank.  It should
    // delete all records from the database in this case.
    if (typeOfDel == DB_DEL_WHERE && (pWhereClause.length() == 0))
    {
        pSqlStmt.Printf(wxT("DELETE FROM %s"),
                        pDb->SQLTableName(tableName.c_str()).c_str());
        return;
    }

    pSqlStmt.Printf(wxT("DELETE FROM %s WHERE "),
                    pDb->SQLTableName(tableName.c_str()).c_str());

    // Append the WHERE clause to the SQL DELETE statement
    switch(typeOfDel)
    {
        case DB_DEL_KEYFIELDS:
            // If the datasource supports the ROWID column, build
            // the where on ROWID for efficiency purposes.
            // e.g. DELETE FROM PARTS WHERE ROWID = '111.222.333'
            if (CanUpdateByROWID())
            {
                SQLLEN cb;
                wxChar   rowid[wxDB_ROWID_LEN+1];

                // Get the ROWID value.  If not successful retreiving the ROWID,
                // simply fall down through the code and build the WHERE clause
                // based on the key fields.
                if (SQLGetData(hstmt, (UWORD)(m_numCols+1), SQL_C_WXCHAR, (UCHAR*) rowid, sizeof(rowid), &cb) == SQL_SUCCESS)
                {
                    pSqlStmt += wxT("ROWID = '");
                    pSqlStmt += rowid;
                    pSqlStmt += wxT("'");
                    break;
                }
            }
            // Unable to delete by ROWID, so build a WHERE
            // clause based on the keyfields.
            BuildWhereClause(whereClause, DB_WHERE_KEYFIELDS);
            pSqlStmt += whereClause;
            break;
        case DB_DEL_WHERE:
            pSqlStmt += pWhereClause;
            break;
        case DB_DEL_MATCHING:
            BuildWhereClause(whereClause, DB_WHERE_MATCHING);
            pSqlStmt += whereClause;
            break;
    }

}  // BuildDeleteStmt()


/***** DEPRECATED: use wxDbTable::BuildDeleteStmt(wxString &....) form *****/
void wxDbTable::BuildDeleteStmt(wxChar *pSqlStmt, int typeOfDel, const wxString &pWhereClause)
{
    wxString tempSqlStmt;
    BuildDeleteStmt(tempSqlStmt, typeOfDel, pWhereClause);
    wxStrcpy(pSqlStmt, tempSqlStmt);
}  // wxDbTable::BuildDeleteStmt()


/********** wxDbTable::BuildSelectStmt() **********/
void wxDbTable::BuildSelectStmt(wxString &pSqlStmt, int typeOfSelect, bool distinct)
{
    wxString whereClause;
    whereClause.Empty();

    // Build a select statement to query the database
    pSqlStmt = wxT("SELECT ");

    // SELECT DISTINCT values only?
    if (distinct)
        pSqlStmt += wxT("DISTINCT ");

    // Was a FROM clause specified to join tables to the base table?
    // Available for ::Query() only!!!
    bool appendFromClause = false;
#if wxODBC_BACKWARD_COMPATABILITY
    if (typeOfSelect == DB_SELECT_WHERE && from && wxStrlen(from))
        appendFromClause = true;
#else
    if (typeOfSelect == DB_SELECT_WHERE && from.length())
        appendFromClause = true;
#endif

    // Add the column list
    int i;
    wxString tStr;
    for (i = 0; i < m_numCols; i++)
    {
        tStr = colDefs[i].ColName;
        // If joining tables, the base table column names must be qualified to avoid ambiguity
        if ((appendFromClause || pDb->Dbms() == dbmsACCESS) && tStr.Find(wxT('.')) == wxNOT_FOUND)
        {
            pSqlStmt += pDb->SQLTableName(queryTableName.c_str());
            pSqlStmt += wxT(".");
        }
        pSqlStmt += pDb->SQLColumnName(colDefs[i].ColName);
        if (i + 1 < m_numCols)
            pSqlStmt += wxT(",");
    }

    // If the datasource supports ROWID, get this column as well.  Exception: Don't retrieve
    // the ROWID if querying distinct records.  The rowid will always be unique.
    if (!distinct && CanUpdateByROWID())
    {
        // If joining tables, the base table column names must be qualified to avoid ambiguity
        if (appendFromClause || pDb->Dbms() == dbmsACCESS)
        {
            pSqlStmt += wxT(",");
            pSqlStmt += pDb->SQLTableName(queryTableName);
            pSqlStmt += wxT(".ROWID");
        }
        else
            pSqlStmt += wxT(",ROWID");
    }

    // Append the FROM tablename portion
    pSqlStmt += wxT(" FROM ");
    pSqlStmt += pDb->SQLTableName(queryTableName);
//    pSqlStmt += queryTableName;

    // Sybase uses the HOLDLOCK keyword to lock a record during query.
    // The HOLDLOCK keyword follows the table name in the from clause.
    // Each table in the from clause must specify HOLDLOCK or
    // NOHOLDLOCK (the default).  Note: The "FOR UPDATE" clause
    // is parsed but ignored in SYBASE Transact-SQL.
    if (selectForUpdate && (pDb->Dbms() == dbmsSYBASE_ASA || pDb->Dbms() == dbmsSYBASE_ASE))
        pSqlStmt += wxT(" HOLDLOCK");

    if (appendFromClause)
        pSqlStmt += from;

    // Append the WHERE clause.  Either append the where clause for the class
    // or build a where clause.  The typeOfSelect determines this.
    switch(typeOfSelect)
    {
        case DB_SELECT_WHERE:
#if wxODBC_BACKWARD_COMPATABILITY
            if (where && wxStrlen(where))   // May not want a where clause!!!
#else
            if (where.length())   // May not want a where clause!!!
#endif
            {
                pSqlStmt += wxT(" WHERE ");
                pSqlStmt += where;
            }
            break;
        case DB_SELECT_KEYFIELDS:
            BuildWhereClause(whereClause, DB_WHERE_KEYFIELDS);
            if (whereClause.length())
            {
                pSqlStmt += wxT(" WHERE ");
                pSqlStmt += whereClause;
            }
            break;
        case DB_SELECT_MATCHING:
            BuildWhereClause(whereClause, DB_WHERE_MATCHING);
            if (whereClause.length())
            {
                pSqlStmt += wxT(" WHERE ");
                pSqlStmt += whereClause;
            }
            break;
    }

    // Append the ORDER BY clause
#if wxODBC_BACKWARD_COMPATABILITY
    if (orderBy && wxStrlen(orderBy))
#else
    if (orderBy.length())
#endif
    {
        pSqlStmt += wxT(" ORDER BY ");
        pSqlStmt += orderBy;
    }

    // SELECT FOR UPDATE if told to do so and the datasource is capable.  Sybase
    // parses the FOR UPDATE clause but ignores it.  See the comment above on the
    // HOLDLOCK for Sybase.
    if (selectForUpdate && CanSelectForUpdate())
        pSqlStmt += wxT(" FOR UPDATE");

}  // wxDbTable::BuildSelectStmt()


/***** DEPRECATED: use wxDbTable::BuildSelectStmt(wxString &....) form *****/
void wxDbTable::BuildSelectStmt(wxChar *pSqlStmt, int typeOfSelect, bool distinct)
{
    wxString tempSqlStmt;
    BuildSelectStmt(tempSqlStmt, typeOfSelect, distinct);
    wxStrcpy(pSqlStmt, tempSqlStmt);
}  // wxDbTable::BuildSelectStmt()


/********** wxDbTable::BuildUpdateStmt() **********/
void wxDbTable::BuildUpdateStmt(wxString &pSqlStmt, int typeOfUpdate, const wxString &pWhereClause)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return;

    wxString whereClause;
    whereClause.Empty();

    bool firstColumn = true;

    pSqlStmt.Printf(wxT("UPDATE %s SET "),
                    pDb->SQLTableName(tableName.c_str()).c_str());

    // Append a list of columns to be updated
    int i;
    for (i = 0; i < m_numCols; i++)
    {
        // Only append Updateable columns
        if (colDefs[i].Updateable)
        {
            if (!firstColumn)
                pSqlStmt += wxT(",");
            else
                firstColumn = false;

            pSqlStmt += pDb->SQLColumnName(colDefs[i].ColName);
//            pSqlStmt += colDefs[i].ColName;
            pSqlStmt += wxT(" = ?");
        }
    }

    // Append the WHERE clause to the SQL UPDATE statement
    pSqlStmt += wxT(" WHERE ");
    switch(typeOfUpdate)
    {
        case DB_UPD_KEYFIELDS:
            // If the datasource supports the ROWID column, build
            // the where on ROWID for efficiency purposes.
            // e.g. UPDATE PARTS SET Col1 = ?, Col2 = ? WHERE ROWID = '111.222.333'
            if (CanUpdateByROWID())
            {
                SQLLEN cb;
                wxChar rowid[wxDB_ROWID_LEN+1];

                // Get the ROWID value.  If not successful retreiving the ROWID,
                // simply fall down through the code and build the WHERE clause
                // based on the key fields.
                if (SQLGetData(hstmt, (UWORD)(m_numCols+1), SQL_C_WXCHAR, (UCHAR*) rowid, sizeof(rowid), &cb) == SQL_SUCCESS)
                {
                    pSqlStmt += wxT("ROWID = '");
                    pSqlStmt += rowid;
                    pSqlStmt += wxT("'");
                    break;
                }
            }
            // Unable to delete by ROWID, so build a WHERE
            // clause based on the keyfields.
            BuildWhereClause(whereClause, DB_WHERE_KEYFIELDS);
            pSqlStmt += whereClause;
            break;
        case DB_UPD_WHERE:
            pSqlStmt += pWhereClause;
            break;
    }
}  // BuildUpdateStmt()


/***** DEPRECATED: use wxDbTable::BuildUpdateStmt(wxString &....) form *****/
void wxDbTable::BuildUpdateStmt(wxChar *pSqlStmt, int typeOfUpdate, const wxString &pWhereClause)
{
    wxString tempSqlStmt;
    BuildUpdateStmt(tempSqlStmt, typeOfUpdate, pWhereClause);
    wxStrcpy(pSqlStmt, tempSqlStmt);
}  // BuildUpdateStmt()


/********** wxDbTable::BuildWhereClause() **********/
void wxDbTable::BuildWhereClause(wxString &pWhereClause, int typeOfWhere,
                                 const wxString &qualTableName, bool useLikeComparison)
/*
 * Note: BuildWhereClause() currently ignores timestamp columns.
 *       They are not included as part of the where clause.
 */
{
    bool moreThanOneColumn = false;
    wxString colValue;

    // Loop through the columns building a where clause as you go
    int colNumber;
    for (colNumber = 0; colNumber < m_numCols; colNumber++)
    {
        // Determine if this column should be included in the WHERE clause
        if ((typeOfWhere == DB_WHERE_KEYFIELDS && colDefs[colNumber].KeyField) ||
             (typeOfWhere == DB_WHERE_MATCHING  && (!IsColNull((UWORD)colNumber))))
        {
            // Skip over timestamp columns
            if (colDefs[colNumber].SqlCtype == SQL_C_TIMESTAMP)
                continue;
            // If there is more than 1 column, join them with the keyword "AND"
            if (moreThanOneColumn)
                pWhereClause += wxT(" AND ");
            else
                moreThanOneColumn = true;

            // Concatenate where phrase for the column
            wxString tStr = colDefs[colNumber].ColName;

            if (qualTableName.length() && tStr.Find(wxT('.')) == wxNOT_FOUND)
            {
                pWhereClause += pDb->SQLTableName(qualTableName);
                pWhereClause += wxT(".");
            }
            pWhereClause += pDb->SQLColumnName(colDefs[colNumber].ColName);

            if (useLikeComparison && (colDefs[colNumber].SqlCtype == SQL_C_WXCHAR))
                pWhereClause += wxT(" LIKE ");
            else
                pWhereClause += wxT(" = ");

            switch(colDefs[colNumber].SqlCtype)
            {
                case SQL_C_CHAR:
#ifdef SQL_C_WCHAR
                case SQL_C_WCHAR:
#endif
                //case SQL_C_WXCHAR:  SQL_C_WXCHAR is covered by either SQL_C_CHAR or SQL_C_WCHAR
                    colValue.Printf(wxT("'%s'"), GetDb()->EscapeSqlChars((wxChar *)colDefs[colNumber].PtrDataObj).c_str());
                    break;
                case SQL_C_SHORT:
                case SQL_C_SSHORT:
                    colValue.Printf(wxT("%hi"), *((SWORD *) colDefs[colNumber].PtrDataObj));
                    break;
                case SQL_C_USHORT:
                    colValue.Printf(wxT("%hu"), *((UWORD *) colDefs[colNumber].PtrDataObj));
                    break;
                case SQL_C_LONG:
                case SQL_C_SLONG:
                    colValue.Printf(wxT("%li"), *((SDWORD *) colDefs[colNumber].PtrDataObj));
                    break;
                case SQL_C_ULONG:
                    colValue.Printf(wxT("%lu"), *((UDWORD *) colDefs[colNumber].PtrDataObj));
                    break;
                case SQL_C_FLOAT:
                    colValue.Printf(wxT("%.6f"), *((SFLOAT *) colDefs[colNumber].PtrDataObj));
                    break;
                case SQL_C_DOUBLE:
                    colValue.Printf(wxT("%.6f"), *((SDOUBLE *) colDefs[colNumber].PtrDataObj));
                    break;
                default:
                    {
                        wxString strMsg;
                        strMsg.Printf(wxT("wxDbTable::bindParams(): Unknown column type for colDefs %d colName %s"),
                                    colNumber,colDefs[colNumber].ColName);
                        wxFAIL_MSG(strMsg.c_str());
                    }
                    break;
            }
            pWhereClause += colValue;
        }
    }
}  // wxDbTable::BuildWhereClause()


/***** DEPRECATED: use wxDbTable::BuildWhereClause(wxString &....) form *****/
void wxDbTable::BuildWhereClause(wxChar *pWhereClause, int typeOfWhere,
                                 const wxString &qualTableName, bool useLikeComparison)
{
    wxString tempSqlStmt;
    BuildWhereClause(tempSqlStmt, typeOfWhere, qualTableName, useLikeComparison);
    wxStrcpy(pWhereClause, tempSqlStmt);
}  // wxDbTable::BuildWhereClause()


/********** wxDbTable::GetRowNum() **********/
UWORD wxDbTable::GetRowNum(void)
{
    UDWORD rowNum;

    if (SQLGetStmtOption(hstmt, SQL_ROW_NUMBER, (UCHAR*) &rowNum) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, hstmt);
        return(0);
    }

    // Completed successfully
    return((UWORD) rowNum);

}  // wxDbTable::GetRowNum()


/********** wxDbTable::CloseCursor() **********/
bool wxDbTable::CloseCursor(HSTMT cursor)
{
    if (SQLFreeStmt(cursor, SQL_CLOSE) != SQL_SUCCESS)
        return(pDb->DispAllErrors(henv, hdbc, cursor));

    // Completed successfully
    return true;

}  // wxDbTable::CloseCursor()


/********** wxDbTable::CreateTable() **********/
bool wxDbTable::CreateTable(bool attemptDrop)
{
    if (!pDb)
        return false;

    int i, j;
    wxString sqlStmt;

#ifdef DBDEBUG_CONSOLE
    cout << wxT("Creating Table ") << tableName << wxT("...") << endl;
#endif

    // Drop table first
    if (attemptDrop && !DropTable())
        return false;

    // Create the table
#ifdef DBDEBUG_CONSOLE
    for (i = 0; i < m_numCols; i++)
    {
        // Exclude derived columns since they are NOT part of the base table
        if (colDefs[i].DerivedCol)
            continue;
        cout << i + 1 << wxT(": ") << colDefs[i].ColName << wxT("; ");
        switch(colDefs[i].DbDataType)
        {
            case DB_DATA_TYPE_VARCHAR:
                cout << pDb->GetTypeInfVarchar().TypeName << wxT("(") << (int)(colDefs[i].SzDataObj / sizeof(wxChar)) << wxT(")");
                break;
            case DB_DATA_TYPE_MEMO:
                cout << pDb->GetTypeInfMemo().TypeName;
                break;
            case DB_DATA_TYPE_INTEGER:
                cout << pDb->GetTypeInfInteger().TypeName;
                break;
            case DB_DATA_TYPE_FLOAT:
                cout << pDb->GetTypeInfFloat().TypeName;
                break;
            case DB_DATA_TYPE_DATE:
                cout << pDb->GetTypeInfDate().TypeName;
                break;
            case DB_DATA_TYPE_BLOB:
                cout << pDb->GetTypeInfBlob().TypeName;
                break;
        }
        cout << endl;
    }
#endif

    // Build a CREATE TABLE string from the colDefs structure.
    bool needComma = false;

    sqlStmt.Printf(wxT("CREATE TABLE %s ("),
                   pDb->SQLTableName(tableName.c_str()).c_str());

    for (i = 0; i < m_numCols; i++)
    {
        // Exclude derived columns since they are NOT part of the base table
        if (colDefs[i].DerivedCol)
            continue;
        // Comma Delimiter
        if (needComma)
            sqlStmt += wxT(",");
        // Column Name
        sqlStmt += pDb->SQLColumnName(colDefs[i].ColName);
//        sqlStmt += colDefs[i].ColName;
        sqlStmt += wxT(" ");
        // Column Type
        switch(colDefs[i].DbDataType)
        {
            case DB_DATA_TYPE_VARCHAR:
                sqlStmt += pDb->GetTypeInfVarchar().TypeName;
                break;
            case DB_DATA_TYPE_MEMO:
                sqlStmt += pDb->GetTypeInfMemo().TypeName;
                break;
            case DB_DATA_TYPE_INTEGER:
                sqlStmt += pDb->GetTypeInfInteger().TypeName;
                break;
            case DB_DATA_TYPE_FLOAT:
                sqlStmt += pDb->GetTypeInfFloat().TypeName;
                break;
            case DB_DATA_TYPE_DATE:
                sqlStmt += pDb->GetTypeInfDate().TypeName;
                break;
            case DB_DATA_TYPE_BLOB:
                sqlStmt += pDb->GetTypeInfBlob().TypeName;
                break;
        }
        // For varchars, append the size of the string
        if (colDefs[i].DbDataType == DB_DATA_TYPE_VARCHAR &&
            (pDb->Dbms() != dbmsMY_SQL || pDb->GetTypeInfVarchar().TypeName != _T("text")))// ||
//            colDefs[i].DbDataType == DB_DATA_TYPE_BLOB)
        {
            wxString s;
            s.Printf(wxT("(%d)"), (int)(colDefs[i].SzDataObj / sizeof(wxChar)));
            sqlStmt += s;
        }

        if (pDb->Dbms() == dbmsDB2 ||
            pDb->Dbms() == dbmsMY_SQL ||
            pDb->Dbms() == dbmsSYBASE_ASE  ||
            pDb->Dbms() == dbmsINTERBASE  ||
            pDb->Dbms() == dbmsFIREBIRD  ||
            pDb->Dbms() == dbmsMS_SQL_SERVER)
        {
            if (colDefs[i].KeyField)
            {
                sqlStmt += wxT(" NOT NULL");
            }
        }

        needComma = true;
    }
    // If there is a primary key defined, include it in the create statement
    for (i = j = 0; i < m_numCols; i++)
    {
        if (colDefs[i].KeyField)
        {
            j++;
            break;
        }
    }
    if ( j && (pDb->Dbms() != dbmsDBASE)
        && (pDb->Dbms() != dbmsXBASE_SEQUITER) )  // Found a keyfield
    {
        switch (pDb->Dbms())
        {
            case dbmsACCESS:
            case dbmsINFORMIX:
            case dbmsSYBASE_ASA:
            case dbmsSYBASE_ASE:
            case dbmsMY_SQL:
            case dbmsFIREBIRD:
            {
                // MySQL goes out on this one. We also declare the relevant key NON NULL above
                sqlStmt += wxT(",PRIMARY KEY (");
                break;
            }
            default:
            {
                sqlStmt += wxT(",CONSTRAINT ");
                //  DB2 is limited to 18 characters for index names
                if (pDb->Dbms() == dbmsDB2)
                {
                    wxASSERT_MSG((tableName && wxStrlen(tableName) <= 13), wxT("DB2 table/index names must be no longer than 13 characters in length.\n\nTruncating table name to 13 characters."));
                    sqlStmt += pDb->SQLTableName(tableName.substr(0, 13).c_str());
//                    sqlStmt += tableName.substr(0, 13);
                }
                else
                    sqlStmt += pDb->SQLTableName(tableName.c_str());
//                    sqlStmt += tableName;

                sqlStmt += wxT("_PIDX PRIMARY KEY (");
                break;
            }
        }

        // List column name(s) of column(s) comprising the primary key
        for (i = j = 0; i < m_numCols; i++)
        {
            if (colDefs[i].KeyField)
            {
                if (j++) // Multi part key, comma separate names
                    sqlStmt += wxT(",");
                sqlStmt += pDb->SQLColumnName(colDefs[i].ColName);

                if (pDb->Dbms() == dbmsMY_SQL &&
                    colDefs[i].DbDataType ==  DB_DATA_TYPE_VARCHAR)
                {
                    wxString s;
                    s.Printf(wxT("(%d)"), (int)(colDefs[i].SzDataObj / sizeof(wxChar)));
                    sqlStmt += s;
                }
            }
        }
        sqlStmt += wxT(")");

        if (pDb->Dbms() == dbmsINFORMIX ||
            pDb->Dbms() == dbmsSYBASE_ASA ||
            pDb->Dbms() == dbmsSYBASE_ASE)
        {
            sqlStmt += wxT(" CONSTRAINT ");
            sqlStmt += pDb->SQLTableName(tableName);
//            sqlStmt += tableName;
            sqlStmt += wxT("_PIDX");
        }
    }
    // Append the closing parentheses for the create table statement
    sqlStmt += wxT(")");

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl;
#endif

    // Execute the CREATE TABLE statement
    RETCODE retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        pDb->DispAllErrors(henv, hdbc, hstmt);
        pDb->RollbackTrans();
        CloseCursor(hstmt);
        return false;
    }

    // Commit the transaction and close the cursor
    if (!pDb->CommitTrans())
        return false;
    if (!CloseCursor(hstmt))
        return false;

    // Database table created successfully
    return true;

} // wxDbTable::CreateTable()


/********** wxDbTable::DropTable() **********/
bool wxDbTable::DropTable()
{
    // NOTE: This function returns true if the Table does not exist, but
    //       only for identified databases.  Code will need to be added
    //       below for any other databases when those databases are defined
    //       to handle this situation consistently

    wxString sqlStmt;

    sqlStmt.Printf(wxT("DROP TABLE %s"),
                   pDb->SQLTableName(tableName.c_str()).c_str());

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl;
#endif

    RETCODE retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS)
    {
        // Check for "Base table not found" error and ignore
        pDb->GetNextError(henv, hdbc, hstmt);
        if (wxStrcmp(pDb->sqlState, wxT("S0002")) /*&&
            wxStrcmp(pDb->sqlState, wxT("S1000"))*/)  // "Base table not found"
        {
            // Check for product specific error codes
            if (!((pDb->Dbms() == dbmsSYBASE_ASA    && !wxStrcmp(pDb->sqlState,wxT("42000")))   ||  // 5.x (and lower?)
                  (pDb->Dbms() == dbmsSYBASE_ASE    && !wxStrcmp(pDb->sqlState,wxT("37000")))   ||
                  (pDb->Dbms() == dbmsPERVASIVE_SQL && !wxStrcmp(pDb->sqlState,wxT("S1000")))   ||  // Returns an S1000 then an S0002
                  (pDb->Dbms() == dbmsPOSTGRES      && !wxStrcmp(pDb->sqlState,wxT("08S01")))))
            {
                pDb->DispNextError();
                pDb->DispAllErrors(henv, hdbc, hstmt);
                pDb->RollbackTrans();
//                CloseCursor(hstmt);
                return false;
            }
        }
    }

    // Commit the transaction and close the cursor
    if (! pDb->CommitTrans())
        return false;
    if (! CloseCursor(hstmt))
        return false;

    return true;
}  // wxDbTable::DropTable()


/********** wxDbTable::CreateIndex() **********/
bool wxDbTable::CreateIndex(const wxString &indexName, bool unique, UWORD numIndexColumns,
                                     wxDbIdxDef *pIndexDefs, bool attemptDrop)
{
    wxString sqlStmt;

    // Drop the index first
    if (attemptDrop && !DropIndex(indexName))
        return false;

    // MySQL (and possibly Sybase ASE?? - gt) require that any columns which are used as portions
    // of an index have the columns defined as "NOT NULL".  During initial table creation though,
    // it may not be known which columns are necessarily going to be part of an index (e.g. the
    // table was created, then months later you determine that an additional index while
    // give better performance, so you want to add an index).
    //
    // The following block of code will modify the column definition to make the column be
    // defined with the "NOT NULL" qualifier.
    if (pDb->Dbms() == dbmsMY_SQL)
    {
        wxString sqlStmt;
        int i;
        bool ok = true;
        for (i = 0; i < numIndexColumns && ok; i++)
        {
            int   j = 0;
            bool  found = false;
            // Find the column definition that has the ColName that matches the
            // index column name.  We need to do this to get the DB_DATA_TYPE of
            // the index column, as MySQL's syntax for the ALTER column requires
            // this information
            while (!found && (j < this->m_numCols))
            {
                if (wxStrcmp(colDefs[j].ColName,pIndexDefs[i].ColName) == 0)
                    found = true;
                if (!found)
                    j++;
            }

            if (found)
            {
                ok = pDb->ModifyColumn(tableName, pIndexDefs[i].ColName,
                                        colDefs[j].DbDataType, (int)(colDefs[j].SzDataObj / sizeof(wxChar)),
                                        wxT("NOT NULL"));

                if (!ok)
                {
                    #if 0
                    // retcode is not used
                    wxODBC_ERRORS retcode;
                    // Oracle returns a DB_ERR_GENERAL_ERROR if the column is already
                    // defined to be NOT NULL, but reportedly MySQL doesn't mind.
                    // This line is just here for debug checking of the value
                    retcode = (wxODBC_ERRORS)pDb->DB_STATUS;
                    #endif
                }
            }
            else
                ok = false;
        }
        if (ok)
            pDb->CommitTrans();
        else
        {
            pDb->RollbackTrans();
            return false;
        }
    }

    // Build a CREATE INDEX statement
    sqlStmt = wxT("CREATE ");
    if (unique)
        sqlStmt += wxT("UNIQUE ");

    sqlStmt += wxT("INDEX ");
    sqlStmt += pDb->SQLTableName(indexName);
    sqlStmt += wxT(" ON ");

    sqlStmt += pDb->SQLTableName(tableName);
//    sqlStmt += tableName;
    sqlStmt += wxT(" (");

    // Append list of columns making up index
    int i;
    for (i = 0; i < numIndexColumns; i++)
    {
        sqlStmt += pDb->SQLColumnName(pIndexDefs[i].ColName);
//        sqlStmt += pIndexDefs[i].ColName;

        // MySQL requires a key length on VARCHAR keys
        if ( pDb->Dbms() == dbmsMY_SQL )
        {
            // Find the details on this column
            int j;
            for ( j = 0; j < m_numCols; ++j )
            {
                if ( wxStrcmp( pIndexDefs[i].ColName, colDefs[j].ColName ) == 0 )
                {
                    break;
                }
            }
            if ( colDefs[j].DbDataType ==  DB_DATA_TYPE_VARCHAR)
            {
                wxString s;
                s.Printf(wxT("(%d)"), (int)(colDefs[i].SzDataObj / sizeof(wxChar)));
                sqlStmt += s;
            }
        }

        // Postgres and SQL Server 7 do not support the ASC/DESC keywords for index columns
        if (!((pDb->Dbms() == dbmsMS_SQL_SERVER) && (wxStrncmp(pDb->dbInf.dbmsVer,_T("07"),2)==0)) &&
            !(pDb->Dbms() == dbmsFIREBIRD) &&
            !(pDb->Dbms() == dbmsPOSTGRES))
        {
            if (pIndexDefs[i].Ascending)
                sqlStmt += wxT(" ASC");
            else
                sqlStmt += wxT(" DESC");
        }
        else
            wxASSERT_MSG(pIndexDefs[i].Ascending, _T("Datasource does not support DESCending index columns"));

        if ((i + 1) < numIndexColumns)
            sqlStmt += wxT(",");
    }

    // Append closing parentheses
    sqlStmt += wxT(")");

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl << endl;
#endif

    // Execute the CREATE INDEX statement
    RETCODE retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, hstmt);
        pDb->RollbackTrans();
        CloseCursor(hstmt);
        return false;
    }

    // Commit the transaction and close the cursor
    if (! pDb->CommitTrans())
        return false;
    if (! CloseCursor(hstmt))
        return false;

    // Index Created Successfully
    return true;

}  // wxDbTable::CreateIndex()


/********** wxDbTable::DropIndex() **********/
bool wxDbTable::DropIndex(const wxString &indexName)
{
    // NOTE: This function returns true if the Index does not exist, but
    //       only for identified databases.  Code will need to be added
    //       below for any other databases when those databases are defined
    //       to handle this situation consistently

    wxString sqlStmt;

    if (pDb->Dbms() == dbmsACCESS || pDb->Dbms() == dbmsMY_SQL ||
        pDb->Dbms() == dbmsDBASE /*|| Paradox needs this syntax too when we add support*/)
        sqlStmt.Printf(wxT("DROP INDEX %s ON %s"),
                       pDb->SQLTableName(indexName.c_str()).c_str(),
                       pDb->SQLTableName(tableName.c_str()).c_str());
    else if ((pDb->Dbms() == dbmsMS_SQL_SERVER) ||
             (pDb->Dbms() == dbmsSYBASE_ASE) ||
             (pDb->Dbms() == dbmsXBASE_SEQUITER))
        sqlStmt.Printf(wxT("DROP INDEX %s.%s"),
                       pDb->SQLTableName(tableName.c_str()).c_str(),
                       pDb->SQLTableName(indexName.c_str()).c_str());
    else
        sqlStmt.Printf(wxT("DROP INDEX %s"),
                       pDb->SQLTableName(indexName.c_str()).c_str());

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl;
#endif
    RETCODE retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS)
    {
        // Check for "Index not found" error and ignore
        pDb->GetNextError(henv, hdbc, hstmt);
        if (wxStrcmp(pDb->sqlState,wxT("S0012")))  // "Index not found"
        {
            // Check for product specific error codes
            if (!((pDb->Dbms() == dbmsSYBASE_ASA    && !wxStrcmp(pDb->sqlState,wxT("42000"))) ||  // v5.x (and lower?)
                  (pDb->Dbms() == dbmsSYBASE_ASE    && !wxStrcmp(pDb->sqlState,wxT("37000"))) ||
                  (pDb->Dbms() == dbmsMS_SQL_SERVER && !wxStrcmp(pDb->sqlState,wxT("S1000"))) ||
                  (pDb->Dbms() == dbmsINTERBASE     && !wxStrcmp(pDb->sqlState,wxT("S1000"))) ||
                  (pDb->Dbms() == dbmsMAXDB         && !wxStrcmp(pDb->sqlState,wxT("S1000"))) ||
                  (pDb->Dbms() == dbmsFIREBIRD      && !wxStrcmp(pDb->sqlState,wxT("HY000"))) ||
                  (pDb->Dbms() == dbmsSYBASE_ASE    && !wxStrcmp(pDb->sqlState,wxT("S0002"))) ||  // Base table not found
                  (pDb->Dbms() == dbmsMY_SQL        && !wxStrcmp(pDb->sqlState,wxT("42S12"))) ||  // tested by Christopher Ludwik Marino-Cebulski using v3.23.21beta
                  (pDb->Dbms() == dbmsPOSTGRES      && !wxStrcmp(pDb->sqlState,wxT("08S01")))
               ))
            {
                pDb->DispNextError();
                pDb->DispAllErrors(henv, hdbc, hstmt);
                pDb->RollbackTrans();
                CloseCursor(hstmt);
                return false;
            }
        }
    }

    // Commit the transaction and close the cursor
    if (! pDb->CommitTrans())
        return false;
    if (! CloseCursor(hstmt))
        return false;

    return true;
}  // wxDbTable::DropIndex()


/********** wxDbTable::SetOrderByColNums() **********/
bool wxDbTable::SetOrderByColNums(UWORD first, ... )
{
    int         colNumber = first;  // using 'int' to be able to look for wxDB_NO_MORE_COLUN_NUMBERS
    va_list     argptr;

    bool        abort = false;
    wxString    tempStr;

    va_start(argptr, first);     /* Initialize variable arguments. */
    while (!abort && (colNumber != wxDB_NO_MORE_COLUMN_NUMBERS))
    {
        // Make sure the passed in column number
        // is within the valid range of columns
        //
        // Valid columns are 0 thru m_numCols-1
        if (colNumber >= m_numCols || colNumber < 0)
        {
            abort = true;
            continue;
        }

        if (colNumber != first)
            tempStr += wxT(",");

        tempStr += colDefs[colNumber].ColName;
        colNumber = va_arg (argptr, int);
    }
    va_end (argptr);              /* Reset variable arguments.      */

    SetOrderByClause(tempStr);

    return (!abort);
}  // wxDbTable::SetOrderByColNums()


/********** wxDbTable::Insert() **********/
int wxDbTable::Insert(void)
{
    wxASSERT(!queryOnly);
    if (queryOnly || !insertable)
        return(DB_FAILURE);

    bindInsertParams();

    // Insert the record by executing the already prepared insert statement
    RETCODE retcode;
    retcode = SQLExecute(hstmtInsert);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO &&
        retcode != SQL_NEED_DATA)
    {
        // Check to see if integrity constraint was violated
        pDb->GetNextError(henv, hdbc, hstmtInsert);
        if (! wxStrcmp(pDb->sqlState, wxT("23000")))  // Integrity constraint violated
            return(DB_ERR_INTEGRITY_CONSTRAINT_VIOL);
        else
        {
            pDb->DispNextError();
            pDb->DispAllErrors(henv, hdbc, hstmtInsert);
            return(DB_FAILURE);
        }
    }
    if (retcode == SQL_NEED_DATA)
    {
        PTR pParmID;
        retcode = SQLParamData(hstmtInsert, &pParmID);
        while (retcode == SQL_NEED_DATA)
        {
            // Find the parameter
            int i;
            for (i=0; i < m_numCols; i++)
            {
                if (colDefs[i].PtrDataObj == pParmID)
                {
                    // We found it.  Store the parameter.
                    retcode = SQLPutData(hstmtInsert, pParmID, colDefs[i].SzDataObj);
                    if (retcode != SQL_SUCCESS)
                    {
                        pDb->DispNextError();
                        pDb->DispAllErrors(henv, hdbc, hstmtInsert);
                        return(DB_FAILURE);
                    }
                    break;
                }
            }
            retcode = SQLParamData(hstmtInsert, &pParmID);
            if (retcode != SQL_SUCCESS &&
                retcode != SQL_SUCCESS_WITH_INFO)
            {
                // record was not inserted
                pDb->DispNextError();
                pDb->DispAllErrors(henv, hdbc, hstmtInsert);
                return(DB_FAILURE);
            }
        }
    }

    // Record inserted into the datasource successfully
    return(DB_SUCCESS);

}  // wxDbTable::Insert()


/********** wxDbTable::Update() **********/
bool wxDbTable::Update(void)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    wxString sqlStmt;

    // Build the SQL UPDATE statement
    BuildUpdateStmt(sqlStmt, DB_UPD_KEYFIELDS);

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl << endl;
#endif

    // Execute the SQL UPDATE statement
    return(execUpdate(sqlStmt));

}  // wxDbTable::Update()


/********** wxDbTable::Update(pSqlStmt) **********/
bool wxDbTable::Update(const wxString &pSqlStmt)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    pDb->WriteSqlLog(pSqlStmt);

    return(execUpdate(pSqlStmt));

}  // wxDbTable::Update(pSqlStmt)


/********** wxDbTable::UpdateWhere() **********/
bool wxDbTable::UpdateWhere(const wxString &pWhereClause)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    wxString sqlStmt;

    // Build the SQL UPDATE statement
    BuildUpdateStmt(sqlStmt, DB_UPD_WHERE, pWhereClause);

    pDb->WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl << endl;
#endif

    // Execute the SQL UPDATE statement
    return(execUpdate(sqlStmt));

}  // wxDbTable::UpdateWhere()


/********** wxDbTable::Delete() **********/
bool wxDbTable::Delete(void)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    wxString sqlStmt;
    sqlStmt.Empty();

    // Build the SQL DELETE statement
    BuildDeleteStmt(sqlStmt, DB_DEL_KEYFIELDS);

    pDb->WriteSqlLog(sqlStmt);

    // Execute the SQL DELETE statement
    return(execDelete(sqlStmt));

}  // wxDbTable::Delete()


/********** wxDbTable::DeleteWhere() **********/
bool wxDbTable::DeleteWhere(const wxString &pWhereClause)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    wxString sqlStmt;
    sqlStmt.Empty();

    // Build the SQL DELETE statement
    BuildDeleteStmt(sqlStmt, DB_DEL_WHERE, pWhereClause);

    pDb->WriteSqlLog(sqlStmt);

    // Execute the SQL DELETE statement
    return(execDelete(sqlStmt));

}  // wxDbTable::DeleteWhere()


/********** wxDbTable::DeleteMatching() **********/
bool wxDbTable::DeleteMatching(void)
{
    wxASSERT(!queryOnly);
    if (queryOnly)
        return false;

    wxString sqlStmt;
    sqlStmt.Empty();

    // Build the SQL DELETE statement
    BuildDeleteStmt(sqlStmt, DB_DEL_MATCHING);

    pDb->WriteSqlLog(sqlStmt);

    // Execute the SQL DELETE statement
    return(execDelete(sqlStmt));

}  // wxDbTable::DeleteMatching()


/********** wxDbTable::IsColNull() **********/
bool wxDbTable::IsColNull(UWORD colNumber) const
{
/*
    This logic is just not right.  It would indicate true
    if a numeric field were set to a value of 0.

    switch(colDefs[colNumber].SqlCtype)
    {
        case SQL_C_CHAR:
        case SQL_C_WCHAR:
        //case SQL_C_WXCHAR:  SQL_C_WXCHAR is covered by either SQL_C_CHAR or SQL_C_WCHAR
            return(((UCHAR FAR *) colDefs[colNumber].PtrDataObj)[0] == 0);
        case SQL_C_SSHORT:
            return((  *((SWORD *) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_USHORT:
            return((   *((UWORD*) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_SLONG:
            return(( *((SDWORD *) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_ULONG:
            return(( *((UDWORD *) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_FLOAT:
            return(( *((SFLOAT *) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_DOUBLE:
            return((*((SDOUBLE *) colDefs[colNumber].PtrDataObj))   == 0);
        case SQL_C_TIMESTAMP:
            TIMESTAMP_STRUCT *pDt;
            pDt = (TIMESTAMP_STRUCT *) colDefs[colNumber].PtrDataObj;
            if (pDt->year == 0 && pDt->month == 0 && pDt->day == 0)
                return true;
            else
                return false;
        default:
            return true;
    }
*/
    return (colDefs[colNumber].Null);
}  // wxDbTable::IsColNull()


/********** wxDbTable::CanSelectForUpdate() **********/
bool wxDbTable::CanSelectForUpdate(void)
{
    if (queryOnly)
        return false;

    if (pDb->Dbms() == dbmsMY_SQL)
        return false;

    if ((pDb->Dbms() == dbmsORACLE) ||
        (pDb->dbInf.posStmts & SQL_PS_SELECT_FOR_UPDATE))
        return true;
    else
        return false;

}  // wxDbTable::CanSelectForUpdate()


/********** wxDbTable::CanUpdateByROWID() **********/
bool wxDbTable::CanUpdateByROWID(void)
{
/*
 * NOTE: Returning false for now until this can be debugged,
 *        as the ROWID is not getting updated correctly
 */
    return false;
/*
    if (pDb->Dbms() == dbmsORACLE)
        return true;
    else
        return false;
*/
}  // wxDbTable::CanUpdateByROWID()


/********** wxDbTable::IsCursorClosedOnCommit() **********/
bool wxDbTable::IsCursorClosedOnCommit(void)
{
    if (pDb->dbInf.cursorCommitBehavior == SQL_CB_PRESERVE)
        return false;
    else
        return true;

}  // wxDbTable::IsCursorClosedOnCommit()



/********** wxDbTable::ClearMemberVar() **********/
void wxDbTable::ClearMemberVar(UWORD colNumber, bool setToNull)
{
    wxASSERT(colNumber < m_numCols);

    switch(colDefs[colNumber].SqlCtype)
    {
        case SQL_C_CHAR:
#ifdef SQL_C_WCHAR
        case SQL_C_WCHAR:
#endif
        //case SQL_C_WXCHAR:  SQL_C_WXCHAR is covered by either SQL_C_CHAR or SQL_C_WCHAR
            ((UCHAR FAR *) colDefs[colNumber].PtrDataObj)[0]    = 0;
            break;
        case SQL_C_SSHORT:
            *((SWORD *) colDefs[colNumber].PtrDataObj)          = 0;
            break;
        case SQL_C_USHORT:
            *((UWORD*) colDefs[colNumber].PtrDataObj)           = 0;
            break;
        case SQL_C_LONG:
        case SQL_C_SLONG:
            *((SDWORD *) colDefs[colNumber].PtrDataObj)         = 0;
            break;
        case SQL_C_ULONG:
            *((UDWORD *) colDefs[colNumber].PtrDataObj)         = 0;
            break;
        case SQL_C_FLOAT:
            *((SFLOAT *) colDefs[colNumber].PtrDataObj)         = 0.0f;
            break;
        case SQL_C_DOUBLE:
            *((SDOUBLE *) colDefs[colNumber].PtrDataObj)        = 0.0f;
            break;
        case SQL_C_TIMESTAMP:
            TIMESTAMP_STRUCT *pDt;
            pDt = (TIMESTAMP_STRUCT *) colDefs[colNumber].PtrDataObj;
            pDt->year = 0;
            pDt->month = 0;
            pDt->day = 0;
            pDt->hour = 0;
            pDt->minute = 0;
            pDt->second = 0;
            pDt->fraction = 0;
            break;
        case SQL_C_DATE:
            DATE_STRUCT *pDtd;
            pDtd = (DATE_STRUCT *) colDefs[colNumber].PtrDataObj;
            pDtd->year = 0;
            pDtd->month = 0;
            pDtd->day = 0;
            break;
        case SQL_C_TIME:
            TIME_STRUCT *pDtt;
            pDtt = (TIME_STRUCT *) colDefs[colNumber].PtrDataObj;
            pDtt->hour = 0;
            pDtt->minute = 0;
            pDtt->second = 0;
            break;
    }

    if (setToNull)
        SetColNull(colNumber);
}  // wxDbTable::ClearMemberVar()


/********** wxDbTable::ClearMemberVars() **********/
void wxDbTable::ClearMemberVars(bool setToNull)
{
    int i;

    // Loop through the columns setting each member variable to zero
    for (i=0; i < m_numCols; i++)
        ClearMemberVar((UWORD)i,setToNull);

}  // wxDbTable::ClearMemberVars()


/********** wxDbTable::SetQueryTimeout() **********/
bool wxDbTable::SetQueryTimeout(UDWORD nSeconds)
{
    if (SQLSetStmtOption(hstmtInsert, SQL_QUERY_TIMEOUT, nSeconds) != SQL_SUCCESS)
        return(pDb->DispAllErrors(henv, hdbc, hstmtInsert));
    if (SQLSetStmtOption(hstmtUpdate, SQL_QUERY_TIMEOUT, nSeconds) != SQL_SUCCESS)
        return(pDb->DispAllErrors(henv, hdbc, hstmtUpdate));
    if (SQLSetStmtOption(hstmtDelete, SQL_QUERY_TIMEOUT, nSeconds) != SQL_SUCCESS)
        return(pDb->DispAllErrors(henv, hdbc, hstmtDelete));
    if (SQLSetStmtOption(hstmtInternal, SQL_QUERY_TIMEOUT, nSeconds) != SQL_SUCCESS)
        return(pDb->DispAllErrors(henv, hdbc, hstmtInternal));

    // Completed Successfully
    return true;

}  // wxDbTable::SetQueryTimeout()


/********** wxDbTable::SetColDefs() **********/
bool wxDbTable::SetColDefs(UWORD index, const wxString &fieldName, int dataType, void *pData,
                           SWORD cType, int size, bool keyField, bool updateable,
                           bool insertAllowed, bool derivedColumn)
{
    wxString tmpStr;

    if (index >= m_numCols)  // Columns numbers are zero based....
    {
        tmpStr.Printf(wxT("Specified column index (%d) exceeds the maximum number of columns (%d) registered for this table definition.  Column definition not added."), index, m_numCols);
        wxFAIL_MSG(tmpStr);
        wxLogDebug(tmpStr);
        return false;
    }

    if (!colDefs)  // May happen if the database connection fails
        return false;

    if (fieldName.length() > (unsigned int) DB_MAX_COLUMN_NAME_LEN)
    {
        wxStrncpy(colDefs[index].ColName, fieldName, DB_MAX_COLUMN_NAME_LEN);
        colDefs[index].ColName[DB_MAX_COLUMN_NAME_LEN] = 0;  // Prevent buffer overrun

        tmpStr.Printf(wxT("Column name '%s' is too long. Truncated to '%s'."),
                      fieldName.c_str(),colDefs[index].ColName);
        wxFAIL_MSG(tmpStr);
        wxLogDebug(tmpStr);
    }
    else
        wxStrcpy(colDefs[index].ColName, fieldName);

    colDefs[index].DbDataType       = dataType;
    colDefs[index].PtrDataObj       = pData;
    colDefs[index].SqlCtype         = cType;
    colDefs[index].SzDataObj        = size;  //TODO: glt ??? * sizeof(wxChar) ???
    colDefs[index].KeyField         = keyField;
    colDefs[index].DerivedCol       = derivedColumn;
    // Derived columns by definition would NOT be "Insertable" or "Updateable"
    if (derivedColumn)
    {
        colDefs[index].Updateable       = false;
        colDefs[index].InsertAllowed    = false;
    }
    else
    {
        colDefs[index].Updateable       = updateable;
        colDefs[index].InsertAllowed    = insertAllowed;
    }

    colDefs[index].Null                 = false;

    return true;

}  // wxDbTable::SetColDefs()


/********** wxDbTable::SetColDefs() **********/
wxDbColDataPtr* wxDbTable::SetColDefs(wxDbColInf *pColInfs, UWORD numCols)
{
    wxASSERT(pColInfs);
    wxDbColDataPtr *pColDataPtrs = NULL;

    if (pColInfs)
    {
        UWORD index;

        pColDataPtrs = new wxDbColDataPtr[numCols+1];

        for (index = 0; index < numCols; index++)
        {
            // Process the fields
            switch (pColInfs[index].dbDataType)
            {
                case DB_DATA_TYPE_VARCHAR:
                   pColDataPtrs[index].PtrDataObj = new wxChar[pColInfs[index].bufferSize+(1*sizeof(wxChar))];
                   pColDataPtrs[index].SzDataObj  = pColInfs[index].bufferSize+(1*sizeof(wxChar));
                   pColDataPtrs[index].SqlCtype   = SQL_C_WXCHAR;
                   break;
                case DB_DATA_TYPE_MEMO:
                   pColDataPtrs[index].PtrDataObj = new wxChar[pColInfs[index].bufferSize+(1*sizeof(wxChar))];
                   pColDataPtrs[index].SzDataObj  = pColInfs[index].bufferSize+(1*sizeof(wxChar));
                   pColDataPtrs[index].SqlCtype   = SQL_C_WXCHAR;
                   break;
                case DB_DATA_TYPE_INTEGER:
                    // Can be long or short
                    if (pColInfs[index].bufferSize == sizeof(long))
                    {
                      pColDataPtrs[index].PtrDataObj = new long;
                      pColDataPtrs[index].SzDataObj  = sizeof(long);
                      pColDataPtrs[index].SqlCtype   = SQL_C_SLONG;
                    }
                    else
                    {
                        pColDataPtrs[index].PtrDataObj = new short;
                        pColDataPtrs[index].SzDataObj  = sizeof(short);
                        pColDataPtrs[index].SqlCtype   = SQL_C_SSHORT;
                    }
                    break;
                case DB_DATA_TYPE_FLOAT:
                    // Can be float or double
                    if (pColInfs[index].bufferSize == sizeof(float))
                    {
                        pColDataPtrs[index].PtrDataObj = new float;
                        pColDataPtrs[index].SzDataObj  = sizeof(float);
                        pColDataPtrs[index].SqlCtype   = SQL_C_FLOAT;
                    }
                    else
                    {
                        pColDataPtrs[index].PtrDataObj = new double;
                        pColDataPtrs[index].SzDataObj  = sizeof(double);
                        pColDataPtrs[index].SqlCtype   = SQL_C_DOUBLE;
                    }
                    break;
                case DB_DATA_TYPE_DATE:
                    pColDataPtrs[index].PtrDataObj = new TIMESTAMP_STRUCT;
                    pColDataPtrs[index].SzDataObj  = sizeof(TIMESTAMP_STRUCT);
                    pColDataPtrs[index].SqlCtype   = SQL_C_TIMESTAMP;
                    break;
                case DB_DATA_TYPE_BLOB:
                    wxFAIL_MSG(wxT("This form of ::SetColDefs() cannot be used with BLOB columns"));
                    pColDataPtrs[index].PtrDataObj = /*BLOB ADDITION NEEDED*/NULL;
                    pColDataPtrs[index].SzDataObj  = /*BLOB ADDITION NEEDED*/sizeof(void *);
                    pColDataPtrs[index].SqlCtype   = SQL_VARBINARY;
                    break;
            }
            if (pColDataPtrs[index].PtrDataObj != NULL)
                SetColDefs (index,pColInfs[index].colName,pColInfs[index].dbDataType, pColDataPtrs[index].PtrDataObj, pColDataPtrs[index].SqlCtype, pColDataPtrs[index].SzDataObj);
            else
            {
                // Unable to build all the column definitions, as either one of
                // the calls to "new" failed above, or there was a BLOB field
                // to have a column definition for.  If BLOBs are to be used,
                // the other form of ::SetColDefs() must be used, as it is impossible
                // to know the maximum size to create the PtrDataObj to be.
                delete [] pColDataPtrs;
                return NULL;
            }
        }
    }

    return (pColDataPtrs);

} // wxDbTable::SetColDefs()


/********** wxDbTable::SetCursor() **********/
void wxDbTable::SetCursor(HSTMT *hstmtActivate)
{
    if (hstmtActivate == wxDB_DEFAULT_CURSOR)
        hstmt = *hstmtDefault;
    else
        hstmt = *hstmtActivate;

}  // wxDbTable::SetCursor()


/********** wxDbTable::Count(const wxString &) **********/
ULONG wxDbTable::Count(const wxString &args)
{
    ULONG count;
    wxString sqlStmt;
    SQLLEN cb;

    // Build a "SELECT COUNT(*) FROM queryTableName [WHERE whereClause]" SQL Statement
    sqlStmt  = wxT("SELECT COUNT(");
    sqlStmt += args;
    sqlStmt += wxT(") FROM ");
    sqlStmt += pDb->SQLTableName(queryTableName);
//    sqlStmt += queryTableName;
#if wxODBC_BACKWARD_COMPATABILITY
    if (from && wxStrlen(from))
#else
    if (from.length())
#endif
        sqlStmt += from;

    // Add the where clause if one is provided
#if wxODBC_BACKWARD_COMPATABILITY
    if (where && wxStrlen(where))
#else
    if (where.length())
#endif
    {
        sqlStmt += wxT(" WHERE ");
        sqlStmt += where;
    }

    pDb->WriteSqlLog(sqlStmt);

    // Initialize the Count cursor if it's not already initialized
    if (!hstmtCount)
    {
        hstmtCount = GetNewCursor(false,false);
        wxASSERT(hstmtCount);
        if (!hstmtCount)
            return(0);
    }

    // Execute the SQL statement
    if (SQLExecDirect(*hstmtCount, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, *hstmtCount);
        return(0);
    }

    // Fetch the record
    if (SQLFetch(*hstmtCount) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, *hstmtCount);
        return(0);
    }

    // Obtain the result
    if (SQLGetData(*hstmtCount, (UWORD)1, SQL_C_ULONG, &count, sizeof(count), &cb) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, *hstmtCount);
        return(0);
    }

    // Free the cursor
    if (SQLFreeStmt(*hstmtCount, SQL_CLOSE) != SQL_SUCCESS)
        pDb->DispAllErrors(henv, hdbc, *hstmtCount);

    // Return the record count
    return(count);

}  // wxDbTable::Count()


/********** wxDbTable::Refresh() **********/
bool wxDbTable::Refresh(void)
{
    bool result = true;

    // Switch to the internal cursor so any active cursors are not corrupted
    HSTMT currCursor = GetCursor();
    hstmt = hstmtInternal;
#if wxODBC_BACKWARD_COMPATABILITY
    // Save the where and order by clauses
    wxChar *saveWhere = where;
    wxChar *saveOrderBy = orderBy;
#else
    wxString saveWhere = where;
    wxString saveOrderBy = orderBy;
#endif
    // Build a where clause to refetch the record with.  Try and use the
    // ROWID if it's available, ow use the key fields.
    wxString whereClause;
    whereClause.Empty();

    if (CanUpdateByROWID())
    {
        SQLLEN cb;
        wxChar rowid[wxDB_ROWID_LEN+1];

        // Get the ROWID value.  If not successful retreiving the ROWID,
        // simply fall down through the code and build the WHERE clause
        // based on the key fields.
        if (SQLGetData(hstmt, (UWORD)(m_numCols+1), SQL_C_WXCHAR, (UCHAR*) rowid, sizeof(rowid), &cb) == SQL_SUCCESS)
        {
            whereClause += pDb->SQLTableName(queryTableName);
//            whereClause += queryTableName;
            whereClause += wxT(".ROWID = '");
            whereClause += rowid;
            whereClause += wxT("'");
        }
    }

    // If unable to use the ROWID, build a where clause from the keyfields
    if (wxStrlen(whereClause) == 0)
        BuildWhereClause(whereClause, DB_WHERE_KEYFIELDS, queryTableName);

    // Requery the record
    where = whereClause;
    orderBy.Empty();
    if (!Query())
        result = false;

    if (result && !GetNext())
        result = false;

    // Switch back to original cursor
    SetCursor(&currCursor);

    // Free the internal cursor
    if (SQLFreeStmt(hstmtInternal, SQL_CLOSE) != SQL_SUCCESS)
        pDb->DispAllErrors(henv, hdbc, hstmtInternal);

    // Restore the original where and order by clauses
    where   = saveWhere;
    orderBy = saveOrderBy;

    return(result);

}  // wxDbTable::Refresh()


/********** wxDbTable::SetColNull() **********/
bool wxDbTable::SetColNull(UWORD colNumber, bool set)
{
    if (colNumber < m_numCols)
    {
        colDefs[colNumber].Null = set;
        if (set)  // Blank out the values in the member variable
           ClearMemberVar(colNumber, false);  // Must call with false here, or infinite recursion will happen

        setCbValueForColumn(colNumber);

        return true;
    }
    else
        return false;

}  // wxDbTable::SetColNull()


/********** wxDbTable::SetColNull() **********/
bool wxDbTable::SetColNull(const wxString &colName, bool set)
{
    int colNumber;
    for (colNumber = 0; colNumber < m_numCols; colNumber++)
    {
        if (!wxStricmp(colName, colDefs[colNumber].ColName))
            break;
    }

    if (colNumber < m_numCols)
    {
        colDefs[colNumber].Null = set;
        if (set)  // Blank out the values in the member variable
           ClearMemberVar((UWORD)colNumber,false);  // Must call with false here, or infinite recursion will happen

        setCbValueForColumn(colNumber);

        return true;
    }
    else
        return false;

}  // wxDbTable::SetColNull()


/********** wxDbTable::GetNewCursor() **********/
HSTMT *wxDbTable::GetNewCursor(bool setCursor, bool bindColumns)
{
    HSTMT *newHSTMT = new HSTMT;
    wxASSERT(newHSTMT);
    if (!newHSTMT)
        return(0);

    if (SQLAllocStmt(hdbc, newHSTMT) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc);
        delete newHSTMT;
        return(0);
    }

    if (SQLSetStmtOption(*newHSTMT, SQL_CURSOR_TYPE, cursorType) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc, *newHSTMT);
        delete newHSTMT;
        return(0);
    }

    if (bindColumns)
    {
        if (!bindCols(*newHSTMT))
        {
            delete newHSTMT;
            return(0);
        }
    }

    if (setCursor)
        SetCursor(newHSTMT);

    return(newHSTMT);

}   // wxDbTable::GetNewCursor()


/********** wxDbTable::DeleteCursor() **********/
bool wxDbTable::DeleteCursor(HSTMT *hstmtDel)
{
    bool result = true;

    if (!hstmtDel)  // Cursor already deleted
        return(result);

/*
ODBC 3.0 says to use this form
    if (SQLFreeHandle(*hstmtDel, SQL_DROP) != SQL_SUCCESS)

*/
    if (SQLFreeStmt(*hstmtDel, SQL_DROP) != SQL_SUCCESS)
    {
        pDb->DispAllErrors(henv, hdbc);
        result = false;
    }

    delete hstmtDel;

    return(result);

}  // wxDbTable::DeleteCursor()

//////////////////////////////////////////////////////////////
// wxDbGrid support functions
//////////////////////////////////////////////////////////////

void wxDbTable::SetRowMode(const rowmode_t rowmode)
{
    if (!m_hstmtGridQuery)
    {
        m_hstmtGridQuery = GetNewCursor(false,false);
        if (!bindCols(*m_hstmtGridQuery))
            return;
    }

    m_rowmode = rowmode;
    switch (m_rowmode)
    {
        case WX_ROW_MODE_QUERY:
            SetCursor(m_hstmtGridQuery);
            break;
        case WX_ROW_MODE_INDIVIDUAL:
            SetCursor(hstmtDefault);
            break;
        default:
           wxASSERT(0);
    }
}  // wxDbTable::SetRowMode()


wxVariant wxDbTable::GetColumn(const int colNumber) const
{
    wxVariant val;
    if ((colNumber < m_numCols) && (!IsColNull((UWORD)colNumber)))
    {
        switch (colDefs[colNumber].SqlCtype)
        {
#if wxUSE_UNICODE
    #if defined(SQL_WCHAR)
            case SQL_WCHAR:
    #endif
    #if defined(SQL_WVARCHAR)
            case SQL_WVARCHAR:
    #endif
#endif
            case SQL_CHAR:
            case SQL_VARCHAR:
                val = (wxChar *)(colDefs[colNumber].PtrDataObj);
                break;
            case SQL_C_LONG:
            case SQL_C_SLONG:
                val = *(long *)(colDefs[colNumber].PtrDataObj);
                break;
            case SQL_C_SHORT:
            case SQL_C_SSHORT:
                val = (long int )(*(short *)(colDefs[colNumber].PtrDataObj));
                break;
            case SQL_C_ULONG:
                val = (long)(*(unsigned long *)(colDefs[colNumber].PtrDataObj));
                break;
            case SQL_C_TINYINT:
                val = (long)(*(wxChar *)(colDefs[colNumber].PtrDataObj));
                break;
            case SQL_C_UTINYINT:
                val = (long)(*(wxChar *)(colDefs[colNumber].PtrDataObj));
                break;
            case SQL_C_USHORT:
                val = (long)(*(UWORD *)(colDefs[colNumber].PtrDataObj));
                break;
            case SQL_C_DATE:
                val = (DATE_STRUCT *)(colDefs[colNumber].PtrDataObj);
                break;
            case SQL_C_TIME:
                val = (TIME_STRUCT *)(colDefs[colNumber].PtrDataObj);
                break;
            case SQL_C_TIMESTAMP:
                val = (TIMESTAMP_STRUCT *)(colDefs[colNumber].PtrDataObj);
                break;
            case SQL_C_DOUBLE:
                val = *(double *)(colDefs[colNumber].PtrDataObj);
                break;
            default:
                assert(0);
        }
    }
    return val;
}  // wxDbTable::GetCol()


void wxDbTable::SetColumn(const int colNumber, const wxVariant val)
{
    //FIXME: Add proper wxDateTime support to wxVariant..
    wxDateTime dateval;

    SetColNull((UWORD)colNumber, val.IsNull());

    if (!val.IsNull())
    {
        if ((colDefs[colNumber].SqlCtype == SQL_C_DATE)
            || (colDefs[colNumber].SqlCtype == SQL_C_TIME)
            || (colDefs[colNumber].SqlCtype == SQL_C_TIMESTAMP))
        {
            //Returns null if invalid!
            if (!dateval.ParseDate(val.GetString()))
                SetColNull((UWORD)colNumber, true);
        }

        switch (colDefs[colNumber].SqlCtype)
        {
#if wxUSE_UNICODE
    #if defined(SQL_WCHAR)
            case SQL_WCHAR:
    #endif
    #if defined(SQL_WVARCHAR)
            case SQL_WVARCHAR:
    #endif
#endif
            case SQL_CHAR:
            case SQL_VARCHAR:
                csstrncpyt((wxChar *)(colDefs[colNumber].PtrDataObj),
                           val.GetString().c_str(),
                           colDefs[colNumber].SzDataObj-1);  //TODO: glt ??? * sizeof(wxChar) ???
                break;
            case SQL_C_LONG:
            case SQL_C_SLONG:
                *(long *)(colDefs[colNumber].PtrDataObj) = val;
                break;
            case SQL_C_SHORT:
            case SQL_C_SSHORT:
                *(short *)(colDefs[colNumber].PtrDataObj) = (short)val.GetLong();
                break;
            case SQL_C_ULONG:
                *(unsigned long *)(colDefs[colNumber].PtrDataObj) = val.GetLong();
                break;
            case SQL_C_TINYINT:
                *(wxChar *)(colDefs[colNumber].PtrDataObj) = val.GetChar();
                break;
            case SQL_C_UTINYINT:
                *(wxChar *)(colDefs[colNumber].PtrDataObj) = val.GetChar();
                break;
            case SQL_C_USHORT:
                *(unsigned short *)(colDefs[colNumber].PtrDataObj) = (unsigned short)val.GetLong();
                break;
            //FIXME: Add proper wxDateTime support to wxVariant..
            case SQL_C_DATE:
                {
                    DATE_STRUCT *dataptr =
                        (DATE_STRUCT *)colDefs[colNumber].PtrDataObj;

                    dataptr->year   = (SWORD)dateval.GetYear();
                    dataptr->month  = (UWORD)(dateval.GetMonth()+1);
                    dataptr->day    = (UWORD)dateval.GetDay();
                }
                break;
            case SQL_C_TIME:
                {
                    TIME_STRUCT *dataptr =
                        (TIME_STRUCT *)colDefs[colNumber].PtrDataObj;

                    dataptr->hour   = dateval.GetHour();
                    dataptr->minute = dateval.GetMinute();
                    dataptr->second = dateval.GetSecond();
                }
                break;
            case SQL_C_TIMESTAMP:
                {
                    TIMESTAMP_STRUCT *dataptr =
                        (TIMESTAMP_STRUCT *)colDefs[colNumber].PtrDataObj;
                    dataptr->year   = (SWORD)dateval.GetYear();
                    dataptr->month  = (UWORD)(dateval.GetMonth()+1);
                    dataptr->day    = (UWORD)dateval.GetDay();

                    dataptr->hour   = dateval.GetHour();
                    dataptr->minute = dateval.GetMinute();
                    dataptr->second = dateval.GetSecond();
                }
                break;
            case SQL_C_DOUBLE:
                *(double *)(colDefs[colNumber].PtrDataObj) = val;
                break;
            default:
                assert(0);
        }  // switch
    }  // if (!val.IsNull())
}  // wxDbTable::SetCol()


GenericKey wxDbTable::GetKey()
{
    void *blk;
    wxChar *blkptr;

    blk = malloc(m_keysize);
    blkptr = (wxChar *) blk;

    int i;
    for (i=0; i < m_numCols; i++)
    {
        if (colDefs[i].KeyField)
        {
            memcpy(blkptr,colDefs[i].PtrDataObj, colDefs[i].SzDataObj);
            blkptr += colDefs[i].SzDataObj;
        }
    }

    GenericKey k = GenericKey(blk, m_keysize);
    free(blk);

    return k;
}  // wxDbTable::GetKey()


void wxDbTable::SetKey(const GenericKey& k)
{
    void    *blk;
    wxChar  *blkptr;

    blk = k.GetBlk();
    blkptr = (wxChar *)blk;

    int i;
    for (i=0; i < m_numCols; i++)
    {
        if (colDefs[i].KeyField)
        {
            SetColNull((UWORD)i, false);
            memcpy(colDefs[i].PtrDataObj, blkptr, colDefs[i].SzDataObj);
            blkptr += colDefs[i].SzDataObj;
        }
    }
}  // wxDbTable::SetKey()


#endif  // wxUSE_ODBC
