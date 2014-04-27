///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/db.cpp
// Purpose:     Implementation of the wxDb class.  The wxDb class represents a connection
//              to an ODBC data source.  The wxDb class allows operations on the data
//              source such as opening and closing the data source.
// Author:      Doug Card
// Modified by: George Tasker
//              Bart Jourquin
//              Mark Johnson, wxWindows@mj10777.de
// Mods:        Dec, 1998:
//                -Added support for SQL statement logging and database cataloging
// Mods:        April, 1999
//                -Added QUERY_ONLY mode support to reduce default number of cursors
//                -Added additional SQL logging code
//                -Added DEBUG-ONLY tracking of wxTable objects to detect orphaned DB connections
//                -Set ODBC option to only read committed writes to the DB so all
//                   databases operate the same in that respect
// Created:     9.96
// RCS-ID:      $Id: db.cpp 52489 2008-03-14 14:14:57Z JS $
// Copyright:   (c) 1996 Remstar International, Inc.
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

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
    #include "wx/app.h"
#endif

#ifdef DBDEBUG_CONSOLE
    #include "wx/ioswrap.h"
#endif

#include "wx/filefn.h"
#include "wx/wxchar.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "wx/db.h"

// DLL options compatibility check:
WX_CHECK_BUILD_OPTIONS("wxODBC")

WXDLLIMPEXP_DATA_ODBC(wxDbList*) PtrBegDbList = 0;

wxChar const *SQL_LOG_FILENAME         = wxT("sqllog.txt");
wxChar const *SQL_CATALOG_FILENAME     = wxT("catalog.txt");

#ifdef __WXDEBUG__
    #include "wx/thread.h"

    extern wxList TablesInUse;
#if wxUSE_THREADS
    extern wxCriticalSection csTablesInUse;
#endif // wxUSE_THREADS
#endif

// SQL Log defaults to be used by GetDbConnection
wxDbSqlLogState SQLLOGstate = sqlLogOFF;

static wxString SQLLOGfn = SQL_LOG_FILENAME;

// The wxDb::errorList is copied to this variable when the wxDb object
// is closed.  This way, the error list is still available after the
// database object is closed.  This is necessary if the database
// connection fails so the calling application can show the operator
// why the connection failed.  Note: as each wxDb object is closed, it
// will overwrite the errors of the previously destroyed wxDb object in
// this variable.  NOTE: This occurs during a CLOSE, not a FREEing of the
// connection
wxChar DBerrorList[DB_MAX_ERROR_HISTORY][DB_MAX_ERROR_MSG_LEN+1];


// This type defines the return row-struct form
// SQLTablePrivileges, and is used by wxDB::TablePrivileges.
typedef struct
{
   wxChar        tableQual[128+1];
   wxChar        tableOwner[128+1];
   wxChar        tableName[128+1];
   wxChar        grantor[128+1];
   wxChar        grantee[128+1];
   wxChar        privilege[128+1];
   wxChar        grantable[3+1];
} wxDbTablePrivilegeInfo;


/********** wxDbConnectInf Constructor - form 1 **********/
wxDbConnectInf::wxDbConnectInf()
{
    Henv = 0;
    freeHenvOnDestroy = false;

    Initialize();
}  // Constructor


/********** wxDbConnectInf Constructor - form 2 **********/
wxDbConnectInf::wxDbConnectInf(HENV henv, const wxString &dsn, const wxString &userID,
                       const wxString &password, const wxString &defaultDir,
                       const wxString &fileType, const wxString &description)
{
    Henv = 0;
    freeHenvOnDestroy = false;

    Initialize();

    if (henv)
        SetHenv(henv);
    else
        AllocHenv();

    SetDsn(dsn);
    SetUserID(userID);
    SetPassword(password);
    SetDescription(description);
    SetFileType(fileType);
    SetDefaultDir(defaultDir);
}  // wxDbConnectInf Constructor


wxDbConnectInf::~wxDbConnectInf()
{
    if (freeHenvOnDestroy)
    {
        FreeHenv();
    }
}  // wxDbConnectInf Destructor



/********** wxDbConnectInf::Initialize() **********/
bool wxDbConnectInf::Initialize()
{
    freeHenvOnDestroy = false;

    if (freeHenvOnDestroy && Henv)
        FreeHenv();

    Henv = 0;
    Dsn[0] = 0;
    Uid[0] = 0;
    AuthStr[0] = 0;
    ConnectionStr[0] = 0;
    Description.Empty();
    FileType.Empty();
    DefaultDir.Empty();

    useConnectionStr = false;

    return true;
}  // wxDbConnectInf::Initialize()


/********** wxDbConnectInf::AllocHenv() **********/
bool wxDbConnectInf::AllocHenv()
{
    // This is here to help trap if you are getting a new henv
    // without releasing an existing henv
    wxASSERT(!Henv);

    // Initialize the ODBC Environment for Database Operations
    if (SQLAllocEnv(&Henv) != SQL_SUCCESS)
    {
        wxLogDebug(wxT("A problem occurred while trying to get a connection to the data source"));
        return false;
    }

    freeHenvOnDestroy = true;

    return true;
}  // wxDbConnectInf::AllocHenv()


void wxDbConnectInf::FreeHenv()
{
    wxASSERT(Henv);

    if (Henv)
        SQLFreeEnv(Henv);

    Henv = 0;
    freeHenvOnDestroy = false;

}  // wxDbConnectInf::FreeHenv()


void wxDbConnectInf::SetDsn(const wxString &dsn)
{
    wxASSERT(dsn.length() < WXSIZEOF(Dsn));

    wxStrncpy(Dsn, dsn, WXSIZEOF(Dsn)-1);
    Dsn[WXSIZEOF(Dsn)-1] = 0;  // Prevent buffer overrun
}  // wxDbConnectInf::SetDsn()


void wxDbConnectInf::SetUserID(const wxString &uid)
{
    wxASSERT(uid.length() < WXSIZEOF(Uid));
    wxStrncpy(Uid, uid, WXSIZEOF(Uid)-1);
    Uid[WXSIZEOF(Uid)-1] = 0;  // Prevent buffer overrun
}  // wxDbConnectInf::SetUserID()


void wxDbConnectInf::SetPassword(const wxString &password)
{
    wxASSERT(password.length() < WXSIZEOF(AuthStr));

    wxStrncpy(AuthStr, password, WXSIZEOF(AuthStr)-1);
    AuthStr[WXSIZEOF(AuthStr)-1] = 0;  // Prevent buffer overrun
}  // wxDbConnectInf::SetPassword()

void wxDbConnectInf::SetConnectionStr(const wxString &connectStr)
{
    wxASSERT(connectStr.length() < WXSIZEOF(ConnectionStr));

    useConnectionStr = wxStrlen(connectStr) > 0;

    wxStrncpy(ConnectionStr, connectStr, WXSIZEOF(ConnectionStr)-1);
    ConnectionStr[WXSIZEOF(ConnectionStr)-1] = 0;  // Prevent buffer overrun
}  // wxDbConnectInf::SetConnectionStr()


/********** wxDbColFor Constructor **********/
wxDbColFor::wxDbColFor()
{
    Initialize();
}  // wxDbColFor::wxDbColFor()


/********** wxDbColFor::Initialize() **********/
void wxDbColFor::Initialize()
{
    s_Field.Empty();
    int i;
    for (i=0; i<7; i++)
    {
        s_Format[i].Empty();
        s_Amount[i].Empty();
        i_Amount[i] = 0;
    }
    i_Nation      = 0;                     // 0=EU, 1=UK, 2=International, 3=US
    i_dbDataType  = 0;
    i_sqlDataType = 0;
    Format(1,DB_DATA_TYPE_VARCHAR,0,0,0);  // the Function that does the work
}  // wxDbColFor::Initialize()


/********** wxDbColFor::Format() **********/
int wxDbColFor::Format(int Nation, int dbDataType, SWORD sqlDataType,
                       short columnLength, short decimalDigits)
{
    // ----------------------------------------------------------------------------------------
    // -- 19991224 : mj10777 : Create
    // There is still a lot of work to do here, but it is a start
    // It handles all the basic data-types that I have run into up to now
    // The main work will have be with Dates and float Formatting
    //    (US 1,000.00 ; EU 1.000,00)
    // There are wxWindow plans for locale support and the new wxDateTime.  If
    //    they define some constants (wxEUROPEAN) that can be gloably used,
    //    they should be used here.
    // ----------------------------------------------------------------------------------------
    // There should also be a function to scan in a string to fill the variable
    // ----------------------------------------------------------------------------------------
    wxString tempStr;
    i_Nation      = Nation;                                       // 0 = timestamp , 1=EU, 2=UK, 3=International, 4=US
    i_dbDataType  = dbDataType;
    i_sqlDataType = sqlDataType;
    s_Field.Printf(wxT("%s%d"),s_Amount[1].c_str(),i_Amount[1]);  // OK for VARCHAR, INTEGER and FLOAT

    if (i_dbDataType == 0)                                        // Filter unsupported dbDataTypes
    {
        if ((i_sqlDataType == SQL_VARCHAR)
#if wxUSE_UNICODE
    #if defined(SQL_WCHAR)
            || (i_sqlDataType == SQL_WCHAR)
    #endif
    #if defined(SQL_WVARCHAR)
            || (i_sqlDataType == SQL_WVARCHAR)
    #endif
#endif
            || (i_sqlDataType == SQL_LONGVARCHAR))
            i_dbDataType = DB_DATA_TYPE_VARCHAR;
        if ((i_sqlDataType == SQL_C_DATE) || (i_sqlDataType == SQL_C_TIMESTAMP))
            i_dbDataType = DB_DATA_TYPE_DATE;
        if (i_sqlDataType == SQL_C_BIT)
            i_dbDataType = DB_DATA_TYPE_INTEGER;
        if (i_sqlDataType == SQL_NUMERIC)
            i_dbDataType = DB_DATA_TYPE_VARCHAR;   // glt - ??? is this right?
        if (i_sqlDataType == SQL_REAL)
            i_dbDataType = DB_DATA_TYPE_FLOAT;
        if (i_sqlDataType == SQL_C_BINARY)
            i_dbDataType = DB_DATA_TYPE_BLOB;
    }

    if ((i_dbDataType == DB_DATA_TYPE_INTEGER) && (i_sqlDataType == SQL_C_DOUBLE))
    {   // DBASE Numeric
        i_dbDataType = DB_DATA_TYPE_FLOAT;
    }

    switch(i_dbDataType)     // TBD: Still a lot of proper formatting to do
    {
        case DB_DATA_TYPE_VARCHAR:
            s_Field = wxT("%s");
            break;
        case DB_DATA_TYPE_INTEGER:
            s_Field = wxT("%d");
            break;
        case DB_DATA_TYPE_FLOAT:
            if (decimalDigits == 0)
                decimalDigits = 2;
            tempStr.Printf(wxT("%%%d.%d"), columnLength, decimalDigits);
            s_Field.Printf(wxT("%sf"), tempStr.c_str());
            break;
        case DB_DATA_TYPE_DATE:
            if (i_Nation == 0)      // timestamp       YYYY-MM-DD HH:MM:SS.SSS (tested for SYBASE)
            {
                s_Field = wxT("%04d-%02d-%02d %02d:%02d:%02d.%03d");
            }
            if (i_Nation == 1)      // European        DD.MM.YYYY HH:MM:SS.SSS
            {
                s_Field = wxT("%02d.%02d.%04d %02d:%02d:%02d.%03d");
            }
            if (i_Nation == 2)      // UK              DD/MM/YYYY HH:MM:SS.SSS
            {
                s_Field = wxT("%02d/%02d/%04d %02d:%02d:%02d.%03d");
            }
            if (i_Nation == 3)      // International   YYYY-MM-DD HH:MM:SS.SSS
            {
                s_Field = wxT("%04d-%02d-%02d %02d:%02d:%02d.%03d");
            }
            if (i_Nation == 4)      // US              MM/DD/YYYY HH:MM:SS.SSS
            {
                s_Field = wxT("%02d/%02d/%04d %02d:%02d:%02d.%03d");
            }
            break;
          case DB_DATA_TYPE_BLOB:
            s_Field.Printf(wxT("Unable to format(%d)-SQL(%d)"), dbDataType,sqlDataType);        //
                break;
        default:
            s_Field.Printf(wxT("Unknown Format(%d)-SQL(%d)"), dbDataType,sqlDataType);        //
            break;
    };
    return TRUE;
}  // wxDbColFor::Format()


/********** wxDbColInf Constructor **********/
wxDbColInf::wxDbColInf()
{
    Initialize();
}  // wxDbColInf::wxDbColInf()


/********** wxDbColInf Destructor ********/
wxDbColInf::~wxDbColInf()
{
    if (pColFor)
        delete pColFor;
    pColFor = NULL;
}  // wxDbColInf::~wxDbColInf()


bool wxDbColInf::Initialize()
{
    catalog[0]      = 0;
    schema[0]       = 0;
    tableName[0]    = 0;
    colName[0]      = 0;
    sqlDataType     = 0;
    typeName[0]     = 0;
    columnLength    = 0;
    bufferSize      = 0;
    decimalDigits   = 0;
    numPrecRadix    = 0;
    nullable        = 0;
    remarks[0]      = 0;
    dbDataType      = 0;
    PkCol           = 0;
    PkTableName[0]  = 0;
    FkCol           = 0;
    FkTableName[0]  = 0;
    pColFor         = NULL;

    return true;
}  // wxDbColInf::Initialize()


/********** wxDbTableInf Constructor ********/
wxDbTableInf::wxDbTableInf()
{
    Initialize();
}  // wxDbTableInf::wxDbTableInf()


/********** wxDbTableInf Constructor ********/
wxDbTableInf::~wxDbTableInf()
{
    if (pColInf)
        delete [] pColInf;
    pColInf = NULL;
}  // wxDbTableInf::~wxDbTableInf()


bool wxDbTableInf::Initialize()
{
    tableName[0]    = 0;
    tableType[0]    = 0;
    tableRemarks[0] = 0;
    numCols         = 0;
    pColInf         = NULL;

    return true;
}  // wxDbTableInf::Initialize()


/********** wxDbInf Constructor *************/
wxDbInf::wxDbInf()
{
    Initialize();
}  // wxDbInf::wxDbInf()


/********** wxDbInf Destructor *************/
wxDbInf::~wxDbInf()
{
  if (pTableInf)
    delete [] pTableInf;
  pTableInf = NULL;
}  // wxDbInf::~wxDbInf()


/********** wxDbInf::Initialize() *************/
bool wxDbInf::Initialize()
{
    catalog[0]      = 0;
    schema[0]       = 0;
    numTables       = 0;
    pTableInf       = NULL;

    return true;
}  // wxDbInf::Initialize()


/********** wxDb Constructor **********/
wxDb::wxDb(const HENV &aHenv, bool FwdOnlyCursors)
{
    // Copy the HENV into the db class
    henv = aHenv;
    fwdOnlyCursors = FwdOnlyCursors;

    initialize();
} // wxDb::wxDb()


/********** wxDb Destructor **********/
wxDb::~wxDb()
{
    wxASSERT_MSG(!IsCached(),wxT("Cached connections must not be manually deleted, use\nwxDbFreeConnection() or wxDbCloseConnections()."));

    if (IsOpen())
    {
        Close();
    }
}  // wxDb destructor



/********** PRIVATE! wxDb::initialize PRIVATE! **********/
/********** wxDb::initialize() **********/
void wxDb::initialize()
/*
 * Private member function that sets all wxDb member variables to
 * known values at creation of the wxDb
 */
{
    int i;

    fpSqlLog      = 0;            // Sql Log file pointer
    sqlLogState   = sqlLogOFF;    // By default, logging is turned off
    nTables       = 0;
    dbmsType      = dbmsUNIDENTIFIED;

    wxStrcpy(sqlState,wxEmptyString);
    wxStrcpy(errorMsg,wxEmptyString);
    nativeError = cbErrorMsg = 0;
    for (i = 0; i < DB_MAX_ERROR_HISTORY; i++)
        wxStrcpy(errorList[i], wxEmptyString);

    // Init typeInf structures
    typeInfVarchar.TypeName.Empty();
    typeInfVarchar.FsqlType      = 0;
    typeInfVarchar.Precision     = 0;
    typeInfVarchar.CaseSensitive = 0;
    typeInfVarchar.MaximumScale  = 0;

    typeInfInteger.TypeName.Empty();
    typeInfInteger.FsqlType      = 0;
    typeInfInteger.Precision     = 0;
    typeInfInteger.CaseSensitive = 0;
    typeInfInteger.MaximumScale  = 0;

    typeInfFloat.TypeName.Empty();
    typeInfFloat.FsqlType      = 0;
    typeInfFloat.Precision     = 0;
    typeInfFloat.CaseSensitive = 0;
    typeInfFloat.MaximumScale  = 0;

    typeInfDate.TypeName.Empty();
    typeInfDate.FsqlType      = 0;
    typeInfDate.Precision     = 0;
    typeInfDate.CaseSensitive = 0;
    typeInfDate.MaximumScale  = 0;

    typeInfBlob.TypeName.Empty();
    typeInfBlob.FsqlType      = 0;
    typeInfBlob.Precision     = 0;
    typeInfBlob.CaseSensitive = 0;
    typeInfBlob.MaximumScale  = 0;

    typeInfMemo.TypeName.Empty();
    typeInfMemo.FsqlType      = 0;
    typeInfMemo.Precision     = 0;
    typeInfMemo.CaseSensitive = 0;
    typeInfMemo.MaximumScale  = 0;

    // Error reporting is turned OFF by default
    silent = true;

    // Allocate a data source connection handle
    if (SQLAllocConnect(henv, &hdbc) != SQL_SUCCESS)
        DispAllErrors(henv);

    // Initialize the db status flag
    DB_STATUS = 0;

    // Mark database as not open as of yet
    dbIsOpen = false;
    dbIsCached = false;
    dbOpenedWithConnectionString = false;
}  // wxDb::initialize()


/********** PRIVATE! wxDb::convertUserID PRIVATE! **********/
//
// NOTE: Return value from this function MUST be copied
//       immediately, as the value is not good after
//       this function has left scope.
//
const wxChar *wxDb::convertUserID(const wxChar *userID, wxString &UserID)
{
    if (userID)
    {
        if (!wxStrlen(userID))
            UserID = uid;
        else
            UserID = userID;
    }
    else
        UserID.Empty();

    // dBase does not use user names, and some drivers fail if you try to pass one
    if ( Dbms() == dbmsDBASE
         || Dbms() == dbmsXBASE_SEQUITER )
        UserID.Empty();

    // Some databases require user names to be specified in uppercase,
    // so force the name to uppercase
    if ((Dbms() == dbmsORACLE) ||
        (Dbms() == dbmsMAXDB))
        UserID = UserID.Upper();

    return UserID.c_str();
}  // wxDb::convertUserID()


