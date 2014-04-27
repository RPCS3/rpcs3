/*
 *  isql.h
 *
 *  $Id: isql.h 35518 2005-09-16 11:22:35Z JS $
 *
 *  iODBC defines
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _ISQL_H
#define _ISQL_H

#if defined(WIN32)
#define SQL_API                 __stdcall

#ifndef FAR
#define FAR
#endif

#elif defined(__OS2__)
#define SQL_API                 _Optlink

#ifndef FAR
#define FAR
#define EXPORT
#define CALLBACK
#endif

#else
#define FAR
#define EXPORT
#define CALLBACK
#define SQL_API                 EXPORT CALLBACK

#ifndef __EMX__
typedef void *HWND;
#endif
#endif

typedef void *SQLHWND;

typedef unsigned char UCHAR;
typedef long int SDWORD;
typedef short int SWORD;
typedef unsigned long int UDWORD;
typedef unsigned short int UWORD;
typedef long SQLINTEGER;
typedef UDWORD SQLUINTEGER;

typedef void FAR *PTR;
typedef void FAR *HENV;
typedef void FAR *HDBC;
typedef void FAR *HSTMT;

typedef signed short RETCODE;
#define SQLRETURN RETCODE


#define ODBCVER                 0x0250

#define SQL_MAX_MESSAGE_LENGTH  512
#define SQL_MAX_DSN_LENGTH      32

/*
 *  Function return codes
 */
#define SQL_INVALID_HANDLE      (-2)
#define SQL_ERROR               (-1)
#define SQL_SUCCESS             0
#define SQL_SUCCESS_WITH_INFO   1
#define SQL_NO_DATA_FOUND       100

/*
 *  Standard SQL datatypes, using ANSI type numbering
 */
#define SQL_CHAR                1
#define SQL_NUMERIC             2
#define SQL_DECIMAL             3
#define SQL_INTEGER             4
#define SQL_SMALLINT            5
#define SQL_FLOAT               6
#define SQL_REAL                7
#define SQL_DOUBLE              8
#define SQL_VARCHAR             12

#define SQL_TYPE_MIN            SQL_CHAR
#define SQL_TYPE_NULL           0
#define SQL_TYPE_MAX            SQL_VARCHAR

/*
 *  C datatype to SQL datatype mapping
 */
#define SQL_C_CHAR              SQL_CHAR
#define SQL_C_LONG              SQL_INTEGER
#define SQL_C_SHORT             SQL_SMALLINT
#define SQL_C_FLOAT             SQL_REAL
#define SQL_C_DOUBLE            SQL_DOUBLE
#define SQL_C_DEFAULT           99

/*
 *  NULL status constants.
 */
#define SQL_NO_NULLS            0
#define SQL_NULLABLE            1
#define SQL_NULLABLE_UNKNOWN    2

/*
 *  Special length values
 */
#define SQL_NULL_DATA           (-1)
#define SQL_DATA_AT_EXEC        (-2)
#define SQL_NTS                 (-3)

/*
 *  SQLFreeStmt
 */
#define SQL_CLOSE               0
#define SQL_DROP                1
#define SQL_UNBIND              2
#define SQL_RESET_PARAMS        3

/*
 *  SQLTransact
 */
#define SQL_COMMIT              0
#define SQL_ROLLBACK            1

/*
 *  SQLColAttributes
 */
#define SQL_COLUMN_COUNT                0
#define SQL_COLUMN_NAME                 1
#define SQL_COLUMN_TYPE                 2
#define SQL_COLUMN_LENGTH               3
#define SQL_COLUMN_PRECISION            4
#define SQL_COLUMN_SCALE                5
#define SQL_COLUMN_DISPLAY_SIZE         6
#define SQL_COLUMN_NULLABLE             7
#define SQL_COLUMN_UNSIGNED             8
#define SQL_COLUMN_MONEY                9
#define SQL_COLUMN_UPDATABLE            10
#define SQL_COLUMN_AUTO_INCREMENT       11
#define SQL_COLUMN_CASE_SENSITIVE       12
#define SQL_COLUMN_SEARCHABLE           13
#define SQL_COLUMN_TYPE_NAME            14
#define SQL_COLUMN_TABLE_NAME           15
#define SQL_COLUMN_OWNER_NAME           16
#define SQL_COLUMN_QUALIFIER_NAME       17
#define SQL_COLUMN_LABEL                18

