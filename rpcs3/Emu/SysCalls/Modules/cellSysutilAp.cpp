#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

namespace vm { using namespace ps3; }

extern Module cellSysutilAp;

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

s32 cellSysutilApOn()
{
	UNIMPLEMENTED_FUNC(cellSysutilAp);
	return CELL_OK;
}

s32 cellSysutilApOff()
{
	UNIMPLEMENTED_FUNC(cellSysutilAp);
	return CELL_OK;
}

Module cellSysutilAp("cellSysutilAp", []()
{
	REG_FUNC(cellSysutilAp, cellSysutilApGetRequiredMemSize);
	REG_FUNC(cellSysutilAp, cellSysutilApOn);
	REG_FUNC(cellSysutilAp, cellSysutilApOff);
});
