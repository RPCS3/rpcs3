/*
 *  connect.c
 *
 *  $Id: connect.c 5545 2000-01-20 13:19:20Z GT $
 *
 *  Connect (load) driver
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

extern	char*	_iodbcdm_getkeyvalbydsn();
extern	char*	_iodbcdm_getkeyvalinstr();
extern	RETCODE	_iodbcdm_driverunload();

/*
 *   Following id string is a copyright mark. Removing(i.e. use 
 *   souce code of this package without it or make it not appear 
 *   in the final object file) or modifing it without permission 
 *   from original author(kejin@empress.com) are copyright 
 *   violation.
 */
static	char sccsid[] 
	= "@(#)iODBC driver manager 2.5, Copyright(c) 1995 by Ke Jin";

/* - Load driver share library( or increase its reference count 
 *   if it has already been loaded by another active connection)
 * - Call driver's SQLAllocEnv() (for the first reference only)
 * - Call driver's SQLAllocConnect()
 * - Call driver's SQLSetConnectOption() (set login time out)
 * - Increase the bookkeeping reference count
 */
static RETCODE 
_iodbcdm_driverload (
    char FAR * path,
    HDBC hdbc)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  GENV_t FAR *genv;
  ENV_t FAR *penv = NULL;
  HDLL hdll;
  HPROC hproc;
  RETCODE retcode = SQL_SUCCESS;
  int sqlstat = en_00000;

  if (path == NULL || path[0] == '\0')
    {
      PUSHSQLERR (pdbc->herr, en_IM002);

      return SQL_ERROR;
    }

  if (hdbc == SQL_NULL_HDBC || pdbc->genv == SQL_NULL_HENV)
    {
      return SQL_INVALID_HANDLE;
    }

  genv = (GENV_t FAR *) pdbc->genv;

  /* This will either load the driver dll or increase its reference count */
  hdll = _iodbcdm_dllopen ((char FAR *) path);

  if (hdll == SQL_NULL_HDLL)
    {
      PUSHSYSERR (pdbc->herr, _iodbcdm_dllerror ());
      PUSHSQLERR (pdbc->herr, en_IM003);
      return SQL_ERROR;
    }

  penv = (ENV_t FAR *) (pdbc->henv);

  if (penv != NULL)
    {
      if (penv->hdll != hdll)
	{
	  _iodbcdm_driverunload (hdbc);
	}
      else
	{
	  /* 
  	   * this will not unload the driver but only decrease its internal
	   * reference count 
	   */
	  _iodbcdm_dllclose (hdll);
	}
    }

  if (penv == NULL)
    {
      /* 
       * find out whether this dll has already been loaded on another 
       * connection 
       */
      for (penv = (ENV_t FAR *) genv->henv; 
	  penv != NULL;
	  penv = (ENV_t FAR *) penv->next)
	{
	  if (penv->hdll == hdll)
	    {
	      /* 
  	       * this will not unload the driver but only decrease its internal
	       * reference count 
	       */
	      _iodbcdm_dllclose (hdll);
	      break;
	    }
	}

      if (penv == NULL)
	/* no connection attaching with this dll */
	{
	  int i;

	  /* create a new dll env instance */
	  penv = (ENV_t FAR *) MEM_ALLOC (sizeof (ENV_t));

	  if (penv == NULL)
	    {
	      _iodbcdm_dllclose (hdll);

	      PUSHSQLERR (pdbc->herr, en_S1001);

	      return SQL_ERROR;
	    }

	  for (i = 0; i < SQL_EXT_API_LAST + 1; i++)
	    {
	      (penv->dllproc_tab)[i] = SQL_NULL_HPROC;
	    }

	  pdbc->henv = penv;
	  penv->hdll = hdll;

	  /* call driver's SQLAllocHandle() or SQLAllocEnv() */

#if (ODBCVER >= 0x0300)
	  hproc = _iodbcdm_getproc (hdbc, en_AllocHandle);

	  if (hproc)
	    {
	      CALL_DRIVER (hdbc, retcode, hproc, en_AllocHandle,
		  (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(penv->dhenv)))
	    }
	  else			/* try driver's SQLAllocEnv() */
#endif
	    {
	      hproc = _iodbcdm_getproc (hdbc, en_AllocEnv);

	      if (hproc == SQL_NULL_HPROC)
		{
		  sqlstat = en_IM004;
		}
	      else
		{
		  CALL_DRIVER (hdbc, retcode, hproc,
		      en_AllocEnv, (&(penv->dhenv)))
		}
	    }

	  if (retcode == SQL_ERROR)
	    {
	      sqlstat = en_IM004;
	    }

	  if (sqlstat != en_00000)
	    {
	      _iodbcdm_dllclose (hdll);
	      MEM_FREE (penv);
	      PUSHSQLERR (pdbc->herr, en_IM004);

	      return SQL_ERROR;
	    }

	  /* insert into dll env list */
	  penv->next = (ENV_t FAR *) genv->henv;
	  genv->henv = penv;

	  /* initiate this new env entry */
	  penv->refcount = 0;	/* we will increase it after
				 * driver's SQLAllocConnect()
				 * success
				 */
	}

      pdbc->henv = penv;

      if (pdbc->dhdbc == SQL_NULL_HDBC)
	{

#if (ODBCVER >= 0x0300)
	  hproc = _iodbcdm_getproc (hdbc, en_AllocHandle);

	  if (hproc)
	    {
	      CALL_DRIVER (hdbc, retcode, hproc, en_AllocHandle,
		  (SQL_HANDLE_DBC, penv->dhenv, &(pdbc->dhdbc)))
	    }
	  else
#endif

	    {
	      hproc = _iodbcdm_getproc (hdbc, en_AllocConnect);

	      if (hproc == SQL_NULL_HPROC)
		{
		  sqlstat = en_IM005;
		}
	      else
		{
		  CALL_DRIVER (hdbc, retcode, hproc,
		      en_AllocConnect, (penv->dhenv, &(pdbc->dhdbc)))
		}
	    }

	  if (retcode == SQL_ERROR)
	    {
	      sqlstat = en_IM005;
	    }

	  if (sqlstat != en_00000)
	    {
	      _iodbcdm_driverunload (hdbc);

	      pdbc->dhdbc = SQL_NULL_HDBC;
	      PUSHSQLERR (pdbc->herr, en_IM005);

	      return SQL_ERROR;
	    }
	}

      pdbc->henv = penv;
      penv->refcount++;		/* bookkeeping reference count on this driver */
    }

  /* driver's login timeout option must been set before 
   * its SQLConnect() call */
  if (pdbc->login_timeout != 0UL)
    {
      hproc = _iodbcdm_getproc (hdbc, en_SetConnectOption);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM004;
	}
      else
	{
	  CALL_DRIVER (hdbc, retcode, hproc,
	      en_SetConnectOption, (
		  pdbc->dhdbc,
		  SQL_LOGIN_TIMEOUT,
		  pdbc->login_timeout))

	      if (retcode == SQL_ERROR)
	    {
	      PUSHSQLERR (pdbc->herr, en_IM006);

	      return SQL_SUCCESS_WITH_INFO;
	    }
	}
    }

  return SQL_SUCCESS;
}