bool wxDb::determineDataTypes(bool failOnDataTypeUnsupported)
{
    size_t iIndex;

    // These are the possible SQL types we check for use against the datasource we are connected
    // to for the purpose of determining which data type to use for the basic character strings
    // column types
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlCharTypes[] = {
#if wxUSE_UNICODE && defined(SQL_WVARCHAR)
        SQL_WVARCHAR,
#endif
        SQL_VARCHAR,
#if wxUSE_UNICODE && defined(SQL_WVARCHAR)
        SQL_WCHAR,
#endif
        SQL_CHAR
    };

    // These are the possible SQL types we check for use against the datasource we are connected
    // to for the purpose of determining which data type to use for the basic non-floating point
    // column types
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlIntegerTypes[] = {
        SQL_INTEGER
    };

    // These are the possible SQL types we check for use against the datasource we are connected
    // to for the purpose of determining which data type to use for the basic floating point number
    // column types
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlFloatTypes[] = {
        SQL_DOUBLE,
        SQL_REAL,
        SQL_FLOAT,
        SQL_DECIMAL,
        SQL_NUMERIC
    };

    // These are the possible SQL types we check for use agains the datasource we are connected
    // to for the purpose of determining which data type to use for the date/time column types
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlDateTypes[] = {
        SQL_TIMESTAMP,
        SQL_DATE,
#ifdef SQL_DATETIME
        SQL_DATETIME
#endif
    };

    // These are the possible SQL types we check for use agains the datasource we are connected
    // to for the purpose of determining which data type to use for the BLOB column types.
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlBlobTypes[] = {
        SQL_LONGVARBINARY,
        SQL_VARBINARY
    };

    // These are the possible SQL types we check for use agains the datasource we are connected
    // to for the purpose of determining which data type to use for the MEMO column types
    // (a type which allow to store large strings; like VARCHAR just with a bigger precision)
    //
    // NOTE: The first type in this enumeration that is determined to be supported by the
    //       datasource/driver is the one that will be used.
    SWORD PossibleSqlMemoTypes[] = {
        SQL_LONGVARCHAR,
    };


    // Query the data source regarding data type information

    //
    // The way it was determined which SQL data types to use was by calling SQLGetInfo
    // for all of the possible SQL data types to see which ones were supported.  If
    // a type is not supported, the SQLFetch() that's called from getDataTypeInfo()
    // fails with SQL_NO_DATA_FOUND.  This is ugly because I'm sure the three SQL data
    // types I've selected below will not always be what we want.  These are just
    // what happened to work against an Oracle 7/Intersolv combination.  The following is
    // a complete list of the results I got back against the Oracle 7 database:
    //
    // SQL_BIGINT             SQL_NO_DATA_FOUND
    // SQL_BINARY             SQL_NO_DATA_FOUND
    // SQL_BIT                SQL_NO_DATA_FOUND
    // SQL_CHAR               type name = 'CHAR', Precision = 255
    // SQL_DATE               SQL_NO_DATA_FOUND
    // SQL_DECIMAL            type name = 'NUMBER', Precision = 38
    // SQL_DOUBLE             type name = 'NUMBER', Precision = 15
    // SQL_FLOAT              SQL_NO_DATA_FOUND
    // SQL_INTEGER            SQL_NO_DATA_FOUND
    // SQL_LONGVARBINARY      type name = 'LONG RAW', Precision = 2 billion
    // SQL_LONGVARCHAR        type name = 'LONG', Precision = 2 billion
    // SQL_NUMERIC            SQL_NO_DATA_FOUND
    // SQL_REAL               SQL_NO_DATA_FOUND
    // SQL_SMALLINT           SQL_NO_DATA_FOUND
    // SQL_TIME               SQL_NO_DATA_FOUND
    // SQL_TIMESTAMP          type name = 'DATE', Precision = 19
    // SQL_VARBINARY          type name = 'RAW', Precision = 255
    // SQL_VARCHAR            type name = 'VARCHAR2', Precision = 2000
    // =====================================================================
    // Results from a Microsoft Access 7.0 db, using a driver from Microsoft
    //
    // SQL_VARCHAR            type name = 'TEXT', Precision = 255
    // SQL_TIMESTAMP          type name = 'DATETIME'
    // SQL_DECIMAL            SQL_NO_DATA_FOUND
    // SQL_NUMERIC            type name = 'CURRENCY', Precision = 19
    // SQL_FLOAT              SQL_NO_DATA_FOUND
    // SQL_REAL               type name = 'SINGLE', Precision = 7
    // SQL_DOUBLE             type name = 'DOUBLE', Precision = 15
    // SQL_INTEGER            type name = 'LONG', Precision = 10

    // Query the data source for info about itself
    if (!getDbInfo(failOnDataTypeUnsupported))
        return false;

    // --------------- Varchar - (Variable length character string) ---------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlCharTypes) &&
                     !getDataTypeInfo(PossibleSqlCharTypes[iIndex], typeInfVarchar); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlCharTypes))
        typeInfVarchar.FsqlType = PossibleSqlCharTypes[iIndex];
    else if (failOnDataTypeUnsupported)
        return false;

    // --------------- Float ---------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlFloatTypes) &&
                     !getDataTypeInfo(PossibleSqlFloatTypes[iIndex], typeInfFloat); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlFloatTypes))
        typeInfFloat.FsqlType = PossibleSqlFloatTypes[iIndex];
    else if (failOnDataTypeUnsupported)
        return false;

    // --------------- Integer -------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlIntegerTypes) &&
                     !getDataTypeInfo(PossibleSqlIntegerTypes[iIndex], typeInfInteger); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlIntegerTypes))
        typeInfInteger.FsqlType = PossibleSqlIntegerTypes[iIndex];
    else if (failOnDataTypeUnsupported)
    {
        // If no non-floating point data types are supported, we'll
        // use the type assigned for floats to store integers as well
        if (!getDataTypeInfo(typeInfFloat.FsqlType, typeInfInteger))
        {
            if (failOnDataTypeUnsupported)
                return false;
        }
        else
            typeInfInteger.FsqlType = typeInfFloat.FsqlType;
    }

    // --------------- Date/Time ---------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlDateTypes) &&
                     !getDataTypeInfo(PossibleSqlDateTypes[iIndex], typeInfDate); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlDateTypes))
        typeInfDate.FsqlType = PossibleSqlDateTypes[iIndex];
    else if (failOnDataTypeUnsupported)
        return false;

    // --------------- BLOB ---------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlBlobTypes) &&
                     !getDataTypeInfo(PossibleSqlBlobTypes[iIndex], typeInfBlob); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlBlobTypes))
        typeInfBlob.FsqlType = PossibleSqlBlobTypes[iIndex];
    else if (failOnDataTypeUnsupported)
        return false;

    // --------------- MEMO ---------------
    for (iIndex = 0; iIndex < WXSIZEOF(PossibleSqlMemoTypes) &&
                     !getDataTypeInfo(PossibleSqlMemoTypes[iIndex], typeInfMemo); ++iIndex)
    {}

    if (iIndex < WXSIZEOF(PossibleSqlMemoTypes))
        typeInfMemo.FsqlType = PossibleSqlMemoTypes[iIndex];
    else if (failOnDataTypeUnsupported)
        return false;

    return true;
}  // wxDb::determineDataTypes


bool wxDb::open(bool failOnDataTypeUnsupported)
{
/*
    If using Intersolv branded ODBC drivers, this is the place where you would substitute
    your branded driver license information

    SQLSetConnectOption(hdbc, 1041, (UDWORD) wxEmptyString);
    SQLSetConnectOption(hdbc, 1042, (UDWORD) wxEmptyString);
*/

    // Mark database as open
    dbIsOpen = true;

    // Allocate a statement handle for the database connection
    if (SQLAllocStmt(hdbc, &hstmt) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));

    // Set Connection Options
    if (!setConnectionOptions())
        return false;

    if (!determineDataTypes(failOnDataTypeUnsupported))
        return false;

#ifdef DBDEBUG_CONSOLE
    cout << wxT("VARCHAR DATA TYPE: ") << typeInfVarchar.TypeName << endl;
    cout << wxT("INTEGER DATA TYPE: ") << typeInfInteger.TypeName << endl;
    cout << wxT("FLOAT   DATA TYPE: ") << typeInfFloat.TypeName << endl;
    cout << wxT("DATE    DATA TYPE: ") << typeInfDate.TypeName << endl;
    cout << wxT("BLOB    DATA TYPE: ") << typeInfBlob.TypeName << endl;
    cout << wxT("MEMO    DATA TYPE: ") << typeInfMemo.TypeName << endl;
    cout << endl;
#endif

    // Completed Successfully
    return true;
}

bool wxDb::Open(const wxString& inConnectStr, bool failOnDataTypeUnsupported)
{
    wxASSERT(inConnectStr.length());
    return Open(inConnectStr, NULL, failOnDataTypeUnsupported);
}

bool wxDb::Open(const wxString& inConnectStr, SQLHWND parentWnd, bool failOnDataTypeUnsupported)
{
    dsn        = wxEmptyString;
    uid        = wxEmptyString;
    authStr    = wxEmptyString;

    RETCODE retcode;

    if (!FwdOnlyCursors())
    {
        // Specify that the ODBC cursor library be used, if needed.  This must be
        // specified before the connection is made.
        retcode = SQLSetConnectOption(hdbc, SQL_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED);

#ifdef DBDEBUG_CONSOLE
        if (retcode == SQL_SUCCESS)
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) successful") << endl;
        else
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) failed") << endl;
#else
        wxUnusedVar(retcode);
#endif
    }

    // Connect to the data source
    SQLTCHAR outConnectBuffer[SQL_MAX_CONNECTSTR_LEN+1];  // MS recommends at least 1k buffer
    short outConnectBufferLen;

    inConnectionStr = inConnectStr;

    retcode = SQLDriverConnect(hdbc, parentWnd, (SQLTCHAR FAR *)inConnectionStr.c_str(),
                        (SWORD)inConnectionStr.length(), (SQLTCHAR FAR *)outConnectBuffer,
                        WXSIZEOF(outConnectBuffer), &outConnectBufferLen, SQL_DRIVER_COMPLETE );

    if ((retcode != SQL_SUCCESS) &&
        (retcode != SQL_SUCCESS_WITH_INFO))
        return(DispAllErrors(henv, hdbc));

    outConnectBuffer[outConnectBufferLen] = 0;
    outConnectionStr = outConnectBuffer;
    dbOpenedWithConnectionString = true;

    return open(failOnDataTypeUnsupported);
}

/********** wxDb::Open() **********/
bool wxDb::Open(const wxString &Dsn, const wxString &Uid, const wxString &AuthStr, bool failOnDataTypeUnsupported)
{
    wxASSERT(!Dsn.empty());
    dsn        = Dsn;
    uid        = Uid;
    authStr    = AuthStr;

    inConnectionStr = wxEmptyString;
    outConnectionStr = wxEmptyString;

    RETCODE retcode;

    if (!FwdOnlyCursors())
    {
        // Specify that the ODBC cursor library be used, if needed.  This must be
        // specified before the connection is made.
        retcode = SQLSetConnectOption(hdbc, SQL_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED);

#ifdef DBDEBUG_CONSOLE
        if (retcode == SQL_SUCCESS)
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) successful") << endl;
        else
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) failed") << endl;
#else
        wxUnusedVar( retcode );
#endif
    }

    // Connect to the data source
    retcode = SQLConnect(hdbc, (SQLTCHAR FAR *) dsn.c_str(), SQL_NTS,
                         (SQLTCHAR FAR *) uid.c_str(), SQL_NTS,
                         (SQLTCHAR FAR *) authStr.c_str(), SQL_NTS);

    if ((retcode != SQL_SUCCESS) &&
        (retcode != SQL_SUCCESS_WITH_INFO))
        return(DispAllErrors(henv, hdbc));

    return open(failOnDataTypeUnsupported);

} // wxDb::Open()


bool wxDb::Open(wxDbConnectInf *dbConnectInf, bool failOnDataTypeUnsupported)
{
    wxASSERT(dbConnectInf);

    // Use the connection string if one is present
    if (dbConnectInf->UseConnectionStr())
        return Open(dbConnectInf->GetConnectionStr(), failOnDataTypeUnsupported);
    else
        return Open(dbConnectInf->GetDsn(), dbConnectInf->GetUserID(),
                    dbConnectInf->GetPassword(), failOnDataTypeUnsupported);
}  // wxDb::Open()


bool wxDb::Open(wxDb *copyDb)
{
    dsn              = copyDb->GetDatasourceName();
    uid              = copyDb->GetUsername();
    authStr          = copyDb->GetPassword();
    inConnectionStr  = copyDb->GetConnectionInStr();
    outConnectionStr = copyDb->GetConnectionOutStr();

    RETCODE retcode;

    if (!FwdOnlyCursors())
    {
        // Specify that the ODBC cursor library be used, if needed.  This must be
        // specified before the connection is made.
        retcode = SQLSetConnectOption(hdbc, SQL_ODBC_CURSORS, SQL_CUR_USE_IF_NEEDED);

#ifdef DBDEBUG_CONSOLE
        if (retcode == SQL_SUCCESS)
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) successful") << endl;
        else
            cout << wxT("SQLSetConnectOption(CURSOR_LIB) failed") << endl;
#else
        wxUnusedVar( retcode );
#endif
    }

    if (copyDb->OpenedWithConnectionString())
    {
        // Connect to the data source
        SQLTCHAR outConnectBuffer[SQL_MAX_CONNECTSTR_LEN+1];
        short outConnectBufferLen;

        inConnectionStr = copyDb->GetConnectionInStr();

        retcode = SQLDriverConnect(hdbc, NULL, (SQLTCHAR FAR *)inConnectionStr.c_str(),
                            (SWORD)inConnectionStr.length(), (SQLTCHAR FAR *)outConnectBuffer,
                            WXSIZEOF(outConnectBuffer), &outConnectBufferLen, SQL_DRIVER_COMPLETE);

        if ((retcode != SQL_SUCCESS) &&
            (retcode != SQL_SUCCESS_WITH_INFO))
            return(DispAllErrors(henv, hdbc));

        outConnectBuffer[outConnectBufferLen] = 0;
        outConnectionStr = outConnectBuffer;
        dbOpenedWithConnectionString = true;
    }
    else
    {
        // Connect to the data source
        retcode = SQLConnect(hdbc, (SQLTCHAR FAR *) dsn.c_str(), SQL_NTS,
                             (SQLTCHAR FAR *) uid.c_str(), SQL_NTS,
                             (SQLTCHAR FAR *) authStr.c_str(), SQL_NTS);
    }

    if ((retcode != SQL_SUCCESS) &&
        (retcode != SQL_SUCCESS_WITH_INFO))
        return(DispAllErrors(henv, hdbc));

/*
    If using Intersolv branded ODBC drivers, this is the place where you would substitute
    your branded driver license information

    SQLSetConnectOption(hdbc, 1041, (UDWORD) wxEmptyString);
    SQLSetConnectOption(hdbc, 1042, (UDWORD) wxEmptyString);
*/

    // Mark database as open
    dbIsOpen = true;

    // Allocate a statement handle for the database connection
    if (SQLAllocStmt(hdbc, &hstmt) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));

    // Set Connection Options
    if (!setConnectionOptions())
        return false;

    // Instead of Querying the data source for info about itself, it can just be copied
    // from the wxDb instance that was passed in (copyDb).
    wxStrcpy(dbInf.serverName,copyDb->dbInf.serverName);
    wxStrcpy(dbInf.databaseName,copyDb->dbInf.databaseName);
    wxStrcpy(dbInf.dbmsName,copyDb->dbInf.dbmsName);
    wxStrcpy(dbInf.dbmsVer,copyDb->dbInf.dbmsVer);
    dbInf.maxConnections = copyDb->dbInf.maxConnections;
    dbInf.maxStmts = copyDb->dbInf.maxStmts;
    wxStrcpy(dbInf.driverName,copyDb->dbInf.driverName);
    wxStrcpy(dbInf.odbcVer,copyDb->dbInf.odbcVer);
    wxStrcpy(dbInf.drvMgrOdbcVer,copyDb->dbInf.drvMgrOdbcVer);
    wxStrcpy(dbInf.driverVer,copyDb->dbInf.driverVer);
    dbInf.apiConfLvl = copyDb->dbInf.apiConfLvl;
    dbInf.cliConfLvl = copyDb->dbInf.cliConfLvl;
    dbInf.sqlConfLvl = copyDb->dbInf.sqlConfLvl;
    wxStrcpy(dbInf.outerJoins,copyDb->dbInf.outerJoins);
    wxStrcpy(dbInf.procedureSupport,copyDb->dbInf.procedureSupport);
    wxStrcpy(dbInf.accessibleTables,copyDb->dbInf.accessibleTables);
    dbInf.cursorCommitBehavior = copyDb->dbInf.cursorCommitBehavior;
    dbInf.cursorRollbackBehavior = copyDb->dbInf.cursorRollbackBehavior;
    dbInf.supportNotNullClause = copyDb->dbInf.supportNotNullClause;
    wxStrcpy(dbInf.supportIEF,copyDb->dbInf.supportIEF);
    dbInf.txnIsolation = copyDb->dbInf.txnIsolation;
    dbInf.txnIsolationOptions = copyDb->dbInf.txnIsolationOptions;
    dbInf.fetchDirections = copyDb->dbInf.fetchDirections;
    dbInf.lockTypes = copyDb->dbInf.lockTypes;
    dbInf.posOperations = copyDb->dbInf.posOperations;
    dbInf.posStmts = copyDb->dbInf.posStmts;
    dbInf.scrollConcurrency = copyDb->dbInf.scrollConcurrency;
    dbInf.scrollOptions = copyDb->dbInf.scrollOptions;
    dbInf.staticSensitivity = copyDb->dbInf.staticSensitivity;
    dbInf.txnCapable = copyDb->dbInf.txnCapable;
    dbInf.loginTimeout = copyDb->dbInf.loginTimeout;

    // VARCHAR = Variable length character string
    typeInfVarchar.FsqlType         = copyDb->typeInfVarchar.FsqlType;
    typeInfVarchar.TypeName         = copyDb->typeInfVarchar.TypeName;
    typeInfVarchar.Precision        = copyDb->typeInfVarchar.Precision;
    typeInfVarchar.CaseSensitive    = copyDb->typeInfVarchar.CaseSensitive;
    typeInfVarchar.MaximumScale     = copyDb->typeInfVarchar.MaximumScale;

    // Float
    typeInfFloat.FsqlType         = copyDb->typeInfFloat.FsqlType;
    typeInfFloat.TypeName         = copyDb->typeInfFloat.TypeName;
    typeInfFloat.Precision        = copyDb->typeInfFloat.Precision;
    typeInfFloat.CaseSensitive    = copyDb->typeInfFloat.CaseSensitive;
    typeInfFloat.MaximumScale     = copyDb->typeInfFloat.MaximumScale;

    // Integer
    typeInfInteger.FsqlType         = copyDb->typeInfInteger.FsqlType;
    typeInfInteger.TypeName         = copyDb->typeInfInteger.TypeName;
    typeInfInteger.Precision        = copyDb->typeInfInteger.Precision;
    typeInfInteger.CaseSensitive    = copyDb->typeInfInteger.CaseSensitive;
    typeInfInteger.MaximumScale     = copyDb->typeInfInteger.MaximumScale;

    // Date/Time
    typeInfDate.FsqlType         = copyDb->typeInfDate.FsqlType;
    typeInfDate.TypeName         = copyDb->typeInfDate.TypeName;
    typeInfDate.Precision        = copyDb->typeInfDate.Precision;
    typeInfDate.CaseSensitive    = copyDb->typeInfDate.CaseSensitive;
    typeInfDate.MaximumScale     = copyDb->typeInfDate.MaximumScale;

    // Blob
    typeInfBlob.FsqlType         = copyDb->typeInfBlob.FsqlType;
    typeInfBlob.TypeName         = copyDb->typeInfBlob.TypeName;
    typeInfBlob.Precision        = copyDb->typeInfBlob.Precision;
    typeInfBlob.CaseSensitive    = copyDb->typeInfBlob.CaseSensitive;
    typeInfBlob.MaximumScale     = copyDb->typeInfBlob.MaximumScale;

    // Memo
    typeInfMemo.FsqlType         = copyDb->typeInfMemo.FsqlType;
    typeInfMemo.TypeName         = copyDb->typeInfMemo.TypeName;
    typeInfMemo.Precision        = copyDb->typeInfMemo.Precision;
    typeInfMemo.CaseSensitive    = copyDb->typeInfMemo.CaseSensitive;
    typeInfMemo.MaximumScale     = copyDb->typeInfMemo.MaximumScale;

#ifdef DBDEBUG_CONSOLE
    cout << wxT("VARCHAR DATA TYPE: ") << typeInfVarchar.TypeName << endl;
    cout << wxT("INTEGER DATA TYPE: ") << typeInfInteger.TypeName << endl;
    cout << wxT("FLOAT   DATA TYPE: ") << typeInfFloat.TypeName << endl;
    cout << wxT("DATE    DATA TYPE: ") << typeInfDate.TypeName << endl;
    cout << wxT("BLOB    DATA TYPE: ") << typeInfBlob.TypeName << endl;
    cout << wxT("MEMO    DATA TYPE: ") << typeInfMemo.TypeName << endl;
    cout << endl;
#endif

    // Completed Successfully
    return true;
} // wxDb::Open() 2


