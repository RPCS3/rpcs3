#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellVdec_init();
Module cellVdec(0x0005, cellVdec_init);

// Error Codes
enum
{
	CELL_VDEC_ERROR_ARG		= 0x80610101,
	CELL_VDEC_ERROR_SEQ		= 0x80610102,
	CELL_VDEC_ERROR_BUSY	= 0x80610103,
	CELL_VDEC_ERROR_EMPTY	= 0x80610104,
	CELL_VDEC_ERROR_FATAL	= 0x80610180,
};

int cellVdecQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecQueryAttrEx()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecOpen()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecOpenEx()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecClose()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecStartSeq()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecEndSeq()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecDecodeAu()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecGetPicture()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecGetPicItem()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

int cellVdecSetFrameRate()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

void cellVdec_init()
{
	cellVdec.AddFunc(0xff6f6ebe, cellVdecQueryAttr);
	cellVdec.AddFunc(0xc982a84a, cellVdecQueryAttrEx);
	cellVdec.AddFunc(0xb6bbcd5d, cellVdecOpen);
	cellVdec.AddFunc(0x0053e2d8, cellVdecOpenEx);
	cellVdec.AddFunc(0x16698e83, cellVdecClose);
	cellVdec.AddFunc(0xc757c2aa, cellVdecStartSeq);
	cellVdec.AddFunc(0x824433f0, cellVdecEndSeq);
	cellVdec.AddFunc(0x2bf4ddd2, cellVdecDecodeAu);
	cellVdec.AddFunc(0x807c861a, cellVdecGetPicture);
	cellVdec.AddFunc(0x17c702b9, cellVdecGetPicItem);
	cellVdec.AddFunc(0xe13ef6fc, cellVdecSetFrameRate);
}