/* - Call driver's SQLFreeConnect()
 * - Call driver's SQLFreeEnv() ( for the last reference only)
 * - Unload the share library( or decrease its reference
 *   count if it is not the last referenct )
 * - decrease bookkeeping reference count
 * - state transition to allocated
 */
RETCODE 
_iodbcdm_driverunload (HDBC hdbc)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  ENV_t FAR *penv;
  ENV_t FAR *tpenv;
  GENV_t FAR *genv;
  HPROC hproc;
  RETCODE retcode = SQL_SUCCESS;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* no pointer check will be performed in this function */
  penv = (ENV_t FAR *) pdbc->henv;
  genv = (GENV_t FAR *) pdbc->genv;

  if (penv == NULL || penv->hdll == SQL_NULL_HDLL)
    {
      return SQL_SUCCESS;
    }

#if (ODBCVER >= 0x0300)
  hproc = _iodbcdm_getproc (hdbc, en_FreeHandle);

  if (hproc)
    {
      CALL_DRIVER (hdbc, retcode, hproc, en_FreeHandle,
	  (SQL_HANDLE_DBC, pdbc->dhdbc))
    }
  else
#endif

    {
      hproc = _iodbcdm_getproc (hdbc, en_FreeConnect);

      if (hproc != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (hdbc, retcode, hproc,
	      en_FreeConnect, (pdbc->dhdbc))

	      pdbc->dhdbc = SQL_NULL_HDBC;
	}
    }

  penv->refcount--;

  if (!penv->refcount)
    /* no other connections still attaching with this driver */
    {

#if (ODBCVER >= 0x0300)
      hproc = _iodbcdm_getproc (hdbc, en_FreeHandle);

      if (hproc)
	{
	  CALL_DRIVER (hdbc, retcode, hproc, en_FreeHandle,
	      (SQL_HANDLE_ENV, penv->dhenv))
	}
      else
#endif

	{
	  hproc = _iodbcdm_getproc (hdbc, en_FreeEnv);

	  if (hproc != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (hdbc, retcode, hproc, en_FreeEnv,
		  (penv->dhenv))

		  penv->dhenv = SQL_NULL_HENV;
	    }
	}

      _iodbcdm_dllclose (penv->hdll);

      penv->hdll = SQL_NULL_HDLL;

      for (tpenv = (ENV_t FAR *) genv->henv;
	  tpenv != NULL;
	  tpenv = (ENV_t FAR *) penv->next)
	{
	  if (tpenv == penv)
	    {
	      genv->henv = penv->next;
	      break;
	    }

	  if (tpenv->next == penv)
	    {
	      tpenv->next = penv->next;
	      break;
	    }
	}

      MEM_FREE (penv);
    }

  pdbc->henv = SQL_NULL_HENV;
  pdbc->hstmt = SQL_NULL_HSTMT;
  /* pdbc->herr = SQL_NULL_HERR; 
     -- delay to DM's SQLFreeConnect() */
  pdbc->dhdbc = SQL_NULL_HDBC;
  pdbc->state = en_dbc_allocated;

  /* set connect options to default values */
	/**********
	pdbc->access_mode	= SQL_MODE_DEFAULT;
	pdbc->autocommit	= SQL_AUTOCOMMIT_DEFAULT;
	pdbc->login_timeout 	= 0UL;
	**********/
  pdbc->odbc_cursors = SQL_CUR_DEFAULT;
  pdbc->packet_size = 0UL;
  pdbc->quiet_mode = (UDWORD) NULL;
  pdbc->txn_isolation = SQL_TXN_READ_UNCOMMITTED;

  if (pdbc->current_qualifier != NULL)
    {
      MEM_FREE (pdbc->current_qualifier);
      pdbc->current_qualifier = NULL;
    }

  return SQL_SUCCESS;
}


