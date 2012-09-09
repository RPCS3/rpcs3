/*
 *	Copyright (C) 2011-2012 Hainaut gregory
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"

EXPORT_C GSsetSettingsDir(const char* dir);
EXPORT_C GSReplay(char* lpszCmdLine, int renderer);


void help()
{
	fprintf(stderr, "Loader gs file\n");
	fprintf(stderr, "ARG1 Ini directory\n");
	fprintf(stderr, "ARG2 .gs file\n");
	exit(1);
}

int main ( int argc, char *argv[] )
{
  if ( argc == 3) {
	  GSsetSettingsDir(argv[1]);
	  GSReplay(argv[2], 12);
  } else if ( argc == 2) {
#ifdef XDG_STD
	  std::string home("HOME");
	  char * val = getenv( home.c_str() );
	  if (val == NULL) {
		  fprintf(stderr, "Failed to get the home dir\n");
		  help();
	  }

	  std::string ini_dir(val);
	  ini_dir += "/.config/pcsx2/inis";

	  GSsetSettingsDir(ini_dir.c_str());
	  GSReplay(argv[1], 12);
#else
	  fprintf(stderr, "default ini dir only supported on XDG\n");
	  help();
#endif
  } else
	  help();

}
