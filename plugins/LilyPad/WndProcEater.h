/*  LilyPad - Pad plugin for PS2 Emulator
 *  Copyright (C) 2002-2014  PCSX2 Dev Team/ChickenLiver
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Found- ation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with PCSX2.  If not, see <http://www.gnu.org/licenses/>.
 */

#define EATPROC_NO_UPDATE_WHILE_UPDATING_DEVICES	1

/* Need this to let window be subclassed multiple times but still clean up nicely.
 */
enum ExtraWndProcResult {
	CONTINUE_BLISSFULLY,
	// Calls ReleaseExtraProc without messing up order.
	CONTINUE_BLISSFULLY_AND_RELEASE_PROC,
	USE_DEFAULT_WND_PROC,
	NO_WND_PROC
};

typedef ExtraWndProcResult (*ExtraWndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *out);

struct ExtraWndProcInfo {
	ExtraWndProc proc;
	DWORD flags;
};

class WndProcEater
{
public:
	HWND hWndEaten;
	WNDPROC eatenWndProc;
	ExtraWndProcInfo* extraProcs;
	int numExtraProcs;

	HANDLE hMutex;

public:
	WndProcEater();
	virtual ~WndProcEater() throw();

	bool SetWndHandle(HWND hWnd);
	void Eat(ExtraWndProc proc, DWORD flags);
	void ReleaseExtraProc(ExtraWndProc proc);
	void Release();

	LRESULT _OverrideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern WndProcEater hWndGSProc;
extern WndProcEater hWndTopProc;
extern WndProcEater hWndButtonProc;
