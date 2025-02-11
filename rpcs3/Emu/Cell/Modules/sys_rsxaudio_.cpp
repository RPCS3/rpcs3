#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(sysPrxForUser);

error_code sys_rsxaudio_close_connection()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_create_connection()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_finalize()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_import_shared_memory()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_initialize()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_prepare_process()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_start_process()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_stop_process()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

error_code sys_rsxaudio_unimport_shared_memory()
{
	UNIMPLEMENTED_FUNC(sysPrxForUser);
	return CELL_OK;
}

void sysPrxForUser_sys_rsxaudio_init()
{
	REG_FUNC(sysPrxForUser, sys_rsxaudio_close_connection);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_create_connection);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_finalize);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_import_shared_memory);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_initialize);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_prepare_process);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_start_process);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_stop_process);
	REG_FUNC(sysPrxForUser, sys_rsxaudio_unimport_shared_memory);
}
