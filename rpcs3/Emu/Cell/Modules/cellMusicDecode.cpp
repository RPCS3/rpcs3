#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "cellMusic.h"
#include "cellSearch.h"
#include "cellSpurs.h"
#include "cellSysutil.h"



LOG_CHANNEL(cellMusicDecode);

// Return Codes
enum CellMusicDecodeError : u32
{
	CELL_MUSIC_DECODE_DECODE_FINISHED         = 0x8002C101,
	CELL_MUSIC_DECODE_ERROR_PARAM             = 0x8002C102,
	CELL_MUSIC_DECODE_ERROR_BUSY              = 0x8002C103,
	CELL_MUSIC_DECODE_ERROR_NO_ACTIVE_CONTENT = 0x8002C104,
	CELL_MUSIC_DECODE_ERROR_NO_MATCH_FOUND    = 0x8002C105,
	CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT   = 0x8002C106,
	CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE    = 0x8002C107,
	CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT   = 0x8002C108,
	CELL_MUSIC_DECODE_DIALOG_OPEN             = 0x8002C109,
	CELL_MUSIC_DECODE_DIALOG_CLOSE            = 0x8002C10A,
	CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA      = 0x8002C10B,
	CELL_MUSIC_DECODE_NEXT_CONTENTS_READY     = 0x8002C10C,
	CELL_MUSIC_DECODE_ERROR_GENERIC           = 0x8002C1FF,
};

template<>
void fmt_class_string<CellMusicDecodeError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_DECODE_DECODE_FINISHED);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC_DECODE_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC_DECODE_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA);
			STR_CASE(CELL_MUSIC_DECODE_NEXT_CONTENTS_READY);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_GENERIC);
		}

		return unknown;
	});
}

enum
{
	CELL_MUSIC_DECODE_EVENT_STATUS_NOTIFICATION = 0,
	CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT = 1,
	CELL_MUSIC_DECODE_EVENT_FINALIZE_RESULT = 2,
	CELL_MUSIC_DECODE_EVENT_SELECT_CONTENTS_RESULT = 3,
	CELL_MUSIC_DECODE_EVENT_SET_DECODE_COMMAND_RESULT = 4,
	CELL_MUSIC_DECODE_EVENT_SET_SELECTION_CONTEXT_RESULT = 5,
	CELL_MUSIC_DECODE_EVENT_UI_NOTIFICATION = 6,
	CELL_MUSIC_DECODE_EVENT_NEXT_CONTENTS_READY_RESULT = 7,
};

enum
{
	CELL_MUSIC_DECODE2_EVENT_STATUS_NOTIFICATION = 0,
	CELL_MUSIC_DECODE2_EVENT_INITIALIZE_RESULT = 1,
	CELL_MUSIC_DECODE2_EVENT_FINALIZE_RESULT = 2,
	CELL_MUSIC_DECODE2_EVENT_SELECT_CONTENTS_RESULT = 3,
	CELL_MUSIC_DECODE2_EVENT_SET_DECODE_COMMAND_RESULT = 4,
	CELL_MUSIC_DECODE2_EVENT_SET_SELECTION_CONTEXT_RESULT = 5,
	CELL_MUSIC_DECODE2_EVENT_UI_NOTIFICATION = 6,
	CELL_MUSIC_DECODE2_EVENT_NEXT_CONTENTS_READY_RESULT = 7,
};

using CellMusicDecodeCallback = void(u32, vm::ptr<void> param, vm::ptr<void> userData);
using CellMusicDecode2Callback = void(u32, vm::ptr<void> param, vm::ptr<void> userData);

struct music_decode
{
	vm::ptr<CellMusicDecodeCallback> func;
	vm::ptr<void> userData;
};

struct music_decode2
{
	vm::ptr<CellMusicDecode2Callback> func;
	vm::ptr<void> userData;
};

