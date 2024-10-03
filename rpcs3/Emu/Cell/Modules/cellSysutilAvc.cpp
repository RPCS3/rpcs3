#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/cellSysutilAvc.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/IdManager.h"

LOG_CHANNEL(cellSysutil);

template<>
void fmt_class_string<CellAvcError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AVC_ERROR_UNKNOWN);
			STR_CASE(CELL_AVC_ERROR_NOT_SUPPORTED);
			STR_CASE(CELL_AVC_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_AVC_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_AVC_ERROR_INVALID_ARGUMENT);
			STR_CASE(CELL_AVC_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_AVC_ERROR_BAD_ID);
			STR_CASE(CELL_AVC_ERROR_INVALID_STATUS);
			STR_CASE(CELL_AVC_ERROR_TIMEOUT);
			STR_CASE(CELL_AVC_ERROR_NO_SESSION);
			STR_CASE(CELL_AVC_ERROR_INCOMPATIBLE_PROTOCOL);
			STR_CASE(CELL_AVC_ERROR_PEER_UNREACHABLE);
		}

		return unknown;
	});
}

// Callback handle tag type
struct avc_cb_handle_t{};

struct avc_settings
{
	avc_settings() = default;

	avc_settings(const avc_settings&) = delete;
	avc_settings& operator=(const avc_settings&) = delete;

	SAVESTATE_INIT_POS(53);

	shared_mutex mutex_cb;
	vm::ptr<CellSysutilAvcCallback> avc_cb{};
	vm::ptr<void> avc_cb_arg{};

	atomic_t<u32> req_id_cnt = 0;

	static bool saveable(bool /*is_writing*/) noexcept
	{
		return GET_SERIALIZATION_VERSION(cellSysutil) != 0;
	}

	avc_settings(utils::serial& ar) noexcept
	{
		[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(cellSysutil);

		if (version == 0)
		{
			return;
		}

		save(ar);
	}

	void save(utils::serial& ar)
	{
		[[maybe_unused]] const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellSysutil);
		ar(avc_cb, avc_cb_arg);
	}

	void register_cb_call(CellSysutilAvcRequestId req_id, CellSysutilAvcEvent event_id, CellSysUtilAvcEventParam param)
	{
		// This is equivalent to the dispatcher code
		sysutil_register_cb_with_id<avc_cb_handle_t>([=, this](ppu_thread& cb_ppu) -> s32
			{
				vm::ptr<CellSysutilAvcCallback> avc_cb{};
				vm::ptr<void> avc_cb_arg{};

				{
					std::lock_guard lock(this->mutex_cb);
					avc_cb = this->avc_cb;
					avc_cb_arg = this->avc_cb_arg;
				}

				if (avc_cb)
				{
					// TODO: lots of checks before calling the cb
					if (event_id == CELL_AVC_EVENT_UNLOAD_SUCCEEDED)
					{
						std::lock_guard lock(this->mutex_cb);
						this->avc_cb = {};
						this->avc_cb_arg = {};
					}

					avc_cb(cb_ppu, req_id, event_id, param, avc_cb_arg);
				}

				return 0;
			});
	}
};

