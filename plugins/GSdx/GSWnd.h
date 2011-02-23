/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
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

#pragma once

#include "GSVector.h"

#ifdef _WINDOWS

class GSWnd
{
	HWND m_hWnd;

	bool m_managed; // set true when we're attached to a 3rdparty window that's amanged by the emulator
	bool m_frame;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

public:
	GSWnd();
	virtual ~GSWnd();

	bool Create(const string& title, int w, int h);
	bool Attach(void* handle, bool managed = true);
	void Detach();
	bool IsManaged() const {return m_managed;}

	void* GetDisplay() {return m_hWnd;}
	void* GetHandle() {return m_hWnd;}
	GSVector4i GetClientRect();
	bool SetWindowText(const char* title);

	void Show();
	void Hide();
	void HideFrame();
};

#else

#include <X11/Xlib.h>

class GSWnd
{
	Display* m_display;
	Window m_window;

	bool m_managed; // set true when we're attached to a 3rdparty window that's amanged by the emulator
	bool m_frame;

public:
	GSWnd();
	virtual ~GSWnd();

	bool Create(const string& title, int w, int h);
	bool Attach(void* handle, bool managed = true) {return false;}
	void Detach() {}
	bool IsManaged() const {return m_managed;}

	Display* GetDisplay() {return m_display;}
	void* GetHandle() {return (void*)m_window;}
	GSVector4i GetClientRect();
	bool SetWindowText(const char* title);

	void Show();
	void Hide();
	void HideFrame();
};

#endif

