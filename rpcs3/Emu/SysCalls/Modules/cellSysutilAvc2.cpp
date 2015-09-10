#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "rpcs3/Ini.h"

#include "sceNp.h"
#include "sceNp2.h"
#include "cellSysutilAvc2.h"

extern Module cellSysutilAvc2;

s32 cellSysutilAvc2GetPlayerInfo()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2JoinChat()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StopStreaming()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2ChangeVideoResolution()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2ShowScreen()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetVideoMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetWindowAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StopStreaming2()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StartVoiceDetection()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2UnloadAsync()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StopVoiceDetection()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2LoadAsync()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetSpeakerVolumeLevel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetWindowString()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2EstimateMemoryContainerSize()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetVideoMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetPlayerVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetStreamingTarget()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2Unload()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2DestroyWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetWindowPosition()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetSpeakerVolumeLevel()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2IsCameraAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.Todo("cellSysutilAvc2IsCameraAttached()");

	if (Ini.Camera.GetValue() == 0)
	{
		*status = CELL_AVC2_CAMERA_STATUS_DETACHED;
	}
	else
	{
		// TODO: We need to check if the camera has been turned on, but this requires further implementation of cellGem/cellCamera.
		*status = CELL_AVC2_CAMERA_STATUS_ATTACHED_OFF;
	}

	return CELL_OK;
}

s32 cellSysutilAvc2MicRead()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetPlayerVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2JoinChatRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StartStreaming()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetWindowAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetWindowShowStatus()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2InitParam(u16 version, vm::ptr<CellSysutilAvc2InitParam> option)
{
	cellSysutilAvc2.Warning("cellSysutilAvc2InitParam(version=%d, option=*0x%x)", version, option);

	if (version >= 110)
	{
		// Notify the user that, a version different from the one, that we know the constants for, is used.
		// Other versions shouldn't differ by too much, if at all - they most likely differ in other functions.
		if (version != 140)
		{
			cellSysutilAvc2.Todo("cellSysutilAvc2InitParam(): Older/newer version %d used, might cause problems.", version);
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
				cellSysutilAvc2.Error("Unknown frame mode 0x%x", option->video_param.frame_mode);
			}
		}
		else
		{
			cellSysutilAvc2.Error("Unknown media type 0x%x", option->media_type);
		}
	}
	else
	{
		cellSysutilAvc2.Error("cellSysutilAvc2InitParam(): Unknown version %d used, please report this to a developer.", version);
	}

	return CELL_OK;
}

s32 cellSysutilAvc2GetWindowSize()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetStreamPriority()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2LeaveChatRequest()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2IsMicAttached()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2CreateWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetSpeakerMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2ShowWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetWindowSize()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2EnumPlayers()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetWindowString()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2LeaveChat()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetSpeakerMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2Load()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2SetAttribute()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2UnloadAsync2()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2StartStreaming2()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2HideScreen()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2HideWindow()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetVoiceMuting()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetScreenShowStatus()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2Unload2()
{
	throw EXCEPTION("");
}

s32 cellSysutilAvc2GetWindowPosition()
{
	throw EXCEPTION("");
}


Module cellSysutilAvc2("cellSysutilAvc2", []()
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
