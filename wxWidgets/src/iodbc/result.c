/*
 *  result.c
 *
 *  $Id: result.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Prepare for getting query result
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

#include        "dlproc.h"

#include	"herr.h"
#include	"henv.h"
#include	"hdbc.h"
#include	"hstmt.h"

#include	"itrace.h"

RETCODE SQL_API 
SQLBindCol (
    HSTMT hstmt,
    UWORD icol,
    SWORD fCType,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * pcbValue)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  switch (fCType)
     {
     case SQL_C_DEFAULT:
     case SQL_C_CHAR:
     case SQL_C_BINARY:
     case SQL_C_BIT:
     case SQL_C_TINYINT:
     case SQL_C_STINYINT:
     case SQL_C_UTINYINT:
     case SQL_C_SHORT:
     case SQL_C_SSHORT:
     case SQL_C_USHORT:
     case SQL_C_LONG:
     case SQL_C_SLONG:
     case SQL_C_ULONG:
     case SQL_C_FLOAT:
     case SQL_C_DOUBLE:
     case SQL_C_DATE:
     case SQL_C_TIME:
     case SQL_C_TIMESTAMP:
       break;

     default:
       PUSHSQLERR (pstmt->herr, en_S1003);
       return SQL_ERROR;
     }

  if (cbValueMax < 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state > en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);
      return SQL_ERROR;
    }

  /* call driver's function */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_BindCol);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_BindCol,
    (pstmt->dhstmt, icol, fCType, rgbValue, cbValueMax, pcbValue))

  return retcode;
}


RETCODE SQL_API 
SQLGetCursorName (
    HSTMT hstmt,
    UCHAR FAR * szCursor,
    SWORD cbCursorMax,
    SWORD FAR * pcbCursor)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  if (cbCursorMax < (SWORD) 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  if (pstmt->state < en_stmt_cursoropen
      && pstmt->cursor_state == en_stmt_cursor_no)
    {
      PUSHSQLERR (pstmt->herr, en_S1015);

      return SQL_ERROR;
    }

  /* call driver's function */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetCursorName);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_GetCursorName,
    (pstmt->dhstmt, szCursor, cbCursorMax, pcbCursor))

  return retcode;
}


RETCODE SQL_API 
SQLRowCount (
    HSTMT hstmt,
    SDWORD FAR * pcrow)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata
      || pstmt->state <= en_stmt_prepared
      || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_RowCount);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_RowCount,
    (pstmt->dhstmt, pcrow))

  return retcode;
}


RETCODE SQL_API 
SQLNumResultCols (
    HSTMT hstmt,
    SWORD FAR * pccol)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;
  SWORD ccol;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state == en_stmt_allocated
	  || pstmt->state >= en_stmt_needdata)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  return SQL_ERROR;
	}
    }
  else if (pstmt->asyn_on != en_NumResultCols)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_NumResultCols);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_NumResultCols,
    (pstmt->dhstmt, &ccol))

  /* state transition */
  if (pstmt->asyn_on == en_NumResultCols)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;

	 case SQL_STILL_EXECUTING:
	 default:
	   break;
	 }
    }

  switch (retcode)
     {
     case SQL_SUCCESS:
     case SQL_SUCCESS_WITH_INFO:
       break;

     case SQL_STILL_EXECUTING:
       ccol = 0;
       pstmt->asyn_on = en_NumResultCols;
       break;

     default:
       ccol = 0;
       break;
     }

  if (pccol)
    {
      *pccol = ccol;
    }

  return retcode;
}


RETCODE SQL_API 
SQLDescribeCol (
    HSTMT hstmt,
    UWORD icol,
    UCHAR FAR * szColName,
    SWORD cbColNameMax,
    SWORD FAR * pcbColName,
    SWORD FAR * pfSqlType,
    UDWORD FAR * pcbColDef,
    SWORD FAR * pibScale,
    SWORD FAR * pfNullable)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT
      || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check arguments */
  if (icol == 0)
    {
      sqlstat = en_S1002;
    }
  else if (cbColNameMax < 0)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->asyn_on == en_stmt_allocated
	  || pstmt->asyn_on >= en_stmt_needdata)
	{
	  sqlstat = en_S1010;
	}
    }
  else if (pstmt->asyn_on != en_DescribeCol)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_DescribeCol);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_DescribeCol,
    (pstmt->dhstmt, icol, szColName, cbColNameMax, pcbColName,
      pfSqlType, pcbColDef, pibScale, pfNullable))

  /* state transition */
  if (pstmt->asyn_on == en_DescribeCol)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_prepared:
     case en_stmt_cursoropen:
     case en_stmt_fetched:
     case en_stmt_xfetched:
       if (retcode == SQL_STILL_EXECUTING)
	 {
	   pstmt->asyn_on = en_DescribeCol;
	 }
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLColAttributes (
    HSTMT hstmt,
    UWORD icol,
    UWORD fDescType,
    PTR rgbDesc,
    SWORD cbDescMax,
    SWORD FAR * pcbDesc,
    SDWORD FAR * pfDesc)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check arguments */
  if (icol == 0 && fDescType != SQL_COLUMN_COUNT)
    {
      sqlstat = en_S1002;
    }
  else if (cbDescMax < 0)
    {
      sqlstat = en_S1090;
    }
  else if (			/* fDescType < SQL_COLATT_OPT_MIN || *//* turnoff warning */
	(fDescType > SQL_COLATT_OPT_MAX
	  && fDescType < SQL_COLUMN_DRIVER_START))
    {
      sqlstat = en_S1091;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->asyn_on == en_stmt_allocated
	  || pstmt->asyn_on >= en_stmt_needdata)
	{
	  sqlstat = en_S1010;
	}
    }
  else if (pstmt->asyn_on != en_ColAttributes)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ColAttributes);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ColAttributes,
    (pstmt->dhstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc))

  /* state transition */
  if (pstmt->asyn_on == en_ColAttributes)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_prepared:
     case en_stmt_cursoropen:
     case en_stmt_fetched:
     case en_stmt_xfetched:
       if (retcode == SQL_STILL_EXECUTING)
	 {
	   pstmt->asyn_on = en_ColAttributes;
	 }
       break;

     default:
       break;
     }

  return retcode;
}