static RETCODE 
_iodbcdm_dbcdelayset (HDBC hdbc)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  ENV_t FAR *penv;
  HPROC hproc;
  RETCODE retcode = SQL_SUCCESS;
  RETCODE ret;

  penv = pdbc->henv;

  hproc = _iodbcdm_getproc (hdbc, en_SetConnectOption);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM006);

      return SQL_SUCCESS_WITH_INFO;
    }

  if (pdbc->access_mode != SQL_MODE_DEFAULT)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      SQL_ACCESS_MODE,
	      pdbc->access_mode))

	  retcode |= ret;
    }

  if (pdbc->autocommit != SQL_AUTOCOMMIT_DEFAULT)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      pdbc->dhdbc,
	      SQL_AUTOCOMMIT,
	      pdbc->autocommit))

	  retcode |= ret;
    }

  if (pdbc->current_qualifier != NULL)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      pdbc->dhdbc,
	      SQL_CURRENT_QUALIFIER,
	      pdbc->current_qualifier))

	  retcode |= ret;
    }

  if (pdbc->packet_size != 0UL)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      pdbc->dhdbc,
	      SQL_PACKET_SIZE,
	      pdbc->packet_size))

	  retcode |= ret;
    }

  if (pdbc->quiet_mode != (UDWORD) NULL)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      pdbc->dhdbc,
	      SQL_QUIET_MODE,
	      pdbc->quiet_mode))

	  retcode |= ret;
    }

  if (pdbc->txn_isolation != SQL_TXN_READ_UNCOMMITTED)
    {
      CALL_DRIVER (hdbc, ret, hproc,
	  en_SetConnectOption, (
	      pdbc->dhdbc,
	      SQL_TXN_ISOLATION,
	      pdbc->txn_isolation))
    }

  /* check error code for driver's SQLSetConnectOption() call */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      PUSHSQLERR (pdbc->herr, en_IM006);

      retcode = SQL_ERROR;
    }

  /* get cursor behavior on transaction commit or rollback */
  hproc = _iodbcdm_getproc (hdbc, en_GetInfo);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_01000);

      return retcode;
    }

  CALL_DRIVER (hdbc, ret, hproc,
      en_GetInfo, (
	  pdbc->dhdbc,
	  SQL_CURSOR_COMMIT_BEHAVIOR,
	  (PTR) & (pdbc->cb_commit),
	  sizeof (pdbc->cb_commit),
	  NULL))

      retcode |= ret;

  CALL_DRIVER (hdbc, ret, hproc,
      en_GetInfo, (
	  pdbc->dhdbc,
	  SQL_CURSOR_ROLLBACK_BEHAVIOR,
	  (PTR) & (pdbc->cb_rollback),
	  sizeof (pdbc->cb_rollback),
	  NULL))

      retcode |= ret;

  if (retcode != SQL_SUCCESS
      && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return SQL_ERROR;
    }

  return retcode;
}


