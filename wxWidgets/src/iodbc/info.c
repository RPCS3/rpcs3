/*
 *  info.c
 *
 *  $Id: info.c 5714 2000-01-27 22:47:49Z GT $
 *
 *  Information functions
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

#include	<stdio.h>
#include	<ctype.h>

#define SECT1			"ODBC Data Sources"
#define SECT2			"Default"
#define MAX_ENTRIES		1024

extern char * _iodbcdm_getinifile (char *buf, int size);
extern char * _iodbcdm_getkeyvalbydsn (char *dsn, int dsnlen, char *keywd, char *value, int size);

static int 
stricmp (const char *s1, const char *s2)
{
  int cmp;

  while (*s1)
    {
      if ((cmp = toupper (*s1) - toupper (*s2)) != 0)
	return cmp;
      s1++;
      s2++;
    }
  return (*s2) ? -1 : 0;
}

static int 
SectSorter (const void *p1, const void *p2)
{
  char **s1 = (char **) p1;
  char **s2 = (char **) p2;

  return stricmp (*s1, *s2);
}


RETCODE SQL_API 
SQLDataSources (
    HENV henv,
    UWORD fDir,
    UCHAR FAR * szDSN,
    SWORD cbDSNMax,
    SWORD FAR * pcbDSN,
    UCHAR FAR * szDesc,
    SWORD cbDescMax,
    SWORD FAR * pcbDesc)
{
  GENV_t FAR *genv = (GENV_t FAR *) henv;
  char *path;
  char buf[1024];
  FILE *fp;
  int i;
  static int cur_entry = -1;
  static int num_entries = 0;
  static char **sect = NULL;

  if (henv == SQL_NULL_HENV)
    {
      return SQL_INVALID_HANDLE;
    }
  /* check argument */
  if (cbDSNMax < 0 || cbDescMax < 0)
    {
      PUSHSQLERR (genv->herr, en_S1090);

      return SQL_ERROR;
    }
  if (fDir != SQL_FETCH_FIRST
      && fDir != SQL_FETCH_NEXT)
    {
      PUSHSQLERR (genv->herr, en_S1103);

      return SQL_ERROR;
    }
  if (cur_entry < 0 || fDir == SQL_FETCH_FIRST)
    {
      cur_entry = 0;
      num_entries = 0;


      /* 
       *  Open the odbc.ini file
       */
      path = (char *) _iodbcdm_getinifile (buf, sizeof (buf));
      if ((fp = fopen (path, "r")) == NULL)
	{
	  return SQL_NO_DATA_FOUND;
	}
      /*
       *  Free old section list
       */
      if (sect)
	{
	  for (i = 0; i < MAX_ENTRIES; i++)
	    if (sect[i])
	      free (sect[i]);
	  free (sect);
	}
      if ((sect = (char **) calloc (MAX_ENTRIES, sizeof (char *))) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1011);

	  return SQL_ERROR;
	}
      /*
       *  Build a dynamic list of sections
       */
      while (1)
	{
	  char *str, *p;

	  str = fgets (buf, sizeof (buf), fp);

	  if (str == NULL)
	    break;

	  if (*str == '[')
	    {
	      str++;
	      for (p = str; *p; p++)
		if (*p == ']')
		  *p = '\0';

	      if (!strcmp (str, SECT1))
		continue;
	      if (!strcmp (str, SECT2))
		continue;

	      /*
	       *  Add this section to the comma separated list
	       */
	      if (num_entries >= MAX_ENTRIES)
		break;		/* Skip the rest */

	      sect[num_entries++] = (char *) strdup (str);
	    }
	}

      /*
       *  Sort all entries so we can present a nice list
       */
      if (num_entries > 1)
	qsort (sect, num_entries, sizeof (char *), SectSorter);
    }
  /*
   *  Try to get to the next item
   */
  if (cur_entry >= num_entries)
    {
      cur_entry = 0;		/* Next time, start all over again */
      return SQL_NO_DATA_FOUND;
    }
  /*
   *  Copy DSN information 
   */
  STRNCPY (szDSN, sect[cur_entry], cbDSNMax);
