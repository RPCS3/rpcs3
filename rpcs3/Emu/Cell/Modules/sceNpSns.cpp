#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNpSns.h"

logs::channel sceNpSns("sceNpSns");

template<>
void fmt_class_string<sceNpSnsError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(SCE_NP_SNS_ERROR_UNKNOWN);
			STR_CASE(SCE_NP_SNS_ERROR_NOT_SIGN_IN);
			STR_CASE(SCE_NP_SNS_ERROR_INVALID_ARGUMENT);
			STR_CASE(SCE_NP_SNS_ERROR_OUT_OF_MEMORY);
			STR_CASE(SCE_NP_SNS_ERROR_SHUTDOWN);
			STR_CASE(SCE_NP_SNS_ERROR_BUSY);
			STR_CASE(SCE_NP_SNS_FB_ERROR_ALREADY_INITIALIZED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_EXCEEDS_MAX);
			STR_CASE(SCE_NP_SNS_FB_ERROR_UNKNOWN_HANDLE);
			STR_CASE(SCE_NP_SNS_FB_ERROR_ABORTED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_ALREADY_ABORTED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_CONFIG_DISABLED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_FBSERVER_ERROR_RESPONSE);
			STR_CASE(SCE_NP_SNS_FB_ERROR_THROTTLE_CLOSED);
			STR_CASE(SCE_NP_SNS_FB_ERROR_OPERATION_INTERVAL_VIOLATION);
			STR_CASE(SCE_NP_SNS_FB_ERROR_UNLOADED_THROTTLE);
			STR_CASE(SCE_NP_SNS_FB_ERROR_ACCESS_NOT_ALLOWED);
		}

		return unknown;
	});
}

error_code sceNpSnsFbInit(vm::cptr<SceNpSnsFbInitParams> params)
{
	sceNpSns.todo("sceNpSnsFbInit(params=*0x%x)", params);

	if (!params)
	{
		return SCE_NP_SNS_ERROR_INVALID_ARGUMENT;
	}

	// TODO: Use the initialization parameters somewhere

	return CELL_OK;
}

error_code sceNpSnsFbTerm()
{
	sceNpSns.warning("sceNpSnsFbTerm()");

	return CELL_OK;
}

error_code sceNpSnsFbCreateHandle(vm::ptr<u32> handle)
{
	sceNpSns.warning("sceNpSnsFbCreateHandle(handle=*0x%x)", handle);

	if (!handle)
	{
		return SCE_NP_SNS_FB_ERROR_NOT_INITIALIZED;
	}

	*handle = idm::make<sns_fb_handle_t>();

	return CELL_OK;
}

error_code sceNpSnsFbDestroyHandle(u32 handle)
{
	sceNpSns.warning("sceNpSnsFbDestroyHandle(handle=%d)", handle);

	if (handle == SCE_NP_SNS_FB_INVALID_HANDLE || handle > SCE_NP_SNS_FB_HANDLE_SLOT_MAX)
	{
		return SCE_NP_SNS_ERROR_INVALID_ARGUMENT;
	}

	const auto sfh = idm::get<sns_fb_handle_t>(handle);

	if (!sfh)
	{
		return SCE_NP_SNS_FB_ERROR_UNKNOWN_HANDLE;
	}

	idm::remove<sns_fb_handle_t>(handle);

	return CELL_OK;
}

error_code sceNpSnsFbAbortHandle(u32 handle)
{
	sceNpSns.todo("sceNpSnsFbAbortHandle(handle=%d)", handle);

	if (handle == SCE_NP_SNS_FB_INVALID_HANDLE || handle > SCE_NP_SNS_FB_HANDLE_SLOT_MAX)
	{
		return SCE_NP_SNS_ERROR_INVALID_ARGUMENT;
	}

	const auto sfh = idm::get<sns_fb_handle_t>(handle);

	if (!sfh)
	{
		return SCE_NP_SNS_FB_ERROR_UNKNOWN_HANDLE;
	}

	// TODO

	return CELL_OK;
}

error_code sceNpSnsFbGetAccessToken(u32 handle, vm::cptr<SceNpSnsFbAccessTokenParam> param, vm::ptr<SceNpSnsFbAccessTokenResult> result)
{
	sceNpSns.todo("sceNpSnsFbGetAccessToken(handle=%d, param=*0x%x, result=*0x%x)", handle, param, result);

	if (handle == SCE_NP_SNS_FB_INVALID_HANDLE || handle > SCE_NP_SNS_FB_HANDLE_SLOT_MAX || !param || !result || !param->fb_app_id)
	{
		return SCE_NP_SNS_ERROR_INVALID_ARGUMENT;
	}

	const auto sfh = idm::get<sns_fb_handle_t>(handle);

	if (!sfh)
	{
		return SCE_NP_SNS_FB_ERROR_UNKNOWN_HANDLE;
	}

	// TODO

	return CELL_OK;
}

s32 sceNpSnsFbStreamPublish()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbCheckThrottle()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbCheckConfig()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbLoadThrottle()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNpSnsFbGetLongAccessToken()
{
	fmt::throw_exception("Unimplemented" HERE);
}


DECLARE(ppu_module_manager::sceNpSns)("sceNpSns", []()
{
	REG_FUNC(sceNpSns, sceNpSnsFbInit);
	REG_FUNC(sceNpSns, sceNpSnsFbTerm);
	REG_FUNC(sceNpSns, sceNpSnsFbCreateHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbDestroyHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbAbortHandle);
	REG_FUNC(sceNpSns, sceNpSnsFbGetAccessToken);
	REG_FUNC(sceNpSns, sceNpSnsFbGetLongAccessToken);
	REG_FUNC(sceNpSns, sceNpSnsFbStreamPublish);
	REG_FUNC(sceNpSns, sceNpSnsFbCheckThrottle);
	REG_FUNC(sceNpSns, sceNpSnsFbCheckConfig);
	REG_FUNC(sceNpSns, sceNpSnsFbLoadThrottle);
});
