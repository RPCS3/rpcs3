/*
 *  catalog.c
 *
 *  $Id: catalog.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Catalog functions
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

#include	"config.h"

#include	"isql.h"
#include	"isqlext.h"

#include	"dlproc.h"

#include	"herr.h"
#include	"henv.h"
#include	"hdbc.h"
#include	"hstmt.h"

#include	"itrace.h"

#ifndef NULL
#define NULL 0
#endif

static RETCODE 
_iodbcdm_cata_state_ok (
    HSTMT hstmt,
    int fidx)
/* check state for executing catalog functions */
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  int sqlstat = en_00000;

  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   sqlstat = en_S1010;
	   break;

	 case en_stmt_fetched:
	 case en_stmt_xfetched:
	   sqlstat = en_24000;
	   break;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != fidx)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  return SQL_SUCCESS;
}


static RETCODE 
_iodbcdm_cata_state_tr (
    HSTMT hstmt,
    int fidx,
    RETCODE result)
/* state transition for catalog function */
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  DBC_t FAR *pdbc;

  pdbc = (DBC_t FAR *) (pstmt->hdbc);

  if (pstmt->asyn_on == fidx)
    {
      switch (result)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return result;
	 }
    }

  if (pstmt->state <= en_stmt_executed)
    {
      switch (result)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	   pstmt->state = en_stmt_cursoropen;
	   break;

	 case SQL_ERROR:
	   pstmt->state = en_stmt_allocated;
	   pstmt->prep_state = 0;
	   break;

	 case SQL_STILL_EXECUTING:
	   pstmt->asyn_on = fidx;
	   break;

	 default:
	   break;
	 }
    }

  return result;
}


RETCODE SQL_API 
SQLGetTypeInfo (
    HSTMT hstmt,
    SWORD fSqlType)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  int sqlstat = en_00000;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if (fSqlType > SQL_TYPE_MAX)
	{
	  sqlstat = en_S1004;
	  break;
	}

      /* Note: SQL_TYPE_DRIVER_START is a negative number So, we use ">" */
      if (fSqlType < SQL_TYPE_MIN && fSqlType > SQL_TYPE_DRIVER_START)
	{
	  sqlstat = en_S1004;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_GetTypeInfo);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetTypeInfo);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;
      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER ( pstmt->hdbc, retcode, hproc, en_GetTypeInfo, 
    (pstmt->dhstmt, fSqlType))

  return _iodbcdm_cata_state_tr (hstmt, en_GetTypeInfo, retcode);
}


RETCODE SQL_API 
SQLSpecialColumns (
    HSTMT hstmt,
    UWORD fColType,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName,
    UWORD fScope,
    UWORD fNullable)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      if (fColType != SQL_BEST_ROWID && fColType != SQL_ROWVER)
	{
	  sqlstat = en_S1097;
	  break;
	}

      if (fScope != SQL_SCOPE_CURROW
	  && fScope != SQL_SCOPE_TRANSACTION
	  && fScope != SQL_SCOPE_SESSION)
	{
	  sqlstat = en_S1098;
	  break;
	}

      if (fNullable != SQL_NO_NULLS && fNullable != SQL_NULLABLE)
	{
	  sqlstat = en_S1099;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_SpecialColumns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_SpecialColumns);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;
      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_SpecialColumns, (
	  pstmt->dhstmt,
	  fColType,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName,
	  fScope,
	  fNullable))

  return _iodbcdm_cata_state_tr (hstmt, en_SpecialColumns, retcode);
}


RETCODE SQL_API 
SQLStatistics (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName,
    UWORD fUnique,
    UWORD fAccuracy)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      if (fUnique != SQL_INDEX_UNIQUE && fUnique != SQL_INDEX_ALL)
	{
	  sqlstat = en_S1100;
	  break;
	}

      if (fAccuracy != SQL_ENSURE && fAccuracy != SQL_QUICK)
	{
	  sqlstat = en_S1101;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_Statistics);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Statistics);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Statistics, (
	  pstmt->dhstmt,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName,
	  fUnique,
	  fAccuracy))

  return _iodbcdm_cata_state_tr (hstmt, en_Statistics, retcode);
}


RETCODE SQL_API 
SQLTables (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName,
    UCHAR FAR * szTableType,
    SWORD cbTableType)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbTableType < 0 && cbTableType != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_Tables);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Tables);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Tables, (
	  pstmt->dhstmt,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName,
	  szTableType,
	  cbTableType))

  return _iodbcdm_cata_state_tr (hstmt, en_Tables, retcode);
}


RETCODE SQL_API 
SQLColumnPrivileges (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName,
    UCHAR FAR * szColumnName,
    SWORD cbColumnName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_ColumnPrivileges);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_ColumnPrivileges);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ColumnPrivileges, (
	  pstmt->dhstmt,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName,
	  szColumnName,
	  cbColumnName))

  return _iodbcdm_cata_state_tr (hstmt, en_ColumnPrivileges, retcode);
}


RETCODE SQL_API 
SQLColumns (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName,
    UCHAR FAR * szColumnName,
    SWORD cbColumnName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_Columns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Columns);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Columns, (
	  pstmt->dhstmt,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName,
	  szColumnName,
	  cbColumnName))

  return _iodbcdm_cata_state_tr (hstmt, en_Columns, retcode);
}


