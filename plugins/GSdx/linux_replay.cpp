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
#include <dlfcn.h>

void help()
{
	fprintf(stderr, "Loader gs file\n");
	fprintf(stderr, "ARG1 GSdx plugin\n");
	fprintf(stderr, "ARG2 .gs file\n");
	fprintf(stderr, "ARG3 Ini directory\n");
	exit(1);
}

int main ( int argc, char *argv[] )
{
	if (argc < 3) help();

	void *handle = dlopen(argv[1], RTLD_LAZY|RTLD_GLOBAL);
	if (handle == NULL) {
		fprintf(stderr, "Failed to dlopen plugin %s\n", argv[1]);
		help();
	}

	__attribute__((stdcall)) void (*GSsetSettingsDir_ptr)(const char*);
	__attribute__((stdcall)) void (*GSReplay_ptr)(char*, int);

	*(void**)(&GSsetSettingsDir_ptr) = dlsym(handle, "GSsetSettingsDir");
	*(void**)(&GSReplay_ptr) = dlsym(handle, "GSReplay");

	if ( argc == 4) {
		(void)GSsetSettingsDir_ptr(argv[3]);
	} else if ( argc == 3) {
#ifdef XDG_STD
		char *val = getenv("HOME");
		if (val == NULL) {
			dlclose(handle);
			fprintf(stderr, "Failed to get the home dir\n");
			help();
		}

		std::string ini_dir(val);
		ini_dir += "/.config/pcsx2/inis";

		GSsetSettingsDir_ptr(ini_dir.c_str());
#else
		fprintf(stderr, "default ini dir only supported on XDG\n");
		dlclose(handle);
		help();
#endif
	}

	GSReplay_ptr(argv[2], 12);
	dlclose(handle);
}
