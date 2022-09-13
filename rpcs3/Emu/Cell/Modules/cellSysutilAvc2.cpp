#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNp.h"
#include "sceNp2.h"
#include "cellSysutilAvc2.h"
#include "cellSysutil.h"

LOG_CHANNEL(cellSysutilAvc2);

template<>
void fmt_class_string<CellSysutilAvc2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AVC2_ERROR_UNKNOWN);
			STR_CASE(CELL_AVC2_ERROR_NOT_SUPPORTED);
			STR_CASE(CELL_AVC2_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_AVC2_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_AVC2_ERROR_INVALID_ARGUMENT);
			STR_CASE(CELL_AVC2_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_AVC2_ERROR_ERROR_BAD_ID);
			STR_CASE(CELL_AVC2_ERROR_INVALID_STATUS);
			STR_CASE(CELL_AVC2_ERROR_TIMEOUT);
			STR_CASE(CELL_AVC2_ERROR_NO_SESSION);
			STR_CASE(CELL_AVC2_ERROR_WINDOW_ALREADY_EXISTS);
			STR_CASE(CELL_AVC2_ERROR_TOO_MANY_WINDOWS);
			STR_CASE(CELL_AVC2_ERROR_TOO_MANY_PEER_WINDOWS);
			STR_CASE(CELL_AVC2_ERROR_WINDOW_NOT_FOUND);
		}

		return unknown;
	});
}

vm::ptr<CellSysutilAvc2Callback> avc2_cb{};
vm::ptr<void> avc2_cb_arg{};

error_code cellSysutilAvc2GetPlayerInfo(vm::cptr<SceNpMatching2RoomMemberId> player_id, vm::ptr<CellSysutilAvc2PlayerInfo> player_info)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetPlayerInfo(player_id=*0x%x, player_info=*0x%x)", player_id, player_info);

	if (!player_id || !player_info)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	player_info->connected = 1;
	player_info->joined = 1;
	player_info->mic_attached = 0;
	player_info->member_id = *player_id;

	return CELL_OK;
}

error_code cellSysutilAvc2JoinChat(vm::cptr<SceNpMatching2RoomId> room_id, vm::ptr<CellSysutilAvc2EventId> eventId, vm::ptr<CellSysutilAvc2EventParam> eventParam)
{
	cellSysutilAvc2.todo("cellSysutilAvc2JoinChat(room_id=*0x%x, eventId=*0x%x, eventParam=*0x%x)", room_id, eventId, eventParam);

	// NOTE: room_id should be null if the current mode is Direct WAN/LAN

	[[maybe_unused]] u64 id = 0UL;

	if (room_id)
	{
		id = *room_id;
	}
	else if (false/*streaming_mode != CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL*/) // TODO
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	// TODO: join chat

	return CELL_OK;
}

error_code cellSysutilAvc2StopStreaming()
{
	cellSysutilAvc2.todo("cellSysutilAvc2StopStreaming()");
	return CELL_OK;
}

error_code cellSysutilAvc2ChangeVideoResolution(u32 resolution)
{
	cellSysutilAvc2.todo("cellSysutilAvc2ChangeVideoResolution(resolution=0x%x)", resolution);
	return CELL_OK;
}

error_code cellSysutilAvc2ShowScreen()
{
	cellSysutilAvc2.todo("cellSysutilAvc2ShowScreen()");
	return CELL_OK;
}

error_code cellSysutilAvc2GetVideoMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetVideoMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*muting = 1;

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowAttribute(SceNpMatching2RoomMemberId member_id, vm::ptr<CellSysutilAvc2WindowAttribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowAttribute(member_id=0x%x, attr=*0x%x)", member_id, attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ALPHA:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_DURATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_STRING_VISIBLE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ROTATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ZORDER:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_SURFACE:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2StopStreaming2(u32 mediaType)
{
	cellSysutilAvc2.todo("cellSysutilAvc2StopStreaming2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2SetVoiceMuting(u8 muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetVoiceMuting(muting=0x%x)", muting);
	return CELL_OK;
}

error_code cellSysutilAvc2StartVoiceDetection()
{
	cellSysutilAvc2.todo("cellSysutilAvc2StartVoiceDetection()");
	return CELL_OK;
}

error_code cellSysutilAvc2UnloadAsync()
{
	cellSysutilAvc2.todo("cellSysutilAvc2UnloadAsync()");

	if (avc2_cb)
	{
		sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
		{
			avc2_cb(cb_ppu, CELL_AVC2_EVENT_UNLOAD_SUCCEEDED, 0, avc2_cb_arg);
			return 0;
		});
	}

	return CELL_OK;
}

