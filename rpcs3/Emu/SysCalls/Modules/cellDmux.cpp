#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellDmux_init();
Module cellDmux(0x0007, cellDmux_init);

// Error Codes
enum
{
	CELL_DMUX_ERROR_ARG		= 0x80610201,
	CELL_DMUX_ERROR_SEQ		= 0x80610202,
	CELL_DMUX_ERROR_BUSY	= 0x80610203,
	CELL_DMUX_ERROR_EMPTY	= 0x80610204,
	CELL_DMUX_ERROR_FATAL	= 0x80610205,
};

int cellDmuxQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxQueryAttr2()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxOpen()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxOpenEx()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxOpen2()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxClose()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxSetStream()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxResetStream()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxQueryEsAttr()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxQueryEsAttr2()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxEnableEs()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxDisableEs()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxResetEs()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxGetAu()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxPeekAu()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxGetAuEx()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxPeekAuEx()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxReleaseAu()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

int cellDmuxFlushEs()
{
	UNIMPLEMENTED_FUNC(cellDmux);
	return CELL_OK;
}

void cellDmux_init()
{
	cellDmux.AddFunc(0xa2d4189b, cellDmuxQueryAttr);
	cellDmux.AddFunc(0x3f76e3cd, cellDmuxQueryAttr2);
	cellDmux.AddFunc(0x68492de9, cellDmuxOpen);
	cellDmux.AddFunc(0xf6c23560, cellDmuxOpenEx);
	cellDmux.AddFunc(0x11bc3a6c, cellDmuxOpen2);
	cellDmux.AddFunc(0x8c692521, cellDmuxClose);
	cellDmux.AddFunc(0x04e7499f, cellDmuxSetStream);
	cellDmux.AddFunc(0x5d345de9, cellDmuxResetStream);
	cellDmux.AddFunc(0xccff1284, cellDmuxResetStreamAndWaitDone);
	cellDmux.AddFunc(0x02170d1a, cellDmuxQueryEsAttr);
	cellDmux.AddFunc(0x52911bcf, cellDmuxQueryEsAttr2);
	cellDmux.AddFunc(0x7b56dc3f, cellDmuxEnableEs);
	cellDmux.AddFunc(0x05371c8d, cellDmuxDisableEs);
	cellDmux.AddFunc(0x21d424f0, cellDmuxResetEs);
	cellDmux.AddFunc(0x42c716b5, cellDmuxGetAu);
	cellDmux.AddFunc(0x2750c5e0, cellDmuxPeekAu);
	cellDmux.AddFunc(0x2c9a5857, cellDmuxGetAuEx);
	cellDmux.AddFunc(0x002e8da2, cellDmuxPeekAuEx);
	cellDmux.AddFunc(0x24ea6474, cellDmuxReleaseAu);
	cellDmux.AddFunc(0xebb3b2bd, cellDmuxFlushEs);
}