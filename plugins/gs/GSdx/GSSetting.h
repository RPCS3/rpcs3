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

struct GSSetting 
{
	DWORD id; 
	const TCHAR* name; 
	const TCHAR* note;

	static void InitComboBox(const GSSetting* settings, int count, CComboBox& combobox, DWORD sel, DWORD maxid = ~0)
	{
		for(int i = 0; i < count; i++)
		{
			if(settings[i].id <= maxid)
			{
				CString str = settings[i].name;
				if(settings[i].note != NULL) str = str + _T(" (") + settings[i].note + _T(")");
				int item = combobox.AddString(str);
				combobox.SetItemData(item, settings[i].id);
				if(settings[i].id == sel) combobox.SetCurSel(item);
			}
		}
	}

};
