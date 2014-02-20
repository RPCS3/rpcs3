#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellAdec.h"

void cellAdec_init();
Module cellAdec(0x0006, cellAdec_init);

int cellAdecQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecOpen()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecOpenEx()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecClose()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecStartSeq()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecEndSeq()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecDecodeAu()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecGetPcm()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

int cellAdecGetPcmItem()
{
	UNIMPLEMENTED_FUNC(cellAdec);
	return CELL_OK;
}

void cellAdec_init()
{
	cellAdec.AddFunc(0x7e4a4a49, cellAdecQueryAttr);
	cellAdec.AddFunc(0xd00a6988, cellAdecOpen);
	cellAdec.AddFunc(0x8b5551a4, cellAdecOpenEx);
	cellAdec.AddFunc(0x847d2380, cellAdecClose);
	cellAdec.AddFunc(0x487b613e, cellAdecStartSeq);
	cellAdec.AddFunc(0xe2ea549b, cellAdecEndSeq);
	cellAdec.AddFunc(0x1529e506, cellAdecDecodeAu);
	cellAdec.AddFunc(0x97ff2af1, cellAdecGetPcm);
	cellAdec.AddFunc(0xbd75f78b, cellAdecGetPcmItem);
}