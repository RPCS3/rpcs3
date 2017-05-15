#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNp.h"
#include "sceNp2.h"
#include "cellSysutilAvc2.h"

logs::channel cellSysutilAvc2("cellSysutilAvc2");

s32 cellSysutilAvc2GetPlayerInfo()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2JoinChat()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StopStreaming()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2ChangeVideoResolution()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2ShowScreen()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetVideoMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetWindowAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StopStreaming2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetVoiceMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StartVoiceDetection()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2UnloadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StopVoiceDetection()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2LoadAsync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetSpeakerVolumeLevel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetWindowString()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2EstimateMemoryContainerSize()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvc2); 
  	return CELL_OK;
}

s32 cellSysutilAvc2SetVideoMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetPlayerVoiceMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetStreamingTarget()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2Unload()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2DestroyWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetWindowPosition()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetSpeakerVolumeLevel()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2IsCameraAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.todo("cellSysutilAvc2IsCameraAttached()");

	*status = CELL_AVC2_CAMERA_STATUS_DETACHED;
	return CELL_OK;
}

s32 cellSysutilAvc2MicRead()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetPlayerVoiceMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2JoinChatRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StartStreaming()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetWindowAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetWindowShowStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2InitParam(u16 version, vm::ptr<CellSysutilAvc2InitParam> option)
{
	cellSysutilAvc2.warning("cellSysutilAvc2InitParam(version=%d, option=*0x%x)", version, option);

	if (version >= 110)
	{
		// Notify the user that, a version different from the one, that we know the constants for, is used.
		// Other versions shouldn't differ by too much, if at all - they most likely differ in other functions.
		if (version != 140)
		{
			cellSysutilAvc2.todo("cellSysutilAvc2InitParam(): Older/newer version %d used, might cause problems.", version);
		}

		option->avc_init_param_version = version;

		if (option->media_type == CELL_SYSUTIL_AVC2_VOICE_CHAT)
		{
			option->max_players = 16;
		}
		else if (option->media_type == CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		{
			if (option->video_param.frame_mode == CELL_SYSUTIL_AVC2_FRAME_MODE_NORMAL)
			{
				option->max_players = 6;
			}
			else if (option->video_param.frame_mode == CELL_SYSUTIL_AVC2_FRAME_MODE_INTRA_ONLY)
			{
				option->max_players = 16;
			}
			else
			{
				cellSysutilAvc2.error("Unknown frame mode 0x%x", option->video_param.frame_mode);
			}
		}
		else
		{
			cellSysutilAvc2.error("Unknown media type 0x%x", option->media_type);
		}
	}
	else
	{
		cellSysutilAvc2.error("cellSysutilAvc2InitParam(): Unknown version %d used, please report this to a developer.", version);
	}

	return CELL_OK;
}

s32 cellSysutilAvc2GetWindowSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetStreamPriority()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2LeaveChatRequest()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2IsMicAttached()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2CreateWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetSpeakerMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2ShowWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetWindowSize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2EnumPlayers()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetWindowString()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2LeaveChat()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetSpeakerMuting()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2Load()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2SetAttribute()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2UnloadAsync2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2StartStreaming2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2HideScreen()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2HideWindow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetVoiceMuting()
{
	UNIMPLEMENTED_FUNC(cellSysutilAvc2); 
  	return CELL_OK;
}

s32 cellSysutilAvc2GetScreenShowStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2Unload2()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilAvc2GetWindowPosition()
{
	fmt::throw_exception("Unimplemented" HERE);
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