RETCODE SQL_API 
SQLForeignKeys (
    HSTMT hstmt,
    UCHAR FAR * szPkTableQualifier,
    SWORD cbPkTableQualifier,
    UCHAR FAR * szPkTableOwner,
    SWORD cbPkTableOwner,
    UCHAR FAR * szPkTableName,
    SWORD cbPkTableName,
    UCHAR FAR * szFkTableQualifier,
    SWORD cbFkTableQualifier,
    UCHAR FAR * szFkTableOwner,
    SWORD cbFkTableOwner,
    UCHAR FAR * szFkTableName,
    SWORD cbFkTableName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbPkTableQualifier < 0 && cbPkTableQualifier != SQL_NTS)
	  || (cbPkTableOwner < 0 && cbPkTableOwner != SQL_NTS)
	  || (cbPkTableName < 0 && cbPkTableName != SQL_NTS)
	  || (cbFkTableQualifier < 0 && cbFkTableQualifier != SQL_NTS)
	  || (cbFkTableOwner < 0 && cbFkTableOwner != SQL_NTS)
	  || (cbFkTableName < 0 && cbFkTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_ForeignKeys);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_ForeignKeys);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ForeignKeys, (
	  pstmt->dhstmt,
	  szPkTableQualifier,
	  cbPkTableQualifier,
	  szPkTableOwner,
	  cbPkTableOwner,
	  szPkTableName,
	  cbPkTableName,
	  szFkTableQualifier,
	  cbFkTableQualifier,
	  szFkTableOwner,
	  cbFkTableOwner,
	  szFkTableName,
	  cbFkTableName))

  return _iodbcdm_cata_state_tr (hstmt, en_ForeignKeys, retcode);
}


RETCODE SQL_API 
SQLPrimaryKeys (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_PrimaryKeys);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_PrimaryKeys);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_PrimaryKeys, (
	  pstmt->dhstmt,
	  szTableQualifier,
	  cbTableQualifier,
	  szTableOwner,
	  cbTableOwner,
	  szTableName,
	  cbTableName))

  return _iodbcdm_cata_state_tr (hstmt, en_PrimaryKeys, retcode);
}


RETCODE SQL_API 
SQLProcedureColumns (
    HSTMT hstmt,
    UCHAR FAR * szProcQualifier,
    SWORD cbProcQualifier,
    UCHAR FAR * szProcOwner,
    SWORD cbProcOwner,
    UCHAR FAR * szProcName,
    SWORD cbProcName,
    UCHAR FAR * szColumnName,
    SWORD cbColumnName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbProcQualifier < 0 && cbProcQualifier != SQL_NTS)
	  || (cbProcOwner < 0 && cbProcOwner != SQL_NTS)
	  || (cbProcName < 0 && cbProcName != SQL_NTS)
	  || (cbColumnName < 0 && cbColumnName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_ProcedureColumns);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_ProcedureColumns);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ProcedureColumns, (
	  pstmt->dhstmt,
	  szProcQualifier,
	  cbProcQualifier,
	  szProcOwner,
	  cbProcOwner,
	  szProcName,
	  cbProcName,
	  szColumnName,
	  cbColumnName))

  return _iodbcdm_cata_state_tr (hstmt, en_ProcedureColumns, retcode);
}


RETCODE SQL_API 
SQLProcedures (
    HSTMT hstmt,
    UCHAR FAR * szProcQualifier,
    SWORD cbProcQualifier,
    UCHAR FAR * szProcOwner,
    SWORD cbProcOwner,
    UCHAR FAR * szProcName,
    SWORD cbProcName)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbProcQualifier < 0 && cbProcQualifier != SQL_NTS)
	  || (cbProcOwner < 0 && cbProcOwner != SQL_NTS)
	  || (cbProcName < 0 && cbProcName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_Procedures);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Procedures);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Procedures, (
	  pstmt->dhstmt,
	  szProcQualifier,
	  cbProcQualifier,
	  szProcOwner,
	  cbProcOwner,
	  szProcName,
	  cbProcName))

  return _iodbcdm_cata_state_tr (hstmt, en_Procedures, retcode);
}


RETCODE SQL_API 
SQLTablePrivileges (
    HSTMT hstmt,
    UCHAR FAR * szTableQualifier,
    SWORD cbTableQualifier,
    UCHAR FAR * szTableOwner,
    SWORD cbTableOwner,
    UCHAR FAR * szTableName,
    SWORD cbTableName)
{

  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if ((cbTableQualifier < 0 && cbTableQualifier != SQL_NTS)
	  || (cbTableOwner < 0 && cbTableOwner != SQL_NTS)
	  || (cbTableName < 0 && cbTableName != SQL_NTS))
	{
	  sqlstat = en_S1090;
	  break;
	}

      retcode = _iodbcdm_cata_state_ok (hstmt, en_TablePrivileges);

      if (retcode != SQL_SUCCESS)
	{
	  return SQL_ERROR;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_TablePrivileges);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	  break;
	}

      sqlstat = en_00000;

      if (1)			/* turn off solaris warning message */
	break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_TablePrivileges,
    (pstmt->dhstmt, szTableQualifier, cbTableQualifier, szTableOwner,
	cbTableOwner, szTableName, cbTableName))

  return _iodbcdm_cata_state_tr (hstmt, en_TablePrivileges, retcode);
}
