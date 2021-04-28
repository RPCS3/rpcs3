#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(libmedi);

error_code cellMediatorCloseContext()
{
	libmedi.todo("cellMediatorCloseContext");
	return CELL_OK;
}

error_code cellMediatorCreateContext()
{
	libmedi.todo("cellMediatorCreateContext");
	return CELL_OK;
}

error_code cellMediatorFlushCache()
{
	libmedi.todo("cellMediatorFlushCache");
	return CELL_OK;
}

error_code cellMediatorGetProviderUrl()
{
	libmedi.todo("cellMediatorGetProviderUrl");
	return CELL_OK;
}

error_code cellMediatorGetSignatureLength()
{
	libmedi.todo("cellMediatorGetSignatureLength");
	return CELL_OK;
}

error_code cellMediatorGetStatus()
{
	libmedi.todo("cellMediatorGetStatus");
	return CELL_OK;
}

error_code cellMediatorGetUserInfo()
{
	libmedi.todo("cellMediatorGetUserInfo");
	return CELL_OK;
}

error_code cellMediatorPostReports()
{
	libmedi.todo("cellMediatorPostReports");
	return CELL_OK;
}

error_code cellMediatorReliablePostReports()
{
	libmedi.todo("cellMediatorReliablePostReports");
	return CELL_OK;
}

error_code cellMediatorSign()
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
