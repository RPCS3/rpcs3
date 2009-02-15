/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "System.h"
#include "Debug.h"

namespace Console
{
	static HANDLE hConsole = NULL;

	static const int tbl_color_codes[] = 
	{
		0					// black
	,	FOREGROUND_RED | FOREGROUND_INTENSITY
	,	FOREGROUND_GREEN | FOREGROUND_INTENSITY
	,	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
	,	FOREGROUND_BLUE | FOREGROUND_INTENSITY
	,	FOREGROUND_RED | FOREGROUND_BLUE
	,	FOREGROUND_GREEN | FOREGROUND_BLUE
	,	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
	};

	void SetTitle( const string& title )
	{
		if( !hConsole ) return;
		SetConsoleTitle( title.c_str() );
	}

	void Open()
	{
		COORD csize;
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
		SMALL_RECT srect;

		if( hConsole ) return;
		AllocConsole();
		SetConsoleTitle(_("Ps2 Output"));

		csize.X = 100;
		csize.Y = 2048;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), csize);
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo);

		srect = csbiInfo.srWindow;
		srect.Right = srect.Left + 99;
		srect.Bottom = srect.Top + 64;
		SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &srect);
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	void Close()
	{
		if( hConsole == NULL ) return;
		FreeConsole();
		hConsole = NULL;
	}

	__forceinline void __fastcall SetColor( Colors color )
	{
		SetConsoleTextAttribute( hConsole, tbl_color_codes[color] );
	}

	__forceinline void ClearColor()
	{
		SetConsoleTextAttribute( hConsole,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
	}


	// Writes a newline to the console.
	__forceinline bool __fastcall Newline()
	{
		if (hConsole != NULL)
		{
			DWORD tmp;
			WriteConsole(hConsole, "\r\n", 2, &tmp, 0);
		}

		if (emuLog != NULL)
		{
			fputs("\n", emuLog);
			fflush( emuLog );
		}

		return false;
	}

	// Writes an unformatted string of text to the console (fast!)
	// No newline is appended.
	__forceinline bool __fastcall Write( const char* fmt )
	{
		if (hConsole != NULL)
		{
			DWORD tmp;
			WriteConsole(hConsole, fmt, (DWORD)strlen(fmt), &tmp, 0);
		}

		// No flushing here -- only flush after newlines.
		if (emuLog != NULL)
			fputs(fmt, emuLog);

		return false;
	}
}

namespace Msgbox
{
	bool Alert( const char* fmt )
	{
		MessageBox(0, fmt, _("Pcsx2 Msg"), 0);
		return false;
	}

	// Pops up an alert Dialog Box.
	// Replacement for SysMessage.
	bool Alert( const char* fmt, VARG_PARAM dummy, ... )
	{
		varg_assert();

		va_list list;
		va_start(list,dummy);
		MessageBox(0, vfmt_string(fmt,list).c_str(), _("Pcsx2 Msg"), 0);
		va_end(list);

		return false;
	}
}
