/*
 *  fetch.c
 *
 *  $Id: fetch.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Fetch query result
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
SQLFetch (HSTMT hstmt)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	 case en_stmt_xfetched:
	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   PUSHSQLERR (pstmt->herr, en_S1010);
	   return SQL_ERROR;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_Fetch)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);
      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_Fetch);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Fetch,
    (pstmt->dhstmt))

  /* state transition */
  if (pstmt->asyn_on == en_Fetch)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_NO_DATA_FOUND:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_cursoropen:
     case en_stmt_fetched:
       switch (retcode)
	  {
	  case SQL_SUCCESS:
	  case SQL_SUCCESS_WITH_INFO:
	    pstmt->state = en_stmt_fetched;
	    pstmt->cursor_state = en_stmt_cursor_fetched;
	    break;

	  case SQL_NO_DATA_FOUND:
	    if (pstmt->prep_state)
	      {
		pstmt->state = en_stmt_prepared;
	      }
	    else
	      {

		pstmt->state = en_stmt_allocated;
	      }
	    pstmt->cursor_state = en_stmt_cursor_no;
	    break;

	  case SQL_STILL_EXECUTING:
	    pstmt->asyn_on = en_Fetch;
	    break;

	  default:
	    break;
	  }
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLExtendedFetch (
    HSTMT hstmt,
    UWORD fFetchType,
    SDWORD irow,
    UDWORD FAR * pcrow,
    UWORD FAR * rgfRowStatus)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check fetch type */
  if (fFetchType < SQL_FETCH_NEXT || fFetchType > SQL_FETCH_BOOKMARK)
    {
      /* Unlike MS driver manager(i.e. DM),
       * we don't check driver's ODBC version 
       * against SQL_FETCH_RESUME (only 1.0)
       * and SQL_FETCH_BOOKMARK (only 2.0).
       */
      PUSHSQLERR (pstmt->herr, en_S1106);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	 case en_stmt_fetched:
	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   PUSHSQLERR (pstmt->herr, en_S1010);
	   return SQL_ERROR;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_ExtendedFetch)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);
      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ExtendedFetch);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_ExtendedFetch,
    (pstmt->dhstmt, fFetchType, irow, pcrow, rgfRowStatus))

  /* state transition */
  if (pstmt->asyn_on == en_ExtendedFetch)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_NO_DATA_FOUND:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_cursoropen:
     case en_stmt_xfetched:
       switch (retcode)
	  {
	  case SQL_SUCCESS:
	  case SQL_SUCCESS_WITH_INFO:
	  case SQL_NO_DATA_FOUND:
	    pstmt->state = en_stmt_xfetched;
	    pstmt->cursor_state = en_stmt_cursor_xfetched;
	    break;

	  case SQL_STILL_EXECUTING:
	    pstmt->asyn_on = en_ExtendedFetch;
	    break;

	  default:
	    break;
	  }
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLGetData (
    HSTMT hstmt,
    UWORD icol,
    SWORD fCType,
    PTR rgbValue,
    SDWORD cbValueMax,
    SDWORD FAR * pcbValue)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  if (rgbValue == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbValueMax < 0)
    {
      sqlstat = en_S1090;
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
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   sqlstat = en_S1010;
	   break;

	 case en_stmt_executed:
	 case en_stmt_cursoropen:
	   sqlstat = en_24000;
	   break;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_GetData)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_GetData,
    (pstmt->dhstmt, icol, fCType, rgbValue, cbValueMax, pcbValue))

  /* state transition */
  if (pstmt->asyn_on == en_GetData)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_NO_DATA_FOUND:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_fetched:
     case en_stmt_xfetched:
       if (retcode == SQL_STILL_EXECUTING)
	 {
	   pstmt->asyn_on = en_GetData;
	   break;
	 }
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLMoreResults (HSTMT hstmt)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT
      || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	   return SQL_NO_DATA_FOUND;

	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   PUSHSQLERR (pstmt->herr, en_S1010);
	   return SQL_ERROR;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_MoreResults)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_MoreResults);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_MoreResults,
    (pstmt->dhstmt))

  /* state transition */
  if (pstmt->asyn_on == en_MoreResults)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_NO_DATA_FOUND:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  switch (pstmt->state)
     {
     case en_stmt_allocated:
     case en_stmt_prepared:
       /* driver should return SQL_NO_DATA_FOUND */
       break;

     case en_stmt_executed:
       if (retcode == SQL_NO_DATA_FOUND)
	 {
	   if (pstmt->prep_state)
	     {
	       pstmt->state = en_stmt_prepared;
	     }
	   else
	     {
	       pstmt->state = en_stmt_allocated;
	     }
	 }
       else if (retcode == SQL_STILL_EXECUTING)
	 {
	   pstmt->asyn_on = en_MoreResults;
	 }
       break;

     case en_stmt_cursoropen:
     case en_stmt_fetched:
     case en_stmt_xfetched:
       if (retcode == SQL_SUCCESS)
	 {
	   break;
	 }
       else if (retcode == SQL_NO_DATA_FOUND)
	 {
	   if (pstmt->prep_state)
	     {
	       pstmt->state = en_stmt_prepared;
	     }
	   else
	     {
	       pstmt->state = en_stmt_allocated;
	     }
	 }
       else if (retcode == SQL_STILL_EXECUTING)
	 {
	   pstmt->asyn_on = en_MoreResults;
	 }
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLSetPos (
    HSTMT hstmt,
    UWORD irow,
    UWORD fOption,
    UWORD fLock)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;
  int sqlstat = en_00000;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument value */
  if (fOption > SQL_ADD || fLock > SQL_LOCK_UNLOCK)
    {
      PUSHSQLERR (pstmt->herr, en_S1009);
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	 case en_stmt_fetched:
	 case en_stmt_needdata:
	 case en_stmt_mustput:
	 case en_stmt_canput:
	   sqlstat = en_S1010;
	   break;

	 case en_stmt_executed:
	 case en_stmt_cursoropen:
	   sqlstat = en_24000;
	   break;

	 default:
	   break;
	 }
    }
  else if (pstmt->asyn_on != en_SetPos)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetPos);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_SetPos,
    (pstmt->dhstmt, irow, fOption, fLock))

  /* state transition */
  if (pstmt->asyn_on == en_SetPos)
    {
      switch (retcode)
	 {
	 case SQL_SUCCESS:
	 case SQL_SUCCESS_WITH_INFO:
	 case SQL_NEED_DATA:
	 case SQL_ERROR:
	   pstmt->asyn_on = en_NullProc;
	   break;

	 case SQL_STILL_EXECUTING:
	 default:
	   return retcode;
	 }
    }

  /* now, the only possible init state is 'xfetched' */
  switch (retcode)
     {
     case SQL_SUCCESS:
     case SQL_SUCCESS_WITH_INFO:
       break;

     case SQL_NEED_DATA:
       pstmt->state = en_stmt_needdata;
       pstmt->need_on = en_SetPos;
       break;

     case SQL_STILL_EXECUTING:
       pstmt->asyn_on = en_SetPos;
       break;

     default:
       break;
     }

  return retcode;
}