/*
glt???  pcbDSN = strlen(szDSN);
*/
  /*
   *  And find the description that goes with this entry
   */
  _iodbcdm_getkeyvalbydsn (sect[cur_entry], strlen (sect[cur_entry]),
      "Description", (char*) szDesc, cbDescMax);
/*
glt???  pcbDesc = strlen(szDesc);
*/
  /*
   *  Next record
   */
  cur_entry++;

  return SQL_SUCCESS;
}


RETCODE SQL_API 
SQLDrivers (
    HENV henv,
    UWORD fDir,
    UCHAR FAR * szDrvDesc,
    SWORD cbDrvDescMax,
    SWORD FAR * pcbDrvDesc,
    UCHAR FAR * szDrvAttr,
    SWORD cbDrvAttrMax,
    SWORD FAR * pcbDrvAttr)
{
  GENV_t FAR *genv = (GENV_t FAR *) henv;

  if (henv == SQL_NULL_HENV)
    {
      return SQL_INVALID_HANDLE;
    }

  if (cbDrvDescMax < 0 || cbDrvAttrMax < 0 || cbDrvAttrMax == 1)
    {
      PUSHSQLERR (genv->herr, en_S1090);

      return SQL_ERROR;
    }

  if (fDir != SQL_FETCH_FIRST || fDir != SQL_FETCH_NEXT)
    {
      PUSHSQLERR (genv->herr, en_S1103);

      return SQL_ERROR;
    }

/*********************/
  return SQL_NO_DATA_FOUND;
}


RETCODE SQL_API 
SQLGetInfo (
    HDBC hdbc,
    UWORD fInfoType,
    PTR rgbInfoValue,
    SWORD cbInfoValueMax,
    SWORD FAR * pcbInfoValue)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  ENV_t FAR *penv;
  STMT_t FAR *pstmt = NULL;
  STMT_t FAR *tpstmt;
  HPROC hproc;
  RETCODE retcode = SQL_SUCCESS;

  DWORD dword;
  int size = 0, len = 0;
  char buf[16] = {'\0'};

  if (hdbc == SQL_NULL_HDBC || pdbc->henv == SQL_NULL_HENV)
    {
      return SQL_INVALID_HANDLE;
    }

  if (cbInfoValueMax < 0)
    {
      PUSHSQLERR (pdbc->herr, en_S1090);

      return SQL_ERROR;
    }

  if (				/* fInfoType < SQL_INFO_FIRST || */
      (fInfoType > SQL_INFO_LAST
	  && fInfoType < SQL_INFO_DRIVER_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1096);

      return SQL_ERROR;
    }

  if (fInfoType == SQL_ODBC_VER)
    {
      sprintf (buf, "%02d.%02d",
	  (ODBCVER) >> 8, 0x00FF & (ODBCVER));


      if (rgbInfoValue != NULL
	  && cbInfoValueMax > 0)
	{
	  len = STRLEN (buf);

	  if (len < cbInfoValueMax - 1)
	    {
	      len = cbInfoValueMax - 1;
	      PUSHSQLERR (pdbc->herr, en_01004);

	      retcode = SQL_SUCCESS_WITH_INFO;
	    }

	  STRNCPY (rgbInfoValue, buf, len);
	  ((char FAR *) rgbInfoValue)[len] = '\0';
	}

      if (pcbInfoValue != NULL)
	{
	  *pcbInfoValue = (SWORD) len;
	}

      return retcode;
    }

  if (pdbc->state == en_dbc_allocated || pdbc->state == en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08003);

      return SQL_ERROR;
    }

  switch (fInfoType)
     {
     case SQL_DRIVER_HDBC:
       dword = (DWORD) (pdbc->dhdbc);
       size = sizeof (dword);
       break;

     case SQL_DRIVER_HENV:
       penv = (ENV_t FAR *) (pdbc->henv);
       dword = (DWORD) (penv->dhenv);
       size = sizeof (dword);
       break;

     case SQL_DRIVER_HLIB:
       penv = (ENV_t FAR *) (pdbc->henv);
       dword = (DWORD) (penv->hdll);
       size = sizeof (dword);
       break;

     case SQL_DRIVER_HSTMT:
       if (rgbInfoValue != NULL)
	 {
	   pstmt = *((STMT_t FAR **) rgbInfoValue);
	 }

       for (tpstmt = (STMT_t FAR *) (pdbc->hstmt);
	   tpstmt != NULL;
	   tpstmt = tpstmt->next)
	 {
	   if (tpstmt == pstmt)
	     {
	       break;
	     }
	 }

       if (tpstmt == NULL)
	 {
	   PUSHSQLERR (pdbc->herr, en_S1009);

	   return SQL_ERROR;
	 }

       dword = (DWORD) (pstmt->dhstmt);
       size = sizeof (dword);
       break;

     default:
       break;
     }

  if (size)
    {
      if (rgbInfoValue != NULL)
	{
	  *((DWORD *) rgbInfoValue) = dword;
	}

      if (pcbInfoValue != NULL)
	{
	  *(pcbInfoValue) = (SWORD) size;
	}

      return SQL_SUCCESS;
    }

  hproc = _iodbcdm_getproc (hdbc, en_GetInfo);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (hdbc, retcode, hproc, en_GetInfo,
    (pdbc->dhdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue))

  if (retcode == SQL_ERROR
      && fInfoType == SQL_DRIVER_ODBC_VER)
    {
      STRCPY (buf, "01.00");

      if (rgbInfoValue != NULL
	  && cbInfoValueMax > 0)
	{
	  len = STRLEN (buf);

	  if (len < cbInfoValueMax - 1)
	    {
	      len = cbInfoValueMax - 1;
	      PUSHSQLERR (pdbc->herr, en_01004);
	    }

	  STRNCPY (rgbInfoValue, buf, len);
	  ((char FAR *) rgbInfoValue)[len] = '\0';
	}

      if (pcbInfoValue != NULL)
	{
	  *pcbInfoValue = (SWORD) len;
	}

      /* what should we return in this case ???? */
    }

  return retcode;
}