static RETCODE 
_iodbcdm_settracing (HDBC hdbc, char *dsn, int dsnlen)
{
  char buf[256];
  char *ptr;
  RETCODE setopterr = SQL_SUCCESS;

  /* Get Driver's DLL path from specificed or default dsn section */
  ptr = _iodbcdm_getkeyvalbydsn (dsn, dsnlen, "TraceFile",
      (char FAR *) buf, sizeof (buf));

  if (ptr == NULL || ptr[0] == '\0')
    {
      ptr = (char FAR *) (SQL_OPT_TRACE_FILE_DEFAULT);
    }

  setopterr |= SQLSetConnectOption (hdbc, SQL_OPT_TRACEFILE, (UDWORD) (ptr));

  ptr = _iodbcdm_getkeyvalbydsn (dsn, dsnlen, "Trace",
      (char FAR *) buf, sizeof (buf));

  if (ptr != NULL)
    {
      UDWORD opt = (UDWORD) (-1L);

      if (STREQ (ptr, "ON")
	  || STREQ (ptr, "On")
	  || STREQ (ptr, "on")
	  || STREQ (ptr, "1"))
	{
	  opt = SQL_OPT_TRACE_ON;
	}

      if (STREQ (ptr, "OFF")
	  || STREQ (ptr, "Off")
	  || STREQ (ptr, "off")
	  || STREQ (ptr, "0"))
	{
	  opt = SQL_OPT_TRACE_OFF;
	}

      if (opt != (UDWORD) (-1L))
	{
	  setopterr |= SQLSetConnectOption (hdbc,
	      SQL_OPT_TRACE, opt);
	}
    }

  return setopterr;
}