/********** wxDb::setConnectionOptions() **********/
bool wxDb::setConnectionOptions(void)
/*
 * NOTE: The Intersolv/Oracle 7 driver was "Not Capable" of setting the login timeout.
 */
{
    SWORD cb;

    // I need to get the DBMS name here, because some of the connection options
    // are database specific and need to call the Dbms() function.
    RETCODE retcode;

    retcode = SQLGetInfo(hdbc, SQL_DBMS_NAME, (UCHAR *) dbInf.dbmsName, sizeof(dbInf.dbmsName), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        return(DispAllErrors(henv, hdbc));

    /* retcode = */ SQLSetConnectOption(hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
    /* retcode = */ SQLSetConnectOption(hdbc, SQL_OPT_TRACE, SQL_OPT_TRACE_OFF);
//  SQLSetConnectOption(hdbc, SQL_TXN_ISOLATION, SQL_TXN_READ_COMMITTED);  // No dirty reads

    // By default, MS Sql Server closes cursors on commit and rollback.  The following
    // call to SQLSetConnectOption() is needed to force SQL Server to preserve cursors
    // after a transaction.  This is a driver specific option and is not part of the
    // ODBC standard.  Note: this behavior is specific to the ODBC interface to SQL Server.
    // The database settings don't have any effect one way or the other.
    if (Dbms() == dbmsMS_SQL_SERVER)
    {
        const long SQL_PRESERVE_CURSORS = 1204L;
        const long SQL_PC_ON = 1L;
        /* retcode = */ SQLSetConnectOption(hdbc, SQL_PRESERVE_CURSORS, SQL_PC_ON);
    }

    // Display the connection options to verify them
#ifdef DBDEBUG_CONSOLE
    long l;
    cout << wxT("****** CONNECTION OPTIONS ******") << endl;

    retcode = SQLGetConnectOption(hdbc, SQL_AUTOCOMMIT, &l);
    if (retcode != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));
    cout << wxT("AUTOCOMMIT: ") << (l == SQL_AUTOCOMMIT_OFF ? "OFF" : "ON") << endl;

    retcode = SQLGetConnectOption(hdbc, SQL_ODBC_CURSORS, &l);
    if (retcode != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));
    cout << wxT("ODBC CURSORS: ");
    switch(l)
    {
        case(SQL_CUR_USE_IF_NEEDED):
            cout << wxT("SQL_CUR_USE_IF_NEEDED");
            break;
        case(SQL_CUR_USE_ODBC):
            cout << wxT("SQL_CUR_USE_ODBC");
            break;
        case(SQL_CUR_USE_DRIVER):
            cout << wxT("SQL_CUR_USE_DRIVER");
            break;
    }
    cout << endl;

    retcode = SQLGetConnectOption(hdbc, SQL_OPT_TRACE, &l)
    if (retcode != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));
    cout << wxT("TRACING: ") << (l == SQL_OPT_TRACE_OFF ? wxT("OFF") : wxT("ON")) << endl;

    cout << endl;
#endif

    // Completed Successfully
    return true;

} // wxDb::setConnectionOptions()


/********** wxDb::getDbInfo() **********/
bool wxDb::getDbInfo(bool failOnDataTypeUnsupported)
{
    SWORD cb;
    RETCODE retcode;

    retcode = SQLGetInfo(hdbc, SQL_SERVER_NAME, (UCHAR*) dbInf.serverName, sizeof(dbInf.serverName), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DATABASE_NAME, (UCHAR*) dbInf.databaseName, sizeof(dbInf.databaseName), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DBMS_NAME, (UCHAR*) dbInf.dbmsName, sizeof(dbInf.dbmsName), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    // 16-Mar-1999
    // After upgrading to MSVC6, the original 20 char buffer below was insufficient,
    // causing database connectivity to fail in some cases.
    retcode = SQLGetInfo(hdbc, SQL_DBMS_VER, (UCHAR*) dbInf.dbmsVer, sizeof(dbInf.dbmsVer), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ACTIVE_CONNECTIONS, (UCHAR*) &dbInf.maxConnections, sizeof(dbInf.maxConnections), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ACTIVE_STATEMENTS, (UCHAR*) &dbInf.maxStmts, sizeof(dbInf.maxStmts), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DRIVER_NAME, (UCHAR*) dbInf.driverName, sizeof(dbInf.driverName), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DRIVER_ODBC_VER, (UCHAR*) dbInf.odbcVer, sizeof(dbInf.odbcVer), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ODBC_VER, (UCHAR*) dbInf.drvMgrOdbcVer, sizeof(dbInf.drvMgrOdbcVer), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DRIVER_VER, (UCHAR*) dbInf.driverVer, sizeof(dbInf.driverVer), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ODBC_API_CONFORMANCE, (UCHAR*) &dbInf.apiConfLvl, sizeof(dbInf.apiConfLvl), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ODBC_SAG_CLI_CONFORMANCE, (UCHAR*) &dbInf.cliConfLvl, sizeof(dbInf.cliConfLvl), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        // Not all drivers support this call - Nick Gorham(unixODBC)
        dbInf.cliConfLvl = 0;
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ODBC_SQL_CONFORMANCE, (UCHAR*) &dbInf.sqlConfLvl, sizeof(dbInf.sqlConfLvl), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_OUTER_JOINS, (UCHAR*) dbInf.outerJoins, sizeof(dbInf.outerJoins), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_PROCEDURES, (UCHAR*) dbInf.procedureSupport, sizeof(dbInf.procedureSupport), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ACCESSIBLE_TABLES, (UCHAR*) dbInf.accessibleTables, sizeof(dbInf.accessibleTables), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_CURSOR_COMMIT_BEHAVIOR, (UCHAR*) &dbInf.cursorCommitBehavior, sizeof(dbInf.cursorCommitBehavior), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_CURSOR_ROLLBACK_BEHAVIOR, (UCHAR*) &dbInf.cursorRollbackBehavior, sizeof(dbInf.cursorRollbackBehavior), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_NON_NULLABLE_COLUMNS, (UCHAR*) &dbInf.supportNotNullClause, sizeof(dbInf.supportNotNullClause), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_ODBC_SQL_OPT_IEF, (UCHAR*) dbInf.supportIEF, sizeof(dbInf.supportIEF), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_DEFAULT_TXN_ISOLATION, (UCHAR*) &dbInf.txnIsolation, sizeof(dbInf.txnIsolation), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_TXN_ISOLATION_OPTION, (UCHAR*) &dbInf.txnIsolationOptions, sizeof(dbInf.txnIsolationOptions), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_FETCH_DIRECTION, (UCHAR*) &dbInf.fetchDirections, sizeof(dbInf.fetchDirections), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_LOCK_TYPES, (UCHAR*) &dbInf.lockTypes, sizeof(dbInf.lockTypes), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_POS_OPERATIONS, (UCHAR*) &dbInf.posOperations, sizeof(dbInf.posOperations), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_POSITIONED_STATEMENTS, (UCHAR*) &dbInf.posStmts, sizeof(dbInf.posStmts), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_SCROLL_CONCURRENCY, (UCHAR*) &dbInf.scrollConcurrency, sizeof(dbInf.scrollConcurrency), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_SCROLL_OPTIONS, (UCHAR*) &dbInf.scrollOptions, sizeof(dbInf.scrollOptions), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_STATIC_SENSITIVITY, (UCHAR*) &dbInf.staticSensitivity, sizeof(dbInf.staticSensitivity), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_TXN_CAPABLE, (UCHAR*) &dbInf.txnCapable, sizeof(dbInf.txnCapable), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

    retcode = SQLGetInfo(hdbc, SQL_LOGIN_TIMEOUT, (UCHAR*) &dbInf.loginTimeout, sizeof(dbInf.loginTimeout), &cb);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        DispAllErrors(henv, hdbc);
        if (failOnDataTypeUnsupported)
            return false;
    }

#ifdef DBDEBUG_CONSOLE
    cout << wxT("***** DATA SOURCE INFORMATION *****") << endl;
    cout << wxT(wxT("SERVER Name: ") << dbInf.serverName << endl;
    cout << wxT("DBMS Name: ") << dbInf.dbmsName << wxT("; DBMS Version: ") << dbInf.dbmsVer << endl;
    cout << wxT("ODBC Version: ") << dbInf.odbcVer << wxT("; Driver Version: ") << dbInf.driverVer << endl;

    cout << wxT("API Conf. Level: ");
    switch(dbInf.apiConfLvl)
    {
        case SQL_OAC_NONE:      cout << wxT("None");       break;
        case SQL_OAC_LEVEL1:    cout << wxT("Level 1");    break;
        case SQL_OAC_LEVEL2:    cout << wxT("Level 2");    break;
    }
    cout << endl;

    cout << wxT("SAG CLI Conf. Level: ");
    switch(dbInf.cliConfLvl)
    {
        case SQL_OSCC_NOT_COMPLIANT:    cout << wxT("Not Compliant");    break;
        case SQL_OSCC_COMPLIANT:        cout << wxT("Compliant");        break;
    }
    cout << endl;

    cout << wxT("SQL Conf. Level: ");
    switch(dbInf.sqlConfLvl)
    {
        case SQL_OSC_MINIMUM:     cout << wxT("Minimum Grammar");     break;
        case SQL_OSC_CORE:        cout << wxT("Core Grammar");        break;
        case SQL_OSC_EXTENDED:    cout << wxT("Extended Grammar");    break;
    }
    cout << endl;

    cout << wxT("Max. Connections: ")       << dbInf.maxConnections   << endl;
    cout << wxT("Outer Joins: ")            << dbInf.outerJoins       << endl;
    cout << wxT("Support for Procedures: ") << dbInf.procedureSupport << endl;
    cout << wxT("All tables accessible : ") << dbInf.accessibleTables << endl;
    cout << wxT("Cursor COMMIT Behavior: ");
    switch(dbInf.cursorCommitBehavior)
    {
        case SQL_CB_DELETE:        cout << wxT("Delete cursors");      break;
        case SQL_CB_CLOSE:         cout << wxT("Close cursors");       break;
        case SQL_CB_PRESERVE:      cout << wxT("Preserve cursors");    break;
    }
    cout << endl;

    cout << wxT("Cursor ROLLBACK Behavior: ");
    switch(dbInf.cursorRollbackBehavior)
    {
        case SQL_CB_DELETE:      cout << wxT("Delete cursors");      break;
        case SQL_CB_CLOSE:       cout << wxT("Close cursors");       break;
        case SQL_CB_PRESERVE:    cout << wxT("Preserve cursors");    break;
    }
    cout << endl;

    cout << wxT("Support NOT NULL clause: ");
    switch(dbInf.supportNotNullClause)
    {
        case SQL_NNC_NULL:        cout << wxT("No");        break;
        case SQL_NNC_NON_NULL:    cout << wxT("Yes");       break;
    }
    cout << endl;

    cout << wxT("Support IEF (Ref. Integrity): ") << dbInf.supportIEF   << endl;
    cout << wxT("Login Timeout: ")                << dbInf.loginTimeout << endl;

    cout << endl << endl << wxT("more ...") << endl;
    getchar();

    cout << wxT("Default Transaction Isolation: ";
    switch(dbInf.txnIsolation)
    {
        case SQL_TXN_READ_UNCOMMITTED:  cout << wxT("Read Uncommitted");    break;
        case SQL_TXN_READ_COMMITTED:    cout << wxT("Read Committed");      break;
        case SQL_TXN_REPEATABLE_READ:   cout << wxT("Repeatable Read");     break;
        case SQL_TXN_SERIALIZABLE:      cout << wxT("Serializable");        break;
#ifdef ODBC_V20
        case SQL_TXN_VERSIONING:        cout << wxT("Versioning");          break;
#endif
    }
    cout << endl;

    cout << wxT("Transaction Isolation Options: ");
    if (dbInf.txnIsolationOptions & SQL_TXN_READ_UNCOMMITTED)
        cout << wxT("Read Uncommitted, ");
    if (dbInf.txnIsolationOptions & SQL_TXN_READ_COMMITTED)
        cout << wxT("Read Committed, ");
    if (dbInf.txnIsolationOptions & SQL_TXN_REPEATABLE_READ)
        cout << wxT("Repeatable Read, ");
    if (dbInf.txnIsolationOptions & SQL_TXN_SERIALIZABLE)
        cout << wxT("Serializable, ");
#ifdef ODBC_V20
    if (dbInf.txnIsolationOptions & SQL_TXN_VERSIONING)
        cout << wxT("Versioning");
#endif
    cout << endl;

    cout << wxT("Fetch Directions Supported:") << endl << wxT("   ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_NEXT)
        cout << wxT("Next, ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_PRIOR)
        cout << wxT("Prev, ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_FIRST)
        cout << wxT("First, ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_LAST)
        cout << wxT("Last, ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_ABSOLUTE)
        cout << wxT("Absolute, ");
    if (dbInf.fetchDirections & SQL_FD_FETCH_RELATIVE)
        cout << wxT("Relative, ");
#ifdef ODBC_V20
    if (dbInf.fetchDirections & SQL_FD_FETCH_RESUME)
        cout << wxT("Resume, ");
#endif
    if (dbInf.fetchDirections & SQL_FD_FETCH_BOOKMARK)
        cout << wxT("Bookmark");
    cout << endl;

    cout << wxT("Lock Types Supported (SQLSetPos): ");
    if (dbInf.lockTypes & SQL_LCK_NO_CHANGE)
        cout << wxT("No Change, ");
    if (dbInf.lockTypes & SQL_LCK_EXCLUSIVE)
        cout << wxT("Exclusive, ");
    if (dbInf.lockTypes & SQL_LCK_UNLOCK)
        cout << wxT("UnLock");
    cout << endl;

    cout << wxT("Position Operations Supported (SQLSetPos): ");
    if (dbInf.posOperations & SQL_POS_POSITION)
        cout << wxT("Position, ");
    if (dbInf.posOperations & SQL_POS_REFRESH)
        cout << wxT("Refresh, ");
    if (dbInf.posOperations & SQL_POS_UPDATE)
        cout << wxT("Upd, "));
    if (dbInf.posOperations & SQL_POS_DELETE)
        cout << wxT("Del, ");
    if (dbInf.posOperations & SQL_POS_ADD)
        cout << wxT("Add");
    cout << endl;

    cout << wxT("Positioned Statements Supported: ");
    if (dbInf.posStmts & SQL_PS_POSITIONED_DELETE)
        cout << wxT("Pos delete, ");
    if (dbInf.posStmts & SQL_PS_POSITIONED_UPDATE)
        cout << wxT("Pos update, ");
    if (dbInf.posStmts & SQL_PS_SELECT_FOR_UPDATE)
        cout << wxT("Select for update");
    cout << endl;

    cout << wxT("Scroll Concurrency: ");
    if (dbInf.scrollConcurrency & SQL_SCCO_READ_ONLY)
        cout << wxT("Read Only, ");
    if (dbInf.scrollConcurrency & SQL_SCCO_LOCK)
        cout << wxT("Lock, ");
    if (dbInf.scrollConcurrency & SQL_SCCO_OPT_ROWVER)
        cout << wxT("Opt. Rowver, ");
    if (dbInf.scrollConcurrency & SQL_SCCO_OPT_VALUES)
        cout << wxT("Opt. Values");
    cout << endl;

    cout << wxT("Scroll Options: ");
    if (dbInf.scrollOptions & SQL_SO_FORWARD_ONLY)
        cout << wxT("Fwd Only, ");
    if (dbInf.scrollOptions & SQL_SO_STATIC)
        cout << wxT("Static, ");
    if (dbInf.scrollOptions & SQL_SO_KEYSET_DRIVEN)
        cout << wxT("Keyset Driven, ");
    if (dbInf.scrollOptions & SQL_SO_DYNAMIC)
        cout << wxT("Dynamic, ");
    if (dbInf.scrollOptions & SQL_SO_MIXED)
        cout << wxT("Mixed");
    cout << endl;

    cout << wxT("Static Sensitivity: ");
    if (dbInf.staticSensitivity & SQL_SS_ADDITIONS)
        cout << wxT("Additions, ");
    if (dbInf.staticSensitivity & SQL_SS_DELETIONS)
        cout << wxT("Deletions, ");
    if (dbInf.staticSensitivity & SQL_SS_UPDATES)
        cout << wxT("Updates");
    cout << endl;

    cout << wxT("Transaction Capable?: ");
    switch(dbInf.txnCapable)
    {
        case SQL_TC_NONE:          cout << wxT("No");            break;
        case SQL_TC_DML:           cout << wxT("DML Only");      break;
        case SQL_TC_DDL_COMMIT:    cout << wxT("DDL Commit");    break;
        case SQL_TC_DDL_IGNORE:    cout << wxT("DDL Ignore");    break;
        case SQL_TC_ALL:           cout << wxT("DDL & DML");     break;
    }
    cout << endl;

    cout << endl;
#endif

    // Completed Successfully
    return true;

} // wxDb::getDbInfo()


/********** wxDb::getDataTypeInfo() **********/
bool wxDb::getDataTypeInfo(SWORD fSqlType, wxDbSqlTypeInfo &structSQLTypeInfo)
{
/*
 * fSqlType will be something like SQL_VARCHAR.  This parameter determines
 * the data type inf. is gathered for.
 *
 * wxDbSqlTypeInfo is a structure that is filled in with data type information,
 */
    RETCODE retcode;
    SQLLEN  cbRet;

    // Get information about the data type specified
    if (SQLGetTypeInfo(hstmt, fSqlType) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));

    // Fetch the record
    retcode = SQLFetch(hstmt);
    if (retcode != SQL_SUCCESS)
    {
#ifdef DBDEBUG_CONSOLE
        if (retcode == SQL_NO_DATA_FOUND)
            cout << wxT("SQL_NO_DATA_FOUND fetching information about data type.") << endl;
#endif
        DispAllErrors(henv, hdbc, hstmt);
        SQLFreeStmt(hstmt, SQL_CLOSE);
        return false;
    }

    wxChar typeName[DB_TYPE_NAME_LEN+1];

    // Obtain columns from the record
    if (SQLGetData(hstmt, 1, SQL_C_WXCHAR, typeName, sizeof(typeName), &cbRet) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));

    structSQLTypeInfo.TypeName = typeName;

    // BJO 20000503: no more needed with new GetColumns...
#if  OLD_GETCOLUMNS
    // BJO 991209
    if (Dbms() == dbmsMY_SQL)
    {
        if (structSQLTypeInfo.TypeName == wxT("middleint"))
            structSQLTypeInfo.TypeName = wxT("mediumint");
        else if (structSQLTypeInfo.TypeName == wxT("middleint unsigned"))
            structSQLTypeInfo.TypeName = wxT("mediumint unsigned");
        else if (structSQLTypeInfo.TypeName == wxT("integer"))
            structSQLTypeInfo.TypeName = wxT("int");
        else if (structSQLTypeInfo.TypeName == wxT("integer unsigned"))
            structSQLTypeInfo.TypeName = wxT("int unsigned");
        else if (structSQLTypeInfo.TypeName == wxT("middleint"))
            structSQLTypeInfo.TypeName = wxT("mediumint");
        else if (structSQLTypeInfo.TypeName == wxT("varchar"))
            structSQLTypeInfo.TypeName = wxT("char");
    }

    // BJO 20000427 : OpenLink driver
    if (!wxStrncmp(dbInf.driverName, wxT("oplodbc"), 7) ||
        !wxStrncmp(dbInf.driverName, wxT("OLOD"), 4))
    {
        if (structSQLTypeInfo.TypeName == wxT("double precision"))
            structSQLTypeInfo.TypeName = wxT("real");
    }
#endif

    if (SQLGetData(hstmt, 3, SQL_C_LONG, (UCHAR*) &structSQLTypeInfo.Precision, 0, &cbRet) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));
    if (SQLGetData(hstmt, 8, SQL_C_SHORT, (UCHAR*) &structSQLTypeInfo.CaseSensitive, 0, &cbRet) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));
//    if (SQLGetData(hstmt, 14, SQL_C_SHORT, (UCHAR*) &structSQLTypeInfo.MinimumScale, 0, &cbRet) != SQL_SUCCESS)
//        return(DispAllErrors(henv, hdbc, hstmt));

    if (SQLGetData(hstmt, 15, SQL_C_SHORT,(UCHAR*)  &structSQLTypeInfo.MaximumScale, 0, &cbRet) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));

    if (structSQLTypeInfo.MaximumScale < 0)
        structSQLTypeInfo.MaximumScale = 0;

    // Close the statement handle which closes open cursors
    if (SQLFreeStmt(hstmt, SQL_CLOSE) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));

    // Completed Successfully
    return true;

} // wxDb::getDataTypeInfo()


/********** wxDb::Close() **********/
void wxDb::Close(void)
{
    // Close the Sql Log file
    if (fpSqlLog)
    {
        fclose(fpSqlLog);
        fpSqlLog = 0;
    }

    // Free statement handle
    if (dbIsOpen)
    {
        if (SQLFreeStmt(hstmt, SQL_DROP) != SQL_SUCCESS)
            DispAllErrors(henv, hdbc);
    }

    // Disconnect from the datasource
    if (SQLDisconnect(hdbc) != SQL_SUCCESS)
        DispAllErrors(henv, hdbc);

    // Free the connection to the datasource
    if (SQLFreeConnect(hdbc) != SQL_SUCCESS)
        DispAllErrors(henv, hdbc);

    // There should be zero Ctable objects still connected to this db object
    wxASSERT(nTables == 0);

#ifdef __WXDEBUG__
    {
#if wxUSE_THREADS
        wxCriticalSectionLocker lock(csTablesInUse);
#endif // wxUSE_THREADS
        wxTablesInUse *tiu;
        wxList::compatibility_iterator pNode;
        pNode = TablesInUse.GetFirst();
        wxString s,s2;
        while (pNode)
        {
            tiu = (wxTablesInUse *)pNode->GetData();
            if (tiu->pDb == this)
            {
                s.Printf(wxT("(%-20s)     tableID:[%6lu]     pDb:[%p]"),
                        tiu->tableName, tiu->tableID, wx_static_cast(void*, tiu->pDb));
                s2.Printf(wxT("Orphaned table found using pDb:[%p]"), wx_static_cast(void*, this));
                wxLogDebug(s.c_str(),s2.c_str());
            }
            pNode = pNode->GetNext();
        }
    }
#endif

    // Copy the error messages to a global variable
    int i;
    for (i = 0; i < DB_MAX_ERROR_HISTORY; i++)
        wxStrcpy(DBerrorList[i], errorList[i]);

    dbmsType = dbmsUNIDENTIFIED;
    dbIsOpen = false;

} // wxDb::Close()


/********** wxDb::CommitTrans() **********/
bool wxDb::CommitTrans(void)
{
    if (this)
    {
        // Commit the transaction
        if (SQLTransact(henv, hdbc, SQL_COMMIT) != SQL_SUCCESS)
            return(DispAllErrors(henv, hdbc));
    }

    // Completed successfully
    return true;

} // wxDb::CommitTrans()


/********** wxDb::RollbackTrans() **********/
bool wxDb::RollbackTrans(void)
{
    // Rollback the transaction
    if (SQLTransact(henv, hdbc, SQL_ROLLBACK) != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc));

    // Completed successfully
    return true;

} // wxDb::RollbackTrans()


