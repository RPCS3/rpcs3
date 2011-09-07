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
 
 ding - Play a windows notification sound
 
\*----------------------------------------------------------*/	

#include <windows.h>
#include <stdio.h>

LONG sounds[]={
	MB_OK,
	MB_ICONINFORMATION,
	MB_ICONWARNING,
	MB_ICONERROR,
	MB_ICONQUESTION,
	0xFFFFFFFF //"Simple" (with fallback to PC speaker beep)
	};

int usage( int res )
{
	printf(	"Usage: ding [-h] [<n>=1]\n"
			"Where <n> is a number as follows:\n"
			"1-Default, 2-Information, 3-Warning, 4-Error, 5-Question, 6-Simple\n");
	return res;
}

int main( int argc, char* argv[] ){
	LONG s = sounds[0];
	
	if( argc > 2 )
		return usage( 1 );
	
	if( argc == 2 ){
		if( !strcmp( argv[1], "-h" ) )
			return usage( 0 );
		
		if( atoi( argv[1] ) < 1 || atoi( argv[1] ) > sizeof(sounds)/sizeof(LONG) )
			return usage( 1 );
		
		s=sounds[atoi( argv[1] ) - 1];
	}
	
	MessageBeep( s );
	Sleep( 250 );
	return 0;
}