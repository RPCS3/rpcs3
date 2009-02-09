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

#include "stdafx.h"
#include "GSdx.h"

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

BEGIN_MESSAGE_MAP(GSdxApp, CWinApp)
END_MESSAGE_MAP()

GSdxApp::GSdxApp()
{
}

GSdxApp theApp;

BOOL GSdxApp::InitInstance()
{
	__super::InitInstance();

	SetRegistryKey(_T("Gabest"));

	CString str;
	GetModuleFileName(AfxGetInstanceHandle(), str.GetBuffer(MAX_PATH), MAX_PATH);
	str.ReleaseBuffer();

	CPath path(str);
	path.RenameExtension(_T(".ini"));
	
	CPath fn = path;
	fn.StripPath();

	path.RemoveFileSpec();
	path.Append(_T("..\\inis"));
	CreateDirectory(path, NULL);
	path.Append(fn);

	if(m_pszRegistryKey)
	{
		free((void*)m_pszRegistryKey);
	}

	m_pszRegistryKey = NULL;
	
	if(m_pszProfileName)
	{
		free((void*)m_pszProfileName);
	}

	m_pszProfileName = _tcsdup((LPCTSTR)path);

	return TRUE;
}