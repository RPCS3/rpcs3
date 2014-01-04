#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellPamf_init();
Module cellPamf(0x0012, cellPamf_init);

// Error Codes
enum
{
	CELL_PAMF_ERROR_STREAM_NOT_FOUND	= 0x80610501,
	CELL_PAMF_ERROR_INVALID_PAMF		= 0x80610502,
	CELL_PAMF_ERROR_INVALID_ARG			= 0x80610503,
	CELL_PAMF_ERROR_UNKNOWN_TYPE		= 0x80610504,
	CELL_PAMF_ERROR_UNSUPPORTED_VERSION	= 0x80610505,
	CELL_PAMF_ERROR_UNKNOWN_STREAM		= 0x80610506,
	CELL_PAMF_ERROR_EP_NOT_FOUND		= 0x80610507,
};

int cellPamfGetHeaderSize(mem8_ptr_t pAddr, u64 fileSize, mem64_t pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize(pAddr=0x%x, fileSize=%d, pSize_addr=0x%x)",
		pAddr.GetAddr(), fileSize, pSize.GetAddr());

	pSize = 2048; // PAMF headers seem to be always 2048 bytes in size
	return CELL_OK;
}

int cellPamfGetHeaderSize2(mem8_ptr_t pAddr, u64 fileSize, u32 attribute, mem64_t pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize2(pAddr=0x%x, fileSize=%d, attribute=%d, pSize_addr=0x%x)",
		pAddr.GetAddr(), fileSize, attribute, pSize.GetAddr());

	pSize = 2048; // PAMF headers seem to be always 2048 bytes in size
	return CELL_OK;
}

int cellPamfGetStreamOffsetAndSize()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfVerify()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderInitialize()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetPresentationStartTime()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetPresentationEndTime()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetMuxRateBound()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetNumberOfStreams()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetNumberOfSpecificStreams()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderSetStreamWithIndex()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderSetStreamWithTypeAndChannel()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderSetStreamWithTypeAndIndex()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfStreamTypeToEsFilterId()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetStreamIndex()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetStreamTypeAndChannel()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetEsFilterId()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetStreamInfo()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetNumberOfEp()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetEpIteratorWithIndex()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfReaderGetEpIteratorWithTimeStamp()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfEpIteratorGetEp()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfEpIteratorMove()
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

void cellPamf_init()
{
	cellPamf.AddFunc(0xca8181c1, cellPamfGetHeaderSize);
	cellPamf.AddFunc(0x90fc9a59, cellPamfGetHeaderSize2);
	cellPamf.AddFunc(0x44f5c9e3, cellPamfGetStreamOffsetAndSize);
	cellPamf.AddFunc(0xd1a40ef4, cellPamfVerify);
	cellPamf.AddFunc(0xb8436ee5, cellPamfReaderInitialize);
	cellPamf.AddFunc(0x4de501b1, cellPamfReaderGetPresentationStartTime);
	cellPamf.AddFunc(0xf61609d6, cellPamfReaderGetPresentationEndTime);
	cellPamf.AddFunc(0xdb70296c, cellPamfReaderGetMuxRateBound);
	cellPamf.AddFunc(0x37f723f7, cellPamfReaderGetNumberOfStreams);
	cellPamf.AddFunc(0xd0230671, cellPamfReaderGetNumberOfSpecificStreams);
	cellPamf.AddFunc(0x461534b4, cellPamfReaderSetStreamWithIndex);
	cellPamf.AddFunc(0x03fd2caa, cellPamfReaderSetStreamWithTypeAndChannel);
	cellPamf.AddFunc(0x28b4e2c1, cellPamfReaderSetStreamWithTypeAndIndex);
	cellPamf.AddFunc(0x01067e22, cellPamfStreamTypeToEsFilterId);
	cellPamf.AddFunc(0x041cc708, cellPamfReaderGetStreamIndex);
	cellPamf.AddFunc(0x9ab20793, cellPamfReaderGetStreamTypeAndChannel);
	cellPamf.AddFunc(0x71df326a, cellPamfReaderGetEsFilterId);
	cellPamf.AddFunc(0x67fd273b, cellPamfReaderGetStreamInfo);
	cellPamf.AddFunc(0xd9ea3457, cellPamfReaderGetNumberOfEp);
	cellPamf.AddFunc(0xe8586ec6, cellPamfReaderGetEpIteratorWithIndex);
	cellPamf.AddFunc(0x439fba17, cellPamfReaderGetEpIteratorWithTimeStamp);
	cellPamf.AddFunc(0x1abeb9d6, cellPamfEpIteratorGetEp);
	cellPamf.AddFunc(0x50b83205, cellPamfEpIteratorMove);
}