/********** wxDb::DispAllErrors() **********/
bool wxDb::DispAllErrors(HENV aHenv, HDBC aHdbc, HSTMT aHstmt)
/*
 * This function is called internally whenever an error condition prevents the user's
 * request from being executed.  This function will query the datasource as to the
 * actual error(s) that just occurred on the previous request of the datasource.
 *
 * The function will retrieve each error condition from the datasource and
 * Printf the codes/text values into a string which it then logs via logError().
 * If in DBDEBUG_CONSOLE mode, the constructed string will be displayed in the console
 * window and program execution will be paused until the user presses a key.
 *
 * This function always returns false, so that functions which call this function
 * can have a line like "return (DispAllErrors(henv, hdbc));" to indicate the failure
 * of the user's request, so that the calling code can then process the error message log.
 */
{
    wxString odbcErrMsg;

   while (SQLError(aHenv, aHdbc, aHstmt, (SQLTCHAR FAR *) sqlState, &nativeError, (SQLTCHAR FAR *) errorMsg, SQL_MAX_MESSAGE_LENGTH - 1, &cbErrorMsg) == SQL_SUCCESS)
     {
        odbcErrMsg.Printf(wxT("SQL State = %s\nNative Error Code = %li\nError Message = %s\n"),
                          sqlState, (long)nativeError, errorMsg);
        logError(odbcErrMsg, sqlState);
        if (!silent)
        {
#ifdef DBDEBUG_CONSOLE
            // When run in console mode, use standard out to display errors.
            cout << odbcErrMsg.c_str() << endl;
            cout << wxT("Press any key to continue...") << endl;
            getchar();
#endif

#ifdef __WXDEBUG__
            wxLogDebug(odbcErrMsg,wxT("ODBC DEBUG MESSAGE from DispAllErrors()"));
#endif
        }
    }

    return false;  // This function always returns false.

} // wxDb::DispAllErrors()


/********** wxDb::GetNextError() **********/
bool wxDb::GetNextError(HENV aHenv, HDBC aHdbc, HSTMT aHstmt)
{
   if (SQLError(aHenv, aHdbc, aHstmt, (SQLTCHAR FAR *) sqlState, &nativeError, (SQLTCHAR FAR *) errorMsg, SQL_MAX_MESSAGE_LENGTH - 1, &cbErrorMsg) == SQL_SUCCESS)
     return true;
    else
        return false;

} // wxDb::GetNextError()


/********** wxDb::DispNextError() **********/
void wxDb::DispNextError(void)
{
    wxString odbcErrMsg;

    odbcErrMsg.Printf(wxT("SQL State = %s\nNative Error Code = %li\nError Message = %s\n"),
                      sqlState, (long)nativeError, errorMsg);
    logError(odbcErrMsg, sqlState);

    if (silent)
        return;

#ifdef DBDEBUG_CONSOLE
    // When run in console mode, use standard out to display errors.
    cout << odbcErrMsg.c_str() << endl;
    cout << wxT("Press any key to continue...")  << endl;
    getchar();
#endif

#ifdef __WXDEBUG__
    wxLogDebug(odbcErrMsg,wxT("ODBC DEBUG MESSAGE"));
#endif  // __WXDEBUG__

} // wxDb::DispNextError()


/********** wxDb::logError() **********/
void wxDb::logError(const wxString &errMsg, const wxString &SQLState)
{
    wxASSERT(errMsg.length());

    static int pLast = -1;
    int dbStatus;

    if (++pLast == DB_MAX_ERROR_HISTORY)
    {
        int i;
        for (i = 0; i < DB_MAX_ERROR_HISTORY-1; i++)
            wxStrcpy(errorList[i], errorList[i+1]);
        pLast--;
    }

    wxStrncpy(errorList[pLast], errMsg, DB_MAX_ERROR_MSG_LEN);
    errorList[pLast][DB_MAX_ERROR_MSG_LEN-1] = 0;

    if (SQLState.length())
        if ((dbStatus = TranslateSqlState(SQLState)) != DB_ERR_FUNCTION_SEQUENCE_ERROR)
            DB_STATUS = dbStatus;

    // Add the errmsg to the sql log
    WriteSqlLog(errMsg);

}  // wxDb::logError()