RETCODE SQL_API 
SQLGetFunctions (
    HDBC hdbc,
    UWORD fFunc,
    UWORD FAR * pfExists)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  HPROC hproc;
  RETCODE retcode;

  if (hdbc == SQL_NULL_HDBC)
    {
      return SQL_INVALID_HANDLE;
    }

  if (fFunc > SQL_EXT_API_LAST)
    {
      PUSHSQLERR (pdbc->herr, en_S1095);

      return SQL_ERROR;
    }

  if (pdbc->state == en_dbc_allocated
      || pdbc->state == en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_S1010);

      return SQL_ERROR;
    }

  if (pfExists == NULL)
    {
      return SQL_SUCCESS;
    }

  hproc = _iodbcdm_getproc (hdbc, en_GetFunctions);

  if (hproc != SQL_NULL_HPROC)
    {
      CALL_DRIVER (hdbc, retcode, hproc, en_GetFunctions,
	(pdbc->dhdbc, fFunc, pfExists))

      return retcode;
    }

  if (fFunc == SQL_API_SQLSETPARAM)
    {
      fFunc = SQL_API_SQLBINDPARAMETER;
    }

  if (fFunc != SQL_API_ALL_FUNCTIONS)
    {
      hproc = _iodbcdm_getproc (hdbc, fFunc);

      if (hproc == SQL_NULL_HPROC)
	{
	  *pfExists = (UWORD) 0;
	}
      else
	{
	  *pfExists = (UWORD) 1;
	}

      return SQL_SUCCESS;
    }

  for (fFunc = 0; fFunc < 100; fFunc++)
    {
      hproc = _iodbcdm_getproc (hdbc, fFunc);

      if (hproc == SQL_NULL_HPROC)
	{
	  pfExists[fFunc] = (UWORD) 0;
	}
      else
	{
	  pfExists[fFunc] = (UWORD) 1;
	}
    }

  return SQL_SUCCESS;
}
