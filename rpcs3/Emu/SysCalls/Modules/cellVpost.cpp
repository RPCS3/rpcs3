#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellVpost.h"

void cellVpost_init();
Module cellVpost(0x0008, cellVpost_init);

int cellVpostQueryAttr()
{
	UNIMPLEMENTED_FUNC(cellVpost);
	return CELL_OK;
}

int cellVpostOpen()
{
	UNIMPLEMENTED_FUNC(cellVpost);
	return CELL_OK;
}

int cellVpostOpenEx()
{
	UNIMPLEMENTED_FUNC(cellVpost);
	return CELL_OK;
}

int cellVpostClose()
{
	UNIMPLEMENTED_FUNC(cellVpost);
	return CELL_OK;
}

int cellVpostExec()
{
	UNIMPLEMENTED_FUNC(cellVpost);
	return CELL_OK;
}

void cellVpost_init()
{
	cellVpost.AddFunc(0x95e788c3, cellVpostQueryAttr);
	cellVpost.AddFunc(0xcd33f3e2, cellVpostOpen);
	cellVpost.AddFunc(0x40524325, cellVpostOpenEx);
	cellVpost.AddFunc(0x10ef39f6, cellVpostClose);
	cellVpost.AddFunc(0xabb8cc3d, cellVpostExec);
}