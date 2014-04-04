#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSysutilAp_init();
Module cellSysutilAp(0x0039, cellSysutilAp_init);

// Return Codes
enum
{
	CELL_SYSUTIL_AP_ERROR_OUT_OF_MEMORY        = 0x8002cd00,
	CELL_SYSUTIL_AP_ERROR_FATAL                = 0x8002cd01,
	CELL_SYSUTIL_AP_ERROR_INVALID_VALUE        = 0x8002cd02,
	CELL_SYSUTIL_AP_ERROR_NOT_INITIALIZED      = 0x8002cd03,
	CELL_SYSUTIL_AP_ERROR_ZERO_REGISTERED      = 0x8002cd13,
	CELL_SYSUTIL_AP_ERROR_NETIF_DISABLED       = 0x8002cd14,
	CELL_SYSUTIL_AP_ERROR_NETIF_NO_CABLE       = 0x8002cd15,
	CELL_SYSUTIL_AP_ERROR_NETIF_CANNOT_CONNECT = 0x8002cd16,
};

s32 cellSysutilApGetRequiredMemSize()
{
	cellSysutilAp.Log("cellSysutilApGetRequiredMemSize()");
	return 1024*1024; // Return 1 MB as required size
}

int cellSysutilApOn()
{
	UNIMPLEMENTED_FUNC(cellSysutilAp);
	return CELL_OK;
}

int cellSysutilApOff()
{
	UNIMPLEMENTED_FUNC(cellSysutilAp);
	return CELL_OK;
}

void cellSysutilAp_init()
{
	cellSysutilAp.AddFunc(0x9e67e0dd, cellSysutilApGetRequiredMemSize);
	cellSysutilAp.AddFunc(0x3343824c, cellSysutilApOn);
	cellSysutilAp.AddFunc(0x90c2bb19, cellSysutilApOff);
}