/**********wxDb::TranslateSqlState()  **********/
int wxDb::TranslateSqlState(const wxString &SQLState)
{
    if (!wxStrcmp(SQLState, wxT("01000")))
        return(DB_ERR_GENERAL_WARNING);
    if (!wxStrcmp(SQLState, wxT("01002")))
        return(DB_ERR_DISCONNECT_ERROR);
    if (!wxStrcmp(SQLState, wxT("01004")))
        return(DB_ERR_DATA_TRUNCATED);
    if (!wxStrcmp(SQLState, wxT("01006")))
        return(DB_ERR_PRIV_NOT_REVOKED);
    if (!wxStrcmp(SQLState, wxT("01S00")))
        return(DB_ERR_INVALID_CONN_STR_ATTR);
    if (!wxStrcmp(SQLState, wxT("01S01")))
        return(DB_ERR_ERROR_IN_ROW);
    if (!wxStrcmp(SQLState, wxT("01S02")))
        return(DB_ERR_OPTION_VALUE_CHANGED);
    if (!wxStrcmp(SQLState, wxT("01S03")))
        return(DB_ERR_NO_ROWS_UPD_OR_DEL);
    if (!wxStrcmp(SQLState, wxT("01S04")))
        return(DB_ERR_MULTI_ROWS_UPD_OR_DEL);
    if (!wxStrcmp(SQLState, wxT("07001")))
        return(DB_ERR_WRONG_NO_OF_PARAMS);
    if (!wxStrcmp(SQLState, wxT("07006")))
        return(DB_ERR_DATA_TYPE_ATTR_VIOL);
    if (!wxStrcmp(SQLState, wxT("08001")))
        return(DB_ERR_UNABLE_TO_CONNECT);
    if (!wxStrcmp(SQLState, wxT("08002")))
        return(DB_ERR_CONNECTION_IN_USE);
    if (!wxStrcmp(SQLState, wxT("08003")))
        return(DB_ERR_CONNECTION_NOT_OPEN);
    if (!wxStrcmp(SQLState, wxT("08004")))
        return(DB_ERR_REJECTED_CONNECTION);
    if (!wxStrcmp(SQLState, wxT("08007")))
        return(DB_ERR_CONN_FAIL_IN_TRANS);
    if (!wxStrcmp(SQLState, wxT("08S01")))
        return(DB_ERR_COMM_LINK_FAILURE);
    if (!wxStrcmp(SQLState, wxT("21S01")))
        return(DB_ERR_INSERT_VALUE_LIST_MISMATCH);
    if (!wxStrcmp(SQLState, wxT("21S02")))
        return(DB_ERR_DERIVED_TABLE_MISMATCH);
    if (!wxStrcmp(SQLState, wxT("22001")))
        return(DB_ERR_STRING_RIGHT_TRUNC);
    if (!wxStrcmp(SQLState, wxT("22003")))
        return(DB_ERR_NUMERIC_VALUE_OUT_OF_RNG);
    if (!wxStrcmp(SQLState, wxT("22005")))
        return(DB_ERR_ERROR_IN_ASSIGNMENT);
    if (!wxStrcmp(SQLState, wxT("22008")))
        return(DB_ERR_DATETIME_FLD_OVERFLOW);
    if (!wxStrcmp(SQLState, wxT("22012")))
        return(DB_ERR_DIVIDE_BY_ZERO);
    if (!wxStrcmp(SQLState, wxT("22026")))
        return(DB_ERR_STR_DATA_LENGTH_MISMATCH);
    if (!wxStrcmp(SQLState, wxT("23000")))
        return(DB_ERR_INTEGRITY_CONSTRAINT_VIOL);
    if (!wxStrcmp(SQLState, wxT("24000")))
        return(DB_ERR_INVALID_CURSOR_STATE);
    if (!wxStrcmp(SQLState, wxT("25000")))
        return(DB_ERR_INVALID_TRANS_STATE);
    if (!wxStrcmp(SQLState, wxT("28000")))
        return(DB_ERR_INVALID_AUTH_SPEC);
    if (!wxStrcmp(SQLState, wxT("34000")))
        return(DB_ERR_INVALID_CURSOR_NAME);
    if (!wxStrcmp(SQLState, wxT("37000")))
        return(DB_ERR_SYNTAX_ERROR_OR_ACCESS_VIOL);
    if (!wxStrcmp(SQLState, wxT("3C000")))
        return(DB_ERR_DUPLICATE_CURSOR_NAME);
    if (!wxStrcmp(SQLState, wxT("40001")))
        return(DB_ERR_SERIALIZATION_FAILURE);
    if (!wxStrcmp(SQLState, wxT("42000")))
        return(DB_ERR_SYNTAX_ERROR_OR_ACCESS_VIOL2);
    if (!wxStrcmp(SQLState, wxT("70100")))
        return(DB_ERR_OPERATION_ABORTED);
    if (!wxStrcmp(SQLState, wxT("IM001")))
        return(DB_ERR_UNSUPPORTED_FUNCTION);
    if (!wxStrcmp(SQLState, wxT("IM002")))
        return(DB_ERR_NO_DATA_SOURCE);
    if (!wxStrcmp(SQLState, wxT("IM003")))
        return(DB_ERR_DRIVER_LOAD_ERROR);
    if (!wxStrcmp(SQLState, wxT("IM004")))
        return(DB_ERR_SQLALLOCENV_FAILED);
    if (!wxStrcmp(SQLState, wxT("IM005")))
        return(DB_ERR_SQLALLOCCONNECT_FAILED);
    if (!wxStrcmp(SQLState, wxT("IM006")))
        return(DB_ERR_SQLSETCONNECTOPTION_FAILED);
    if (!wxStrcmp(SQLState, wxT("IM007")))
        return(DB_ERR_NO_DATA_SOURCE_DLG_PROHIB);
    if (!wxStrcmp(SQLState, wxT("IM008")))
        return(DB_ERR_DIALOG_FAILED);
    if (!wxStrcmp(SQLState, wxT("IM009")))
        return(DB_ERR_UNABLE_TO_LOAD_TRANSLATION_DLL);
    if (!wxStrcmp(SQLState, wxT("IM010")))
        return(DB_ERR_DATA_SOURCE_NAME_TOO_LONG);
    if (!wxStrcmp(SQLState, wxT("IM011")))
        return(DB_ERR_DRIVER_NAME_TOO_LONG);
    if (!wxStrcmp(SQLState, wxT("IM012")))
        return(DB_ERR_DRIVER_KEYWORD_SYNTAX_ERROR);
    if (!wxStrcmp(SQLState, wxT("IM013")))
        return(DB_ERR_TRACE_FILE_ERROR);
    if (!wxStrcmp(SQLState, wxT("S0001")))
        return(DB_ERR_TABLE_OR_VIEW_ALREADY_EXISTS);
    if (!wxStrcmp(SQLState, wxT("S0002")))
        return(DB_ERR_TABLE_NOT_FOUND);
    if (!wxStrcmp(SQLState, wxT("S0011")))
        return(DB_ERR_INDEX_ALREADY_EXISTS);
    if (!wxStrcmp(SQLState, wxT("S0012")))
        return(DB_ERR_INDEX_NOT_FOUND);
    if (!wxStrcmp(SQLState, wxT("S0021")))
        return(DB_ERR_COLUMN_ALREADY_EXISTS);
    if (!wxStrcmp(SQLState, wxT("S0022")))
        return(DB_ERR_COLUMN_NOT_FOUND);
    if (!wxStrcmp(SQLState, wxT("S0023")))
        return(DB_ERR_NO_DEFAULT_FOR_COLUMN);
    if (!wxStrcmp(SQLState, wxT("S1000")))
        return(DB_ERR_GENERAL_ERROR);
    if (!wxStrcmp(SQLState, wxT("S1001")))
        return(DB_ERR_MEMORY_ALLOCATION_FAILURE);
    if (!wxStrcmp(SQLState, wxT("S1002")))
        return(DB_ERR_INVALID_COLUMN_NUMBER);
    if (!wxStrcmp(SQLState, wxT("S1003")))
        return(DB_ERR_PROGRAM_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1004")))
        return(DB_ERR_SQL_DATA_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1008")))
        return(DB_ERR_OPERATION_CANCELLED);
    if (!wxStrcmp(SQLState, wxT("S1009")))
        return(DB_ERR_INVALID_ARGUMENT_VALUE);
    if (!wxStrcmp(SQLState, wxT("S1010")))
        return(DB_ERR_FUNCTION_SEQUENCE_ERROR);
    if (!wxStrcmp(SQLState, wxT("S1011")))
        return(DB_ERR_OPERATION_INVALID_AT_THIS_TIME);
    if (!wxStrcmp(SQLState, wxT("S1012")))
        return(DB_ERR_INVALID_TRANS_OPERATION_CODE);
    if (!wxStrcmp(SQLState, wxT("S1015")))
        return(DB_ERR_NO_CURSOR_NAME_AVAIL);
    if (!wxStrcmp(SQLState, wxT("S1090")))
        return(DB_ERR_INVALID_STR_OR_BUF_LEN);
    if (!wxStrcmp(SQLState, wxT("S1091")))
        return(DB_ERR_DESCRIPTOR_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1092")))
        return(DB_ERR_OPTION_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1093")))
        return(DB_ERR_INVALID_PARAM_NO);
    if (!wxStrcmp(SQLState, wxT("S1094")))
        return(DB_ERR_INVALID_SCALE_VALUE);
    if (!wxStrcmp(SQLState, wxT("S1095")))
        return(DB_ERR_FUNCTION_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1096")))
        return(DB_ERR_INF_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1097")))
        return(DB_ERR_COLUMN_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1098")))
        return(DB_ERR_SCOPE_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1099")))
        return(DB_ERR_NULLABLE_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1100")))
        return(DB_ERR_UNIQUENESS_OPTION_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1101")))
        return(DB_ERR_ACCURACY_OPTION_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1103")))
        return(DB_ERR_DIRECTION_OPTION_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1104")))
        return(DB_ERR_INVALID_PRECISION_VALUE);
    if (!wxStrcmp(SQLState, wxT("S1105")))
        return(DB_ERR_INVALID_PARAM_TYPE);
    if (!wxStrcmp(SQLState, wxT("S1106")))
        return(DB_ERR_FETCH_TYPE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1107")))
        return(DB_ERR_ROW_VALUE_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1108")))
        return(DB_ERR_CONCURRENCY_OPTION_OUT_OF_RANGE);
    if (!wxStrcmp(SQLState, wxT("S1109")))
        return(DB_ERR_INVALID_CURSOR_POSITION);
    if (!wxStrcmp(SQLState, wxT("S1110")))
        return(DB_ERR_INVALID_DRIVER_COMPLETION);
    if (!wxStrcmp(SQLState, wxT("S1111")))
        return(DB_ERR_INVALID_BOOKMARK_VALUE);
    if (!wxStrcmp(SQLState, wxT("S1C00")))
        return(DB_ERR_DRIVER_NOT_CAPABLE);
    if (!wxStrcmp(SQLState, wxT("S1T00")))
        return(DB_ERR_TIMEOUT_EXPIRED);

    // No match
    return(0);

}  // wxDb::TranslateSqlState()


/**********  wxDb::Grant() **********/
bool wxDb::Grant(int privileges, const wxString &tableName, const wxString &userList)
{
    wxString sqlStmt;

    // Build the grant statement
    sqlStmt  = wxT("GRANT ");
    if (privileges == DB_GRANT_ALL)
        sqlStmt += wxT("ALL");
    else
    {
        int c = 0;
        if (privileges & DB_GRANT_SELECT)
        {
            sqlStmt += wxT("SELECT");
            c++;
        }
        if (privileges & DB_GRANT_INSERT)
        {
            if (c++)
                sqlStmt += wxT(", ");
            sqlStmt += wxT("INSERT");
        }
        if (privileges & DB_GRANT_UPDATE)
        {
            if (c++)
                sqlStmt += wxT(", ");
            sqlStmt += wxT("UPDATE");
        }
        if (privileges & DB_GRANT_DELETE)
        {
            if (c++)
                sqlStmt += wxT(", ");
            sqlStmt += wxT("DELETE");
        }
    }

    sqlStmt += wxT(" ON ");
    sqlStmt += SQLTableName(tableName);
    sqlStmt += wxT(" TO ");
    sqlStmt += userList;

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl;
#endif

    WriteSqlLog(sqlStmt);

    return(ExecSql(sqlStmt));

}  // wxDb::Grant()


/********** wxDb::CreateView() **********/
bool wxDb::CreateView(const wxString &viewName, const wxString &colList,
                      const wxString &pSqlStmt, bool attemptDrop)
{
    wxString sqlStmt;

    // Drop the view first
    if (attemptDrop && !DropView(viewName))
        return false;

    // Build the create view statement
    sqlStmt  = wxT("CREATE VIEW ");
    sqlStmt += viewName;

    if (colList.length())
    {
        sqlStmt += wxT(" (");
        sqlStmt += colList;
        sqlStmt += wxT(")");
    }

    sqlStmt += wxT(" AS ");
    sqlStmt += pSqlStmt;

    WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << sqlStmt.c_str() << endl;
#endif

    return(ExecSql(sqlStmt));

}  // wxDb::CreateView()


/********** wxDb::DropView()  **********/
bool wxDb::DropView(const wxString &viewName)
{
/*
 * NOTE: This function returns true if the View does not exist, but
 *       only for identified databases.  Code will need to be added
 *            below for any other databases when those databases are defined
 *       to handle this situation consistently
 */
    wxString sqlStmt;

    sqlStmt.Printf(wxT("DROP VIEW %s"), viewName.c_str());

    WriteSqlLog(sqlStmt);

#ifdef DBDEBUG_CONSOLE
    cout << endl << sqlStmt.c_str() << endl;
#endif

    if (SQLExecDirect(hstmt, (SQLTCHAR FAR *) sqlStmt.c_str(), SQL_NTS) != SQL_SUCCESS)
    {
        // Check for "Base table not found" error and ignore
        GetNextError(henv, hdbc, hstmt);
        if (wxStrcmp(sqlState,wxT("S0002")))  // "Base table not found"
        {
            // Check for product specific error codes
            if (!((Dbms() == dbmsSYBASE_ASA    && !wxStrcmp(sqlState,wxT("42000")))))  // 5.x (and lower?)
            {
                DispNextError();
                DispAllErrors(henv, hdbc, hstmt);
                RollbackTrans();
                return false;
            }
        }
    }

    // Commit the transaction
    if (!CommitTrans())
        return false;

    return true;

}  // wxDb::DropView()


/********** wxDb::ExecSql()  **********/
bool wxDb::ExecSql(const wxString &pSqlStmt)
{
    RETCODE retcode;

    SQLFreeStmt(hstmt, SQL_CLOSE);

    retcode = SQLExecDirect(hstmt, (SQLTCHAR FAR *) pSqlStmt.c_str(), SQL_NTS);
    if (retcode == SQL_SUCCESS ||
        (Dbms() == dbmsDB2 && (retcode == SQL_SUCCESS_WITH_INFO || retcode == SQL_NO_DATA_FOUND)))
    {
        return true;
    }
    else
    {
        DispAllErrors(henv, hdbc, hstmt);
        return false;
    }

}  // wxDb::ExecSql()


/********** wxDb::ExecSql() with column info **********/
bool wxDb::ExecSql(const wxString &pSqlStmt, wxDbColInf** columns, short& numcols)
{
    //execute the statement first
    if (!ExecSql(pSqlStmt))
        return false;

    SWORD noCols;
    if (SQLNumResultCols(hstmt, &noCols) != SQL_SUCCESS)
    {
        DispAllErrors(henv, hdbc, hstmt);
        return false;
    }

    if (noCols == 0)
        return false;
    else
        numcols = noCols;

    //  Get column information
    short colNum;
    wxChar name[DB_MAX_COLUMN_NAME_LEN+1];
    SWORD Sword;
    SQLLEN Sqllen;
    wxDbColInf* pColInf = new wxDbColInf[noCols];

    // Fill in column information (name, datatype)
    for (colNum = 0; colNum < noCols; colNum++)
    {
        if (SQLColAttributes(hstmt, (UWORD)(colNum+1), SQL_COLUMN_NAME,
            name, sizeof(name),
            &Sword, &Sqllen) != SQL_SUCCESS)
        {
            DispAllErrors(henv, hdbc, hstmt);
            delete[] pColInf;
            return false;
        }

        wxStrncpy(pColInf[colNum].colName, name, DB_MAX_COLUMN_NAME_LEN);
        pColInf[colNum].colName[DB_MAX_COLUMN_NAME_LEN] = 0;  // Prevent buffer overrun

        if (SQLColAttributes(hstmt, (UWORD)(colNum+1), SQL_COLUMN_TYPE,
            NULL, 0, &Sword, &Sqllen) != SQL_SUCCESS)
        {
            DispAllErrors(henv, hdbc, hstmt);
            delete[] pColInf;
            return false;
        }

        switch (Sqllen)
        {
#if wxUSE_UNICODE
    #if defined(SQL_WCHAR)
            case SQL_WCHAR:
    #endif
    #if defined(SQL_WVARCHAR)
            case SQL_WVARCHAR:
    #endif
#endif
            case SQL_VARCHAR:
            case SQL_CHAR:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_VARCHAR;
                break;
            case SQL_LONGVARCHAR:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_MEMO;
                break;
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_BIT:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_INTEGER;
                break;
            case SQL_DOUBLE:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_FLOAT:
            case SQL_REAL:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_FLOAT;
                break;
            case SQL_DATE:
            case SQL_TIMESTAMP:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_DATE;
                break;
            case SQL_BINARY:
                pColInf[colNum].dbDataType = DB_DATA_TYPE_BLOB;
                break;
#ifdef __WXDEBUG__
            default:
                wxString errMsg;
                errMsg.Printf(wxT("SQL Data type %ld currently not supported by wxWidgets"), (long)Sqllen);
                wxLogDebug(errMsg,wxT("ODBC DEBUG MESSAGE"));
#endif
        }
    }

    *columns = pColInf;
    return true;
}  // wxDb::ExecSql()

/********** wxDb::GetNext()  **********/
bool wxDb::GetNext(void)
{
    if (SQLFetch(hstmt) == SQL_SUCCESS)
        return true;
    else
    {
        DispAllErrors(henv, hdbc, hstmt);
        return false;
    }

}  // wxDb::GetNext()


/********** wxDb::GetData()  **********/
bool wxDb::GetData(UWORD colNo, SWORD cType, PTR pData, SDWORD maxLen, SQLLEN FAR *cbReturned)
{
    wxASSERT(pData);
    wxASSERT(cbReturned);

    long bufferSize = maxLen;

    if (cType == SQL_C_WXCHAR)
        bufferSize = maxLen * sizeof(wxChar);

    if (SQLGetData(hstmt, colNo, cType, pData, bufferSize, cbReturned) == SQL_SUCCESS)
        return true;
    else
    {
        DispAllErrors(henv, hdbc, hstmt);
        return false;
    }

}  // wxDb::GetData()


/********** wxDb::GetKeyFields() **********/
int wxDb::GetKeyFields(const wxString &tableName, wxDbColInf* colInf, UWORD noCols)
{
    wxChar       szPkTable[DB_MAX_TABLE_NAME_LEN+1];  /* Primary key table name */
    wxChar       szFkTable[DB_MAX_TABLE_NAME_LEN+1];  /* Foreign key table name */
    SWORD        iKeySeq;
    wxChar       szPkCol[DB_MAX_COLUMN_NAME_LEN+1];   /* Primary key column     */
    wxChar       szFkCol[DB_MAX_COLUMN_NAME_LEN+1];   /* Foreign key column     */
    SQLRETURN    retcode;
    SQLLEN       cb;
    SWORD        i;
    wxString     tempStr;
    /*
     * -----------------------------------------------------------------------
     * -- 19991224 : mj10777 : Create                                   ------
     * --          : Three things are done and stored here :            ------
     * --          : 1) which Column(s) is/are Primary Key(s)           ------
     * --          : 2) which tables use this Key as a Foreign Key      ------
     * --          : 3) which columns are Foreign Key and the name      ------
     * --          :     of the Table where the Key is the Primary Key  -----
     * --          : Called from GetColumns(const wxString &tableName,  ------
     * --                           int *numCols,const wxChar *userID ) ------
     * -----------------------------------------------------------------------
     */

    /*---------------------------------------------------------------------*/
    /* Get the names of the columns in the primary key.                    */
    /*---------------------------------------------------------------------*/
    retcode = SQLPrimaryKeys(hstmt,
                             NULL, 0,                               /* Catalog name  */
                             NULL, 0,                               /* Schema name   */
                             (SQLTCHAR FAR *) tableName.c_str(), SQL_NTS); /* Table name    */

    /*---------------------------------------------------------------------*/
    /* Fetch and display the result set. This will be a list of the        */
    /* columns in the primary key of the tableName table.                  */
    /*---------------------------------------------------------------------*/
    while ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO))
    {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            GetData( 4, SQL_C_WXCHAR,  szPkCol,    DB_MAX_COLUMN_NAME_LEN+1, &cb);
            GetData( 5, SQL_C_SSHORT, &iKeySeq,    0,                        &cb);
            //-------
            for (i=0;i<noCols;i++)                          // Find the Column name
                if (!wxStrcmp(colInf[i].colName,szPkCol))   // We have found the Column
                    colInf[i].PkCol = iKeySeq;              // Which Primary Key is this (first, second usw.) ?
        }  // if
    }  // while
    SQLFreeStmt(hstmt, SQL_CLOSE);  /* Close the cursor (the hstmt is still allocated).      */

    /*---------------------------------------------------------------------*/
    /* Get all the foreign keys that refer to tableName primary key.       */
    /*---------------------------------------------------------------------*/
    retcode = SQLForeignKeys(hstmt,
                             NULL, 0,                            /* Primary catalog */
                             NULL, 0,                            /* Primary schema  */
                             (SQLTCHAR FAR *)tableName.c_str(), SQL_NTS,/* Primary table   */
                             NULL, 0,                            /* Foreign catalog */
                             NULL, 0,                            /* Foreign schema  */
                             NULL, 0);                           /* Foreign table   */

    /*---------------------------------------------------------------------*/
    /* Fetch and display the result set. This will be all of the foreign   */
    /* keys in other tables that refer to the tableName  primary key.      */
    /*---------------------------------------------------------------------*/
    tempStr.Empty();
    szPkCol[0] = 0;
    while ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO))
    {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            GetData( 3, SQL_C_WXCHAR,  szPkTable,   DB_MAX_TABLE_NAME_LEN+1,   &cb);
            GetData( 4, SQL_C_WXCHAR,  szPkCol,     DB_MAX_COLUMN_NAME_LEN+1,  &cb);
            GetData( 5, SQL_C_SSHORT, &iKeySeq,     0,                         &cb);
            GetData( 7, SQL_C_WXCHAR,  szFkTable,   DB_MAX_TABLE_NAME_LEN+1,   &cb);
            GetData( 8, SQL_C_WXCHAR,  szFkCol,     DB_MAX_COLUMN_NAME_LEN+1,  &cb);
            tempStr << _T('[') << szFkTable << _T(']');  // [ ] in case there is a blank in the Table name
        }  // if
    }  // while

    tempStr.Trim();     // Get rid of any unneeded blanks
    if (!tempStr.empty())
    {
        for (i=0; i<noCols; i++)
        {   // Find the Column name
            if (!wxStrcmp(colInf[i].colName, szPkCol))           // We have found the Column, store the Information
            {
                wxStrncpy(colInf[i].PkTableName, tempStr.c_str(), DB_MAX_TABLE_NAME_LEN);  // Name of the Tables where this Primary Key is used as a Foreign Key
                colInf[i].PkTableName[DB_MAX_TABLE_NAME_LEN] = 0;  // Prevent buffer overrun
            }
        }
    }  // if

    SQLFreeStmt(hstmt, SQL_CLOSE);  /* Close the cursor (the hstmt is still allocated). */

    /*---------------------------------------------------------------------*/
    /* Get all the foreign keys in the tablename table.                    */
    /*---------------------------------------------------------------------*/
    retcode = SQLForeignKeys(hstmt,
                             NULL, 0,                             /* Primary catalog   */
                             NULL, 0,                             /* Primary schema    */
                             NULL, 0,                             /* Primary table     */
                             NULL, 0,                             /* Foreign catalog   */
                             NULL, 0,                             /* Foreign schema    */
                             (SQLTCHAR *)tableName.c_str(), SQL_NTS);/* Foreign table     */

    /*---------------------------------------------------------------------*/
    /*  Fetch and display the result set. This will be all of the          */
    /*  primary keys in other tables that are referred to by foreign       */
    /*  keys in the tableName table.                                       */
    /*---------------------------------------------------------------------*/
    while ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO))
    {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            GetData( 3, SQL_C_WXCHAR,  szPkTable,   DB_MAX_TABLE_NAME_LEN+1,  &cb);
            GetData( 5, SQL_C_SSHORT, &iKeySeq,     0,                        &cb);
            GetData( 8, SQL_C_WXCHAR,  szFkCol,     DB_MAX_COLUMN_NAME_LEN+1, &cb);
            //-------
            for (i=0; i<noCols; i++)                            // Find the Column name
            {
                if (!wxStrcmp(colInf[i].colName,szFkCol))       // We have found the (Foreign Key) Column
                {
                    colInf[i].FkCol = iKeySeq;                  // Which Foreign Key is this (first, second usw.) ?
                    wxStrncpy(colInf[i].FkTableName, szFkTable, DB_MAX_TABLE_NAME_LEN);  // Name of the Table where this Foriegn is the Primary Key
                    colInf[i].FkTableName[DB_MAX_TABLE_NAME_LEN] = 0;  // Prevent buffer overrun
                } // if
            }  // for
        }  // if
    }  // while
    SQLFreeStmt(hstmt, SQL_CLOSE);  /* Close the cursor (the hstmt is still allocated). */

    return TRUE;

}  // wxDb::GetKeyFields()


#if OLD_GETCOLUMNS
/********** wxDb::GetColumns() **********/
wxDbColInf *wxDb::GetColumns(wxChar *tableName[], const wxChar *userID)
/*
 *        1) The last array element of the tableName[] argument must be zero (null).
 *            This is how the end of the array is detected.
 *        2) This function returns an array of wxDbColInf structures.  If no columns
 *            were found, or an error occurred, this pointer will be zero (null).  THE
 *            CALLING FUNCTION IS RESPONSIBLE FOR DELETING THE MEMORY RETURNED WHEN IT
 *            IS FINISHED WITH IT.  i.e.
 *
 *            wxDbColInf *colInf = pDb->GetColumns(tableList, userID);
 *            if (colInf)
 *            {
 *                // Use the column inf
 *                .......
 *                // Destroy the memory
 *                delete [] colInf;
 *            }
 *
 * userID is evaluated in the following manner:
 *        userID == NULL  ... UserID is ignored
 *        userID == ""    ... UserID set equal to 'this->uid'
 *        userID != ""    ... UserID set equal to 'userID'
 *
 * NOTE: ALL column bindings associated with this wxDb instance are unbound
 *       by this function.  This function should use its own wxDb instance
 *       to avoid undesired unbinding of columns.
 */
{
    UWORD       noCols = 0;
    UWORD       colNo  = 0;
    wxDbColInf *colInf = 0;

    RETCODE  retcode;
    SQLLEN   cb;

    wxString TableName;

    wxString UserID;
    convertUserID(userID,UserID);

    // Pass 1 - Determine how many columns there are.
    // Pass 2 - Allocate the wxDbColInf array and fill in
    //                the array with the column information.
    int pass;
    for (pass = 1; pass <= 2; pass++)
    {
        if (pass == 2)
        {
            if (noCols == 0)  // Probably a bogus table name(s)
                break;
            // Allocate n wxDbColInf objects to hold the column information
            colInf = new wxDbColInf[noCols+1];
            if (!colInf)
                break;
            // Mark the end of the array
            wxStrcpy(colInf[noCols].tableName, wxEmptyString);
            wxStrcpy(colInf[noCols].colName, wxEmptyString);
            colInf[noCols].sqlDataType = 0;
        }
        // Loop through each table name
        int tbl;
        for (tbl = 0; tableName[tbl]; tbl++)
        {
            TableName = tableName[tbl];
            // Oracle and Interbase table names are uppercase only, so force
            // the name to uppercase just in case programmer forgot to do this
            if ((Dbms() == dbmsORACLE) ||
                (Dbms() == dbmsFIREBIRD) ||
                (Dbms() == dbmsINTERBASE))
                TableName = TableName.Upper();

            SQLFreeStmt(hstmt, SQL_CLOSE);

            // MySQL, SQLServer, and Access cannot accept a user name when looking up column names, so we
            // use the call below that leaves out the user name
            if (!UserID.empty() &&
                Dbms() != dbmsMY_SQL &&
                Dbms() != dbmsACCESS &&
                Dbms() != dbmsMS_SQL_SERVER)
            {
                retcode = SQLColumns(hstmt,
                                     NULL, 0,                                // All qualifiers
                                     (SQLTCHAR *) UserID.c_str(), SQL_NTS,      // Owner
                                     (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                                     NULL, 0);                               // All columns
            }
            else
            {
                retcode = SQLColumns(hstmt,
                                     NULL, 0,                                // All qualifiers
                                     NULL, 0,                                // Owner
                                     (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                                     NULL, 0);                               // All columns
            }
            if (retcode != SQL_SUCCESS)
            {  // Error occurred, abort
                DispAllErrors(henv, hdbc, hstmt);
                if (colInf)
                    delete [] colInf;
                SQLFreeStmt(hstmt, SQL_CLOSE);
                return(0);
            }

            while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)
            {
                if (pass == 1)  // First pass, just add up the number of columns
                    noCols++;
                else  // Pass 2; Fill in the array of structures
                {
                    if (colNo < noCols)  // Some extra error checking to prevent memory overwrites
                    {
                        // NOTE: Only the ODBC 1.x fields are retrieved
                        GetData( 1, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].catalog,      128+1,                    &cb);
                        GetData( 2, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].schema,       128+1,                    &cb);
                        GetData( 3, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].tableName,    DB_MAX_TABLE_NAME_LEN+1,  &cb);
                        GetData( 4, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].colName,      DB_MAX_COLUMN_NAME_LEN+1, &cb);
                        GetData( 5, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].sqlDataType,  0,                        &cb);
                        GetData( 6, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].typeName,     128+1,                    &cb);
                        GetData( 7, SQL_C_SLONG,  (UCHAR*) &colInf[colNo].columnLength, 0,                        &cb);
                        GetData( 8, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].bufferSize,   0,                        &cb);
                        GetData( 9, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].decimalDigits,0,                        &cb);
                        GetData(10, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].numPrecRadix, 0,                        &cb);
                        GetData(11, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].nullable,     0,                        &cb);
                        GetData(12, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].remarks,      254+1,                    &cb);

                        // Determine the wxDb data type that is used to represent the native data type of this data source
                        colInf[colNo].dbDataType = 0;
                        if (!wxStricmp(typeInfVarchar.TypeName,colInf[colNo].typeName))
                        {
#ifdef _IODBC_
                            // IODBC does not return a correct columnLength, so we set
                            // columnLength = bufferSize if no column length was returned
                            // IODBC returns the columnLength in bufferSize. (bug)
                            if (colInf[colNo].columnLength < 1)
                            {
                               colInf[colNo].columnLength = colInf[colNo].bufferSize;
                            }
#endif
                            colInf[colNo].dbDataType = DB_DATA_TYPE_VARCHAR;
                        }
                        else if (!wxStricmp(typeInfInteger.TypeName, colInf[colNo].typeName))
                            colInf[colNo].dbDataType = DB_DATA_TYPE_INTEGER;
                        else if (!wxStricmp(typeInfFloat.TypeName, colInf[colNo].typeName))
                            colInf[colNo].dbDataType = DB_DATA_TYPE_FLOAT;
                        else if (!wxStricmp(typeInfDate.TypeName, colInf[colNo].typeName))
                            colInf[colNo].dbDataType = DB_DATA_TYPE_DATE;
                        else if (!wxStricmp(typeInfBlob.TypeName, colInf[colNo].typeName))
                            colInf[colNo].dbDataType = DB_DATA_TYPE_BLOB;
                        colNo++;
                    }
                }
            }
            if (retcode != SQL_NO_DATA_FOUND)
            {  // Error occurred, abort
                DispAllErrors(henv, hdbc, hstmt);
                if (colInf)
                    delete [] colInf;
                SQLFreeStmt(hstmt, SQL_CLOSE);
                return(0);
            }
        }
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);
    return colInf;

}  // wxDb::GetColumns()


/********** wxDb::GetColumns() **********/

wxDbColInf *wxDb::GetColumns(const wxString &tableName, UWORD *numCols, const wxChar *userID)
//
// Same as the above GetColumns() function except this one gets columns
// only for a single table, and if 'numCols' is not NULL, the number of
// columns stored in the returned wxDbColInf is set in '*numCols'
//
// userID is evaluated in the following manner:
//        userID == NULL  ... UserID is ignored
//        userID == ""    ... UserID set equal to 'this->uid'
//        userID != ""    ... UserID set equal to 'userID'
//
// NOTE: ALL column bindings associated with this wxDb instance are unbound
//       by this function.  This function should use its own wxDb instance
//       to avoid undesired unbinding of columns.

