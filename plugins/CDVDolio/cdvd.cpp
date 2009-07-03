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

static HMODULE s_hModule;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		s_hModule = hModule;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

CDVDolioApp theApp;

const char* CDVDolioApp::m_ini = "inis/CDVDolio.ini";
const char* CDVDolioApp::m_section = "Settings";

CDVDolioApp::CDVDolioApp()
{
}

HMODULE CDVDolioApp::GetModuleHandle()
{
	return s_hModule;
}

string CDVDolioApp::GetConfig(const char* entry, const char* value)
{
	char buff[4096] = {0};
	GetPrivateProfileString(m_section, entry, value, buff, countof(buff), m_ini);
	return string(buff);
}

void CDVDolioApp::SetConfig(const char* entry, const char* value)
{
	WritePrivateProfileString(m_section, entry, value, m_ini);
}

int CDVDolioApp::GetConfig(const char* entry, int value)
{
	return GetPrivateProfileInt(m_section, entry, value, m_ini);
}

void CDVDolioApp::SetConfig(const char* entry, int value)
{
	char buff[32] = {0};
	itoa(value, buff, 10);
	SetConfig(entry, buff);
}

//

#define PS2E_LT_CDVD 0x08
#define PS2E_CDVD_VERSION 0x0005

EXPORT_C_(uint32) PS2EgetLibType()
{
	return PS2E_LT_CDVD;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return "CDVDolio"; // olio = OverLapped I/O (duh)
}

EXPORT_C_(uint32) PS2EgetLibVersion2(UINT32 type)
{
	const uint32 revision = 0;
	const uint32 build = 1;
	const uint32 minor = 0;

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

bool CDVD::Open(const char* path)
{
	m_label.clear();

	uint32 share = FILE_SHARE_READ;
	uint32 flags = FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED;

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

	m_label = string((const char*)&m_buff[24 + 40], 32);

	// trim

	{
		string::size_type i = m_label.find_first_not_of(' ');
		string::size_type j = m_label.find_last_not_of(' ');

		if(i == string::npos)
		{
			m_label.clear();
		}
		else
		{
			m_label = m_label.substr(i, (j != string::npos ? j + 1 : string::npos) - i);
		}
	}

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

	m_label.clear();
}

const char* CDVD::GetLabel()
{
	return m_label.c_str();
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

uint8* CDVD::GetBuffer()
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
	
uint32 CDVD::GetTN(cdvdTN* buff)
{
	buff->strack = 1;
	buff->etrack = 1;

	return 0;
}

uint32 CDVD::GetTD(BYTE track, cdvdTD* buff)
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

EXPORT_C_(uint32) CDVDinit()
{
	return 0;
}

EXPORT_C CDVDshutdown()
{
}

EXPORT_C_(uint32) CDVDopen(const char* title)
{
	string path;

	int i = theApp.GetConfig("drive", -1);

	if(i >= 'A' && i <= 'Z')
	{
		path = format("\\\\.\\%c:", i);
	}
	else
	{
		path = theApp.GetConfig("iso", "");
	}

	return s_cdvd.Open(path.c_str()) ? 0 : -1;
}

EXPORT_C CDVDclose()
{
	s_cdvd.Close();
}

EXPORT_C_(uint32) CDVDreadTrack(int lsn, int mode)
{
	return s_cdvd.Read(lsn, mode) ? 0 : -1;
}

EXPORT_C_(uint8*) CDVDgetBuffer()
{
	return s_cdvd.GetBuffer();
}

EXPORT_C_(uint32) CDVDreadSubQ(uint32 lsn, cdvdSubQ* subq)
{
	return -1;
}

EXPORT_C_(uint32) CDVDgetTN(cdvdTN* buff)
{
	return s_cdvd.GetTN(buff);
}

EXPORT_C_(uint32) CDVDgetTD(uint8 track, cdvdTD* buff)
{
	return s_cdvd.GetTD(track, buff);
}

EXPORT_C_(uint32) CDVDgetTOC(void* toc)
{
	return -1; // TODO
}

EXPORT_C_(uint32) CDVDgetDiskType()
{
	return CDVD_TYPE_PS2DVD; // TODO
}

EXPORT_C_(uint32) CDVDgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

EXPORT_C_(uint32) CDVDctrlTrayOpen()
{
	return 0;
}

EXPORT_C_(uint32) CDVDctrlTrayClose()
{
	return 0;
}

EXPORT_C CDVDconfigure()
{
	CDVDSettingsDlg dlg;

	if(IDOK == dlg.DoModal())
	{
		CDVDshutdown();
		CDVDinit();
	}
}

EXPORT_C CDVDabout()
{
}

EXPORT_C_(uint32) CDVDtest()
{
	return 0;
}

