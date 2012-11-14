/*
 *  misc.c
 *
 *  $Id: misc.c 2613 1999-06-01 15:32:12Z VZ $
 *
 *  Miscellaneous functions
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

#include	<stdio.h>
#include        <unistd.h>

static int
upper_strneq (
    char *s1,
    char *s2,
    int n)
{
  int i;
  char c1, c2;

  for (i = 1; i < n; i++)
    {
      c1 = s1[i];
      c2 = s2[i];

      if (c1 >= 'a' && c1 <= 'z')
	{
	  c1 += ('A' - 'a');
	}
      else if (c1 == '\n')
	{
	  c1 = '\0';
	}

      if (c2 >= 'a' && c2 <= 'z')
	{
	  c2 += ('A' - 'a');
	}
      else if (c2 == '\n')
	{
	  c2 = '\0';
	}

      if ((c1 - c2) || !c1 || !c2)
	{
	  break;
	}
    }

  return (int) !(c1 - c2);
}

static char *			/* return new position in input str */
readtoken (
    char *istr,			/* old position in input buf */
    char *obuf)			/* token string ( if "\0", then finished ) */
{
  char *start = obuf;

  /* Skip leading white space */
  while (*istr == ' ' || *istr == '\t')
    istr++;

  for (; *istr && *istr != '\n'; istr++)
    {
      char c, nx;

      c = *(istr);
      nx = *(istr + 1);

      if (c == ';')
	{
	  for (; *istr && *istr != '\n'; istr++);
	  break;
	}
      *obuf = c;
      obuf++;

      if (nx == ';' || nx == '=' || c == '=')
	{
	  istr++;
	  break;
	}
    }
  *obuf = '\0';

  /* Trim end of token */
  for (; obuf > start && (*(obuf - 1) == ' ' || *(obuf - 1) == '\t');)
    *--obuf = '\0';

  return istr;
}

#if	!defined(WINDOWS) && !defined(WIN32) && !defined(OS2)
# include	<pwd.h>
# define	UNIX_PWD
#endif

char *
_iodbcdm_getinifile (char *buf, int size)
{
  int i, j;
  char *ptr;

  j = STRLEN ("/odbc.ini") + 1;

  if (size < j)
    {
      return NULL;
    }

#if	!defined(UNIX_PWD)

  i = GetWindowsDirectory ((LPSTR) buf, size);

  if (i == 0 || i > size - j)
    {
      return NULL;
    }

  sprintf (buf + i, "/odbc.ini");

  return buf;
#else
  if ((ptr = getenv ("ODBCINI")) != NULL)
    {
      strcpy (buf, ptr);
      return buf;
    }

  if ((ptr = getenv ("IODBCINI")) != NULL)
    {
      strcpy (buf, ptr);
      return buf;
    }

  if ((ptr = getenv ("HOME")) == NULL)
    {
      ptr = (char *) getpwuid (getuid ());

      if (ptr == NULL)
	{
	  return NULL;
	}

      ptr = ((struct passwd *) ptr)->pw_dir;
    }

  if (ptr == NULL || *ptr == '\0')
    {
      ptr = "/home";
    }

  if (size < STRLEN (ptr) + j)
    {
      return NULL;
    }

  sprintf (buf, "%s%s", ptr, "/.odbc.ini");
  /* i.e. searching ~/.odbc.ini */
#endif

  return buf;
}


/* 
 *  read odbc init file to resolve the value of specified
 *  key from named or defaulted dsn section 
 */
char *
_iodbcdm_getkeyvalbydsn (
    char *dsn,
    int dsnlen,
    char *keywd,
    char *value,
    int size)
{
  char buf[1024];
  char dsntk[SQL_MAX_DSN_LENGTH + 3] = {'[', '\0'};
  char token[1024];		/* large enough */
  FILE *file;
  char pathbuf[1024];
  char *path;

#define DSN_NOMATCH	0
#define DSN_NAMED	1
#define DSN_DEFAULT	2

  int dsnid = DSN_NOMATCH;
  int defaultdsn = DSN_NOMATCH;

  if (dsn == NULL || *dsn == 0)
    {
      dsn = "default";
      dsnlen = STRLEN (dsn);
    }

  if (dsnlen == SQL_NTS)
    {
      dsnlen = STRLEN (dsn);
    }

  if (dsnlen <= 0 || keywd == NULL || buf == 0 || size <= 0)
    {
      return NULL;
    }

  if (dsnlen > sizeof (dsntk) - 2)
    {
      return NULL;
    }

  value[0] = '\0';

  STRNCAT (dsntk, dsn, dsnlen);
  STRCAT (dsntk, "]");

  dsnlen = dsnlen + 2;

  path = _iodbcdm_getinifile (pathbuf, sizeof (pathbuf));

  if (path == NULL)
    {
      return NULL;
    }

  file = (FILE *) fopen (path, "r");

  if (file == NULL)
    {
      return NULL;
    }

  for (;;)
    {
      char *str;

      str = fgets (buf, sizeof (buf), file);

      if (str == NULL)
	{
	  break;
	}

      if (*str == '[')
	{
	  if (upper_strneq (str, "[default]", STRLEN ("[default]")))
	    {
	      /* we only read first dsn default dsn
	       * section (as well as named dsn).
	       */
	      if (defaultdsn == DSN_NOMATCH)
		{
		  dsnid = DSN_DEFAULT;
		  defaultdsn = DSN_DEFAULT;
		}
	      else
		{
		  dsnid = DSN_NOMATCH;
		}

	      continue;
	    }
	  else if (upper_strneq (str, dsntk, dsnlen))
	    {
	      dsnid = DSN_NAMED;
	    }
	  else
	    {
	      dsnid = DSN_NOMATCH;
	    }

	  continue;
	}
      else if (dsnid == DSN_NOMATCH)
	{
	  continue;
	}

      str = readtoken (str, token);

      if (upper_strneq (keywd, token, STRLEN (keywd)))
	{
	  str = readtoken (str, token);

	  if (!STREQ (token, "="))
	    /* something other than = */
	    {
	      continue;
	    }

	  str = readtoken (str, token);

	  if (STRLEN (token) > size - 1)
	    {
	      break;
	    }

	  STRNCPY (value, token, size);
	  /* copy the value(i.e. next token) to buf */

	  if (dsnid != DSN_DEFAULT)
	    {
	      break;
	    }
	}
    }

  fclose (file);

  return (*value) ? value : NULL;
}


char *
_iodbcdm_getkeyvalinstr (
    char *cnstr,
    int cnlen,
    char *keywd,
    char *value,
    int size)
{
  char token[1024] = {'\0'};
  int flag = 0;

  if (cnstr == NULL || value == NULL
      || keywd == NULL || size < 1)
    {
      return NULL;
    }

  if (cnlen == SQL_NTS)
    {
      cnlen = STRLEN (cnstr);
    }

  if (cnlen <= 0)
    {
      return NULL;
    }

  for (;;)
    {
      cnstr = readtoken (cnstr, token);

      if (*token == '\0')
	{
	  break;
	}

      if (STREQ (token, ";"))
	{
	  flag = 0;
	  continue;
	}

      switch (flag)
	{
	case 0:
	  if (upper_strneq (token, keywd, strlen (keywd)))
	    {
	      flag = 1;
	    }
	  break;

	case 1:
	  if (STREQ (token, "="))
	    {
	      flag = 2;
	    }
	  break;

	case 2:
	  if (size < strlen (token) + 1)
	    {
	      return NULL;
	    }

	  STRNCPY (value, token, size);

	  return value;

	default:
	  break;
	}
    }

  return NULL;
}
