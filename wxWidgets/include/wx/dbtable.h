///////////////////////////////////////////////////////////////////////////////
// Name:        dbtable.h
// Purpose:     Declaration of the wxDbTable class.
// Author:      Doug Card
// Modified by: George Tasker
//              Bart Jourquin
//              Mark Johnson
// Created:     9.96
// RCS-ID:      $Id: dbtable.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1996 Remstar International, Inc.
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

/*
// SYNOPSIS START
// SYNOPSIS STOP
*/

#ifndef DBTABLE_DOT_H
#define DBTABLE_DOT_H

#include "wx/defs.h"

#include "wx/db.h"

#include "wx/variant.h"
#include "wx/dbkeyg.h"

const int   wxDB_ROWID_LEN       = 24;  // 18 is the max, 24 is in case it gets larger
const int   wxDB_DEFAULT_CURSOR  = 0;
const bool  wxDB_QUERY_ONLY      = true;
const bool  wxDB_DISABLE_VIEW    = true;

// Used to indicate end of a variable length list of
// column numbers passed to member functions
const int   wxDB_NO_MORE_COLUMN_NUMBERS = -1;

// The following class is used to define a column of a table.
// The wxDbTable constructor will dynamically allocate as many of
// these as there are columns in the table.  The class derived
// from wxDbTable must initialize these column definitions in it's
// constructor.  These column definitions provide inf. to the
// wxDbTable class which allows it to create a table in the data
// source, exchange data between the data source and the C++
// object, and so on.
class WXDLLIMPEXP_ODBC wxDbColDef
{
public:
    wxChar  ColName[DB_MAX_COLUMN_NAME_LEN+1];  // Column Name
    int     DbDataType;                         // Logical Data Type; e.g. DB_DATA_TYPE_INTEGER
    SWORD   SqlCtype;                           // C data type; e.g. SQL_C_LONG
    void   *PtrDataObj;                         // Address of the data object
    int     SzDataObj;                          // Size, in bytes, of the data object
    bool    KeyField;                           // true if this column is part of the PRIMARY KEY to the table; Date fields should NOT be KeyFields.
    bool    Updateable;                         // Specifies whether this column is updateable
    bool    InsertAllowed;                      // Specifies whether this column should be included in an INSERT statement
    bool    DerivedCol;                         // Specifies whether this column is a derived value
    SQLLEN  CbValue;                            // Internal use only!!!
    bool    Null;                               // NOT FULLY IMPLEMENTED - Allows NULL values in Inserts and Updates

    wxDbColDef();

    bool    Initialize();
};  // wxDbColDef


class WXDLLIMPEXP_ODBC wxDbColDataPtr
{
public:
    void    *PtrDataObj;
    int      SzDataObj;
    SWORD    SqlCtype;
};  // wxDbColDataPtr


// This structure is used when creating secondary indexes.
class WXDLLIMPEXP_ODBC wxDbIdxDef
{
public:
    wxChar  ColName[DB_MAX_COLUMN_NAME_LEN+1];
    bool    Ascending;
};  // wxDbIdxDef


class WXDLLIMPEXP_ODBC wxDbTable
{
private:
    ULONG       tableID;  // Used for debugging.  This can help to match up mismatched constructors/destructors

    // Private member variables
    UDWORD      cursorType;
    bool        insertable;

    // Private member functions
    bool        initialize(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
                       const wxString &qryTblName, bool qryOnly, const wxString &tblPath);
    void        cleanup();

    void        setCbValueForColumn(int columnIndex);
    bool        bindParams(bool forUpdate);  // called by the other 'bind' functions
    bool        bindInsertParams(void);
    bool        bindUpdateParams(void);

    bool        bindCols(HSTMT cursor);
    bool        getRec(UWORD fetchType);
    bool        execDelete(const wxString &pSqlStmt);
    bool        execUpdate(const wxString &pSqlStmt);
    bool        query(int queryType, bool forUpdate, bool distinct, const wxString &pSqlStmt=wxEmptyString);

#if !wxODBC_BACKWARD_COMPATABILITY
// these were public
    // Where, Order By and From clauses
    wxString    where;               // Standard SQL where clause, minus the word WHERE
    wxString    orderBy;             // Standard SQL order by clause, minus the ORDER BY
    wxString    from;                // Allows for joins in a wxDbTable::Query().  Format: ",tbl,tbl..."

