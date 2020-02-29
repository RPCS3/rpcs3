#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libmedi);

s32 cellMediatorCloseContext()
{
	libmedi.todo("cellMediatorCloseContext");
	return CELL_OK;
}

s32 cellMediatorCreateContext()
{
	libmedi.todo("cellMediatorCreateContext");
	return CELL_OK;
}

s32 cellMediatorFlushCache()
{
	libmedi.todo("cellMediatorFlushCache");
	return CELL_OK;
}

s32 cellMediatorGetProviderUrl()
{
	libmedi.todo("cellMediatorGetProviderUrl");
	return CELL_OK;
}

s32 cellMediatorGetSignatureLength()
{
	libmedi.todo("cellMediatorGetSignatureLength");
	return CELL_OK;
}

s32 cellMediatorGetStatus()
{
	libmedi.todo("cellMediatorGetStatus");
	return CELL_OK;
}

s32 cellMediatorGetUserInfo()
{
	libmedi.todo("cellMediatorGetUserInfo");
	return CELL_OK;
}

s32 cellMediatorPostReports()
{
	libmedi.todo("cellMediatorPostReports");
	return CELL_OK;
}

s32 cellMediatorReliablePostReports()
{
	libmedi.todo("cellMediatorReliablePostReports");
	return CELL_OK;
}

s32 cellMediatorSign()
{
	libmedi.todo("cellMediatorSign");
	return CELL_OK;
}


DECLARE(ppu_module_manager::libmedi)("libmedi", []()
{
	REG_FUNC(libmedi, cellMediatorCloseContext);
	REG_FUNC(libmedi, cellMediatorCreateContext);
	REG_FUNC(libmedi, cellMediatorFlushCache);
	REG_FUNC(libmedi, cellMediatorGetProviderUrl);
	REG_FUNC(libmedi, cellMediatorGetSignatureLength);
	REG_FUNC(libmedi, cellMediatorGetStatus);
	REG_FUNC(libmedi, cellMediatorGetUserInfo);
	REG_FUNC(libmedi, cellMediatorPostReports);
	REG_FUNC(libmedi, cellMediatorReliablePostReports);
	REG_FUNC(libmedi, cellMediatorSign);
});