error_code cellSysutilAvcByeRequest(vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcByeRequest(request_id=*0x%x)", request_id);

	if (!request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc_settings>();
	const CellSysutilAvcRequestId req_id = settings.req_id_cnt.fetch_add(1);
	*request_id = req_id;
	settings.register_cb_call(req_id, CELL_AVC_EVENT_BYE_SUCCEEDED, static_cast<CellSysUtilAvcEventParam>(0));

	return CELL_OK;
}

error_code cellSysutilAvcCancelByeRequest(vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcCancelByeRequest(request_id=*0x%x)", request_id);

	if (!request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcCancelJoinRequest(vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcCancelJoinRequest(request_id=*0x%x)", request_id);

	if (!request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcEnumPlayers(vm::ptr<SceNpId> players_id, vm::ptr<s32> players_num)
{
	cellSysutil.todo("cellSysutilAvcEnumPlayers(players_id=*0x%x, players_num=*0x%x)", players_id, players_num);

	if (!players_num)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	if (players_id)
	{
		// Fill players_id with players_num participants
	}
	else
	{
		// Return number of participants
		*players_num = 0;
	}

	return CELL_OK;
}

error_code cellSysutilAvcGetAttribute(CellSysUtilAvcAttribute attr_id, vm::pptr<void> param)
{
	cellSysutil.todo("cellSysutilAvcGetAttribute(attr_id=0x%x, param=*0x%x)", +attr_id, param);

	if (!param)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcGetLayoutMode(vm::ptr<CellSysutilAvcLayoutMode> layout)
{
	cellSysutil.todo("cellSysutilAvcGetLayoutMode(layout=*0x%x)", layout);

	if (!layout)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcGetShowStatus(vm::ptr<b8> is_visible)
{
	cellSysutil.todo("cellSysutilAvcGetShowStatus(is_visible=*0x%x)", is_visible);

	if (!is_visible)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcGetSpeakerVolumeLevel(vm::ptr<s32> volumeLevel)
{
	cellSysutil.todo("cellSysutilAvcGetSpeakerVolumeLevel(volumeLevel=*0x%x)", volumeLevel);

	if (!volumeLevel)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcGetVideoMuting(vm::ptr<b8> is_muting)
{
	cellSysutil.todo("cellSysutilAvcGetVideoMuting(is_muting=*0x%x)", is_muting);

	if (!is_muting)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcGetVoiceMuting(vm::ptr<b8> is_muting)
{
	cellSysutil.todo("cellSysutilAvcGetVoiceMuting(is_muting=*0x%x)", is_muting);

	if (!is_muting)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcHidePanel()
{
	cellSysutil.todo("cellSysutilAvcHidePanel()");
	return CELL_OK;
}

error_code cellSysutilAvcJoinRequest(u32 ctx_id, vm::cptr<SceNpRoomId> room_id, vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcJoinRequest(ctx_id=*0x%x, room_id=*0x%x, request_id=*0x%x)", ctx_id, room_id, request_id);

	if (!room_id || !request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc_settings>();
	const CellSysutilAvcRequestId req_id = settings.req_id_cnt.fetch_add(1);
	*request_id = req_id;
	settings.register_cb_call(req_id, CELL_AVC_EVENT_JOIN_SUCCEEDED, static_cast<CellSysUtilAvcEventParam>(0));

	return CELL_OK;
}

error_code cellSysutilAvcLoadAsync(vm::ptr<CellSysutilAvcCallback> func, vm::ptr<void> userdata, sys_memory_container_t container,
	CellSysUtilAvcMediaType media, CellSysUtilAvcVideoQuality videoQuality, CellSysUtilAvcVoiceQuality voiceQuality, vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcLoadAsync(func=*0x%x, userdata=*0x%x, container=0x%x, media=0x%x, videoQuality=0x%x, voiceQuality=0x%x, request_id=*0x%x)",
		func, userdata, container, +media, +videoQuality, +voiceQuality, request_id);

	//if (sys_memory_container_get_size(.., container) != CELL_OK)
	//	return CELL_AVC_ERROR_INVALID_ARGUMENT;

	switch (media)
	{
	case CELL_SYSUTIL_AVC_VOICE_CHAT:
	case CELL_SYSUTIL_AVC_VIDEO_CHAT:
		// TODO: return CELL_AVC_ERROR_OUT_OF_MEMORY
		// TODO: return CELL_AVC_ERROR_INVALID_ARGUMENT
		break;
	default:
		return CELL_AVC_ERROR_INVALID_ARGUMENT;
	}

	if (!func || !request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	if (videoQuality != CELL_SYSUTIL_AVC_VIDEO_QUALITY_DEFAULT || voiceQuality != CELL_SYSUTIL_AVC_VOICE_QUALITY_DEFAULT)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc_settings>();

	{
		std::lock_guard lock(settings.mutex_cb);

		if (settings.avc_cb)
			return CELL_AVC_ERROR_ALREADY_INITIALIZED;

		settings.avc_cb = func;
		settings.avc_cb_arg = userdata;
	}

	const CellSysutilAvcRequestId req_id = settings.req_id_cnt.fetch_add(1);
	*request_id = req_id;
	settings.register_cb_call(req_id, CELL_AVC_EVENT_LOAD_SUCCEEDED, static_cast<CellSysUtilAvcEventParam>(0));

	return CELL_OK;
}

error_code cellSysutilAvcSetAttribute(CellSysUtilAvcAttribute attr_id, vm::ptr<void> param)
{
	cellSysutil.todo("cellSysutilAvcSetAttribute(attr_id=0x%x, param=*0x%x)", +attr_id, param);

	if (!param)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcSetLayoutMode(CellSysutilAvcLayoutMode layout)
{
	cellSysutil.todo("cellSysutilAvcSetLayoutMode(layout=0x%x)", +layout);

	if (layout > CELL_SYSUTIL_AVC_LAYOUT_BOTTOM)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcSetSpeakerVolumeLevel(s32 volumeLevel)
{
	cellSysutil.todo("cellSysutilAvcSetSpeakerVolumeLevel(volumeLevel=%d)", volumeLevel);

	if (volumeLevel < 0 || volumeLevel > 10)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvcSetVideoMuting(b8 is_muting)
{
	cellSysutil.todo("cellSysutilAvcSetVideoMuting(is_muting=%d)", is_muting);
	return CELL_OK;
}

error_code cellSysutilAvcSetVoiceMuting(b8 is_muting)
{
	cellSysutil.todo("cellSysutilAvcSetVoiceMuting(is_muting=%d)", is_muting);
	return CELL_OK;
}

error_code cellSysutilAvcShowPanel()
{
	cellSysutil.todo("cellSysutilAvcShowPanel()");
	return CELL_OK;
}

error_code cellSysutilAvcUnloadAsync(vm::ptr<CellSysutilAvcRequestId> request_id)
{
	cellSysutil.todo("cellSysutilAvcUnloadAsync(request_id=*0x%x)", request_id);

	if (!request_id)
		return CELL_AVC_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc_settings>();
	const CellSysutilAvcRequestId req_id = settings.req_id_cnt.fetch_add(1);
	*request_id = req_id;
	settings.register_cb_call(req_id, CELL_AVC_EVENT_UNLOAD_SUCCEEDED, static_cast<CellSysUtilAvcEventParam>(0));

	return CELL_OK;
}

void cellSysutil_SysutilAvc_init()
{
	REG_FUNC(cellSysutil, cellSysutilAvcByeRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcCancelByeRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcCancelJoinRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcEnumPlayers);
	REG_FUNC(cellSysutil, cellSysutilAvcGetAttribute);
	REG_FUNC(cellSysutil, cellSysutilAvcGetLayoutMode);
	REG_FUNC(cellSysutil, cellSysutilAvcGetShowStatus);
	REG_FUNC(cellSysutil, cellSysutilAvcGetSpeakerVolumeLevel);
	REG_FUNC(cellSysutil, cellSysutilAvcGetVideoMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcGetVoiceMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcHidePanel);
	REG_FUNC(cellSysutil, cellSysutilAvcJoinRequest);
	REG_FUNC(cellSysutil, cellSysutilAvcLoadAsync);
	REG_FUNC(cellSysutil, cellSysutilAvcSetAttribute);
	REG_FUNC(cellSysutil, cellSysutilAvcSetLayoutMode);
	REG_FUNC(cellSysutil, cellSysutilAvcSetSpeakerVolumeLevel);
	REG_FUNC(cellSysutil, cellSysutilAvcSetVideoMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcSetVoiceMuting);
	REG_FUNC(cellSysutil, cellSysutilAvcShowPanel);
	REG_FUNC(cellSysutil, cellSysutilAvcUnloadAsync);
}
