/*----------------------------------------------------------*\
 *
 *	Copyright (C) 2011 Avi Halachmi
 *	avihpit (at) yahoo (dot) com
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
 
 waitForChange - watches a file for modifications and exit when modified
 
\*----------------------------------------------------------*/	

#include <stdio.h>
#include <sys/stat.h>
#include <windows.h>

int usage(){
	printf("Usage: WaitForChange <filename> [<ms between probes>=100]\n");
	return 1;
}

int main(int argc, char** argv){
	struct stat buf;
	struct stat test;

	if( argc<=1 )
		return usage();
	
	if(stat(argv[1], &buf)){
		printf("Error (not found?): '%s'\n", argv[1]);
		return usage();
	};
	
	do{
		Sleep(argc>=2?(atoi(argv[2])?atoi(argv[2]):100):100);
		if(stat(argv[1], &test)){
			printf("Stat error\n");
			return 1;
		};
	} while (test.st_mtime == buf.st_mtime);
	
	return 0;
}