{
    UWORD       noCols = 0;
    UWORD       colNo  = 0;
    wxDbColInf *colInf = 0;

    RETCODE  retcode;
    SQLLEN   cb;

    wxString TableName;

    wxString UserID;
    convertUserID(userID,UserID);

    // Pass 1 - Determine how many columns there are.
    // Pass 2 - Allocate the wxDbColInf array and fill in
    //                the array with the column information.
    int pass;
    for (pass = 1; pass <= 2; pass++)
    {
        if (pass == 2)
        {
            if (noCols == 0)  // Probably a bogus table name(s)
                break;
            // Allocate n wxDbColInf objects to hold the column information
            colInf = new wxDbColInf[noCols+1];
            if (!colInf)
                break;
            // Mark the end of the array
            wxStrcpy(colInf[noCols].tableName, wxEmptyString);
            wxStrcpy(colInf[noCols].colName, wxEmptyString);
            colInf[noCols].sqlDataType = 0;
        }

        TableName = tableName;
        // Oracle and Interbase table names are uppercase only, so force
        // the name to uppercase just in case programmer forgot to do this
        if ((Dbms() == dbmsORACLE) ||
            (Dbms() == dbmsFIREBIRD) ||
            (Dbms() == dbmsINTERBASE))
            TableName = TableName.Upper();

        SQLFreeStmt(hstmt, SQL_CLOSE);

        // MySQL, SQLServer, and Access cannot accept a user name when looking up column names, so we
        // use the call below that leaves out the user name
        if (!UserID.empty() &&
            Dbms() != dbmsMY_SQL &&
            Dbms() != dbmsACCESS &&
            Dbms() != dbmsMS_SQL_SERVER)
        {
            retcode = SQLColumns(hstmt,
                                 NULL, 0,                                // All qualifiers
                                 (SQLTCHAR *) UserID.c_str(), SQL_NTS,    // Owner
                                 (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                                 NULL, 0);                               // All columns
        }
        else
        {
            retcode = SQLColumns(hstmt,
                                 NULL, 0,                                 // All qualifiers
                                 NULL, 0,                                 // Owner
                                 (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                                 NULL, 0);                                // All columns
        }
        if (retcode != SQL_SUCCESS)
        {  // Error occurred, abort
            DispAllErrors(henv, hdbc, hstmt);
            if (colInf)
                delete [] colInf;
            SQLFreeStmt(hstmt, SQL_CLOSE);
            if (numCols)
                *numCols = 0;
            return(0);
        }

        while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)
        {
            if (pass == 1)  // First pass, just add up the number of columns
                noCols++;
            else  // Pass 2; Fill in the array of structures
            {
                if (colNo < noCols)  // Some extra error checking to prevent memory overwrites
                {
                    // NOTE: Only the ODBC 1.x fields are retrieved
                    GetData( 1, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].catalog,      128+1,                     &cb);
                    GetData( 2, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].schema,       128+1,                     &cb);
                    GetData( 3, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].tableName,    DB_MAX_TABLE_NAME_LEN+1,   &cb);
                    GetData( 4, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].colName,      DB_MAX_COLUMN_NAME_LEN+1,  &cb);
                    GetData( 5, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].sqlDataType,  0,                         &cb);
                    GetData( 6, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].typeName,     128+1,                     &cb);
                    GetData( 7, SQL_C_SLONG,  (UCHAR*) &colInf[colNo].columnLength, 0,                         &cb);
                    // BJO 991214 : SQL_C_SSHORT instead of SQL_C_SLONG, otherwise fails on Sparc (probably all 64 bit architectures)
                    GetData( 8, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].bufferSize,   0,                         &cb);
                    GetData( 9, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].decimalDigits,0,                         &cb);
                    GetData(10, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].numPrecRadix, 0,                         &cb);
                    GetData(11, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].nullable,     0,                         &cb);
                    GetData(12, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].remarks,      254+1,                     &cb);
                    // Start Values for Primary/Foriegn Key (=No)
                    colInf[colNo].PkCol = 0;           // Primary key column   0=No; 1= First Key, 2 = Second Key etc.
                    colInf[colNo].PkTableName[0] = 0;  // Tablenames where Primary Key is used as a Foreign Key
                    colInf[colNo].FkCol = 0;           // Foreign key column   0=No; 1= First Key, 2 = Second Key etc.
                    colInf[colNo].FkTableName[0] = 0;  // Foreign key table name

                    // BJO 20000428 : Virtuoso returns type names with upper cases!
                    if (Dbms() == dbmsVIRTUOSO)
                    {
                        wxString s = colInf[colNo].typeName;
                        s = s.MakeLower();
                        wxStrcmp(colInf[colNo].typeName, s.c_str());
                    }

                    // Determine the wxDb data type that is used to represent the native data type of this data source
                    colInf[colNo].dbDataType = 0;
                    if (!wxStricmp(typeInfVarchar.TypeName, colInf[colNo].typeName))
                    {
#ifdef _IODBC_
                        // IODBC does not return a correct columnLength, so we set
                        // columnLength = bufferSize if no column length was returned
                        // IODBC returns the columnLength in bufferSize. (bug)
                        if (colInf[colNo].columnLength < 1)
                        {
                           colInf[colNo].columnLength = colInf[colNo].bufferSize;
                        }
#endif

                        colInf[colNo].dbDataType = DB_DATA_TYPE_VARCHAR;
                    }
                    else if (!wxStricmp(typeInfInteger.TypeName, colInf[colNo].typeName))
                        colInf[colNo].dbDataType = DB_DATA_TYPE_INTEGER;
                    else if (!wxStricmp(typeInfFloat.TypeName, colInf[colNo].typeName))
                        colInf[colNo].dbDataType = DB_DATA_TYPE_FLOAT;
                    else if (!wxStricmp(typeInfDate.TypeName, colInf[colNo].typeName))
                        colInf[colNo].dbDataType = DB_DATA_TYPE_DATE;
                    else if (!wxStricmp(typeInfBlob.TypeName, colInf[colNo].typeName))
                        colInf[colNo].dbDataType = DB_DATA_TYPE_BLOB;

                    colNo++;
                }
            }
        }
        if (retcode != SQL_NO_DATA_FOUND)
        {  // Error occurred, abort
            DispAllErrors(henv, hdbc, hstmt);
            if (colInf)
                delete [] colInf;
            SQLFreeStmt(hstmt, SQL_CLOSE);
            if (numCols)
                *numCols = 0;
            return(0);
        }
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);

    // Store Primary and Foriegn Keys
    GetKeyFields(tableName,colInf,noCols);

    if (numCols)
        *numCols = noCols;
    return colInf;

}  // wxDb::GetColumns()


#else  // New GetColumns


/*
    BJO 20000503
    These are tentative new GetColumns members which should be more database
    independent and which always returns the columns in the order they were
    created.

    - The first one (wxDbColInf *wxDb::GetColumns(wxChar *tableName[], const
      wxChar* userID)) calls the second implementation for each separate table
      before merging the results. This makes the code easier to maintain as
      only one member (the second) makes the real work
    - wxDbColInf *wxDb::GetColumns(const wxString &tableName, int *numCols, const
      wxChar *userID) is a little bit improved
    - It doesn't anymore rely on the type-name to find out which database-type
      each column has
    - It ends by sorting the columns, so that they are returned in the same
      order they were created
*/

typedef struct
{
    UWORD noCols;
    wxDbColInf *colInf;
} _TableColumns;


wxDbColInf *wxDb::GetColumns(wxChar *tableName[], const wxChar *userID)
{
    int i, j;
    // The last array element of the tableName[] argument must be zero (null).
    // This is how the end of the array is detected.

    UWORD noCols = 0;

    // How many tables ?
    int tbl;
    for (tbl = 0 ; tableName[tbl]; tbl++);

    // Create a table to maintain the columns for each separate table
    _TableColumns *TableColumns = new _TableColumns[tbl];

    // Fill the table
    for (i = 0 ; i < tbl ; i++)

    {
        TableColumns[i].colInf = GetColumns(tableName[i], &TableColumns[i].noCols, userID);
        if (TableColumns[i].colInf == NULL)
            return NULL;
        noCols += TableColumns[i].noCols;
    }

    // Now merge all the separate table infos
    wxDbColInf *colInf = new wxDbColInf[noCols+1];

    // Mark the end of the array
    wxStrcpy(colInf[noCols].tableName, wxEmptyString);
    wxStrcpy(colInf[noCols].colName, wxEmptyString);
    colInf[noCols].sqlDataType = 0;

    // Merge ...
    int offset = 0;

    for (i = 0 ; i < tbl ; i++)
    {
        for (j = 0 ; j < TableColumns[i].noCols ; j++)
        {
            colInf[offset++] = TableColumns[i].colInf[j];
        }
    }

    delete [] TableColumns;

    return colInf;
}  // wxDb::GetColumns()  -- NEW


wxDbColInf *wxDb::GetColumns(const wxString &tableName, int *numCols, const wxChar *userID)
//
// Same as the above GetColumns() function except this one gets columns
// only for a single table, and if 'numCols' is not NULL, the number of
// columns stored in the returned wxDbColInf is set in '*numCols'
//
// userID is evaluated in the following manner:
//        userID == NULL  ... UserID is ignored
//        userID == ""    ... UserID set equal to 'this->uid'
//        userID != ""    ... UserID set equal to 'userID'
//
// NOTE: ALL column bindings associated with this wxDb instance are unbound
//       by this function.  This function should use its own wxDb instance
//       to avoid undesired unbinding of columns.
{
    UWORD       noCols = 0;
    UWORD       colNo  = 0;
    wxDbColInf *colInf = 0;

    RETCODE  retcode;
    SDWORD   cb;

    wxString TableName;

    wxString UserID;
    convertUserID(userID,UserID);

    // Pass 1 - Determine how many columns there are.
    // Pass 2 - Allocate the wxDbColInf array and fill in
    //                the array with the column information.
    int pass;
    for (pass = 1; pass <= 2; pass++)
    {
        if (pass == 2)
        {
            if (noCols == 0)  // Probably a bogus table name(s)
                break;
            // Allocate n wxDbColInf objects to hold the column information
            colInf = new wxDbColInf[noCols+1];
            if (!colInf)
                break;
            // Mark the end of the array
            wxStrcpy(colInf[noCols].tableName, wxEmptyString);
            wxStrcpy(colInf[noCols].colName, wxEmptyString);
            colInf[noCols].sqlDataType = 0;
        }

        TableName = tableName;
        // Oracle and Interbase table names are uppercase only, so force
        // the name to uppercase just in case programmer forgot to do this
        if ((Dbms() == dbmsORACLE) ||
            (Dbms() == dbmsFIREBIRD) ||
            (Dbms() == dbmsINTERBASE))
            TableName = TableName.Upper();

        SQLFreeStmt(hstmt, SQL_CLOSE);

        // MySQL, SQLServer, and Access cannot accept a user name when looking up column names, so we
        // use the call below that leaves out the user name
        if (!UserID.empty() &&
            Dbms() != dbmsMY_SQL &&
            Dbms() != dbmsACCESS &&
            Dbms() != dbmsMS_SQL_SERVER)
        {
            retcode = SQLColumns(hstmt,
                                 NULL, 0,                              // All qualifiers
                                 (UCHAR *) UserID.c_str(), SQL_NTS,    // Owner
                                 (UCHAR *) TableName.c_str(), SQL_NTS,
                                 NULL, 0);                             // All columns
        }
        else
        {
            retcode = SQLColumns(hstmt,
                                 NULL, 0,                              // All qualifiers
                                 NULL, 0,                              // Owner
                                 (UCHAR *) TableName.c_str(), SQL_NTS,
                                 NULL, 0);                             // All columns
        }
        if (retcode != SQL_SUCCESS)
        {  // Error occurred, abort
            DispAllErrors(henv, hdbc, hstmt);
            if (colInf)
                delete [] colInf;
            SQLFreeStmt(hstmt, SQL_CLOSE);
            if (numCols)
                *numCols = 0;
            return(0);
        }

        while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)
        {
            if (pass == 1)  // First pass, just add up the number of columns
                noCols++;
            else  // Pass 2; Fill in the array of structures
            {
                if (colNo < noCols)  // Some extra error checking to prevent memory overwrites
                {
                    // NOTE: Only the ODBC 1.x fields are retrieved
                    GetData( 1, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].catalog,      128+1,                     &cb);
                    GetData( 2, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].schema,       128+1,                     &cb);
                    GetData( 3, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].tableName,    DB_MAX_TABLE_NAME_LEN+1,   &cb);
                    GetData( 4, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].colName,      DB_MAX_COLUMN_NAME_LEN+1,  &cb);
                    GetData( 5, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].sqlDataType,  0,                         &cb);
                    GetData( 6, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].typeName,     128+1,                     &cb);
                    GetData( 7, SQL_C_SLONG,  (UCHAR*) &colInf[colNo].columnLength, 0,                         &cb);
                    GetData( 8, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].bufferSize,   0,                         &cb);
                    GetData( 9, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].decimalDigits,0,                         &cb);
                    GetData(10, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].numPrecRadix, 0,                         &cb);
                    GetData(11, SQL_C_SSHORT, (UCHAR*) &colInf[colNo].nullable,     0,                         &cb);
                    GetData(12, SQL_C_WXCHAR, (UCHAR*)  colInf[colNo].remarks,      254+1,                     &cb);
                    // Start Values for Primary/Foriegn Key (=No)
                    colInf[colNo].PkCol = 0;           // Primary key column   0=No; 1= First Key, 2 = Second Key etc.
                    colInf[colNo].PkTableName[0] = 0;  // Tablenames where Primary Key is used as a Foreign Key
                    colInf[colNo].FkCol = 0;           // Foreign key column   0=No; 1= First Key, 2 = Second Key etc.
                    colInf[colNo].FkTableName[0] = 0;  // Foreign key table name

#ifdef _IODBC_
                    // IODBC does not return a correct columnLength, so we set
                    // columnLength = bufferSize if no column length was returned
                    // IODBC returns the columnLength in bufferSize. (bug)
                    if (colInf[colNo].columnLength < 1)
                    {
                       colInf[colNo].columnLength = colInf[colNo].bufferSize;
                    }
#endif

                    // Determine the wxDb data type that is used to represent the native data type of this data source
                    colInf[colNo].dbDataType = 0;
                    // Get the intern datatype
                    switch (colInf[colNo].sqlDataType)
                    {
#if wxUSE_UNICODE
    #if defined(SQL_WCHAR)
                        case SQL_WCHAR:
    #endif
    #if defined(SQL_WVARCHAR)
                        case SQL_WVARCHAR:
    #endif
#endif
                        case SQL_VARCHAR:
                        case SQL_CHAR:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_VARCHAR;
                        break;
                        case SQL_LONGVARCHAR:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_MEMO;
                            break;
                        case SQL_TINYINT:
                        case SQL_SMALLINT:
                        case SQL_INTEGER:
                        case SQL_BIT:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_INTEGER;
                            break;
                        case SQL_DOUBLE:
                        case SQL_DECIMAL:
                        case SQL_NUMERIC:
                        case SQL_FLOAT:
                        case SQL_REAL:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_FLOAT;
                            break;
                        case SQL_DATE:
                        case SQL_TIMESTAMP:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_DATE;
                            break;
                        case SQL_BINARY:
                            colInf[colNo].dbDataType = DB_DATA_TYPE_BLOB;
                            break;
#ifdef __WXDEBUG__
                        default:
                            wxString errMsg;
                            errMsg.Printf(wxT("SQL Data type %d currently not supported by wxWidgets"), colInf[colNo].sqlDataType);
                            wxLogDebug(errMsg,wxT("ODBC DEBUG MESSAGE"));
#endif
                    }
                    colNo++;
                }
            }
        }
        if (retcode != SQL_NO_DATA_FOUND)
        {  // Error occurred, abort
            DispAllErrors(henv, hdbc, hstmt);
            if (colInf)
                delete [] colInf;
            SQLFreeStmt(hstmt, SQL_CLOSE);
            if (numCols)
                *numCols = 0;
            return(0);
        }
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);

    // Store Primary and Foreign Keys
    GetKeyFields(tableName,colInf,noCols);

    ///////////////////////////////////////////////////////////////////////////
    // Now sort the the columns in order to make them appear in the right order
    ///////////////////////////////////////////////////////////////////////////

    // Build a generic SELECT statement which returns 0 rows
    wxString Stmt;

    Stmt.Printf(wxT("select * from \"%s\" where 0=1"), tableName);

    // Execute query
    if (SQLExecDirect(hstmt, (UCHAR FAR *) Stmt.c_str(), SQL_NTS) != SQL_SUCCESS)
    {
        DispAllErrors(henv, hdbc, hstmt);
        return NULL;
    }

    // Get the number of result columns
    if (SQLNumResultCols (hstmt, &noCols) != SQL_SUCCESS)
    {
        DispAllErrors(henv, hdbc, hstmt);
        return NULL;
    }

    if (noCols == 0) // Probably a bogus table name
        return NULL;

    //  Get the name
    int i;
    short colNum;
    UCHAR name[100];
    SWORD Sword;
    SDWORD Sdword;
    for (colNum = 0; colNum < noCols; colNum++)
    {
        if (SQLColAttributes(hstmt,colNum+1, SQL_COLUMN_NAME,
            name, sizeof(name),
            &Sword, &Sdword) != SQL_SUCCESS)
        {
            DispAllErrors(henv, hdbc, hstmt);
            return NULL;
        }

        wxString Name1 = name;
        Name1 = Name1.Upper();

        // Where is this name in the array ?
        for (i = colNum ; i < noCols ; i++)
        {
            wxString Name2 =  colInf[i].colName;
            Name2 = Name2.Upper();
            if (Name2 == Name1)
            {
                if (colNum != i) // swap to sort
                {
                    wxDbColInf tmpColInf = colInf[colNum];
                    colInf[colNum] =  colInf[i];
                    colInf[i] = tmpColInf;
                }
                break;
            }
        }
    }
    SQLFreeStmt(hstmt, SQL_CLOSE);

    ///////////////////////////////////////////////////////////////////////////
    // End sorting
    ///////////////////////////////////////////////////////////////////////////

    if (numCols)
        *numCols = noCols;
    return colInf;

}  // wxDb::GetColumns()


#endif  // #else OLD_GETCOLUMNS


/********** wxDb::GetColumnCount() **********/
int wxDb::GetColumnCount(const wxString &tableName, const wxChar *userID)
/*
 * Returns a count of how many columns are in a table.
 * If an error occurs in computing the number of columns
 * this function will return a -1 for the count
 *
 * userID is evaluated in the following manner:
 *        userID == NULL  ... UserID is ignored
 *        userID == ""    ... UserID set equal to 'this->uid'
 *        userID != ""    ... UserID set equal to 'userID'
 *
 * NOTE: ALL column bindings associated with this wxDb instance are unbound
 *       by this function.  This function should use its own wxDb instance
 *       to avoid undesired unbinding of columns.
 */
{
    UWORD    noCols = 0;

    RETCODE  retcode;

    wxString TableName;

    wxString UserID;
    convertUserID(userID,UserID);

    TableName = tableName;
    // Oracle and Interbase table names are uppercase only, so force
    // the name to uppercase just in case programmer forgot to do this
    if ((Dbms() == dbmsORACLE) ||
        (Dbms() == dbmsFIREBIRD) ||
        (Dbms() == dbmsINTERBASE))
        TableName = TableName.Upper();

    SQLFreeStmt(hstmt, SQL_CLOSE);

    // MySQL, SQLServer, and Access cannot accept a user name when looking up column names, so we
    // use the call below that leaves out the user name
    if (!UserID.empty() &&
        Dbms() != dbmsMY_SQL &&
        Dbms() != dbmsACCESS &&
        Dbms() != dbmsMS_SQL_SERVER)
    {
        retcode = SQLColumns(hstmt,
                             NULL, 0,                                // All qualifiers
                             (SQLTCHAR *) UserID.c_str(), SQL_NTS,      // Owner
                             (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                             NULL, 0);                               // All columns
    }
    else
    {
        retcode = SQLColumns(hstmt,
                             NULL, 0,                                // All qualifiers
                             NULL, 0,                                // Owner
                             (SQLTCHAR *) TableName.c_str(), SQL_NTS,
                             NULL, 0);                               // All columns
    }
    if (retcode != SQL_SUCCESS)
    {  // Error occurred, abort
        DispAllErrors(henv, hdbc, hstmt);
        SQLFreeStmt(hstmt, SQL_CLOSE);
        return(-1);
    }

    // Count the columns
    while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)
        noCols++;

    if (retcode != SQL_NO_DATA_FOUND)
    {  // Error occurred, abort
        DispAllErrors(henv, hdbc, hstmt);
        SQLFreeStmt(hstmt, SQL_CLOSE);
        return(-1);
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);
    return noCols;

}  // wxDb::GetColumnCount()


