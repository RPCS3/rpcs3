/*
 *  prepare.c
 *
 *  $Id: prepare.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Prepare a query
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

#include        <unistd.h>

RETCODE SQL_API 
SQLPrepare (
    HSTMT hstmt,
    UCHAR FAR * szSqlStr,
    SDWORD cbSqlStr)
{
  STMT_t FAR *pstmt = (STMT_t *) hstmt;

  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode = SQL_SUCCESS;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      /* not on asyn state */
      switch (pstmt->state)
	 {
	 case en_stmt_fetched:
	 case en_stmt_xfetched:
	   sqlstat = en_24000;
	   break;

	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   sqlstat = en_S1010;
	   break;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_Prepare)
    {
      /* asyn on other */
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  if (szSqlStr == NULL)
    {
      PUSHSQLERR (pstmt->herr, en_S1009);

      return SQL_ERROR;
    }

  if (cbSqlStr < 0 && cbSqlStr != SQL_NTS)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_Prepare);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Prepare,
    (pstmt->dhstmt, szSqlStr, cbSqlStr))

  /* stmt state transition */
  if (pstmt->asyn_on == en_Prepare)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   return retcode;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  switch (retcode)
     {
     case SQL_STILL_EXECUTING:
       pstmt->asyn_on = en_Prepare;
       break;

     case SQL_SUCCESS:
     case SQL_SUCCESS_WITH_INFO:
       pstmt->state = en_stmt_prepared;
       pstmt->prep_state = 1;
       break;

     case SQL_ERROR:
       switch (pstmt->state)
	  {
	  case en_stmt_prepared:
	  case en_stmt_executed:
	    pstmt->state = en_stmt_allocated;
	    pstmt->prep_state = 0;
	    break;

	  default:
	    break;
	  }

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLSetCursorName (
    HSTMT hstmt,
    UCHAR FAR * szCursor,
    SWORD cbCursor)
{
  STMT_t FAR *pstmt = (STMT_t *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;

  RETCODE retcode = SQL_SUCCESS;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  if (szCursor == NULL)
    {
      PUSHSQLERR (pstmt->herr, en_S1009);

      return SQL_ERROR;
    }

  if (cbCursor < 0 && cbCursor != SQL_NTS)
    {
      PUSHSQLERR (pstmt->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on != en_NullProc)
    {
      sqlstat = en_S1010;
    }
  else
    {
      switch (pstmt->state)
	 {
	 case en_stmt_executed:
	 case en_stmt_cursoropen:
	 case en_stmt_fetched:
	 case en_stmt_xfetched:
	   sqlstat = en_24000;
	   break;

	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   sqlstat = en_S1010;
	   break;

	 default:
	   break;
	 }
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetCursorName);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_SetCursorName,
    (pstmt->dhstmt, szCursor, cbCursor))

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      pstmt->cursor_state = en_stmt_cursor_named;
    }

  return retcode;
}


RETCODE SQL_API 
SQLBindParameter (
    HSTMT hstmt,
    UWORD ipar,
    SWORD fParamType,
    SWORD fCType,
    SWORD fSqlType,
    UDWORD cbColDef,
    SWORD ibScale,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * pcbValue)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;

  int sqlstat = en_00000;
  RETCODE retcode = SQL_SUCCESS;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check param */
  if (fSqlType > SQL_TYPE_MAX || 
	(fSqlType < SQL_TYPE_MIN && fSqlType > SQL_TYPE_DRIVER_START))
    /* Note: SQL_TYPE_DRIVER_START is a nagtive number 
     * So, we use ">" */
    {
      sqlstat = en_S1004;
    }
  else if (ipar < 1)
    {
      sqlstat = en_S1093;
    }
  else if ((rgbValue == NULL && pcbValue == NULL)
      && fParamType != SQL_PARAM_OUTPUT)
    {
      sqlstat = en_S1009;
      /* This means, I allow output to nowhere
       * (i.e. * junk output result). But I can't  
       * allow input from nowhere. 
       */
    }
/**********
	else if( cbValueMax < 0L && cbValueMax != SQL_SETPARAM_VALUE_MAX )
	{
		sqlstat = en_S1090;
	}
**********/
  else if (fParamType != SQL_PARAM_INPUT
	&& fParamType != SQL_PARAM_OUTPUT
      && fParamType != SQL_PARAM_INPUT_OUTPUT)
    {
      sqlstat = en_S1105;
    }
  else
    {
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
	   sqlstat = en_S1003;
	   break;
	 }
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      retcode = SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_BindParameter);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_BindParameter,
    (pstmt->dhstmt, ipar, fParamType, fCType, fSqlType, cbColDef,
      ibScale, rgbValue, cbValueMax, pcbValue))

  return retcode;
}


RETCODE SQL_API 
SQLParamOptions (
    HSTMT hstmt,
    UDWORD crow,
    UDWORD FAR * pirow)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  if (crow == (UDWORD) 0UL)
    {
      PUSHSQLERR (pstmt->herr, en_S1107);

      return SQL_ERROR;
    }

  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ParamOptions);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ParamOptions,
    (pstmt->dhstmt, crow, pirow))

  return retcode;
}


RETCODE SQL_API 
SQLSetScrollOptions (
    HSTMT hstmt,
    UWORD fConcurrency,
    SDWORD crowKeyset,
    UWORD crowRowset)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  int sqlstat = en_00000;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  for (;;)
    {
      if (crowRowset == (UWORD) 0)
	{
	  sqlstat = en_S1107;
	  break;
	}

      if (crowKeyset > (SDWORD) 0L && crowKeyset < (SDWORD) crowRowset)
	{
	  sqlstat = en_S1107;
	  break;
	}

      if (crowKeyset < 1)
	{
	  if (crowKeyset != SQL_SCROLL_FORWARD_ONLY
	      && crowKeyset != SQL_SCROLL_STATIC
	      && crowKeyset != SQL_SCROLL_KEYSET_DRIVEN
	      && crowKeyset != SQL_SCROLL_DYNAMIC)
	    {
	      sqlstat = en_S1107;
	      break;
	    }
	}

      if (fConcurrency != SQL_CONCUR_READ_ONLY
	  && fConcurrency != SQL_CONCUR_LOCK
	  && fConcurrency != SQL_CONCUR_ROWVER
	  && fConcurrency != SQL_CONCUR_VALUES)
	{
	  sqlstat = en_S1108;
	  break;
	}

      if (pstmt->state != en_stmt_allocated)
	{
	  sqlstat = en_S1010;
	  break;
	}

      hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetScrollOptions);

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

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_SetScrollOptions,
    (pstmt->dhstmt, fConcurrency, crowKeyset, crowRowset))

  return retcode;
}


RETCODE SQL_API 
SQLSetParam (
    HSTMT hstmt,
    UWORD ipar,
    SWORD fCType,
    SWORD fSqlType,
    UDWORD cbColDef,
    SWORD ibScale,
    PTR rgbValue,
    SDWORD FAR * pcbValue)
{
  return SQLBindParameter (hstmt,
      ipar,
      (SWORD) SQL_PARAM_INPUT_OUTPUT,
      fCType,
      fSqlType,
      cbColDef,
      ibScale,
      rgbValue,
      SQL_SETPARAM_VALUE_MAX,
      pcbValue);
}
