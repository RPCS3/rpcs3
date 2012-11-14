/*
 *  hstmt.c
 *
 *  $Id: hstmt.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Query statement object management functions
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
SQLAllocStmt (
    HDBC hdbc,
    HSTMT FAR * phstmt)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  STMT_t FAR *pstmt = NULL;
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode = SQL_SUCCESS;

#if (ODBCVER >= 0x0300)
  if (hdbc == SQL_NULL_HDBC || pdbc->type != SQL_HANDLE_DBC)
#else
  if (hdbc == SQL_NULL_HDBC)
#endif
    {
      return SQL_INVALID_HANDLE;
    }

  if (phstmt == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_S1009);

      return SQL_ERROR;
    }

  /* check state */
  switch (pdbc->state)
     {
     case en_dbc_connected:
     case en_dbc_hstmt:
       break;

     case en_dbc_allocated:
     case en_dbc_needdata:
       PUSHSQLERR (pdbc->herr, en_08003);
       *phstmt = SQL_NULL_HSTMT;
       return SQL_ERROR;

     default:
       return SQL_INVALID_HANDLE;
     }

  pstmt = (STMT_t FAR *) MEM_ALLOC (sizeof (STMT_t));

  if (pstmt == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_S1001);
      *phstmt = SQL_NULL_HSTMT;

      return SQL_ERROR;
    }

#if (ODBCVER >= 0x0300)
  pstmt->type = SQL_HANDLE_STMT;
#endif

  /* initiate the object */
  pstmt->herr = SQL_NULL_HERR;
  pstmt->hdbc = hdbc;
  pstmt->state = en_stmt_allocated;
  pstmt->cursor_state = en_stmt_cursor_no;
  pstmt->prep_state = 0;
  pstmt->asyn_on = en_NullProc;
  pstmt->need_on = en_NullProc;

  /* call driver's function */

#if (ODBCVER >= 0x0300)
  hproc = _iodbcdm_getproc (hdbc, en_AllocHandle);

  if (hproc)
    {
      CALL_DRIVER (pstmt->hdbc, hdbc, retcode, hproc, en_AllocHandle, 
        (SQL_HANDLE_STMT, pdbc->dhdbc, &(pstmt->dhstmt)))
    }
  else
#endif

    {
      hproc = _iodbcdm_getproc (hdbc, en_AllocStmt);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  *phstmt = SQL_NULL_HSTMT;
	  MEM_FREE (pstmt);

	  return SQL_ERROR;
	}

      CALL_DRIVER (hdbc, retcode, hproc, en_AllocStmt, 
        (pdbc->dhdbc, &(pstmt->dhstmt)))
    }

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      *phstmt = SQL_NULL_HSTMT;
      MEM_FREE (pstmt);

      return retcode;
    }

  /* insert into list */
  pstmt->next = pdbc->hstmt;
  pdbc->hstmt = pstmt;

  *phstmt = (HSTMT) pstmt;

  /* state transition */
  pdbc->state = en_dbc_hstmt;

  return SQL_SUCCESS;
}


RETCODE 
_iodbcdm_dropstmt (HSTMT hstmt)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  STMT_t FAR *tpstmt;
  DBC_t FAR *pdbc;

  if (hstmt == SQL_NULL_HSTMT)
    {
      return SQL_INVALID_HANDLE;
    }

  pdbc = (DBC_t FAR *) (pstmt->hdbc);

  for (tpstmt = (STMT_t FAR *) pdbc->hstmt;
      tpstmt != NULL;
      tpstmt = tpstmt->next)
    {
      if (tpstmt == pstmt)
	{
	  pdbc->hstmt = (HSTMT) pstmt->next;
	  break;
	}

      if (tpstmt->next == pstmt)
	{
	  tpstmt->next = pstmt->next;
	  break;
	}
    }

  if (tpstmt == NULL)
    {
      return SQL_INVALID_HANDLE;
    }

  _iodbcdm_freesqlerrlist (pstmt->herr);
  MEM_FREE (hstmt);

  return SQL_SUCCESS;
}


RETCODE SQL_API 
SQLFreeStmt (
    HSTMT hstmt,
    UWORD fOption)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  STMT_t FAR *tpstmt;
  DBC_t FAR *pdbc;

  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  pdbc = (DBC_t FAR *) (pstmt->hdbc);

  /* check option */
  switch (fOption)
     {
     case SQL_DROP:
     case SQL_CLOSE:
     case SQL_UNBIND:
     case SQL_RESET_PARAMS:
       break;

     default:
       PUSHSQLERR (pstmt->herr, en_S1092);
       return SQL_ERROR;
     }

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  hproc = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_DROP)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_FreeHandle);

      if (hproc)
	{
	  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_FreeHandle, 
	    (SQL_HANDLE_STMT, pstmt->dhstmt))
	}
    }