RETCODE SQL_API 
SQLConnect (
    HDBC hdbc,
    UCHAR FAR * szDSN,
    SWORD cbDSN,
    UCHAR FAR * szUID,
    SWORD cbUID,
    UCHAR FAR * szAuthStr,
    SWORD cbAuthStr)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  RETCODE retcode = SQL_SUCCESS;
  RETCODE setopterr = SQL_SUCCESS;
  char driver[1024] = {'\0'};	/* MS SDK Guide 
				 * specifies driver
				 * path can't longer
				 * than 255. */
  char *ptr;
  HPROC hproc;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check arguments */
  if ((cbDSN < 0 && cbDSN != SQL_NTS)
      || (cbUID < 0 && cbUID != SQL_NTS)
      || (cbAuthStr < 0 && cbAuthStr != SQL_NTS)
      || (cbDSN > SQL_MAX_DSN_LENGTH))
    {
      PUSHSQLERR (pdbc->herr, en_S1090);

      return SQL_ERROR;
    }

  if (szDSN == NULL || cbDSN == 0)
    {
      PUSHSQLERR (pdbc->herr, en_IM002);

      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_08002);

      return SQL_ERROR;
    }

  setopterr |= _iodbcdm_settracing (hdbc,
      (char *) szDSN, cbDSN);

  ptr = _iodbcdm_getkeyvalbydsn (szDSN, cbDSN, "Driver",
      (char FAR *) driver, sizeof (driver));

  if (ptr == NULL)
    /* No specified or default dsn section or
     * no driver specification in this dsn section */
    {
      PUSHSQLERR (pdbc->herr, en_IM002);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_driverload (driver, hdbc);

  switch (retcode)
     {
     case SQL_SUCCESS:
       break;

     case SQL_SUCCESS_WITH_INFO:
       setopterr = SQL_ERROR;
       /* unsuccessed in calling driver's 
        * SQLSetConnectOption() to set login
        * timeout.
        */
       break;

     default:
       return retcode;
     }

  hproc = _iodbcdm_getproc (hdbc, en_Connect);

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_driverunload (hdbc);

      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_Connect, (
	  pdbc->dhdbc,
	  szDSN, cbDSN,
	  szUID, cbUID,
	  szAuthStr, cbAuthStr))

  if (retcode != SQL_SUCCESS
      && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* not unload driver for retrive error 
       * messge from driver */
		/*********
		_iodbcdm_driverunload( hdbc );
		**********/

      return retcode;
    }

  /* state transition */
  pdbc->state = en_dbc_connected;

  /* do delaid option setting */
  setopterr |= _iodbcdm_dbcdelayset (hdbc);

  if (setopterr != SQL_SUCCESS)
    {
      return SQL_SUCCESS_WITH_INFO;
    }

  return retcode;
}


