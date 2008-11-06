/* 
 *	Copyright (C) 2003-2005 Gabest
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
 *	Special Notes: 
 *
 */

#include "stdafx.h"
#include "afxwin.h"
#include "GSdx9.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

// CGSdx9App

BEGIN_MESSAGE_MAP(CGSdx9App, CWinApp)
END_MESSAGE_MAP()

D3DDEVTYPE CGSdx9App::D3DDEVTYPE_X;

// CGSdx9App construction

CGSdx9App::CGSdx9App()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CGSdx9App object

CGSdx9App theApp;

// CGSdx9App initialization

BOOL CGSdx9App::InitInstance()
{
	__super::InitInstance();

	D3DDEVTYPE_X = D3DDEVTYPE_HAL;

	if(GetSystemMetrics(SM_REMOTESESSION))
	{
		D3DDEVTYPE_X = D3DDEVTYPE_REF;
	}

	return TRUE;
}
