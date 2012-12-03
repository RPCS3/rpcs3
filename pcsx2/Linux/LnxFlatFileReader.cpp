#include "PrecompiledHeader.h"
#include "AsyncFileReader.h"

FlatFileReader::FlatFileReader(void)
{
	m_blocksize = 2048;
	//hOverlappedFile = INVALID_HANDLE_VALUE;
	//hEvent = INVALID_HANDLE_VALUE;
	asyncInProgress = false;
}

FlatFileReader::~FlatFileReader(void)
{
	Close();
}

bool FlatFileReader::Open(const wxString& fileName)
{
	m_filename = fileName;

	//hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	//hOverlappedFile = CreateFile(
	//	fileName,
	//	GENERIC_READ,
	//	FILE_SHARE_READ,
	//	NULL,
	//	OPEN_EXISTING,
	//	FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
	//	NULL);

	//return hOverlappedFile != INVALID_HANDLE_VALUE;
	return true;
}

int FlatFileReader::ReadSync(void* pBuffer, uint sector, uint count)
{
	BeginRead(pBuffer, sector, count);
	return FinishRead();
}

void FlatFileReader::BeginRead(void* pBuffer, uint sector, uint count)
{
	LARGE_INTEGER offset;
	//offset.QuadPart = sector * (__int64)m_blocksize;
	
	//DWORD bytesToRead = count * m_blocksize;

	//ZeroMemory(&asyncOperationContext, sizeof(asyncOperationContext));
	//asyncOperationContext.hEvent = hEvent;
	//asyncOperationContext.Offset = offset.LowPart;
	//asyncOperationContext.OffsetHigh = offset.HighPart;

	//ReadFile(hOverlappedFile, pBuffer, bytesToRead, NULL, &asyncOperationContext);
	asyncInProgress = true;
}

int FlatFileReader::FinishRead(void)
{
	//DWORD bytes;
	//
	//if(!GetOverlappedResult(hOverlappedFile, &asyncOperationContext, &bytes, TRUE))
	//{
	//	asyncInProgress = false;
	//	return -1;
	//}

	asyncInProgress = false;
	//return bytes;
	return 0;
}

void FlatFileReader::CancelRead(void)
{
	//CancelIo(hOverlappedFile);
}

void FlatFileReader::Close(void)
{
	//if(asyncInProgress)
	//	CancelRead();

	//if(hOverlappedFile != INVALID_HANDLE_VALUE)
	//	CloseHandle(hOverlappedFile);

	//if(hEvent != INVALID_HANDLE_VALUE)
	//	CloseHandle(hEvent);

	//hOverlappedFile = INVALID_HANDLE_VALUE;
	//hEvent = INVALID_HANDLE_VALUE;
}

int FlatFileReader::GetBlockCount(void) const
{
	LARGE_INTEGER fileSize;
	//fileSize.LowPart = GetFileSize(hOverlappedFile, (DWORD*)&(fileSize.HighPart));

	//return (int)(fileSize.QuadPart / m_blocksize);
	return 0;
}