RETCODE SQL_API 
SQLDriverConnect (
    HDBC hdbc,
    SQLHWND hwnd,
    UCHAR FAR * szConnStrIn,
    SWORD cbConnStrIn,
    UCHAR FAR * szConnStrOut,
    SWORD cbConnStrOutMax,
    SWORD FAR * pcbConnStrOut,
    UWORD fDriverCompletion)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  HDLL hdll;
  char FAR *drv;
  char drvbuf[1024];
  char FAR *dsn;
  char dsnbuf[SQL_MAX_DSN_LENGTH + 1];
  UCHAR cnstr2drv[1024];

  HPROC hproc;
  HPROC dialproc;

  int sqlstat = en_00000;
  RETCODE retcode = SQL_SUCCESS;
  RETCODE setopterr = SQL_SUCCESS;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check arguments */
  if ((cbConnStrIn < 0 && cbConnStrIn != SQL_NTS)
      || cbConnStrOutMax < 0)
    {
      PUSHSQLERR (pdbc->herr, en_S1090);

      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_08002);

      return SQL_ERROR;
    }

  drv = _iodbcdm_getkeyvalinstr (szConnStrIn, cbConnStrIn,
      "DRIVER", drvbuf, sizeof (drvbuf));

  dsn = _iodbcdm_getkeyvalinstr (szConnStrIn, cbConnStrIn,
      "DSN", dsnbuf, sizeof (dsnbuf));

  switch (fDriverCompletion)
     {
     case SQL_DRIVER_NOPROMPT:
       break;

     case SQL_DRIVER_COMPLETE:
     case SQL_DRIVER_COMPLETE_REQUIRED:
       if (dsn != NULL || drv != NULL)
	 {
	   break;
	 }
       /* fall to next case */
     case SQL_DRIVER_PROMPT:
       /* Get data source dialog box function from
        * current executable */
       hdll = _iodbcdm_dllopen ((char FAR *) NULL);
       dialproc = _iodbcdm_dllproc (hdll,
	   "_iodbcdm_drvconn_dialbox");

       if (dialproc == SQL_NULL_HPROC)
	 {
	   sqlstat = en_IM008;
	   break;
	 }

       retcode = dialproc (
	   hwnd,		/* window or display handle */
	   dsnbuf,		/* input/output dsn buf */
	   sizeof (dsnbuf),	/* buf size */
	   &sqlstat);		/* error code */

       if (retcode != SQL_SUCCESS)
	 {
	   break;
	 }

       if (cbConnStrIn == SQL_NTS)
	 {
	   cbConnStrIn = STRLEN (szConnStrIn);
	 }

       dsn = dsnbuf;

       if (dsn[0] == '\0')
	 {
	   dsn = "default";
	 }

       if (cbConnStrIn > sizeof (cnstr2drv)
	   - STRLEN (dsn) - STRLEN ("DSN=;") - 1)
	 {
	   sqlstat = en_S1001;	/* a lazy way to avoid
				 * using heap memory */
	   break;
	 }

       sprintf ((char*)cnstr2drv, "DSN=%s;", dsn);
       cbConnStrIn += STRLEN (cnstr2drv);
       STRNCAT (cnstr2drv, szConnStrIn, cbConnStrIn);
       szConnStrIn = cnstr2drv;
       break;

     default:
       sqlstat = en_S1110;
       break;
     }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

  if (dsn == NULL || dsn[0] == '\0')
    {
      dsn = "default";
    }
  else
    /* if you want tracing, you must use a DSN */
    {
      setopterr |= _iodbcdm_settracing (hdbc,
	  (char *) dsn, SQL_NTS);
    }

  if (drv == NULL || drv[0] == '\0')
    {
      drv = _iodbcdm_getkeyvalbydsn (dsn, SQL_NTS, "Driver",
	  drvbuf, sizeof (drvbuf));
    }

  if (drv == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_IM002);

      return SQL_ERROR;
    }

  retcode = _iodbcdm_driverload (drv, hdbc);

  switch (retcode)
     {
     case SQL_SUCCESS:
       break;

     case SQL_SUCCESS_WITH_INFO:
       setopterr = SQL_ERROR;
       /* unsuccessed in calling driver's 
        * SQLSetConnectOption() to set login
        * timeout.
        */
       break;

     default:
       return retcode;
     }

  hproc = _iodbcdm_getproc (hdbc, en_DriverConnect);

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_driverunload (hdbc);

      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_DriverConnect, (
	  pdbc->dhdbc, hwnd,
	  szConnStrIn, cbConnStrIn,
	  szConnStrOut, cbConnStrOutMax,
	  pcbConnStrOut, fDriverCompletion))

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* don't unload driver here for retrive 
       * error message from driver */
		/********
		_iodbcdm_driverunload( hdbc );
		*********/

      return retcode;
    }

  /* state transition */
  pdbc->state = en_dbc_connected;

  /* do delaid option setting */
  setopterr |= _iodbcdm_dbcdelayset (hdbc);

  if (setopterr != SQL_SUCCESS)
    {
      return SQL_SUCCESS_WITH_INFO;
    }

  return retcode;
}


