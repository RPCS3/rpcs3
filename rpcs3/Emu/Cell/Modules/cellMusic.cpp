#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "cellSearch.h"
#include "cellSpurs.h"
#include "cellSysutil.h"

#include "cellMusic.h"

LOG_CHANNEL(cellMusic);

template<>
void fmt_class_string<CellMusicError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_CANCELED);
			STR_CASE(CELL_MUSIC_PLAYBACK_FINISHED);
			STR_CASE(CELL_MUSIC_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC_ERROR_PLAYBACK_FAILURE);
			STR_CASE(CELL_MUSIC_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC_ERROR_GENERIC);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellMusic2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC2_CANCELED);
			STR_CASE(CELL_MUSIC2_PLAYBACK_FINISHED);
			STR_CASE(CELL_MUSIC2_ERROR_PARAM);
			STR_CASE(CELL_MUSIC2_ERROR_BUSY);
			STR_CASE(CELL_MUSIC2_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC2_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC2_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC2_ERROR_PLAYBACK_FAILURE);
			STR_CASE(CELL_MUSIC2_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC2_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC2_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC2_ERROR_GENERIC);
		}

		return unknown;
	});
}

struct music_state
{
	vm::ptr<void(u32 event, vm::ptr<void> param, vm::ptr<void> userData)> func;
	vm::ptr<void> userData;
};

error_code cellMusicGetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicGetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicSetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicSetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC2_ERROR_PARAM;

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume2(f32 level)
{
	cellMusic.todo("cellMusicSetVolume2(level=0x%x)", level);

	level = std::clamp(level, 0.0f, 1.0f);

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_SET_VOLUME_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetContentsId(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusic.todo("cellMusicGetContentsId(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicSetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicSetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_ERROR_PARAM;

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize2SystemWorkload(s32 mode, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusic.todo("cellMusicInitialize2SystemWorkload(mode=0x%x, func=*0x%x, userData=*0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, func, userData, spurs, priority, attr);

	if (!func)
		return CELL_MUSIC2_ERROR_PARAM;

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC2_ERROR_PARAM;
	}

	const auto music = g_fxo->get<music_state>();
	music->func = func;
	music->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus2(vm::ptr<s32> status)
{
	cellMusic.todo("cellMusicGetPlaybackStatus2(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC2_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicGetContentsId2(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusic.todo("cellMusicGetContentsId2(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC2_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicFinalize()
{
	cellMusic.todo("cellMusicFinalize()");

	const auto music = g_fxo->get<music_state>();

	if (music->func)
	{
		sysutil_register_cb([=](ppu_thread& ppu) -> s32
		{
			music->func(ppu, CELL_MUSIC_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), music->userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicInitializeSystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicCallback> func, vm::ptr<void> userData, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusic.todo("cellMusicInitializeSystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spurs, priority, attr);

	if (!func)
		return CELL_MUSIC_ERROR_PARAM;

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC_ERROR_PARAM;
	}

	const auto music = g_fxo->get<music_state>();
	music->func = func;
	music->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicCallback> func, vm::ptr<void> userData)
{
	cellMusic.todo("cellMusicInitialize(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x)", mode, container, spuPriority, func, userData);

	if (!func)
		return CELL_MUSIC_ERROR_PARAM;

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC_ERROR_PARAM;
	}

	const auto music = g_fxo->get<music_state>();
	music->func = func;
	music->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicFinalize2()
{
	cellMusic.todo("cellMusicFinalize2()");

	const auto music = g_fxo->get<music_state>();

	if (music->func)
	{
		sysutil_register_cb([=](ppu_thread& ppu) -> s32
		{
			music->func(ppu, CELL_MUSIC2_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), music->userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicGetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicGetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC2_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicGetVolume(vm::ptr<f32> level)
{
	cellMusic.todo("cellMusicGetVolume(level=*0x%x)", level);

	if (!level)
		return CELL_MUSIC_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus(vm::ptr<s32> status)
{
	cellMusic.todo("cellMusicGetPlaybackStatus(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC_ERROR_PARAM;

	return CELL_OK;
}

error_code cellMusicSetPlaybackCommand2(s32 command, vm::ptr<void> param)
{
	cellMusic.todo("cellMusicSetPlaybackCommand2(command=0x%x, param=*0x%x)", command, param);

	if (command < CELL_MUSIC_PB_CMD_STOP || command > CELL_MUSIC_PB_CMD_FASTREVERSE)
		return CELL_MUSIC2_ERROR_PARAM;

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetPlaybackCommand(s32 command, vm::ptr<void> param)
{
	cellMusic.todo("cellMusicSetPlaybackCommand(command=0x%x, param=*0x%x)", command, param);

	if (command < CELL_MUSIC_PB_CMD_STOP || command > CELL_MUSIC_PB_CMD_FASTREVERSE)
		return CELL_MUSIC_ERROR_PARAM;

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSelectContents2()
{
	cellMusic.todo("cellMusicSelectContents2()");

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSelectContents(u32 container)
{
	cellMusic.todo("cellMusicSelectContents(container=0x%x)", container);

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize2(s32 mode, s32 spuPriority, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData)
{
	cellMusic.todo("cellMusicInitialize2(mode=%d, spuPriority=%d, func=*0x%x, userData=*0x%x)", mode, spuPriority, func, userData);

	if (!func)
		return CELL_MUSIC2_ERROR_PARAM;

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC2_ERROR_PARAM;
	}

	const auto music = g_fxo->get<music_state>();
	music->func = func;
	music->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume(f32 level)
{
	cellMusic.todo("cellMusicSetVolume(level=0x%x)", level);

	level = std::clamp(level, 0.0f, 1.0f);

	const auto music = g_fxo->get<music_state>();

	if (!music->func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		music->func(ppu, CELL_MUSIC_EVENT_SET_VOLUME_RESULT, vm::addr_t(CELL_OK), music->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetVolume2(vm::ptr<f32> level)
{
	cellMusic.todo("cellMusicGetVolume2(level=*0x%x)", level);

	if (!level)
		return CELL_MUSIC2_ERROR_PARAM;

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellMusic)("cellMusicUtility", []()
{
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2SystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId2);
	REG_FUNC(cellMusicUtility, cellMusicFinalize);
	REG_FUNC(cellMusicUtility, cellMusicInitializeSystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicInitialize);
	REG_FUNC(cellMusicUtility, cellMusicFinalize2);
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand2);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents2);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume2);
});
