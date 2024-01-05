#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"
#include "sceNp.h"
#include "Emu/IdManager.h"

LOG_CHANNEL(cellSysutilNpEula);

enum SceNpEulaStatus
{
	SCE_NP_EULA_UNKNOWN          = 0,
	SCE_NP_EULA_ACCEPTED         = 1,
	SCE_NP_EULA_ALREADY_ACCEPTED = 2,
	SCE_NP_EULA_REJECTED         = 3,
	SCE_NP_EULA_ABORTED          = 4,
	SCE_NP_EULA_ERROR            = 5,
};

using SceNpEulaVersion = u32;
using SceNpEulaCheckEulaStatusCallback = void(SceNpEulaStatus status, u32 errorCode, SceNpEulaVersion version, vm::ptr<void> arg);

struct sceNpEulaCallbacksRegistered
{
	atomic_t<SceNpEulaStatus> status = SCE_NP_EULA_UNKNOWN;
	atomic_t<bool> sceNpEulaCheckEulaStatus_callback_registered = false;
	atomic_t<bool> sceNpEulaShowCurrentEula_callback_registered = false;
};

error_code sceNpEulaCheckEulaStatus(vm::cptr<SceNpCommunicationId> communicationId, u32 arg2, u64 arg3, vm::ptr<SceNpEulaCheckEulaStatusCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	cellSysutilNpEula.warning("sceNpEulaCheckEulaStatus(communicationId=*0x%x, arg2=0x%x, arg3=0x%x, cbFunc=*0x%x, cbFuncArg=*0x%x)", communicationId, arg2, arg3, cbFunc, cbFuncArg);

	if (!communicationId || !cbFunc)
	{
		return SCE_NP_EULA_ERROR_INVALID_ARGUMENT;
	}

	auto& cb_infos = g_fxo->get<sceNpEulaCallbacksRegistered>();

	if (cb_infos.sceNpEulaCheckEulaStatus_callback_registered || cb_infos.sceNpEulaShowCurrentEula_callback_registered)
	{
		return SCE_NP_EULA_ERROR_ALREADY_INITIALIZED;
	}

	cb_infos.sceNpEulaCheckEulaStatus_callback_registered = true;
	cb_infos.status = SCE_NP_EULA_ALREADY_ACCEPTED;

	sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
	{
		auto& cb_infos = g_fxo->get<sceNpEulaCallbacksRegistered>();
		cbFunc(cb_ppu, cb_infos.status, CELL_OK, 1, cbFuncArg);
		cb_infos.sceNpEulaCheckEulaStatus_callback_registered = false;
		return 0;
	});

	return CELL_OK;
}

error_code sceNpEulaAbort()
{
	cellSysutilNpEula.warning("sceNpEulaAbort()");

	auto& cb_infos = g_fxo->get<sceNpEulaCallbacksRegistered>();

	if (!cb_infos.sceNpEulaCheckEulaStatus_callback_registered && !cb_infos.sceNpEulaShowCurrentEula_callback_registered)
	{
		return SCE_NP_EULA_ERROR_NOT_INITIALIZED;
	}

	// It would forcefully abort the dialog/process of getting the eula but since we don't show the dialog, just alter the status returned
	cb_infos.status = SCE_NP_EULA_ABORTED;

	return CELL_OK;
}

// Seen on: Resistance 3, Uncharted 2
error_code sceNpEulaShowCurrentEula(vm::cptr<SceNpCommunicationId> communicationId, u64 arg2, vm::ptr<CellSysutilCallback> cbFunc, vm::ptr<void> cbFuncArg)
{
	cellSysutilNpEula.todo("sceNpEulaShowCurrentEula(communicationId=*0x%x, arg2=0x%x, cbFunc=*0x%x, cbFuncArg=*0x%x)", communicationId, arg2, cbFunc, cbFuncArg);

	if (!communicationId || !cbFunc)
	{
		return SCE_NP_EULA_ERROR_INVALID_ARGUMENT;
	}

	auto& cb_infos = g_fxo->get<sceNpEulaCallbacksRegistered>();

	if (cb_infos.sceNpEulaCheckEulaStatus_callback_registered || cb_infos.sceNpEulaShowCurrentEula_callback_registered)
	{
		return SCE_NP_EULA_ERROR_ALREADY_INITIALIZED;
	}

	// Call callback (Unknown parameters)

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysutilNpEula)("cellSysutilNpEula", []()
{
	REG_FUNC(cellSysutilNpEula, sceNpEulaCheckEulaStatus);
	REG_FUNC(cellSysutilNpEula, sceNpEulaAbort);
	REG_FUNC(cellSysutilNpEula, sceNpEulaShowCurrentEula);
});