RETCODE SQL_API 
SQLBrowseConnect (
    HDBC hdbc,
    UCHAR FAR * szConnStrIn,
    SWORD cbConnStrIn,
    UCHAR FAR * szConnStrOut,
    SWORD cbConnStrOutMax,
    SWORD FAR * pcbConnStrOut)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  char FAR *drv;
  char drvbuf[1024];
  char FAR *dsn;
  char dsnbuf[SQL_MAX_DSN_LENGTH + 1];

  HPROC hproc;

  RETCODE retcode = SQL_SUCCESS;
  RETCODE setopterr = SQL_SUCCESS;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check arguments */
  if ((cbConnStrIn < 0 && cbConnStrIn != SQL_NTS) || cbConnStrOutMax < 0)
    {
      PUSHSQLERR (pdbc->herr, en_S1090);

      return SQL_ERROR;
    }

  if (pdbc->state == en_dbc_allocated)
    {
      drv = _iodbcdm_getkeyvalinstr (szConnStrIn, cbConnStrIn,
	  "DRIVER", drvbuf, sizeof (drvbuf));

      dsn = _iodbcdm_getkeyvalinstr (szConnStrIn, cbConnStrIn,
	  "DSN", dsnbuf, sizeof (dsnbuf));

      if (dsn == NULL || dsn[0] == '\0')
	{
	  dsn = "default";
	}
      else
	/* if you want tracing, you must use a DSN */
	{
	  setopterr |= _iodbcdm_settracing (hdbc,
	      (char *) dsn, SQL_NTS);
	}

      if (drv == NULL || drv[0] == '\0')
	{
	  drv = _iodbcdm_getkeyvalbydsn (dsn, SQL_NTS, "Driver",
	      drvbuf, sizeof (drvbuf));
	}

      if (drv == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_IM002);

	  return SQL_ERROR;
	}

      retcode = _iodbcdm_driverload (drv, hdbc);

      switch (retcode)
	 {
	 case SQL_SUCCESS:
	   break;

	 case SQL_SUCCESS_WITH_INFO:
	   setopterr = SQL_ERROR;
	   /* unsuccessed in calling driver's 
	    * SQLSetConnectOption() to set login
	    * timeout.
	    */
	   break;

	 default:
	   return retcode;
	 }
    }
  else if (pdbc->state != en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08002);

      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (hdbc, en_BrowseConnect);

  if (hproc == SQL_NULL_HPROC)
    {
      _iodbcdm_driverunload (hdbc);

      pdbc->state = en_dbc_allocated;

      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_BrowseConnect, (
	  pdbc->dhdbc, 
	  szConnStrIn, cbConnStrIn, 
	  szConnStrOut, cbConnStrOutMax,
	  pcbConnStrOut))

  switch (retcode)
     {
     case SQL_SUCCESS:
     case SQL_SUCCESS_WITH_INFO:
       pdbc->state = en_dbc_connected;
       setopterr |= _iodbcdm_dbcdelayset (hdbc);
       if (setopterr != SQL_SUCCESS)
	 {
	   retcode = SQL_SUCCESS_WITH_INFO;
	 }
       break;

     case SQL_NEED_DATA:
       pdbc->state = en_dbc_needdata;
       break;

     case SQL_ERROR:
       pdbc->state = en_dbc_allocated;
       /* but the driver will not unloaded 
        * to allow application retrive err
        * message from driver 
        */
       break;

     default:
       break;
     }

  return retcode;
}


RETCODE SQL_API 
SQLDisconnect (HDBC hdbc)
{
  DBC_t FAR *pdbc = (DBC_t *) hdbc;
  STMT_t FAR *pstmt;
  RETCODE retcode;
  HPROC hproc;

  int sqlstat = en_00000;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check hdbc state */
  if (pdbc->state == en_dbc_allocated)
    {
      sqlstat = en_08003;
    }

  /* check stmt(s) state */
  for (pstmt = (STMT_t FAR *) pdbc->hstmt;
      pstmt != NULL && sqlstat == en_00000;
      pstmt = (STMT_t FAR *) pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata
	  || pstmt->asyn_on != en_NullProc)
	/* In this case one need to call 
	 * SQLCancel() first */
	{
	  sqlstat = en_S1010;
	}
    }

  if (sqlstat == en_00000)
    {
      hproc = _iodbcdm_getproc (hdbc, en_Disconnect);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_Disconnect, (
	  pdbc->dhdbc))

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      /* diff from MS specs. We disallow
       * driver SQLDisconnect() return
       * SQL_SUCCESS_WITH_INFO and post
       * error message.
       */
      retcode = SQL_SUCCESS;
    }
  else
    {
      return retcode;
    }

  /* free all statement handle(s) on this connection */
  for (; pdbc->hstmt;)
    {
      _iodbcdm_dropstmt (pdbc->hstmt);
    }

  /* state transition */
  if (retcode == SQL_SUCCESS)
    {
      pdbc->state = en_dbc_allocated;
    }

  return retcode;
}


RETCODE SQL_API 
SQLNativeSql (
    HDBC hdbc,
    UCHAR FAR * szSqlStrIn,
    SDWORD cbSqlStrIn,
    UCHAR FAR * szSqlStr,
    SDWORD cbSqlStrMax,
    SDWORD FAR * pcbSqlStr)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  HPROC hproc;
  int sqlstat = en_00000;
  RETCODE retcode;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  if (szSqlStrIn == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbSqlStrIn < 0 && cbSqlStrIn != SQL_NTS)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

  /* check state */
  if (pdbc->state <= en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08003);

      return SQL_ERROR;
    }

  /* call driver */
  hproc = _iodbcdm_getproc (hdbc, en_NativeSql);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_NativeSql,
    (pdbc->dhdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr))

  return retcode;
}