    // ODBC Handles
    HENV        henv;           // ODBC Environment handle
    HDBC        hdbc;           // ODBC DB Connection handle
    HSTMT       hstmt;          // ODBC Statement handle
    HSTMT      *hstmtDefault;   // Default cursor
    HSTMT       hstmtInsert;    // ODBC Statement handle used specifically for inserts
    HSTMT       hstmtDelete;    // ODBC Statement handle used specifically for deletes
    HSTMT       hstmtUpdate;    // ODBC Statement handle used specifically for updates
    HSTMT       hstmtInternal;  // ODBC Statement handle used internally only
    HSTMT      *hstmtCount;     // ODBC Statement handle used by Count() function (No binding of columns)

    // Flags
    bool        selectForUpdate;

    // Pointer to the database object this table belongs to
    wxDb       *pDb;

    // Table Inf.
    wxString    tablePath;                                 // needed for dBase tables
    wxString    tableName;                                 // Table name
    wxString    queryTableName;                            // Query Table Name
    UWORD       m_numCols;                               // # of columns in the table
    bool        queryOnly;                                 // Query Only, no inserts, updates or deletes

    // Column Definitions
    wxDbColDef *colDefs;         // Array of wxDbColDef structures
#endif
public:
#if wxODBC_BACKWARD_COMPATABILITY
    // Where, Order By and From clauses
    char       *where;          // Standard SQL where clause, minus the word WHERE
    char       *orderBy;        // Standard SQL order by clause, minus the ORDER BY
    char       *from;           // Allows for joins in a wxDbTable::Query().  Format: ",tbl,tbl..."

    // ODBC Handles
    HENV        henv;           // ODBC Environment handle
    HDBC        hdbc;           // ODBC DB Connection handle
    HSTMT       hstmt;          // ODBC Statement handle
    HSTMT      *hstmtDefault;   // Default cursor
    HSTMT       hstmtInsert;    // ODBC Statement handle used specifically for inserts
    HSTMT       hstmtDelete;    // ODBC Statement handle used specifically for deletes
    HSTMT       hstmtUpdate;    // ODBC Statement handle used specifically for updates
    HSTMT       hstmtInternal;  // ODBC Statement handle used internally only
    HSTMT      *hstmtCount;     // ODBC Statement handle used by Count() function (No binding of columns)

    // Flags
    bool        selectForUpdate;

    // Pointer to the database object this table belongs to
    wxDb       *pDb;

    // Table Inf.
    char        tablePath[wxDB_PATH_MAX];                  // needed for dBase tables
    char        tableName[DB_MAX_TABLE_NAME_LEN+1];        // Table name
    char        queryTableName[DB_MAX_TABLE_NAME_LEN+1];   // Query Table Name
    UWORD       m_numCols;                               // # of columns in the table
    bool        queryOnly;                                 // Query Only, no inserts, updates or deletes

    // Column Definitions
    wxDbColDef *colDefs;         // Array of wxDbColDef structures
#endif
    // Public member functions
    wxDbTable(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
              const wxString &qryTblName=wxEmptyString, bool qryOnly = !wxDB_QUERY_ONLY,
              const wxString &tblPath=wxEmptyString);

#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED(
        wxDbTable(wxDb *pwxDb, const wxString &tblName, const UWORD numColumns,
                  const wxChar *qryTblName, bool qryOnly,
                  const wxString &tblPath)
    );
#endif // WXWIN_COMPATIBILITY_2_4

    virtual ~wxDbTable();

    bool            Open(bool checkPrivileges=false, bool checkTableExists=true);
    bool            CreateTable(bool attemptDrop=true);
    bool            DropTable(void);
    bool            CreateIndex(const wxString &indexName, bool unique, UWORD numIndexColumns,
                                wxDbIdxDef *pIndexDefs, bool attemptDrop=true);
    bool            DropIndex(const wxString &indexName);

    // Accessors

    // The member variables returned by these accessors are all
    // set when the wxDbTable instance is created and cannot be
    // changed, hence there is no corresponding SetXxxx function
    wxDb           *GetDb()              { return pDb; }
    const wxString &GetTableName()       { return tableName; }
    const wxString &GetQueryTableName()  { return queryTableName; }
    const wxString &GetTablePath()       { return tablePath; }

    UWORD           GetNumberOfColumns() { return m_numCols; }  // number of "defined" columns for this wxDbTable instance

    const wxString &GetFromClause()      { return from; }
    const wxString &GetOrderByClause()   { return orderBy; }
    const wxString &GetWhereClause()     { return where; }

    bool            IsQueryOnly()        { return queryOnly; }
