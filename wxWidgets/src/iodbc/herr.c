/*
 *  herr.c
 *
 *  $Id: herr.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Error stack management functions
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

#include        "herr.h"
#include	"henv.h"
#include	"hdbc.h"
#include	"hstmt.h"

#include	"itrace.h"

#include	"herr.ci"

static HERR 
_iodbcdm_popsqlerr (HERR herr)
{
  sqlerr_t *list = (sqlerr_t *) herr;
  sqlerr_t *next;

  if (herr == SQL_NULL_HERR)
    {
      return herr;
    }

  next = list->next;

  MEM_FREE (list);

  return next;
}


void 
_iodbcdm_freesqlerrlist (HERR herrlist)
{
  HERR list;

  for (list = herrlist; list != 0;)
    {
      list = _iodbcdm_popsqlerr (list);
    }
}


HERR 
_iodbcdm_pushsqlerr (
    HERR herr,
    sqlstcode_t code,
    char *msg)
{
  sqlerr_t *ebuf;
  sqlerr_t *perr = (sqlerr_t *) herr;
  int idx = 0;

  if (herr != SQL_NULL_HERR)
    {
      idx = perr->idx + 1;
    }

  if (idx == 64)
    /* over wirte the top entry to prevent error stack blow out */
    {
      perr->code = code;
      perr->msg = msg;

      return herr;
    }

  ebuf = (sqlerr_t *) MEM_ALLOC (sizeof (sqlerr_t));

  if (ebuf == NULL)
    {
      return NULL;
    }

  ebuf->msg = msg;
  ebuf->code = code;
  ebuf->idx = idx;
  ebuf->next = (sqlerr_t *) herr;

  return (HERR) ebuf;
}


static char FAR *
_iodbcdm_getsqlstate (
    HERR herr,
    void FAR * tab)
{
  sqlerr_t *perr = (sqlerr_t *) herr;
  sqlerrmsg_t *ptr;

  if (herr == SQL_NULL_HERR || tab == NULL)
    {
      return (char FAR *) NULL;
    }

  for (ptr = tab;
      ptr->code != en_sqlstat_total;
      ptr++)
    {
      if (ptr->code == perr->code)
	{
	  return (char FAR *) (ptr->stat);
	}
    }

  return (char FAR *) NULL;
}


static char FAR *
_iodbcdm_getsqlerrmsg (
    HERR herr,
    void FAR * errtab)
{
  sqlerr_t *perr = (sqlerr_t *) herr;
  sqlerrmsg_t *ptr;

  if (herr == SQL_NULL_HERR)
    {
      return NULL;
    }

  if (perr->msg == NULL && errtab == NULL)
    {
      return NULL;
    }

  if (perr->msg != NULL)
    {
      return perr->msg;
    }

  for (ptr = (sqlerrmsg_t *) errtab;
      ptr->code != en_sqlstat_total;
      ptr++)
    {
      if (ptr->code == perr->code)
	{
	  return (char FAR *) ptr->msg;
	}
    }

  return (char FAR *) NULL;
}


