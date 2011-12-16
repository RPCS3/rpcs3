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

#include "GSSetting.h"

class GSDialog
{
	int m_id;

	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
	HWND m_hWnd;

	virtual void OnInit() {}
	virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	virtual bool OnCommand(HWND hWnd, UINT id, UINT code);

public:
	GSDialog(UINT id);
	virtual ~GSDialog() {}

	int GetId() const {return m_id;}

	INT_PTR DoModal();

	string GetText(UINT id);
	int GetTextAsInt(UINT id);

	void SetText(UINT id, const char* str);
	void SetTextAsInt(UINT id, int i);

	void ComboBoxInit(UINT id, const vector<GSSetting>& settings, uint32 selid, uint32 maxid = ~0);
	int ComboBoxAppend(UINT id, const char* str, LPARAM data = 0, bool select = false);
	bool ComboBoxGetSelData(UINT id, INT_PTR& data);
};
