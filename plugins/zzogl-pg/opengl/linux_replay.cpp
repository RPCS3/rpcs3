/*
 *	Copyright (C) 20011-2012 Hainaut gregory
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GS.h"

EXPORT_C_(void) GSsetLogDir(const char* dir);
EXPORT_C_(void) GSsetSettingsDir(const char* dir);
EXPORT_C_(void) GSReplay(char* lpszCmdLine);


void help()
{
	fprintf(stderr, "Loader gs file\n");
	fprintf(stderr, "ARG1 Ini directory\n");
	fprintf(stderr, "ARG2 .gs file\n");
	exit(1);
}

int main ( int argc, char *argv[] )
{
  if ( argc < 2 ) help();

  GSsetSettingsDir(argv[1]);
  GSsetLogDir("/tmp");
  GSReplay(argv[2]);
}