#if wxODBC_BACKWARD_COMPATABILITY
    void            SetFromClause(const char *From) { from = (char *)From; }
    void            SetOrderByClause(const char *OrderBy) { orderBy = (char *)OrderBy; }
    void            SetWhereClause(const char *Where) { where = (char *)Where; }
#else
    void            SetFromClause(const wxString &From) { from = From; }
    void            SetOrderByClause(const wxString &OrderBy) { orderBy = OrderBy; }
    bool            SetOrderByColNums(UWORD first, ...);
    void            SetWhereClause(const wxString &Where) { where = Where; }
    void            From(const wxString &From) { from = From; }
    void            OrderBy(const wxString &OrderBy) { orderBy = OrderBy; }
    void            Where(const wxString &Where) { where = Where; }
    const wxString &Where()   { return where; }
    const wxString &OrderBy() { return orderBy; }
    const wxString &From()    { return from; }
#endif
    int             Insert(void);
    bool            Update(void);
    bool            Update(const wxString &pSqlStmt);
    bool            UpdateWhere(const wxString &pWhereClause);
    bool            Delete(void);
    bool            DeleteWhere(const wxString &pWhereClause);
    bool            DeleteMatching(void);
    virtual bool    Query(bool forUpdate = false, bool distinct = false);
    bool            QueryBySqlStmt(const wxString &pSqlStmt);
    bool            QueryMatching(bool forUpdate = false, bool distinct = false);
    bool            QueryOnKeyFields(bool forUpdate = false, bool distinct = false);
    bool            Refresh(void);
    bool            GetNext(void)   { return(getRec(SQL_FETCH_NEXT));  }
    bool            operator++(int) { return(getRec(SQL_FETCH_NEXT));  }

    /***** These four functions only work with wxDb instances that are defined  *****
     ***** as not being FwdOnlyCursors                                          *****/
    bool            GetPrev(void);
    bool            operator--(int);
    bool            GetFirst(void);
    bool            GetLast(void);

    bool            IsCursorClosedOnCommit(void);
    UWORD           GetRowNum(void);

    void            BuildSelectStmt(wxString &pSqlStmt, int typeOfSelect, bool distinct);
    void            BuildSelectStmt(wxChar *pSqlStmt, int typeOfSelect, bool distinct);

    void            BuildDeleteStmt(wxString &pSqlStmt, int typeOfDel, const wxString &pWhereClause=wxEmptyString);
    void            BuildDeleteStmt(wxChar *pSqlStmt, int typeOfDel, const wxString &pWhereClause=wxEmptyString);

    void            BuildUpdateStmt(wxString &pSqlStmt, int typeOfUpdate, const wxString &pWhereClause=wxEmptyString);
    void            BuildUpdateStmt(wxChar *pSqlStmt, int typeOfUpdate, const wxString &pWhereClause=wxEmptyString);

    void            BuildWhereClause(wxString &pWhereClause, int typeOfWhere, const wxString &qualTableName=wxEmptyString, bool useLikeComparison=false);
    void            BuildWhereClause(wxChar *pWhereClause, int typeOfWhere, const wxString &qualTableName=wxEmptyString, bool useLikeComparison=false);

#if wxODBC_BACKWARD_COMPATABILITY
// The following member functions are deprecated.  You should use the BuildXxxxxStmt functions (above)
    void            GetSelectStmt(char *pSqlStmt, int typeOfSelect, bool distinct)
                           { BuildSelectStmt(pSqlStmt,typeOfSelect,distinct); }
    void            GetDeleteStmt(char *pSqlStmt, int typeOfDel, const char *pWhereClause = NULL)
                           { BuildDeleteStmt(pSqlStmt,typeOfDel,pWhereClause); }
    void            GetUpdateStmt(char *pSqlStmt, int typeOfUpdate, const char *pWhereClause = NULL)
                           { BuildUpdateStmt(pSqlStmt,typeOfUpdate,pWhereClause); }
    void            GetWhereClause(char *pWhereClause, int typeOfWhere,
                                   const char *qualTableName = NULL, bool useLikeComparison=false)
                           { BuildWhereClause(pWhereClause,typeOfWhere,qualTableName,useLikeComparison); }
#endif
    bool            CanSelectForUpdate(void);
#if wxODBC_BACKWARD_COMPATABILITY
    bool            CanUpdByROWID(void) { return CanUpdateByRowID(); };
