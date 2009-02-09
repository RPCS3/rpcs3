/* 
 *	Copyright (C) 2007 Gabest
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
#include "cdvd.h"
#include "SettingsDlg.h"
#include <winioctl.h>

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

BEGIN_MESSAGE_MAP(cdvdApp, CWinApp)
END_MESSAGE_MAP()

cdvdApp::cdvdApp()
{
}

cdvdApp theApp;

BOOL cdvdApp::InitInstance()
{
	__super::InitInstance();

	SetRegistryKey(_T("Gabest"));

	return TRUE;
}

//

#define PS2E_LT_CDVD 0x08
#define PS2E_CDVD_VERSION 0x0005

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_CDVD;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return "CDVDolio"; // olio = OverLapped I/O (duh)
}

EXPORT_C_(UINT32) PS2EgetLibVersion2(UINT32 type)
{
	const UINT32 revision = 0;
	const UINT32 build = 1;
	const UINT32 minor = 0;

	return (build << 0) | (revision << 8) | (PS2E_CDVD_VERSION << 16) | (minor << 24);
}

//

CDVD::CDVD() 
	: m_hFile(INVALID_HANDLE_VALUE)
{
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_cache.pending = false;
	m_cache.count = 0;
}

CDVD::~CDVD()
{
	CloseHandle(m_overlapped.hEvent);
}

LARGE_INTEGER CDVD::MakeOffset(int lsn)
{
	LARGE_INTEGER offset;
	offset.QuadPart = (LONGLONG)lsn * m_block.size + m_block.offset;
	return offset;
}

bool CDVD::SyncRead(int lsn)
{
	return Read(lsn) && GetBuffer();
}

bool CDVD::Open(CString path)
{
	m_label.Empty();

	DWORD share = FILE_SHARE_READ;
	DWORD flags = FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED;

	m_hFile = CreateFile(path, GENERIC_READ, share, NULL, OPEN_EXISTING, flags, (HANDLE)NULL);

	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_block.count = 0;
	m_block.size = 2048;
	m_block.offset = 0;

	GET_LENGTH_INFORMATION info;
	DWORD ret = 0;

	if(GetFileSizeEx(m_hFile, &info.Length) || DeviceIoControl(m_hFile, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &info, sizeof(info), &ret, NULL))
	{
		m_block.count = (int)(info.Length.QuadPart / m_block.size);
	}
	else
	{
		Close();

		return false;
	}

	if(!SyncRead(16) || memcmp(&m_buff[24 + 1], "CD001", 5) != 0)
	{
		Close();

		return false;
	}

	m_label = CString(CStringA((char*)&m_buff[24 + 40], 32));
	m_label.Trim();

	// m_block.count = *(DWORD*)&m_buff[24 + 80];

	return true;
}

void CDVD::Close()
{
	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		CancelIo(m_hFile);
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_cache.pending = false;
	m_cache.count = 0;

	m_label.Empty();
}

CString CDVD::GetLabel()
{
	return m_label;
}

bool CDVD::Read(int lsn, int mode)
{
	if(mode != CDVD_MODE_2048) return false;

	if(lsn < 0) lsn += m_block.count;

	if(lsn < 0 || lsn >= m_block.count) return false;

	if(m_cache.pending)
	{
		CancelIo(m_hFile);
		ResetEvent(m_overlapped.hEvent);
		m_cache.pending = false;
	}

	if(lsn >= m_cache.start && lsn < m_cache.start + m_cache.count)
	{
		memcpy(&m_buff[24], &m_cache.buff[(lsn - m_cache.start) * 2048], 2048);

		return true;
	}

	m_cache.pending = true;
	m_cache.start = lsn;
	m_cache.count = min(CACHE_BLOCK_COUNT, m_block.count - m_cache.start);

	LARGE_INTEGER offset = MakeOffset(lsn);

	m_overlapped.Offset = offset.LowPart;
	m_overlapped.OffsetHigh = offset.HighPart;

	if(!ReadFile(m_hFile, m_cache.buff, m_cache.count * 2048, NULL, &m_overlapped))
	{
		switch(GetLastError())
		{
		case ERROR_IO_PENDING:
			break;
		case ERROR_HANDLE_EOF:
			return false;
		}
	}

	return true;
}

BYTE* CDVD::GetBuffer()
{
	DWORD size = 0;

	if(m_cache.pending)
	{
		if(GetOverlappedResult(m_hFile, &m_overlapped, &size, TRUE))
		{
			memcpy(&m_buff[24], m_cache.buff, 2048);

			m_cache.pending = false;
		}
		else
		{
			return NULL;
		}
	}

	return &m_buff[24];
}
	
UINT32 CDVD::GetTN(cdvdTN* buff)
{
	buff->strack = 1;
	buff->etrack = 1;

	return 0;
}

UINT32 CDVD::GetTD(BYTE track, cdvdTD* buff)
{
	if(track == 0)
	{
		buff->lsn = m_block.count;
	}
	else
	{
		buff->type = CDVD_MODE1_TRACK;
		buff->lsn = 0;
	}

	return 0;
}

static CDVD s_cdvd;

//

EXPORT_C_(UINT32) CDVDinit()
{
	return 0;
}

EXPORT_C CDVDshutdown()
{
}

EXPORT_C_(UINT32) CDVDopen(const char* title)
{
	CString path;

	int i = AfxGetApp()->GetProfileInt(_T("Settings"), _T("drive"), -1);

	if(i >= 'A' && i <= 'Z')
	{
		path.Format(_T("\\\\.\\%c:"), i);
	}
	else
	{
		path = AfxGetApp()->GetProfileString(_T("Settings"), _T("iso"), _T(""));
	}

	return s_cdvd.Open(path) ? 0 : -1;
}

EXPORT_C CDVDclose()
{
	s_cdvd.Close();
}

EXPORT_C_(UINT32) CDVDreadTrack(int lsn, int mode)
{
	return s_cdvd.Read(lsn, mode) ? 0 : -1;
}

EXPORT_C_(BYTE*) CDVDgetBuffer()
{
	return s_cdvd.GetBuffer();
}

EXPORT_C_(UINT32) CDVDreadSubQ(UINT32 lsn, cdvdSubQ* subq)
{
	return -1;
}

EXPORT_C_(UINT32) CDVDgetTN(cdvdTN* buff)
{
	return s_cdvd.GetTN(buff);
}

EXPORT_C_(UINT32) CDVDgetTD(BYTE track, cdvdTD* buff)
{
	return s_cdvd.GetTD(track, buff);
}

EXPORT_C_(UINT32) CDVDgetTOC(void* toc)
{
	return -1; // TODO
}

EXPORT_C_(UINT32) CDVDgetDiskType()
{
	return CDVD_TYPE_PS2DVD; // TODO
}

EXPORT_C_(UINT32) CDVDgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

EXPORT_C_(UINT32) CDVDctrlTrayOpen()
{
	return 0;
}

EXPORT_C_(UINT32) CDVDctrlTrayClose()
{
	return 0;
}

EXPORT_C CDVDconfigure()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CSettingsDlg dlg;

	if(IDOK == dlg.DoModal())
	{
		CDVDshutdown();
		CDVDinit();
	}
}

EXPORT_C CDVDabout()
{
}

EXPORT_C_(UINT32) CDVDtest()
{
	return 0;
}

