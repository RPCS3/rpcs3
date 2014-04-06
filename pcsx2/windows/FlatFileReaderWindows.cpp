/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "AsyncFileReader.h"

FlatFileReader::FlatFileReader(void)
{
	m_blocksize = 2048;
	hOverlappedFile = INVALID_HANDLE_VALUE;
	hEvent = INVALID_HANDLE_VALUE;
	asyncInProgress = false;
}

FlatFileReader::~FlatFileReader(void)
{
	Close();
}

bool FlatFileReader::Open(const wxString& fileName)
{
	m_filename = fileName;

	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	hOverlappedFile = CreateFile(
		fileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
		NULL);

	return hOverlappedFile != INVALID_HANDLE_VALUE;
}

int FlatFileReader::ReadSync(void* pBuffer, uint sector, uint count)
{
	//LARGE_INTEGER offset;
	//offset.QuadPart = sector * (__int64)m_blocksize;
	//
	//DWORD bytesToRead = count * m_blocksize;
	//DWORD bytes;

	//if(!ReadFile(hOverlappedFile, pBuffer, bytesToRead, &bytes, NULL))
	//	return -1;

	//return bytes;
	BeginRead(pBuffer, sector, count);
	return FinishRead();
}

void FlatFileReader::BeginRead(void* pBuffer, uint sector, uint count)
{
	LARGE_INTEGER offset;
	offset.QuadPart = sector * (s64)m_blocksize + m_dataoffset;
	
	DWORD bytesToRead = count * m_blocksize;

	ZeroMemory(&asyncOperationContext, sizeof(asyncOperationContext));
	asyncOperationContext.hEvent = hEvent;
	asyncOperationContext.Offset = offset.LowPart;
	asyncOperationContext.OffsetHigh = offset.HighPart;

	ReadFile(hOverlappedFile, pBuffer, bytesToRead, NULL, &asyncOperationContext);
	asyncInProgress = true;
}

int FlatFileReader::FinishRead(void)
{
	DWORD bytes;
	
	if(!GetOverlappedResult(hOverlappedFile, &asyncOperationContext, &bytes, TRUE))
	{
		asyncInProgress = false;
		return -1;
	}

	asyncInProgress = false;
	return bytes;
}

void FlatFileReader::CancelRead(void)
{
	CancelIo(hOverlappedFile);
}

void FlatFileReader::Close(void)
{
	if(asyncInProgress)
		CancelRead();

	if(hOverlappedFile != INVALID_HANDLE_VALUE)
		CloseHandle(hOverlappedFile);

	if(hEvent != INVALID_HANDLE_VALUE)
		CloseHandle(hEvent);

	hOverlappedFile = INVALID_HANDLE_VALUE;
	hEvent = INVALID_HANDLE_VALUE;
}

uint FlatFileReader::GetBlockCount(void) const
{
	LARGE_INTEGER fileSize;
	fileSize.LowPart = GetFileSize(hOverlappedFile, (DWORD*)&(fileSize.HighPart));

	return (int)(fileSize.QuadPart / m_blocksize);
}
