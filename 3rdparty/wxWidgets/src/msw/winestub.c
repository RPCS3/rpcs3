/****************************************************************************
 * Name:        winestub.cpp
 * Purpose:     wxWINE module mapping main() to WinMain()
 * Author:      Robert Roebling
 * Created:     04/01/98
 * RCS-ID:      $Id: winestub.c 29038 2004-09-07 11:11:05Z ABX $
 * Copyright:   (c) Robert Roebling
 * Licence:     wxWidgets Licence
 *****************************************************************************/

#include <string.h>
#include "winuser.h"
#include "xmalloc.h"

extern int PASCAL WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
extern HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );

/* Most Windows C/C++ compilers use something like this to */
/* access argc and argv globally: */
int _ARGC;
char **_ARGV;

int main( int argc, char *argv [] )
{
  HINSTANCE hInstance;
  LPSTR lpszCmdParam;
  int i, len = 0;
  _ARGC = argc;
  _ARGV = (char **)argv;

  if (!(hInstance = MAIN_WinelibInit( &argc, argv ))) return 0;

  /* Alloc szCmdParam */
  for (i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
  lpszCmdParam = (LPSTR) xmalloc(len + 1);
  /* Concatenate arguments */
  if (argc > 1) strcpy(lpszCmdParam, argv[1]);
  else lpszCmdParam[0] = '\0';
  for (i = 2; i < argc; i++) strcat(strcat(lpszCmdParam, " "), argv[i]);

  return WinMain (hInstance,    /* hInstance */
          0,            /* hPrevInstance */
          lpszCmdParam, /* lpszCmdParam */
          SW_NORMAL);   /* nCmdShow */
}