error_code cellSysutilAvc2StopVoiceDetection()
{
	cellSysutilAvc2.todo("cellSysutilAvc2StopVoiceDetection()");
	return CELL_OK;
}

error_code cellSysutilAvc2GetAttribute(vm::ptr<CellSysutilAvc2Attribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetAttribute(attr=*0x%x)", attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2LoadAsync(SceNpMatching2ContextId ctx_id, u32 container, vm::ptr<CellSysutilAvc2Callback> callback_func, vm::ptr<void> user_data, vm::cptr<CellSysutilAvc2InitParam> init_param)
{
	cellSysutilAvc2.warning("cellSysutilAvc2LoadAsync(ctx_id=0x%x, container=0x%x, callback_func=*0x%x, user_data=*0x%x, init_param=*0x%x)", ctx_id, container, callback_func, user_data, init_param);

	avc2_cb = callback_func;
	avc2_cb_arg = user_data;

	if (avc2_cb)
	{
		sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
		{
			avc2_cb(cb_ppu, CELL_AVC2_EVENT_LOAD_SUCCEEDED, 0, avc2_cb_arg);
			return 0;
		});
	}

	return CELL_OK;
}

error_code cellSysutilAvc2SetSpeakerVolumeLevel(f32 level)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetSpeakerVolumeLevel(level=0x%x)", level);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowString(SceNpMatching2RoomMemberId member_id, vm::cptr<char> string)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowString(member_id=0x%x, string=%s)", member_id, string);

	if (!string || std::strlen(string.get_ptr()) >= 64)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2EstimateMemoryContainerSize(vm::cptr<CellSysutilAvc2InitParam> initparam, vm::ptr<u32> size)
{
	cellSysutilAvc2.todo("cellSysutilAvc2EstimateMemoryContainerSize(initparam=*0x%x, size=*0x%x)", initparam, size);

	if (!initparam || !size)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (initparam->avc_init_param_version)
	{
	case 100:
	{
		*size = 0x400000;
		break;
	}
	case 110:
	case 120:
	case 130:
	case 140:
	{
		if (initparam->media_type == CELL_SYSUTIL_AVC2_VOICE_CHAT)
		{
			*size = 0x300000;
		}
		else if (initparam->media_type == CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		{
			u32 estimated_size = 0x40e666;
			u32 max_windows    = initparam->video_param.max_video_windows;
			s32 window_count   = max_windows;

			if (initparam->video_param.video_stream_sharing == CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_2)
			{
				window_count++;
			}

			if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA)
			{
				estimated_size = (static_cast<u32>(window_count) * 0x12c00 & 0xfff00000) + 0x50e666;
			}
			else if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QVGA)
			{
				estimated_size += (static_cast<u32>(window_count) * 0x4b000 & 0xfff00000) + 0x100000;
			}

			if (initparam->video_param.frame_mode == CELL_SYSUTIL_AVC2_FRAME_MODE_NORMAL)
			{
				window_count = max_windows - 1;
			}
			else
			{
				window_count = 1;
			}

			u32 val = max_windows * 10000;

			if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA)
			{
				val += window_count * 0x96000 + 0x10c9e0; // 0x96000 = 160x120x32
			}
			else
			{
				val += static_cast<s32>(static_cast<f64>(window_count) * 1258291.2) + 0x1ed846;
			}

			estimated_size = ((estimated_size + ((static_cast<int>(val) >> 7) + static_cast<u32>(static_cast<int>(val) < 0 && (val & 0x7f) != 0)) * 0x80 + 0x80080) & 0xfff00000) + 0x100000;

			*size = estimated_size;
		}
		else
		{
			*size = 0;
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}
		break;
	}
	default:
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}
	}

	return CELL_OK;
}

error_code cellSysutilAvc2SetVideoMuting(u8 muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetVideoMuting(muting=0x%x)", muting);

	if (muting > 1) // Weird check, lol
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2SetPlayerVoiceMuting(SceNpMatching2RoomMemberId member_id, u8 muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetPlayerVoiceMuting(member_id=0x%x, muting=0x%x)", member_id, muting);
	return CELL_OK;
}

error_code cellSysutilAvc2SetStreamingTarget(vm::cptr<CellSysutilAvc2StreamingTarget> target)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetStreamingTarget(target=*0x%x)", target);

	return CELL_OK;
}

error_code cellSysutilAvc2Unload()
{
	cellSysutilAvc2.todo("cellSysutilAvc2Unload()");

	// TODO: CELL_AVC2_ERROR_NOT_INITIALIZED

	return CELL_OK;
}

