/*
 *  dlproc.c
 *
 *  $Id: dlproc.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Load driver and resolve driver's function entry point
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

#include	"itrace.h"

#include	"henv.ci"

HPROC 
_iodbcdm_getproc (HDBC hdbc, int idx)
{
  DBC_t FAR *pdbc = (DBC_t FAR *) hdbc;
  ENV_t FAR *penv;
  HPROC FAR *phproc;

  if (idx <= 0 || idx > SQL_EXT_API_LAST)
    /* first entry naver used */
    {
      return SQL_NULL_HPROC;
    }

  penv = (ENV_t FAR *) (pdbc->henv);

  if (penv == NULL)
    {
      return SQL_NULL_HPROC;
    }

  phproc = penv->dllproc_tab + idx;

  if (*phproc == SQL_NULL_HPROC)
    {
      int i, en_idx;

      for (i = 0;; i++)
	{
	  en_idx = odbcapi_symtab[i].en_idx;

	  if (en_idx == en_NullProc)
	    {
	      break;
	    }

	  if (en_idx == idx)
	    {
	      *phproc = _iodbcdm_dllproc (penv->hdll,
		  odbcapi_symtab[i].symbol);

	      break;
	    }
	}
    }

  return *phproc;
}


HDLL 
_iodbcdm_dllopen (char FAR * path)
{
  return (HDLL) DLL_OPEN (path);
}


HPROC 
_iodbcdm_dllproc (HDLL hdll, char FAR * sym)
{
  return (HPROC) DLL_PROC (hdll, sym);
}


int 
_iodbcdm_dllclose (HDLL hdll)
{
  DLL_CLOSE (hdll);

  return 0;
}


char *
_iodbcdm_dllerror ()
{
  return DLL_ERROR ();
}

