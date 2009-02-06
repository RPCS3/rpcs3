/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>

#define NUM_BREAKPOINTS     8

extern BOOL APIENTRY DebuggerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern BOOL APIENTRY MemoryProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern void RefreshDebugger(void);

extern unsigned long opcode_addr;

struct breakpoints
{
    int type;
    unsigned long value;
};


LRESULT CALLBACK R5900reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK COP0reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK COP1reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK COP2Freg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK COP2Creg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK VU1Freg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK VU1Creg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void UpdateRegs(void);
int CreatePropertySheet(HWND hwndOwner);
