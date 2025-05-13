#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/localized_string.h"
#include "cellSysutil.h"
#include "cellCrossController.h"
#include "cellMsgDialog.h"


LOG_CHANNEL(cellCrossController);

template <>
void fmt_class_string<CellCrossControllerError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellCrossControllerError value)
	{
		switch (value)
		{
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_CANCEL);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_NETWORK);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_OUT_OF_MEMORY);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_FATAL);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_SIG_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_ICON_FILENAME);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_PKG_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_SIG_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_ICON_FILE_OPEN);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_STATE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILE);
		STR_CASE(CELL_CROSS_CONTROLLER_ERROR_INTERNAL);
		}

		return unknown;
	});
}

void finish_callback(ppu_thread& ppu, s32 button_type, vm::ptr<void> userdata); // Forward declaration

struct cross_controller
{
	atomic_t<s32> status{0};
	std::unique_ptr<named_thread<std::function<void()>>> connection_thread;
	vm::ptr<CellCrossControllerCallback> callback = vm::null;
	vm::ptr<void> userdata = vm::null;

	void on_connection_established(s32 status)
	{
		ensure(!!callback);

		close_msg_dialog();

		sysutil_register_cb([this, status](ppu_thread& ppu) -> s32
		{
			callback(ppu, CELL_CROSS_CONTROLLER_STATUS_FINALIZED, status, vm::null, userdata);
			return CELL_OK;
		});
	}

	void run_thread(vm::cptr<CellCrossControllerPackageInfo> pPkgInfo)
	{
		ensure(!!pPkgInfo);
		ensure(!!callback);

		const std::string msg = fmt::format("%s\n\n%s",
			get_localized_string(localized_string_id::CELL_CROSS_CONTROLLER_MSG, pPkgInfo->pTitle.get_ptr()),
			get_localized_string(localized_string_id::CELL_CROSS_CONTROLLER_FW_MSG));

		vm::bptr<CellMsgDialogCallback> msg_dialog_callback = vm::null;
		msg_dialog_callback.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(finish_callback)));

		// TODO: Show icons from comboplay_plugin.rco in dialog. Maybe use a new dialog or add an optional icon to this one.
		error_code res = open_msg_dialog(false, CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF, vm::make_str(msg), msg_dialog_source::_cellCrossController, msg_dialog_callback, userdata);

		sysutil_register_cb([this, res](ppu_thread& ppu) -> s32
		{
			callback(ppu, CELL_CROSS_CONTROLLER_STATUS_INITIALIZED, res == CELL_OK ? +CELL_OK : +CELL_CROSS_CONTROLLER_ERROR_INTERNAL, vm::null, userdata);
			return CELL_OK;
		});

		status = CELL_CROSS_CONTROLLER_STATUS_INITIALIZED;

		connection_thread = std::make_unique<named_thread<std::function<void()>>>(fmt::format("Cross-Controller Thread"), [this]()
		{
			while (thread_ctrl::state() != thread_state::aborting)
			{
				if (Emu.IsPaused())
				{
					thread_ctrl::wait_for(10'000);
					continue;
				}

				// TODO: establish connection to PS Vita
				if (false)
				{
					on_connection_established(CELL_OK);
				}

				thread_ctrl::wait_for(1000);
			}

			status = CELL_CROSS_CONTROLLER_STATUS_FINALIZED;
		});
	}

	void stop_thread()
	{
		// Join thread
		connection_thread.reset();
	};
};

void finish_callback(ppu_thread& ppu, s32 button_type, vm::ptr<void> userdata)
{
	cross_controller& cc = g_fxo->get<cross_controller>();

	// This function should only be called when the user canceled the dialog
	ensure(cc.callback && button_type == CELL_MSGDIALOG_BUTTON_ESCAPE);

	cc.callback(ppu, CELL_CROSS_CONTROLLER_STATUS_FINALIZED, CELL_CROSS_CONTROLLER_ERROR_CANCEL, vm::null, userdata);
	cc.stop_thread();
}

error_code cellCrossControllerInitialize(vm::cptr<CellCrossControllerParam> pParam, vm::cptr<CellCrossControllerPackageInfo> pPkgInfo, vm::ptr<CellCrossControllerCallback> cb, vm::ptr<void> userdata) // LittleBigPlanet 2 and 3
{
	cellCrossController.todo("cellCrossControllerInitialize(pParam=*0x%x, pPkgInfo=*0x%x, cb=*0x%x, userdata=*0x%x)", pParam, pPkgInfo, cb, userdata);

	if (pParam)
	{
		cellCrossController.notice("cellCrossControllerInitialize: pParam: pPackageFileName=%s, pSignatureFileName=%s, pIconFileName=%s", pParam->pPackageFileName, pParam->pSignatureFileName, pParam->pIconFileName);
	}

	if (pPkgInfo)
	{
		cellCrossController.notice("cellCrossControllerInitialize: pPkgInfo: pTitle=%s, pTitleId=%s, pAppVer=%s", pPkgInfo->pTitle, pPkgInfo->pTitleId, pPkgInfo->pAppVer);
	}

	cross_controller& cc = g_fxo->get<cross_controller>();

	if (cc.status == CELL_CROSS_CONTROLLER_STATUS_INITIALIZED) // TODO: confirm this logic
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_STATE;
	}

	if (!pParam || !pPkgInfo)
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE;
	}

	// Check if the strings exceed the allowed size (not counting null terminators)

	if (!pParam->pPackageFileName || !memchr(pParam->pPackageFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILENAME;
	}

	if (!pParam->pSignatureFileName || !memchr(pParam->pSignatureFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_SIG_FILENAME;
	}

	if (!pParam->pIconFileName || !memchr(pParam->pIconFileName.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN + 1))
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_ICON_FILENAME;
	}

	if (!pPkgInfo->pAppVer || !memchr(pPkgInfo->pAppVer.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_APP_VER_LEN + 1) ||
	    !pPkgInfo->pTitleId || !memchr(pPkgInfo->pTitleId.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_TITLE_ID_LEN + 1) ||
	    !pPkgInfo->pTitle || !memchr(pPkgInfo->pTitle.get_ptr(), '\0', CELL_CROSS_CONTROLLER_PKG_TITLE_LEN + 1) ||
	    !cb)
	{
		return CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE;
	}

	cc.callback = cb;
	cc.userdata = userdata;
	cc.run_thread(pPkgInfo);

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellCrossController)("cellCrossController", []()
{
	REG_FUNC(cellCrossController, cellCrossControllerInitialize);

	// Helper Function
	REG_HIDDEN_FUNC(finish_callback);
});