/********** wxDb::GetCatalog() *******/
wxDbInf *wxDb::GetCatalog(const wxChar *userID)
/*
 * ---------------------------------------------------------------------
 * -- 19991203 : mj10777 : Create                                 ------
 * --          : Creates a wxDbInf with Tables / Cols Array       ------
 * --          : uses SQLTables and fills pTableInf;              ------
 * --          : pColInf is set to NULL and numCols to 0;         ------
 * --          : returns pDbInf (wxDbInf)                         ------
 * --            - if unsuccessful (pDbInf == NULL)               ------
 * --          : pColInf can be filled with GetColumns(..);       ------
 * --          : numCols   can be filled with GetColumnCount(..); ------
 * ---------------------------------------------------------------------
 *
 * userID is evaluated in the following manner:
 *        userID == NULL  ... UserID is ignored
 *        userID == ""    ... UserID set equal to 'this->uid'
 *        userID != ""    ... UserID set equal to 'userID'
 *
 * NOTE: ALL column bindings associated with this wxDb instance are unbound
 *       by this function.  This function should use its own wxDb instance
 *       to avoid undesired unbinding of columns.
 */
{
    int      noTab = 0;     // Counter while filling table entries
    int      pass;
    RETCODE  retcode;
    SQLLEN   cb;
    wxString tblNameSave;

    wxString UserID;
    convertUserID(userID,UserID);

    //-------------------------------------------------------------
    // Create the Database Array of catalog entries

    wxDbInf *pDbInf = new wxDbInf;

    //-------------------------------------------------------------
    // Table Information
    // Pass 1 - Determine how many Tables there are.
    // Pass 2 - Create the Table array and fill it
    //        - Create the Cols array = NULL
    //-------------------------------------------------------------

    for (pass = 1; pass <= 2; pass++)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);   // Close if Open
        tblNameSave.Empty();

        if (!UserID.empty() &&
            Dbms() != dbmsMY_SQL &&
            Dbms() != dbmsACCESS &&
            Dbms() != dbmsMS_SQL_SERVER)
        {
            retcode = SQLTables(hstmt,
                                NULL, 0,                             // All qualifiers
                                (SQLTCHAR *) UserID.c_str(), SQL_NTS,   // User specified
                                NULL, 0,                             // All tables
                                NULL, 0);                            // All columns
        }
        else
        {
            retcode = SQLTables(hstmt,
                                NULL, 0,           // All qualifiers
                                NULL, 0,           // User specified
                                NULL, 0,           // All tables
                                NULL, 0);          // All columns
        }

        if (retcode != SQL_SUCCESS)
        {
            DispAllErrors(henv, hdbc, hstmt);
            pDbInf = NULL;
            SQLFreeStmt(hstmt, SQL_CLOSE);
            return pDbInf;
        }

        while ((retcode = SQLFetch(hstmt)) == SQL_SUCCESS)   // Table Information
        {
            if (pass == 1)  // First pass, just count the Tables
            {
                if (pDbInf->numTables == 0)
                {
                    GetData( 1, SQL_C_WXCHAR,   (UCHAR*)  pDbInf->catalog,  128+1, &cb);
                    GetData( 2, SQL_C_WXCHAR,   (UCHAR*)  pDbInf->schema,   128+1, &cb);
                 }
                 pDbInf->numTables++;      // Counter for Tables
            }  // if (pass == 1)
            if (pass == 2) // Create and fill the Table entries
            {
                if (pDbInf->pTableInf == NULL)   // Has the Table Array been created
                {  // no, then create the Array
                    pDbInf->pTableInf = new wxDbTableInf[pDbInf->numTables];
                    noTab = 0;
                } // if (pDbInf->pTableInf == NULL)   // Has the Table Array been created

                GetData( 3, SQL_C_WXCHAR,   (UCHAR*)  (pDbInf->pTableInf+noTab)->tableName,    DB_MAX_TABLE_NAME_LEN+1, &cb);
                GetData( 4, SQL_C_WXCHAR,   (UCHAR*)  (pDbInf->pTableInf+noTab)->tableType,    30+1,                    &cb);
                GetData( 5, SQL_C_WXCHAR,   (UCHAR*)  (pDbInf->pTableInf+noTab)->tableRemarks, 254+1,                   &cb);

                noTab++;
            }  // if
        }  // while
    }  // for
    SQLFreeStmt(hstmt, SQL_CLOSE);

    // Query how many columns are in each table
    for (noTab=0;noTab<pDbInf->numTables;noTab++)
    {
        (pDbInf->pTableInf+noTab)->numCols = (UWORD)GetColumnCount((pDbInf->pTableInf+noTab)->tableName,UserID);
    }

    return pDbInf;

}  // wxDb::GetCatalog()


/********** wxDb::Catalog() **********/
bool wxDb::Catalog(const wxChar *userID, const wxString &fileName)
/*
 * Creates the text file specified in 'filename' which will contain
 * a minimal data dictionary of all tables accessible by the user specified
 * in 'userID'
 *
 * userID is evaluated in the following manner:
 *        userID == NULL  ... UserID is ignored
 *        userID == ""    ... UserID set equal to 'this->uid'
 *        userID != ""    ... UserID set equal to 'userID'
 *
 * NOTE: ALL column bindings associated with this wxDb instance are unbound
 *       by this function.  This function should use its own wxDb instance
 *       to avoid undesired unbinding of columns.
 */
{
    wxASSERT(fileName.length());

    RETCODE   retcode;
    SQLLEN    cb;
    wxChar    tblName[DB_MAX_TABLE_NAME_LEN+1];
    wxString  tblNameSave;
    wxChar    colName[DB_MAX_COLUMN_NAME_LEN+1];
    SWORD     sqlDataType;
    wxChar    typeName[30+1];
    SDWORD    precision, length;

    FILE *fp = wxFopen(fileName.c_str(),wxT("wt"));
    if (fp == NULL)
        return false;

    SQLFreeStmt(hstmt, SQL_CLOSE);

    wxString UserID;
    convertUserID(userID,UserID);

    if (!UserID.empty() &&
        Dbms() != dbmsMY_SQL &&
        Dbms() != dbmsACCESS &&
        Dbms() != dbmsFIREBIRD &&
        Dbms() != dbmsINTERBASE &&
        Dbms() != dbmsMS_SQL_SERVER)
    {
        retcode = SQLColumns(hstmt,
                             NULL, 0,                                // All qualifiers
                             (SQLTCHAR *) UserID.c_str(), SQL_NTS,      // User specified
                             NULL, 0,                                // All tables
                             NULL, 0);                               // All columns
    }
    else
    {
        retcode = SQLColumns(hstmt,
                             NULL, 0,    // All qualifiers
                             NULL, 0,    // User specified
                             NULL, 0,    // All tables
                             NULL, 0);   // All columns
    }
    if (retcode != SQL_SUCCESS)
    {
        DispAllErrors(henv, hdbc, hstmt);
        fclose(fp);
        return false;
    }

    wxString outStr;
    tblNameSave.Empty();
    int cnt = 0;

    while (true)
    {
        retcode = SQLFetch(hstmt);
        if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            break;

        GetData(3,SQL_C_WXCHAR,  (UCHAR *) tblName,     DB_MAX_TABLE_NAME_LEN+1, &cb);
        GetData(4,SQL_C_WXCHAR,  (UCHAR *) colName,     DB_MAX_COLUMN_NAME_LEN+1,&cb);
        GetData(5,SQL_C_SSHORT,  (UCHAR *)&sqlDataType, 0,                       &cb);
        GetData(6,SQL_C_WXCHAR,  (UCHAR *) typeName,    sizeof(typeName),        &cb);
        GetData(7,SQL_C_SLONG,   (UCHAR *)&precision,   0,                       &cb);
        GetData(8,SQL_C_SLONG,   (UCHAR *)&length,      0,                       &cb);

        if (wxStrcmp(tblName, tblNameSave.c_str()))
        {
            if (cnt)
                wxFputs(wxT("\n"), fp);
            wxFputs(wxT("================================ "), fp);
            wxFputs(wxT("================================ "), fp);
            wxFputs(wxT("===================== "), fp);
            wxFputs(wxT("========= "), fp);
            wxFputs(wxT("=========\n"), fp);
            outStr.Printf(wxT("%-32s %-32s %-21s %9s %9s\n"),
                wxT("TABLE NAME"), wxT("COLUMN NAME"), wxT("DATA TYPE"), wxT("PRECISION"), wxT("LENGTH"));
            wxFputs(outStr.c_str(), fp);
            wxFputs(wxT("================================ "), fp);
            wxFputs(wxT("================================ "), fp);
            wxFputs(wxT("===================== "), fp);
            wxFputs(wxT("========= "), fp);
            wxFputs(wxT("=========\n"), fp);
            tblNameSave = tblName;
        }

        outStr.Printf(wxT("%-32s %-32s (%04d)%-15s %9ld %9ld\n"),
            tblName, colName, sqlDataType, typeName, precision, length);
        if (wxFputs(outStr.c_str(), fp) == EOF)
        {
            SQLFreeStmt(hstmt, SQL_CLOSE);
            fclose(fp);
            return false;
        }
        cnt++;
    }

    if (retcode != SQL_NO_DATA_FOUND)
        DispAllErrors(henv, hdbc, hstmt);

    SQLFreeStmt(hstmt, SQL_CLOSE);

    fclose(fp);
    return(retcode == SQL_NO_DATA_FOUND);

}  // wxDb::Catalog()


bool wxDb::TableExists(const wxString &tableName, const wxChar *userID, const wxString &tablePath)
/*
 * Table name can refer to a table, view, alias or synonym.  Returns true
 * if the object exists in the database.  This function does not indicate
 * whether or not the user has privleges to query or perform other functions
 * on the table.
 *
 * userID is evaluated in the following manner:
 *        userID == NULL  ... UserID is ignored
 *        userID == ""    ... UserID set equal to 'this->uid'
 *        userID != ""    ... UserID set equal to 'userID'
 */
{
    wxASSERT(tableName.length());

    wxString TableName;

    if (Dbms() == dbmsDBASE)
    {
        wxString dbName;
        if (tablePath.length())
            dbName.Printf(wxT("%s/%s.dbf"), tablePath.c_str(), tableName.c_str());
        else
            dbName.Printf(wxT("%s.dbf"), tableName.c_str());

        bool exists;
        exists = wxFileExists(dbName);
        return exists;
    }

    wxString UserID;
    convertUserID(userID,UserID);

    TableName = tableName;
    // Oracle and Interbase table names are uppercase only, so force
    // the name to uppercase just in case programmer forgot to do this
    if ((Dbms() == dbmsORACLE) ||
        (Dbms() == dbmsFIREBIRD) ||
        (Dbms() == dbmsINTERBASE))
        TableName = TableName.Upper();

    SQLFreeStmt(hstmt, SQL_CLOSE);
    RETCODE retcode;

    // Some databases cannot accept a user name when looking up table names,
    // so we use the call below that leaves out the user name
    if (!UserID.empty() &&
        Dbms() != dbmsMY_SQL &&
        Dbms() != dbmsACCESS &&
        Dbms() != dbmsMS_SQL_SERVER &&
        Dbms() != dbmsDB2 &&
        Dbms() != dbmsFIREBIRD &&
        Dbms() != dbmsINTERBASE &&
        Dbms() != dbmsPERVASIVE_SQL)
    {
        retcode = SQLTables(hstmt,
                            NULL, 0,                                  // All qualifiers
                            (SQLTCHAR *) UserID.c_str(), SQL_NTS,        // Only tables owned by this user
                            (SQLTCHAR FAR *)TableName.c_str(), SQL_NTS,
                            NULL, 0);                                 // All table types
    }
    else
    {
        retcode = SQLTables(hstmt,
                            NULL, 0,                                  // All qualifiers
                            NULL, 0,                                  // All owners
                            (SQLTCHAR FAR *)TableName.c_str(), SQL_NTS,
                            NULL, 0);                                 // All table types
    }
    if (retcode != SQL_SUCCESS)
        return(DispAllErrors(henv, hdbc, hstmt));

    retcode = SQLFetch(hstmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        return(DispAllErrors(henv, hdbc, hstmt));
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);

    return true;

}  // wxDb::TableExists()


