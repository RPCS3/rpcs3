#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"



LOG_CHANNEL(cellSysutilAp);

// Return Codes
enum CellSysutilApError : u32
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

template<>
void fmt_class_string<CellSysutilApError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSUTIL_AP_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_FATAL);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_INVALID_VALUE);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_ZERO_REGISTERED);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_NETIF_DISABLED);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_NETIF_NO_CABLE);
			STR_CASE(CELL_SYSUTIL_AP_ERROR_NETIF_CANNOT_CONNECT);
		}

		return unknown;
	});
}

enum
{
	CELL_SYSUTIL_AP_TITLE_ID_LEN = 9,
	CELL_SYSUTIL_AP_SSID_LEN     = 32,
	CELL_SYSUTIL_AP_WPA_KEY_LEN  = 64
};

struct CellSysutilApTitleId
{
	char data[CELL_SYSUTIL_AP_TITLE_ID_LEN];
	char padding[3];
};

struct CellSysutilApSsid
{
	char data[CELL_SYSUTIL_AP_SSID_LEN + 1];
	char padding[3];
};

struct CellSysutilApWpaKey
{
	char data[CELL_SYSUTIL_AP_WPA_KEY_LEN + 1];
	char padding[3];
};

struct CellSysutilApParam
{
	be_t<s32> type;
	be_t<s32> wlanFlag;
	CellSysutilApTitleId titleId;
	CellSysutilApSsid ssid;
	CellSysutilApWpaKey wpakey;
};

s32 cellSysutilApGetRequiredMemSize()
{
	cellSysutilAp.trace("cellSysutilApGetRequiredMemSize()");
	return 1024*1024; // Return 1 MB as required size
}

error_code cellSysutilApOn(vm::ptr<CellSysutilApParam> pParam, u32 container)
{
	cellSysutilAp.todo("cellSysutilApOn(pParam=*0x%x, container=0x%x)", pParam, container);
	return CELL_OK;
}

error_code cellSysutilApOff()
{
	cellSysutilAp.todo("cellSysutilApOff()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysutilAp)("cellSysutilAp", []()
{
	REG_FUNC(cellSysutilAp, cellSysutilApGetRequiredMemSize);
	REG_FUNC(cellSysutilAp, cellSysutilApOn);
	REG_FUNC(cellSysutilAp, cellSysutilApOff);
});