#define SQL_COLATT_OPT_MAX      SQL_COLUMN_LABEL
#define SQL_COLATT_OPT_MIN      SQL_COLUMN_COUNT
#define SQL_COLUMN_DRIVER_START 1000

/*
 *  SQLColAttributes : SQL_COLUMN_UPDATABLE
 */
#define SQL_ATTR_READONLY           0
#define SQL_ATTR_WRITE              1
#define SQL_ATTR_READWRITE_UNKNOWN  2

/*
 *  SQLColAttributes : SQL_COLUMN_SEARCHABLE
 */
#define SQL_UNSEARCHABLE        0
#define SQL_LIKE_ONLY           1
#define SQL_ALL_EXCEPT_LIKE     2
#define SQL_SEARCHABLE          3

/*
 *  NULL Handles
 */
#define SQL_NULL_HENV           0
#define SQL_NULL_HDBC           0
#define SQL_NULL_HSTMT          0


/*
 *  Function Prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif

  RETCODE SQL_API SQLAllocConnect (HENV henv, HDBC FAR * phdbc);
  RETCODE SQL_API SQLAllocEnv (HENV FAR * phenv);
  RETCODE SQL_API SQLAllocStmt (HDBC hdbc, HSTMT FAR * phstmt);
  RETCODE SQL_API SQLBindCol (HSTMT hstmt, UWORD icol, SWORD fCType,
      PTR rgbValue, SDWORD cbValueMax, SDWORD FAR * pcbValue);
  RETCODE SQL_API SQLCancel (HSTMT hstmt);
  RETCODE SQL_API SQLColAttributes (HSTMT hstmt, UWORD icol, UWORD fDescType,
      PTR rgbDesc, SWORD cbDescMax, SWORD FAR * pcbDesc, SDWORD FAR * pfDesc);
  RETCODE SQL_API SQLConnect (HDBC hdbc, UCHAR FAR * szDSN, SWORD cbDSN,
      UCHAR FAR * szUID, SWORD cbUID, UCHAR FAR * szAuthStr, SWORD cbAuthStr);
  RETCODE SQL_API SQLDescribeCol (HSTMT hstmt, UWORD icol,
      UCHAR FAR * szColName, SWORD cbColNameMax, SWORD FAR * pcbColName,
      SWORD FAR * pfSqlType, UDWORD FAR * pcbColDef, SWORD FAR * pibScale,
      SWORD FAR * pfNullable);
  RETCODE SQL_API SQLDisconnect (HDBC hdbc);
  RETCODE SQL_API SQLError (HENV henv, HDBC hdbc, HSTMT hstmt,
      UCHAR FAR * szSqlState, SDWORD FAR * pfNativeError, UCHAR FAR * szErrorMsg,
      SWORD cbErrorMsgMax, SWORD FAR * pcbErrorMsg);
  RETCODE SQL_API SQLExecDirect (HSTMT hstmt, UCHAR FAR * szSqlStr,
      SDWORD cbSqlStr);
  RETCODE SQL_API SQLExecute (HSTMT hstmt);
  RETCODE SQL_API SQLFetch (HSTMT hstmt);
  RETCODE SQL_API SQLFreeConnect (HDBC hdbc);
  RETCODE SQL_API SQLFreeEnv (HENV henv);
  RETCODE SQL_API SQLFreeStmt (HSTMT hstmt, UWORD fOption);
  RETCODE SQL_API SQLGetCursorName (HSTMT hstmt, UCHAR FAR * szCursor,
      SWORD cbCursorMax, SWORD FAR * pcbCursor);
  RETCODE SQL_API SQLNumResultCols (HSTMT hstmt, SWORD FAR * pccol);
  RETCODE SQL_API SQLPrepare (HSTMT hstmt, UCHAR FAR * szSqlStr,
      SDWORD cbSqlStr);
  RETCODE SQL_API SQLRowCount (HSTMT hstmt, SDWORD FAR * pcrow);
  RETCODE SQL_API SQLSetCursorName (HSTMT hstmt, UCHAR FAR * szCursor,
      SWORD cbCursor);
  RETCODE SQL_API SQLTransact (HENV henv, HDBC hdbc, UWORD fType);

/*
 *  Deprecated ODBC 1.0 function - Use SQLBindParameter
 */
  RETCODE SQL_API SQLSetParam (HSTMT hstmt, UWORD ipar, SWORD fCType,
      SWORD fSqlType, UDWORD cbColDef, SWORD ibScale, PTR rgbValue,
      SDWORD FAR * pcbValue);

#ifdef __cplusplus
}
#endif

#endif