error_code cellSysutilAvc2DestroyWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2DestroyWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowPosition(SceNpMatching2RoomMemberId member_id, f32 x, f32 y, f32 z)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowPosition(member_id=0x%x, x=0x%x, y=0x%x, z=0x%x)", member_id, x, y, z);
	return CELL_OK;
}

error_code cellSysutilAvc2GetSpeakerVolumeLevel(vm::ptr<f32> level)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetSpeakerVolumeLevel(level=*0x%x)", level);

	if (!level)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*level = 100;

	return CELL_OK;
}

error_code cellSysutilAvc2IsCameraAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.todo("cellSysutilAvc2IsCameraAttached(status=*0x%x)", status);

	if (!status)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*status = CELL_AVC2_CAMERA_STATUS_DETACHED;

	return CELL_OK;
}

error_code cellSysutilAvc2MicRead(vm::ptr<void> ptr, vm::ptr<u32> pSize)
{
	cellSysutilAvc2.todo("cellSysutilAvc2MicRead(ptr=*0x%x, pSize=*0x%x)", ptr, pSize);

	// TODO: check arguments ?

	return CELL_OK;
}

error_code cellSysutilAvc2GetPlayerVoiceMuting(SceNpMatching2RoomMemberId member_id, vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetPlayerVoiceMuting(member_id=0x%x, muting=*0x%x)", member_id, muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*muting = 0;

	return CELL_OK;
}

error_code cellSysutilAvc2JoinChatRequest(vm::cptr<SceNpMatching2RoomId> room_id)
{
	cellSysutilAvc2.warning("cellSysutilAvc2JoinChatRequest(room_id=*0x%x)", room_id);

	// NOTE: room_id should be null if the current mode is Direct WAN/LAN

	[[maybe_unused]] u64 id = 0UL;

	if (room_id)
	{
		id = *room_id;
	}
	else if (false/*streaming_mode != CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL*/) // TODO
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	// TODO: join chat

	if (avc2_cb)
	{
		sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
		{
			avc2_cb(cb_ppu, CELL_AVC2_EVENT_JOIN_SUCCEEDED, 0, avc2_cb_arg);
			return 0;
		});
	}

	return CELL_OK;
}

error_code cellSysutilAvc2StartStreaming()
{
	cellSysutilAvc2.todo("cellSysutilAvc2StartStreaming()");
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowAttribute(SceNpMatching2RoomMemberId member_id, vm::cptr<CellSysutilAvc2WindowAttribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowAttribute(member_id=0x%x, attr=*0x%x)", member_id, attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ALPHA:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_DURATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_STRING_VISIBLE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ROTATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ZORDER:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_SURFACE:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowShowStatus(SceNpMatching2RoomMemberId member_id, vm::ptr<u8> visible)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowShowStatus(member_id=0x%x, visible=*0x%x)", member_id, visible);

	if (!visible)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*visible = 0;

	return CELL_OK;
}

error_code cellSysutilAvc2InitParam(u16 version, vm::ptr<CellSysutilAvc2InitParam> option)
{
	cellSysutilAvc2.todo("cellSysutilAvc2InitParam(version=%d, option=*0x%x)", version, option);

	if (!option)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*option = {};
	option->avc_init_param_version = version;

	switch (version)
	{
	case 100:
	case 110:
	case 120:
	case 130:
	case 140:
		break;
	default:
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowSize(SceNpMatching2RoomMemberId member_id, vm::ptr<f32> width, vm::ptr<f32> height)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowSize(member_id=0x%x, width=*0x%x, height=*0x%x)", member_id, width, height);

	if (!width || !height)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2SetStreamPriority(u8 priority)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetStreamPriority(priority=0x%x)", priority);
	return CELL_OK;
}

error_code cellSysutilAvc2LeaveChatRequest()
{
	cellSysutilAvc2.notice("cellSysutilAvc2LeaveChatRequest()");

	if (avc2_cb)
	{
		sysutil_register_cb([=](ppu_thread& cb_ppu) -> s32
		{
			avc2_cb(cb_ppu, CELL_AVC2_EVENT_LEAVE_SUCCEEDED, 0, avc2_cb_arg);
			return 0;
		});
	}

	return CELL_OK;
}

error_code cellSysutilAvc2IsMicAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.todo("cellSysutilAvc2IsMicAttached(status=*0x%x)", status);

	ensure(!!status); // I couldn't find any error value for this (should be CELL_AVC2_ERROR_INVALID_ARGUMENT if anything)

	*status = CELL_AVC2_MIC_STATUS_DETACHED;

	return CELL_OK;
}