error_code cellMusicDecodeInitialize(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecodeCallback> func, vm::ptr<void> userData)
{
	cellMusicDecode.todo("cellMusicDecodeInitialize(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x)", mode, container, spuPriority, func, userData);

	const auto musicDecode = g_fxo->get<music_decode>();
	musicDecode->func = func;
	musicDecode->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeInitializeSystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicDecodeCallback> func, vm::ptr<void> userData, s32 spuUsageRate, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusicDecode.todo("cellMusicDecodeInitializeSystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, spurs, priority, attr);

	const auto musicDecode = g_fxo->get<music_decode>();
	musicDecode->func = func;
	musicDecode->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeFinalize()
{
	cellMusicDecode.todo("cellMusicDecodeFinalize()");

	const auto musicDecode = g_fxo->get<music_decode>();

	if (musicDecode->func)
	{
		sysutil_register_cb([=](ppu_thread& ppu) -> s32
		{
			musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicDecodeSelectContents()
{
	cellMusicDecode.todo("cellMusicDecodeSelectContents()");

	const auto musicDecode = g_fxo->get<music_decode>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeSetDecodeCommand(s32 command)
{
	cellMusicDecode.todo("cellMusicDecodeSetDecodeCommand(command=0x%x)", command);

	const auto musicDecode = g_fxo->get<music_decode>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_SET_DECODE_COMMAND_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetDecodeStatus(vm::ptr<s32> status)
{
	cellMusicDecode.todo("cellMusicDecodeGetDecodeStatus(status=*0x%x)", status);
	return CELL_OK;
}

error_code cellMusicDecodeRead(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.todo("cellMusicDecodeRead(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);
	return CELL_OK;
}

error_code cellMusicDecodeGetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeGetSelectionContext(context=*0x%x)", context);
	return CELL_OK;
}

error_code cellMusicDecodeSetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeSetSelectionContext(context=*0x%x)", context);

	const auto musicDecode = g_fxo->get<music_decode>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetContentsId(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusicDecode.todo("cellMusicDecodeGetContentsId(contents_id=*0x%x)", contents_id);
	return CELL_OK;
}

error_code cellMusicDecodeInitialize2(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecode2Callback> func, vm::ptr<void> userData, s32 speed, s32 bufsize)
{
	cellMusicDecode.todo("cellMusicDecodeInitialize2(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x, speed=0x%x, bufsize=0x%x)", mode, container, spuPriority, func, userData, speed, bufsize);

	const auto musicDecode = g_fxo->get<music_decode2>();
	musicDecode->func = func;
	musicDecode->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeInitialize2SystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicDecode2Callback> func, vm::ptr<void> userData, s32 spuUsageRate, s32 bufsize, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<CellSpursSystemWorkloadAttribute> attr)
{
	cellMusicDecode.todo("cellMusicDecodeInitialize2SystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, bufsize=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, bufsize, spurs, priority, attr);

	const auto musicDecode = g_fxo->get<music_decode2>();
	musicDecode->func = func;
	musicDecode->userData = userData;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeFinalize2()
{
	cellMusicDecode.todo("cellMusicDecodeFinalize2()");

	const auto musicDecode = g_fxo->get<music_decode2>();

	if (musicDecode->func)
	{
		sysutil_register_cb([=](ppu_thread& ppu) -> s32
		{
			musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicDecodeSelectContents2()
{
	cellMusicDecode.todo("cellMusicDecodeSelectContents2()");

	const auto musicDecode = g_fxo->get<music_decode2>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeSetDecodeCommand2(s32 command)
{
	cellMusicDecode.todo("cellMusicDecodeSetDecodeCommand2(command=0x%x)", command);

	const auto musicDecode = g_fxo->get<music_decode2>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_SET_DECODE_COMMAND_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetDecodeStatus2(vm::ptr<s32> status)
{
	cellMusicDecode.todo("cellMusicDecodeGetDecodeStatus2(status=*0x%x)", status);
	return CELL_OK;
}

error_code cellMusicDecodeRead2(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.todo("cellMusicDecodeRead2(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);
	return CELL_OK;
}

error_code cellMusicDecodeGetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeGetSelectionContext2(context=*0x%x)", context);
	return CELL_OK;
}

error_code cellMusicDecodeSetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeSetSelectionContext2(context=*0x%x)", context);

	const auto musicDecode = g_fxo->get<music_decode2>();

	if (!musicDecode->func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		musicDecode->func(ppu, CELL_MUSIC_DECODE2_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(CELL_OK), musicDecode->userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetContentsId2(vm::ptr<CellSearchContentId> contents_id )
{
	cellMusicDecode.todo("cellMusicDecodeGetContentsId2(contents_id=*0x%x)", contents_id);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellMusicDecode)("cellMusicDecodeUtility", []()
{
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitializeSystemWorkload);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeFinalize);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSelectContents);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetDecodeCommand);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetDecodeStatus);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeRead);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetSelectionContext);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetSelectionContext);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetContentsId);

	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize2SystemWorkload);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeFinalize2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSelectContents2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetDecodeCommand2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetDecodeStatus2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeRead2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetSelectionContext2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetSelectionContext2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetContentsId2);
});