#endif
    bool            CanUpdateByROWID(void);
    void            ClearMemberVar(UWORD colNumber, bool setToNull=false);
    void            ClearMemberVars(bool setToNull=false);
    bool            SetQueryTimeout(UDWORD nSeconds);

    wxDbColDef     *GetColDefs() { return colDefs; }
    bool            SetColDefs(UWORD index, const wxString &fieldName, int dataType,
                               void *pData, SWORD cType,
                               int size, bool keyField = false, bool updateable = true,
                               bool insertAllowed = true, bool derivedColumn = false);
    wxDbColDataPtr *SetColDefs(wxDbColInf *colInfs, UWORD numCols);

    bool            CloseCursor(HSTMT cursor);
    bool            DeleteCursor(HSTMT *hstmtDel);
    void            SetCursor(HSTMT *hstmtActivate = (void **) wxDB_DEFAULT_CURSOR);
    HSTMT           GetCursor(void) { return(hstmt); }
    HSTMT          *GetNewCursor(bool setCursor = false, bool bindColumns = true);
#if wxODBC_BACKWARD_COMPATABILITY
// The following member function is deprecated.  You should use the GetNewCursor
    HSTMT          *NewCursor(bool setCursor = false, bool bindColumns = true) {  return GetNewCursor(setCursor,bindColumns); }
#endif

    ULONG           Count(const wxString &args=wxT("*"));
    int             DB_STATUS(void) { return(pDb->DB_STATUS); }

    bool            IsColNull(UWORD colNumber) const;
    bool            SetColNull(UWORD colNumber, bool set=true);
    bool            SetColNull(const wxString &colName, bool set=true);
#if wxODBC_BACKWARD_COMPATABILITY
// The following member functions are deprecated.  You should use the SetColNull()
    bool            SetNull(int colNumber, bool set=true) { return (SetNull(colNumber,set)); }
    bool            SetNull(const char *colName, bool set=true) { return (SetNull(colName,set)); }
#endif
#ifdef __WXDEBUG__
    ULONG           GetTableID() { return tableID; }
#endif

//TODO: Need to Document
    typedef     enum  { WX_ROW_MODE_QUERY , WX_ROW_MODE_INDIVIDUAL } rowmode_t;
    virtual     void         SetRowMode(const rowmode_t rowmode);
#if wxODBC_BACKWARD_COMPATABILITY
    virtual     wxVariant    GetCol(const int colNumber) const { return GetColumn(colNumber); };
    virtual     void         SetCol(const int colNumber, const wxVariant value)  { return SetColumn(colNumber, value); };
#endif
    virtual     wxVariant    GetColumn(const int colNumber) const ;
    virtual     void         SetColumn(const int colNumber, const wxVariant value);
    virtual     GenericKey   GetKey(void);
    virtual     void         SetKey(const GenericKey &key);

    private:
        HSTMT      *m_hstmtGridQuery;
        rowmode_t   m_rowmode;
        size_t      m_keysize;

//      typedef enum {unmodified=0, UpdatePending, InsertPending } recStatus;

//      recStatus  get_ModifiedStatus() { return m_recstatus; }

//      void modify() {
//          if (m_recstatus==unmodified)
//              m_recstatus=UpdatePending;
//      }
//  protected:
//      void insertify() {m_recstatus=InsertPending; }
//      void unmodify() {m_recstatus=unmodified; }
//      recStatus m_recstatus;
//TODO: Need to Document
};  // wxDbTable


// Change this to 0 to remove use of all deprecated functions
#if wxODBC_BACKWARD_COMPATABILITY
//#################################################################################
//############### DEPRECATED functions for backward compatibility #################
//#################################################################################

// Backward compability.  These will eventually go away
typedef wxDbTable       wxTable;
typedef wxDbIdxDef      wxIdxDef;
typedef wxDbIdxDef      CidxDef;
typedef wxDbColDef      wxColDef;
typedef wxDbColDef      CcolDef;
typedef wxDbColDataPtr  wxColDataPtr;
typedef wxDbColDataPtr  CcolDataPtr;

const int   ROWID           = wxDB_ROWID_LEN;
const int   DEFAULT_CURSOR  = wxDB_DEFAULT_CURSOR;
const bool  QUERY_ONLY      = wxDB_QUERY_ONLY;
const bool  DISABLE_VIEW    = wxDB_DISABLE_VIEW;
#endif

#endif