error_code cellSysutilAvc2CreateWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2CreateWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2GetSpeakerMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetSpeakerMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*muting = 1;

	return CELL_OK;
}

error_code cellSysutilAvc2ShowWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2ShowWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowSize(SceNpMatching2RoomMemberId member_id, f32 width, f32 height)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowSize(member_id=0x%x, width=0x%x, height=0x%x)", member_id, width, height);
	return CELL_OK;
}

error_code cellSysutilAvc2EnumPlayers(vm::ptr<s32> players_num, vm::ptr<SceNpMatching2RoomMemberId> players_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2EnumPlayers(players_num=*0x%x, players_id=*0x%x)", players_num, players_id);

	if (!players_num)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	// Apparently this function is supposed to be called twice.
	// Once with null to get the player count and then again to fill the ID list.
	if (players_id)
	{
		for (int i = 0; i < *players_num; i++)
		{
			players_id[i] = i + 1;
		}
	}
	else
	{
		*players_num = 1;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowString(SceNpMatching2RoomMemberId member_id, vm::ptr<char> string, vm::ptr<u8> len)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowString(member_id=0x%x, string=*0x%x, len=*0x%x)", member_id, string, len);

	if (!string || !len)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2LeaveChat()
{
	cellSysutilAvc2.todo("cellSysutilAvc2LeaveChat()");
	return CELL_OK;
}

error_code cellSysutilAvc2SetSpeakerMuting(u8 muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetSpeakerMuting(muting=0x%x)", muting);
	return CELL_OK;
}

error_code cellSysutilAvc2Load(SceNpMatching2ContextId ctx_id, u32 container, vm::ptr<CellSysutilAvc2Callback> callback_func, vm::ptr<void> user_data, vm::cptr<CellSysutilAvc2InitParam> init_param)
{
	cellSysutilAvc2.todo("cellSysutilAvc2Load(ctx_id=0x%x, container=0x%x, callback_func=*0x%x, user_data=*0x%x, init_param=*0x%x)", ctx_id, container, callback_func, user_data, init_param);
	return CELL_OK;
}

error_code cellSysutilAvc2SetAttribute(vm::cptr<CellSysutilAvc2Attribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetAttribute(attr=*0x%x)", attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION:
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2UnloadAsync2(u32 mediaType)
{
	cellSysutilAvc2.todo("cellSysutilAvc2UnloadAsync2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2StartStreaming2(u32 mediaType)
{
	cellSysutilAvc2.todo("cellSysutilAvc2StartStreaming2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2HideScreen()
{
	cellSysutilAvc2.todo("cellSysutilAvc2HideScreen()");
	return CELL_OK;
}

error_code cellSysutilAvc2HideWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2HideWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2GetVoiceMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetVoiceMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*muting = 1;

	return CELL_OK;
}

error_code cellSysutilAvc2GetScreenShowStatus(vm::ptr<u8> visible)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetScreenShowStatus(visible=*0x%x)", visible);

	if (!visible)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*visible = 0;

	return CELL_OK;
}

error_code cellSysutilAvc2Unload2(u32 mediaType)
{
	cellSysutilAvc2.todo("cellSysutilAvc2Unload2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowPosition(SceNpMatching2RoomMemberId member_id, vm::ptr<f32> x, vm::ptr<f32> y, vm::ptr<f32> z)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowPosition(member_id=0x%x, x=*0x%x, y=*0x%x, z=*0x%x)", member_id, x, y, z);

	if (!x || !y || !z)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellSysutilAvc2)("cellSysutilAvc2", []()
{
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetPlayerInfo);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2JoinChat);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopStreaming);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ChangeVideoResolution);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ShowScreen);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetVideoMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopStreaming2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartVoiceDetection);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2UnloadAsync);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopVoiceDetection);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LoadAsync);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetSpeakerVolumeLevel);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowString);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2EstimateMemoryContainerSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetVideoMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetPlayerVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetStreamingTarget);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Unload);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2DestroyWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowPosition);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetSpeakerVolumeLevel);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2IsCameraAttached);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2MicRead);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetPlayerVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2JoinChatRequest);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartStreaming);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowShowStatus);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2InitParam);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetStreamPriority);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LeaveChatRequest);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2IsMicAttached);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2CreateWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetSpeakerMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ShowWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2EnumPlayers);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowString);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LeaveChat);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetSpeakerMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Load);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2UnloadAsync2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartStreaming2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2HideScreen);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2HideWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetScreenShowStatus);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Unload2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowPosition);
});