#endif

  if (hproc == SQL_NULL_HPROC)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);

	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_FreeStmt, 
	(pstmt->dhstmt, fOption))
    }

  if (retcode != SQL_SUCCESS
      && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  /* state transition */
  switch (fOption)
     {
     case SQL_DROP:
       /* delet this object (ignore return) */
       _iodbcdm_dropstmt (hstmt);
       break;

     case SQL_CLOSE:
       pstmt->cursor_state = en_stmt_cursor_no;
       /* This means cursor name set by
        * SQLSetCursorName() call will also 
        * be erased.
        */

       switch (pstmt->state)
	  {
	  case en_stmt_allocated:
	  case en_stmt_prepared:
	    break;

	  case en_stmt_executed:
	  case en_stmt_cursoropen:
	  case en_stmt_fetched:
	  case en_stmt_xfetched:
	    if (pstmt->prep_state)
	      {
		pstmt->state =
		    en_stmt_prepared;
	      }
	    else
	      {
		pstmt->state =
		    en_stmt_allocated;
	      }
	    break;

	  default:
	    break;
	  }
       break;

     case SQL_UNBIND:
     case SQL_RESET_PARAMS:
     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLSetStmtOption (
    HSTMT hstmt,
    UWORD fOption,
    UDWORD vParam)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  int sqlstat = en_00000;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check option */
  if (				/* fOption < SQL_STMT_OPT_MIN || */
      fOption > SQL_STMT_OPT_MAX)
    {
      PUSHSQLERR (pstmt->herr, en_S1092);

      return SQL_ERROR;
    }

  if (fOption == SQL_CONCURRENCY
      || fOption == SQL_CURSOR_TYPE
      || fOption == SQL_SIMULATE_CURSOR
      || fOption == SQL_USE_BOOKMARKS)
    {
      if (pstmt->asyn_on != en_NullProc)
	{
	  if (pstmt->prep_state)
	    {
	      sqlstat = en_S1011;
	    }
	}
      else
	{
	  switch (pstmt->state)
	     {
	     case en_stmt_prepared:
	       sqlstat = en_S1011;
	       break;

	     case en_stmt_executed:
	     case en_stmt_cursoropen:
	     case en_stmt_fetched:
	     case en_stmt_xfetched:
	       sqlstat = en_24000;
	       break;

	     case en_stmt_needdata:
	     case en_stmt_mustput:
	     case en_stmt_canput:
	       if (pstmt->prep_state)
		 {
		   sqlstat = en_S1011;
		 }
	       break;

	     default:
	       break;
	     }
	}
    }
  else
    {
      if (pstmt->asyn_on != en_NullProc)
	{
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	}
      else
	{
	  if (pstmt->state >= en_stmt_needdata)
	    {
	      sqlstat = en_S1010;
	    }
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOption);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_SetStmtOption,
    (pstmt->dhstmt, fOption, vParam))

  return retcode;
}


RETCODE SQL_API 
SQLGetStmtOption (
    HSTMT hstmt,
    UWORD fOption,
    PTR pvParam)
{
  STMT_t FAR *pstmt = (STMT_t *) hstmt;
  HPROC hproc;
  int sqlstat = en_00000;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check option */
  if (				/* fOption < SQL_STMT_OPT_MIN || */
      fOption > SQL_STMT_OPT_MAX)
    {
      PUSHSQLERR (pstmt->herr, en_S1092);

      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata
      || pstmt->asyn_on != en_NullProc)
    {
      sqlstat = en_S1010;
    }
  else
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
	 case en_stmt_executed:
	 case en_stmt_cursoropen:
	   if (fOption == SQL_ROW_NUMBER || fOption == SQL_GET_BOOKMARK)
	     {
	       sqlstat = en_24000;
	     }
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

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetStmtOption);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_GetStmtOption,
    (pstmt->dhstmt, fOption, pvParam))

  return retcode;
}


RETCODE SQL_API 
SQLCancel (HSTMT hstmt)
{
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HPROC hproc;
  RETCODE retcode;

  if (hstmt == SQL_NULL_HSTMT || pstmt->hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  /* check state */

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_Cancel);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, retcode, hproc, en_Cancel, 
    (pstmt->dhstmt))

  /* state transition */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  switch (pstmt->state)
     {
     case en_stmt_allocated:
     case en_stmt_prepared:
       break;

     case en_stmt_executed:
       if (pstmt->prep_state)
	 {
	   pstmt->state = en_stmt_prepared;
	 }
       else
	 {
	   pstmt->state = en_stmt_allocated;
	 }
       break;

     case en_stmt_cursoropen:
     case en_stmt_fetched:
     case en_stmt_xfetched:
       if (pstmt->prep_state)
	 {
	   pstmt->state = en_stmt_prepared;
	 }
       else
	 {
	   pstmt->state = en_stmt_allocated;
	 }
       break;

     case en_stmt_needdata:
     case en_stmt_mustput:
     case en_stmt_canput:
       switch (pstmt->need_on)
	  {
	  case en_ExecDirect:
	    pstmt->state = en_stmt_allocated;
	    break;

	  case en_Execute:
	    pstmt->state = en_stmt_prepared;
	    break;

	  case en_SetPos:
	    pstmt->state = en_stmt_xfetched;
	    break;

	  default:
	    break;
	  }
       pstmt->need_on = en_NullProc;
       break;

     default:
       break;
     }

  return retcode;
}