/********** wxDb::TablePrivileges() **********/
bool wxDb::TablePrivileges(const wxString &tableName, const wxString &priv, const wxChar *userID,
                            const wxChar *schema, const wxString &WXUNUSED(tablePath))
{
    wxASSERT(tableName.length());

    wxDbTablePrivilegeInfo  result;
    SQLLEN  cbRetVal;
    RETCODE retcode;

    // We probably need to be able to dynamically set this based on
    // the driver type, and state.
    wxChar curRole[]=wxT("public");

    wxString TableName;

    wxString UserID,Schema;
    convertUserID(userID,UserID);
    convertUserID(schema,Schema);

    TableName = tableName;
    // Oracle and Interbase table names are uppercase only, so force
    // the name to uppercase just in case programmer forgot to do this
    if ((Dbms() == dbmsORACLE) ||
        (Dbms() == dbmsFIREBIRD) ||
        (Dbms() == dbmsINTERBASE))
        TableName = TableName.Upper();

    SQLFreeStmt(hstmt, SQL_CLOSE);

    // Some databases cannot accept a user name when looking up table names,
    // so we use the call below that leaves out the user name
    if (!Schema.empty() &&
        Dbms() != dbmsMY_SQL &&
        Dbms() != dbmsACCESS &&
        Dbms() != dbmsMS_SQL_SERVER)
    {
        retcode = SQLTablePrivileges(hstmt,
                                     NULL, 0,                                    // Catalog
                                     (SQLTCHAR FAR *)Schema.c_str(), SQL_NTS,               // Schema
                                     (SQLTCHAR FAR *)TableName.c_str(), SQL_NTS);
    }
    else
    {
        retcode = SQLTablePrivileges(hstmt,
                                     NULL, 0,                                    // Catalog
                                     NULL, 0,                                    // Schema
                                     (SQLTCHAR FAR *)TableName.c_str(), SQL_NTS);
    }

#ifdef DBDEBUG_CONSOLE
    wxFprintf(stderr ,wxT("SQLTablePrivileges() returned %i \n"),retcode);
#endif

    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
        return (DispAllErrors(henv, hdbc, hstmt));

    bool failed = false;
    retcode = SQLFetch(hstmt);
    while (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
        if (SQLGetData(hstmt, 1, SQL_C_WXCHAR, (UCHAR*) result.tableQual, sizeof(result.tableQual), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 2, SQL_C_WXCHAR, (UCHAR*) result.tableOwner, sizeof(result.tableOwner), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 3, SQL_C_WXCHAR, (UCHAR*) result.tableName, sizeof(result.tableName), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 4, SQL_C_WXCHAR, (UCHAR*) result.grantor, sizeof(result.grantor), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 5, SQL_C_WXCHAR, (UCHAR*) result.grantee, sizeof(result.grantee), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 6, SQL_C_WXCHAR, (UCHAR*) result.privilege, sizeof(result.privilege), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (!failed && SQLGetData(hstmt, 7, SQL_C_WXCHAR, (UCHAR*) result.grantable, sizeof(result.grantable), &cbRetVal) != SQL_SUCCESS)
            failed = true;

        if (failed)
        {
            return(DispAllErrors(henv, hdbc, hstmt));
        }
#ifdef DBDEBUG_CONSOLE
        wxFprintf(stderr,wxT("Scanning %s privilege on table %s.%s granted by %s to %s\n"),
                result.privilege,result.tableOwner,result.tableName,
                result.grantor, result.grantee);
#endif

        if (UserID.IsSameAs(result.tableOwner,false))
        {
            SQLFreeStmt(hstmt, SQL_CLOSE);
            return true;
        }

        if (UserID.IsSameAs(result.grantee,false) &&
            !wxStrcmp(result.privilege,priv))
        {
            SQLFreeStmt(hstmt, SQL_CLOSE);
            return true;
        }

        if (!wxStrcmp(result.grantee,curRole) &&
            !wxStrcmp(result.privilege,priv))
        {
            SQLFreeStmt(hstmt, SQL_CLOSE);
            return true;
        }

        retcode = SQLFetch(hstmt);
    }

    SQLFreeStmt(hstmt, SQL_CLOSE);
    return false;

}  // wxDb::TablePrivileges


const wxString wxDb::SQLTableName(const wxChar *tableName)
{
    wxString TableName;

    if (Dbms() == dbmsACCESS)
        TableName = _T("\"");
    TableName += tableName;
    if (Dbms() == dbmsACCESS)
        TableName += _T("\"");

    return TableName;
}  // wxDb::SQLTableName()


const wxString wxDb::SQLColumnName(const wxChar *colName)
{
    wxString ColName;

    if (Dbms() == dbmsACCESS)
        ColName = _T("\"");
    ColName += colName;
    if (Dbms() == dbmsACCESS)
        ColName += _T("\"");

    return ColName;
}  // wxDb::SQLColumnName()


/********** wxDb::SetSqlLogging() **********/
bool wxDb::SetSqlLogging(wxDbSqlLogState state, const wxString &filename, bool append)
{
    wxASSERT(state == sqlLogON  || state == sqlLogOFF);
    wxASSERT(state == sqlLogOFF || filename.length());

    if (state == sqlLogON)
    {
        if (fpSqlLog == 0)
        {
            fpSqlLog = wxFopen(filename.c_str(), (append ? wxT("at") : wxT("wt")));
            if (fpSqlLog == NULL)
                return false;
        }
    }
    else  // sqlLogOFF
    {
        if (fpSqlLog)
        {
            if (fclose(fpSqlLog))
                return false;
            fpSqlLog = 0;
        }
    }

    sqlLogState = state;
    return true;

}  // wxDb::SetSqlLogging()


/********** wxDb::WriteSqlLog() **********/
bool wxDb::WriteSqlLog(const wxString &logMsg)
{
    wxASSERT(logMsg.length());

    if (fpSqlLog == 0 || sqlLogState == sqlLogOFF)
        return false;

    if (wxFputs(wxT("\n"),   fpSqlLog) == EOF)
        return false;
    if (wxFputs(logMsg, fpSqlLog) == EOF)
        return false;
    if (wxFputs(wxT("\n"),   fpSqlLog) == EOF)
        return false;

    return true;

}  // wxDb::WriteSqlLog()


/********** wxDb::Dbms() **********/
wxDBMS wxDb::Dbms(void)
/*
 * Be aware that not all database engines use the exact same syntax, and not
 * every ODBC compliant database is compliant to the same level of compliancy.
 * Some manufacturers support the minimum Level 1 compliancy, and others up
 * through Level 3.  Others support subsets of features for levels above 1.
 *
 * If you find an inconsistency between the wxDb class and a specific database
 * engine, and an identifier to this section, and special handle the database in
 * the area where behavior is non-conforming with the other databases.
 *
 *
 * NOTES ABOUT ISSUES SPECIFIC TO EACH DATABASE ENGINE
 * ---------------------------------------------------
 *
 * ORACLE
 *        - Currently the only database supported by the class to support VIEWS
 *
 * DBASE
 *        - Does not support the SQL_TIMESTAMP structure
 *        - Supports only one cursor and one connect (apparently? with Microsoft driver only?)
 *        - Does not automatically create the primary index if the 'keyField' param of SetColDef
 *            is true.  The user must create ALL indexes from their program.
 *        - Table names can only be 8 characters long
 *        - Column names can only be 10 characters long
 *
 * SYBASE (all)
 *        - To lock a record during QUERY functions, the reserved word 'HOLDLOCK' must be added
 *            after every table name involved in the query/join if that tables matching record(s)
 *            are to be locked
 *        - Ignores the keywords 'FOR UPDATE'.  Use the HOLDLOCK functionality described above
 *
 * SYBASE (Enterprise)
 *        - If a column is part of the Primary Key, the column cannot be NULL
 *        - Maximum row size is somewhere in the neighborhood of 1920 bytes
 *
 * MY_SQL
 *        - If a column is part of the Primary Key, the column cannot be NULL
 *        - Cannot support selecting for update [::CanSelectForUpdate()].  Always returns FALSE
 *        - Columns that are part of primary or secondary keys must be defined as being NOT NULL
 *            when they are created.  Some code is added in ::CreateIndex to try to adjust the
 *            column definition if it is not defined correctly, but it is experimental
 *        - Does not support sub-queries in SQL statements
 *
 * POSTGRES
 *        - Does not support the keywords 'ASC' or 'DESC' as of release v6.5.0
 *        - Does not support sub-queries in SQL statements
 *
 * DB2
 *        - Primary keys must be declared as NOT NULL
 *        - Table and index names must not be longer than 13 characters in length (technically
 *          table names can be up to 18 characters, but the primary index is created using the
 *          base table name plus "_PIDX", so the limit if the table has a primary index is 13.
 *
 * PERVASIVE SQL
 *
 * INTERBASE
 *        - Columns that are part of primary keys must be defined as being NOT NULL
 *          when they are created.  Some code is added in ::CreateIndex to try to adjust the
 *          column definition if it is not defined correctly, but it is experimental
 */
{
    // Should only need to do this once for each new database connection
    // so return the value we already determined it to be to save time
    // and lots of string comparisons
    if (dbmsType != dbmsUNIDENTIFIED)
        return(dbmsType);

#ifdef DBDEBUG_CONSOLE
               // When run in console mode, use standard out to display errors.
               cout << "Database connecting to: " << dbInf.dbmsName << endl;
#endif  // DBDEBUG_CONSOLE

    wxLogDebug(wxT("Database connecting to: "));
    wxLogDebug(dbInf.dbmsName);

    wxChar baseName[25+1];
    wxStrncpy(baseName, dbInf.dbmsName, 25);
    baseName[25] = 0;

    // RGG 20001025 : add support for Interbase
    // GT : Integrated to base classes on 20001121
    if (!wxStricmp(dbInf.dbmsName,wxT("Interbase")))
        return((wxDBMS)(dbmsType = dbmsINTERBASE));

    // BJO 20000428 : add support for Virtuoso
    if (!wxStricmp(dbInf.dbmsName,wxT("OpenLink Virtuoso VDBMS")))
      return((wxDBMS)(dbmsType = dbmsVIRTUOSO));

    if (!wxStricmp(dbInf.dbmsName,wxT("Adaptive Server Anywhere")))
        return((wxDBMS)(dbmsType = dbmsSYBASE_ASA));

    // BJO 20000427 : The "SQL Server" string is also returned by SQLServer when
    // connected through an OpenLink driver.
    // Is it also returned by Sybase Adapatitve server?
    // OpenLink driver name is OLOD3032.DLL for msw and oplodbc.so for unix
    if (!wxStricmp(dbInf.dbmsName,wxT("SQL Server")))
    {
      if (!wxStrncmp(dbInf.driverName, wxT("oplodbc"), 7) ||
          !wxStrncmp(dbInf.driverName, wxT("OLOD"), 4))
            return ((wxDBMS)(dbmsMS_SQL_SERVER));
        else
            return ((wxDBMS)(dbmsType = dbmsSYBASE_ASE));
    }

    if (!wxStricmp(dbInf.dbmsName,wxT("Microsoft SQL Server")))
        return((wxDBMS)(dbmsType = dbmsMS_SQL_SERVER));

    baseName[10] = 0;
    if (!wxStricmp(baseName,wxT("PostgreSQL")))  // v6.5.0
        return((wxDBMS)(dbmsType = dbmsPOSTGRES));

    baseName[9] = 0;
    if (!wxStricmp(baseName,wxT("Pervasive")))
        return((wxDBMS)(dbmsType = dbmsPERVASIVE_SQL));

    baseName[8] = 0;
    if (!wxStricmp(baseName,wxT("Informix")))
        return((wxDBMS)(dbmsType = dbmsINFORMIX));

    if (!wxStricmp(baseName,wxT("Firebird")))
        return((wxDBMS)(dbmsType = dbmsFIREBIRD));

    baseName[6] = 0;
    if (!wxStricmp(baseName,wxT("Oracle")))
        return((wxDBMS)(dbmsType = dbmsORACLE));
    if (!wxStricmp(baseName,wxT("ACCESS")))
        return((wxDBMS)(dbmsType = dbmsACCESS));
    if (!wxStricmp(baseName,wxT("Sybase")))
      return((wxDBMS)(dbmsType = dbmsSYBASE_ASE));

    baseName[5] = 0;
    if (!wxStricmp(baseName,wxT("DBASE")))
        return((wxDBMS)(dbmsType = dbmsDBASE));
    if (!wxStricmp(baseName,wxT("xBase")))
        return((wxDBMS)(dbmsType = dbmsXBASE_SEQUITER));
    if (!wxStricmp(baseName,wxT("MySQL")))
        return((wxDBMS)(dbmsType = dbmsMY_SQL));
    if (!wxStricmp(baseName,wxT("MaxDB")))
        return((wxDBMS)(dbmsType = dbmsMAXDB));

    baseName[3] = 0;
    if (!wxStricmp(baseName,wxT("DB2")))
        return((wxDBMS)(dbmsType = dbmsDB2));

    return((wxDBMS)(dbmsType = dbmsUNIDENTIFIED));

}  // wxDb::Dbms()


bool wxDb::ModifyColumn(const wxString &tableName, const wxString &columnName,
                        int dataType, ULONG columnLength,
                        const wxString &optionalParam)
{
    wxASSERT(tableName.length());
    wxASSERT(columnName.length());
    wxASSERT((dataType == DB_DATA_TYPE_VARCHAR && columnLength > 0) ||
             dataType != DB_DATA_TYPE_VARCHAR);

    // Must specify a columnLength if modifying a VARCHAR type column
    if (dataType == DB_DATA_TYPE_VARCHAR && !columnLength)
        return false;

    wxString dataTypeName;
    wxString sqlStmt;
    wxString alterSlashModify;

    switch(dataType)
    {
        case DB_DATA_TYPE_VARCHAR :
            dataTypeName = typeInfVarchar.TypeName;
            break;
        case DB_DATA_TYPE_INTEGER :
            dataTypeName = typeInfInteger.TypeName;
            break;
        case DB_DATA_TYPE_FLOAT :
            dataTypeName = typeInfFloat.TypeName;
            break;
        case DB_DATA_TYPE_DATE :
            dataTypeName = typeInfDate.TypeName;
            break;
        case DB_DATA_TYPE_BLOB :
            dataTypeName = typeInfBlob.TypeName;
            break;
        default:
            return false;
    }

    // Set the modify or alter syntax depending on the type of database connected to
    switch (Dbms())
    {
        case dbmsORACLE :
            alterSlashModify = _T("MODIFY");
            break;
        case dbmsMS_SQL_SERVER :
            alterSlashModify = _T("ALTER COLUMN");
            break;
        case dbmsUNIDENTIFIED :
            return false;
        case dbmsSYBASE_ASA :
        case dbmsSYBASE_ASE :
        case dbmsMY_SQL :
        case dbmsPOSTGRES :
        case dbmsACCESS :
        case dbmsDBASE :
        case dbmsXBASE_SEQUITER :
        default :
            alterSlashModify = _T("MODIFY");
            break;
    }

    // create the SQL statement
    if ( Dbms() == dbmsMY_SQL )
    {
        sqlStmt.Printf(wxT("ALTER TABLE %s %s %s %s"), tableName.c_str(), alterSlashModify.c_str(),
              columnName.c_str(), dataTypeName.c_str());
    }
    else
    {
        sqlStmt.Printf(wxT("ALTER TABLE \"%s\" \"%s\" \"%s\" %s"), tableName.c_str(), alterSlashModify.c_str(),
              columnName.c_str(), dataTypeName.c_str());
    }

    // For varchars only, append the size of the column
    if (dataType == DB_DATA_TYPE_VARCHAR &&
        (Dbms() != dbmsMY_SQL || dataTypeName != _T("text")))
    {
        wxString s;
        s.Printf(wxT("(%lu)"), columnLength);
        sqlStmt += s;
    }

    // for passing things like "NOT NULL"
    if (optionalParam.length())
    {
        sqlStmt += wxT(" ");
        sqlStmt += optionalParam;
    }

    return ExecSql(sqlStmt);

} // wxDb::ModifyColumn()

/********** wxDb::EscapeSqlChars() **********/
wxString wxDb::EscapeSqlChars(const wxString& valueOrig)
{
    wxString value(valueOrig);
    switch (Dbms())
    {
        case dbmsACCESS:
            // Access doesn't seem to care about backslashes, so only escape single quotes.
            value.Replace(wxT("'"), wxT("''"));
            break;

        default:
            // All the others are supposed to be the same for now, add special
            // handling for them if necessary
            value.Replace(wxT("\\"), wxT("\\\\"));
            value.Replace(wxT("'"), wxT("\\'"));
            break;
    }

    return value;
} // wxDb::EscapeSqlChars()


/********** wxDbGetConnection() **********/
wxDb WXDLLIMPEXP_ODBC *wxDbGetConnection(wxDbConnectInf *pDbConfig, bool FwdOnlyCursors)
{
    wxDbList *pList;

    // Used to keep a pointer to a DB connection that matches the requested
    // DSN and FwdOnlyCursors settings, even if it is not FREE, so that the
    // data types can be copied from it (using the wxDb::Open(wxDb *) function)
    // rather than having to re-query the datasource to get all the values
    // using the wxDb::Open(Dsn,Uid,AuthStr) function
    wxDb *matchingDbConnection = NULL;

    // Scan the linked list searching for an available database connection
    // that's already been opened but is currently not in use.
    for (pList = PtrBegDbList; pList; pList = pList->PtrNext)
    {
        // The database connection must be for the same datasource
        // name and must currently not be in use.
        if (pList->Free &&
            (pList->PtrDb->FwdOnlyCursors() == FwdOnlyCursors))
        {
            if (pDbConfig->UseConnectionStr())
            {
                if (pList->PtrDb->OpenedWithConnectionString() &&
                     (!wxStrcmp(pDbConfig->GetConnectionStr(), pList->ConnectionStr)))
                {
                    // Found a free connection
                    pList->Free = false;
                    return(pList->PtrDb);
                }
            }
            else
            {
                if (!pList->PtrDb->OpenedWithConnectionString() &&
                     (!wxStrcmp(pDbConfig->GetDsn(), pList->Dsn)))
                {
                    // Found a free connection
                    pList->Free = false;
                    return(pList->PtrDb);
                }
            }
        }

        if (pDbConfig->UseConnectionStr())
        {
            if (!wxStrcmp(pDbConfig->GetConnectionStr(), pList->ConnectionStr))
                matchingDbConnection = pList->PtrDb;
        }
        else
        {
            if (!wxStrcmp(pDbConfig->GetDsn(), pList->Dsn) &&
                !wxStrcmp(pDbConfig->GetUserID(), pList->Uid) &&
                !wxStrcmp(pDbConfig->GetPassword(), pList->AuthStr))
                matchingDbConnection = pList->PtrDb;
        }
    }

    // No available connections.  A new connection must be made and
    // appended to the end of the linked list.
    if (PtrBegDbList)
    {
        // Find the end of the list
        for (pList = PtrBegDbList; pList->PtrNext; pList = pList->PtrNext);
        // Append a new list item
        pList->PtrNext = new wxDbList;
        pList->PtrNext->PtrPrev = pList;
        pList = pList->PtrNext;
    }
    else  // Empty list
    {
        // Create the first node on the list
        pList = PtrBegDbList = new wxDbList;
        pList->PtrPrev = 0;
    }

    // Initialize new node in the linked list
    pList->PtrNext          = 0;
    pList->Free             = false;
    pList->Dsn              = pDbConfig->GetDsn();
    pList->Uid              = pDbConfig->GetUserID();
    pList->AuthStr          = pDbConfig->GetPassword();
    pList->ConnectionStr    = pDbConfig->GetConnectionStr();

    pList->PtrDb = new wxDb(pDbConfig->GetHenv(), FwdOnlyCursors);

    bool opened;

    if (!matchingDbConnection)
    {
        if (pDbConfig->UseConnectionStr())
        {
            opened = pList->PtrDb->Open(pDbConfig->GetConnectionStr());
        }
        else
        {
            opened = pList->PtrDb->Open(pDbConfig->GetDsn(), pDbConfig->GetUserID(), pDbConfig->GetPassword());
        }
    }
    else
        opened = pList->PtrDb->Open(matchingDbConnection);

    // Connect to the datasource
    if (opened)
    {
        pList->PtrDb->setCached(true);  // Prevent a user from deleting a cached connection
        pList->PtrDb->SetSqlLogging(SQLLOGstate, SQLLOGfn, true);
        return(pList->PtrDb);
    }
    else  // Unable to connect, destroy list item
    {
        if (pList->PtrPrev)
            pList->PtrPrev->PtrNext = 0;
        else
            PtrBegDbList = 0;        // Empty list again

        pList->PtrDb->CommitTrans(); // Commit any open transactions on wxDb object
        pList->PtrDb->Close();       // Close the wxDb object
        delete pList->PtrDb;         // Deletes the wxDb object
        delete pList;                // Deletes the linked list object
        return(0);
    }

}  // wxDbGetConnection()


/********** wxDbFreeConnection() **********/
bool WXDLLIMPEXP_ODBC wxDbFreeConnection(wxDb *pDb)
{
    wxDbList *pList;

    // Scan the linked list searching for the database connection
    for (pList = PtrBegDbList; pList; pList = pList->PtrNext)
    {
        if (pList->PtrDb == pDb)  // Found it, now free it!!!
            return (pList->Free = true);
    }

    // Never found the database object, return failure
    return false;

}  // wxDbFreeConnection()


/********** wxDbCloseConnections() **********/
void WXDLLIMPEXP_ODBC wxDbCloseConnections(void)
{
    wxDbList *pList, *pNext;

    // Traverse the linked list closing database connections and freeing memory as I go.
    for (pList = PtrBegDbList; pList; pList = pNext)
    {
        pNext = pList->PtrNext;       // Save the pointer to next
        pList->PtrDb->CommitTrans();  // Commit any open transactions on wxDb object
        pList->PtrDb->Close();        // Close the wxDb object
        pList->PtrDb->setCached(false);  // Allows deletion of the wxDb instance
        delete pList->PtrDb;          // Deletes the wxDb object
        delete pList;                 // Deletes the linked list object
    }

    // Mark the list as empty
    PtrBegDbList = 0;

}  // wxDbCloseConnections()


/********** wxDbConnectionsInUse() **********/
int WXDLLIMPEXP_ODBC wxDbConnectionsInUse(void)
{
    wxDbList *pList;
    int cnt = 0;

    // Scan the linked list counting db connections that are currently in use
    for (pList = PtrBegDbList; pList; pList = pList->PtrNext)
    {
        if (pList->Free == false)
            cnt++;
    }

    return(cnt);

}  // wxDbConnectionsInUse()



/********** wxDbLogExtendedErrorMsg() **********/
// DEBUG ONLY function
const wxChar WXDLLIMPEXP_ODBC *wxDbLogExtendedErrorMsg(const wxChar *userText,
                                                  wxDb *pDb,
                                                  const wxChar *ErrFile,
                                                  int ErrLine)
{
    static wxString msg;
    msg = userText;

    wxString tStr;

    if (ErrFile || ErrLine)
    {
        msg += wxT("File: ");
        msg += ErrFile;
        msg += wxT("   Line: ");
        tStr.Printf(wxT("%d"),ErrLine);
        msg += tStr.c_str();
        msg += wxT("\n");
    }

    msg.Append (wxT("\nODBC errors:\n"));
    msg += wxT("\n");

    // Display errors for this connection
    int i;
    for (i = 0; i < DB_MAX_ERROR_HISTORY; i++)
    {
        if (pDb->errorList[i])
        {
            msg.Append(pDb->errorList[i]);
            if (wxStrcmp(pDb->errorList[i], wxEmptyString) != 0)
                msg.Append(wxT("\n"));
            // Clear the errmsg buffer so the next error will not
            // end up showing the previous error that have occurred
            wxStrcpy(pDb->errorList[i], wxEmptyString);
        }
    }
    msg += wxT("\n");

    wxLogDebug(msg.c_str());

    return msg.c_str();
}  // wxDbLogExtendedErrorMsg()


/********** wxDbSqlLog() **********/
bool wxDbSqlLog(wxDbSqlLogState state, const wxChar *filename)
{
    bool append = false;
    wxDbList *pList;

    for (pList = PtrBegDbList; pList; pList = pList->PtrNext)
    {
        if (!pList->PtrDb->SetSqlLogging(state,filename,append))
            return false;
        append = true;
    }

    SQLLOGstate = state;
    SQLLOGfn = filename;

    return true;

}  // wxDbSqlLog()


#if 0
/********** wxDbCreateDataSource() **********/
int wxDbCreateDataSource(const wxString &driverName, const wxString &dsn, const wxString &description,
                         bool sysDSN, const wxString &defDir, wxWindow *parent)
/*
 * !!!! ONLY FUNCTIONAL UNDER MSW with VC6 !!!!
 * Very rudimentary creation of an ODBC data source.
 *
 * ODBC driver must be ODBC 3.0 compliant to use this function
 */
{
    int result = FALSE;

//!!!! ONLY FUNCTIONAL UNDER MSW with VC6 !!!!
#ifdef __VISUALC__
    int       dsnLocation;
    wxString  setupStr;

    if (sysDSN)
        dsnLocation = ODBC_ADD_SYS_DSN;
    else
        dsnLocation = ODBC_ADD_DSN;

    // NOTE: The decimal 2 is an invalid character in all keyword pairs
    // so that is why I used it, as wxString does not deal well with
    // embedded nulls in strings
    setupStr.Printf(wxT("DSN=%s%cDescription=%s%cDefaultDir=%s%c"),dsn,2,description,2,defDir,2);

    // Replace the separator from above with the '\0' separator needed
    // by the SQLConfigDataSource() function
    int k;
    do
    {
        k = setupStr.Find((wxChar)2,true);
        if (k != wxNOT_FOUND)
            setupStr[(UINT)k] = wxT('\0');
    }
    while (k != wxNOT_FOUND);

    result = SQLConfigDataSource((HWND)parent->GetHWND(), dsnLocation,
                                 driverName, setupStr.c_str());

    if ((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
    {
        // check for errors caused by ConfigDSN based functions
        DWORD retcode = 0;
        WORD cb;
        wxChar errMsg[SQL_MAX_MESSAGE_LENGTH];
        errMsg[0] = wxT('\0');

        // This function is only supported in ODBC drivers v3.0 compliant and above
        SQLInstallerError(1,&retcode,errMsg,SQL_MAX_MESSAGE_LENGTH-1,&cb);
        if (retcode)
        {
#ifdef DBDEBUG_CONSOLE
               // When run in console mode, use standard out to display errors.
               cout << errMsg << endl;
               cout << wxT("Press any key to continue...") << endl;
               getchar();
#endif  // DBDEBUG_CONSOLE

#ifdef __WXDEBUG__
               wxLogDebug(errMsg,wxT("ODBC DEBUG MESSAGE"));
#endif  // __WXDEBUG__
        }
    }
    else
       result = TRUE;
#else
    // Using iODBC/unixODBC or some other compiler which does not support the APIs
    // necessary to use this function, so this function is not supported
#ifdef __WXDEBUG__
    wxLogDebug(wxT("wxDbCreateDataSource() not available except under VC++/MSW"),wxT("ODBC DEBUG MESSAGE"));
#endif
    result = FALSE;
#endif  // __VISUALC__

    return result;

}  // wxDbCreateDataSource()
#endif


/********** wxDbGetDataSource() **********/
bool wxDbGetDataSource(HENV henv, wxChar *Dsn, SWORD DsnMaxLength, wxChar *DsDesc,
                       SWORD DsDescMaxLength, UWORD direction)
/*
 * Dsn and DsDesc will contain the data source name and data source
 * description upon return
 */
{
    SWORD cb1,cb2;
    SWORD lengthDsn = (SWORD)(DsnMaxLength*sizeof(wxChar));
    SWORD lengthDsDesc = (SWORD)(DsDescMaxLength*sizeof(wxChar));

    if (SQLDataSources(henv, direction, (SQLTCHAR FAR *) Dsn, lengthDsn, &cb1,
                       (SQLTCHAR FAR *) DsDesc, lengthDsDesc, &cb2) == SQL_SUCCESS)
        return true;
    else
        return false;

}  // wxDbGetDataSource()


// Change this to 0 to remove use of all deprecated functions
#if wxODBC_BACKWARD_COMPATABILITY
/********************************************************************
 ********************************************************************
 *
 * The following functions are all DEPRECATED and are included for
 * backward compatibility reasons only
 *
 ********************************************************************
 ********************************************************************/
bool SqlLog(sqlLog state, const wxChar *filename)
{
    return wxDbSqlLog((enum wxDbSqlLogState)state, filename);
}
/***** DEPRECATED: use wxGetDataSource() *****/
bool GetDataSource(HENV henv, char *Dsn, SWORD DsnMax, char *DsDesc, SWORD DsDescMax,
                         UWORD direction)
{
    return wxDbGetDataSource(henv, Dsn, DsnMax, DsDesc, DsDescMax, direction);
}
/***** DEPRECATED: use wxDbGetConnection() *****/
wxDb WXDLLIMPEXP_ODBC *GetDbConnection(DbStuff *pDbStuff, bool FwdOnlyCursors)
{
    return wxDbGetConnection((wxDbConnectInf *)pDbStuff, FwdOnlyCursors);
}
/***** DEPRECATED: use wxDbFreeConnection() *****/
bool WXDLLIMPEXP_ODBC FreeDbConnection(wxDb *pDb)
{
    return wxDbFreeConnection(pDb);
}
/***** DEPRECATED: use wxDbCloseConnections() *****/
void WXDLLIMPEXP_ODBC CloseDbConnections(void)
{
    wxDbCloseConnections();
}
/***** DEPRECATED: use wxDbConnectionsInUse() *****/
int WXDLLIMPEXP_ODBC NumberDbConnectionsInUse(void)
{
    return wxDbConnectionsInUse();
}
#endif


#endif
 // wxUSE_ODBC