RETCODE SQL_API 
SQLError (
    HENV henv,
    HDBC hdbc,
    HSTMT hstmt,
    UCHAR FAR * szSqlstate,
    SDWORD FAR * pfNativeError,
    UCHAR FAR * szErrorMsg,
    SWORD cbErrorMsgMax,
    SWORD FAR * pcbErrorMsg)
{
  GENV_t FAR *genv = (GENV_t FAR *) henv;
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  STMT_t FAR *pstmt = (STMT_t FAR *) hstmt;
  HDBC thdbc;

  HENV dhenv = SQL_NULL_HENV;
  HDBC dhdbc = SQL_NULL_HDBC;
  HSTMT dhstmt = SQL_NULL_HSTMT;

  HERR herr = SQL_NULL_HERR;
  HPROC hproc = SQL_NULL_HPROC;

  char FAR *errmsg = NULL;
  char FAR *ststr = NULL;

  int handle = 0;
  RETCODE retcode = SQL_SUCCESS;

  if (hstmt != SQL_NULL_HSTMT)	/* retrive stmt err */
    {
      herr = pstmt->herr;
      thdbc = pstmt->hdbc;

      if (thdbc == SQL_NULL_HDBC)
	{
	  return SQL_INVALID_HANDLE;
	}
      hproc = _iodbcdm_getproc (thdbc, en_Error);
      dhstmt = pstmt->dhstmt;
      handle = 3;
    }
  else if (hdbc != SQL_NULL_HDBC)	/* retrive dbc err */
    {
      herr = pdbc->herr;
      thdbc = hdbc;
      if (thdbc == SQL_NULL_HDBC)
	{
	  return SQL_INVALID_HANDLE;
	}
      hproc = _iodbcdm_getproc (thdbc, en_Error);
      dhdbc = pdbc->dhdbc;
      handle = 2;

      if (herr == SQL_NULL_HERR
	  && pdbc->henv == SQL_NULL_HENV)
	{
	  return SQL_NO_DATA_FOUND;
	}
    }
  else if (henv != SQL_NULL_HENV)	/* retrive env err */
    {
      herr = genv->herr;

      /* Drivers shouldn't push error message 
       * on envoriment handle */

      if (herr == SQL_NULL_HERR)
	{
	  return SQL_NO_DATA_FOUND;
	}

      handle = 1;
    }
  else
    {
      return SQL_INVALID_HANDLE;
    }

  if (szErrorMsg != NULL)
    {
      if (cbErrorMsgMax < 0
	  || cbErrorMsgMax > SQL_MAX_MESSAGE_LENGTH - 1)
	{
	  return SQL_ERROR;
	  /* SQLError() doesn't post error for itself */
	}
    }

  if (herr == SQL_NULL_HERR)	/* no err on drv mng */
    {
      /* call driver */
      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (herr, en_IM001);

	  return SQL_ERROR;
	}

      CALL_DRIVER (thdbc, retcode, hproc, en_Error,
	  (dhenv, dhdbc, dhstmt, szSqlstate, pfNativeError, szErrorMsg,
	      cbErrorMsgMax, pcbErrorMsg))

      return retcode;
    }

  if (szSqlstate != NULL)
    {
      int len;

      /* get sql state  string */
      ststr = (char FAR *) _iodbcdm_getsqlstate (herr,
	  (void FAR *) sqlerrmsg_tab);

      if (ststr == NULL)
	{
	  len = 0;
	}
      else
	{
	  len = (int) STRLEN (ststr);
	}

      STRNCPY (szSqlstate, ststr, len);
      szSqlstate[len] = 0;
      /* buffer size of szSqlstate is not checked. Applications
       * suppose provide enough ( not less than 6 bytes ) buffer
       * or NULL for it.
       */
    }

  if (pfNativeError != NULL)
    {
      /* native error code is specific to data source */
      *pfNativeError = (SDWORD) 0L;
    }

  if (szErrorMsg == NULL || cbErrorMsgMax == 0)
    {
      if (pcbErrorMsg != NULL)
	{
	  *pcbErrorMsg = (SWORD) 0;
	}
    }
  else
    {
      int len;
      char msgbuf[256] = {'\0'};

      /* get sql state message */
      errmsg = _iodbcdm_getsqlerrmsg (herr, (void FAR *) sqlerrmsg_tab);

      if (errmsg == NULL)
	{
	  errmsg = (char FAR *) "";
	}

      sprintf (msgbuf, "%s%s", sqlerrhd, errmsg);

      len = STRLEN (msgbuf);

      if (len < cbErrorMsgMax - 1)
	{
	  retcode = SQL_SUCCESS;
	}
      else
	{
	  len = cbErrorMsgMax - 1;
	  retcode = SQL_SUCCESS_WITH_INFO;
	  /* and not posts error for itself */
	}

      STRNCPY ((char *) szErrorMsg, msgbuf, len);
      szErrorMsg[len] = 0;

      if (pcbErrorMsg != NULL)
	{
	  *pcbErrorMsg = (SWORD) len;
	}
    }

  switch (handle)		/* free this err */
     {
     case 1:
       genv->herr = _iodbcdm_popsqlerr (genv->herr);
       break;

     case 2:
       pdbc->herr = _iodbcdm_popsqlerr (pdbc->herr);
       break;

     case 3:
       pstmt->herr = _iodbcdm_popsqlerr (pstmt->herr);
       break;

     default:
       break;
     }

  return retcode;
}
