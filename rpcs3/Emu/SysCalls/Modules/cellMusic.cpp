#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/Modules.h"

#include "cellMusic.h"
#include "Emu/SysCalls/lv2/sys_spu.h"

extern Module cellMusic;

struct music2_t
{
	vm::ptr<CellMusic2Callback> func;
	vm::ptr<void> userData;
	//vm::ptr<u32> tgId;
	//vm::ptr<u32> thId;
};

s32 cellMusicGetSelectionContext()
{
	throw EXCEPTION("");
}

s32 cellMusicSetSelectionContext2()
{
	throw EXCEPTION("");
}

s32 cellMusicSetVolume2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetContentsId()
{
	throw EXCEPTION("");
}

s32 cellMusicSetSelectionContext()
{
	throw EXCEPTION("");
}

s32 cellMusicInitialize2SystemWorkload()
{
	throw EXCEPTION("");
}

s32 cellMusicGetPlaybackStatus2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetContentsId2()
{
	throw EXCEPTION("");
}

s32 cellMusicFinalize()
{
	throw EXCEPTION("");
}

s32 cellMusicInitializeSystemWorkload()
{
	throw EXCEPTION("");
}

s32 cellMusicInitialize()
{
	throw EXCEPTION("");
}

s32 cellMusicFinalize2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetSelectionContext2()
{
	throw EXCEPTION("");
}

s32 cellMusicGetVolume()
{
	throw EXCEPTION("");
}

s32 cellMusicGetPlaybackStatus()
{
	throw EXCEPTION("");
}

s32 cellMusicSetPlaybackCommand2()
{
	throw EXCEPTION("");
}

s32 cellMusicSetPlaybackCommand()
{
	throw EXCEPTION("");
}

s32 cellMusicSelectContents2()
{
	throw EXCEPTION("");
}

s32 cellMusicSelectContents()
{
	throw EXCEPTION("");
}

s32 cellMusicInitialize2(s32 mode, s32 spuPriority, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData)
{
	cellMusic.Todo("cellMusicInitialize2(mode=%d, spuPriority=%d, func=*0x%x, userData=*0x%x)", mode, spuPriority, func, userData);
	
	named_thread_t(WRAP_EXPR("CellMusicInit"), [=]()
	{
		// TODO: Implement using SPUs for no reason or go the standard way. 
		/*vm::var<sys_spu_thread_group_attribute> tgAttr(ppu);
		vm::var<sys_spu_thread_attribute>       thAttr(ppu);
		vm::var<sys_spu_thread_argument>        thArg(ppu);
		vm::var<sys_spu_image>                  thImg(ppu);
		vm::var<u32>                            tgId(ppu);
		vm::var<u32>                            thId(ppu);
		vm::var<char>                           tgName(ppu, 128);
		vm::var<char>                           thName(ppu, 128);*/
		//vm::var<u32> ret(ppu);

		if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
		{
			cellMusic.Todo("Unknown player mode: 0x%x", mode);
			Emu.GetCallbackManager().Async([=](CPUThread& CPU)
			{
				vm::var<u32> ret(CPU);
				*ret = CELL_MUSIC2_ERROR_PARAM;
				func(static_cast<PPUThread&>(CPU), CELL_MUSIC2_EVENT_INITIALIZE_RESULT, ret, userData);
			});
			return;
		}

		const auto music = fxm::make<music2_t>();

		if (!music)
		{
			Emu.GetCallbackManager().Async([=](CPUThread& CPU)
			{
				vm::var<u32> ret(CPU);
				*ret = CELL_MUSIC2_ERROR_GENERIC;
				func(static_cast<PPUThread&>(CPU), CELL_MUSIC2_EVENT_INITIALIZE_RESULT, ret, userData);
			});
			return;
		}

		/*strcpy(tgName.get_ptr(), "CellMusicGroup");
		sys_spu_thread_group_attribute_initialize(tgAttr);
		tgAttr->name = tgName;
		tgAttr->nsize = (u32)strlen(tgAttr->name.get_ptr()) + 1;
		tgAttr->type = SYS_SPU_THREAD_GROUP_TYPE_SYSTEM;

		s32 ret = sys_spu_thread_group_create(tgId, 1, spuPriority, tgAttr);
		if (ret != CELL_OK)
		{
			cellMusic.Error("Creating a SPU thread group for cellMusic failed.");
			func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::ptr<void>::make(CELL_MUSIC2_ERROR_GENERIC), userData);
		}

		strcpy(thName.get_ptr(), "CellMusic");

		sys_spu_thread_attribute_initialize(thAttr);
		sys_spu_thread_argument_initialize(thArg);
		// TODO: Figure out what the SPU(s) does, implement it, then load the image(s) for it.
		//ret = sys_spu_image_open(thImg, vm::null);

		thAttr->name = thName;
		thAttr->name_len = strlen(thName.get_ptr()) + 1;

		ret = sys_spu_thread_initialize(thId, *tgId, 1, thImg, thAttr, thArg);
		if (ret != CELL_OK)
		{
			cellMusic.Error("Creating a SPU thread for cellMusic failed.");
			func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::ptr<void>::make(CELL_MUSIC2_ERROR_GENERIC), userData);
		}

		music->tgId = tgId;
		music->thId = thId;*/
		music->func = func;
		music->userData = userData;

		Emu.GetCallbackManager().Async([=](CPUThread& CPU)
		{
			vm::var<u32> ret(CPU);
			*ret = CELL_OK;
			func(static_cast<PPUThread&>(CPU), CELL_MUSIC2_EVENT_INITIALIZE_RESULT, ret, userData);
		});
		return;
	}).detach();

	return CELL_OK;
}

s32 cellMusicSetVolume()
{
	throw EXCEPTION("");
}

s32 cellMusicGetVolume2()
{
	throw EXCEPTION("");
}


Module cellMusic("cellMusic", []()
{
	REG_FUNC(cellMusic, cellMusicGetSelectionContext);
	REG_FUNC(cellMusic, cellMusicSetSelectionContext2);
	REG_FUNC(cellMusic, cellMusicSetVolume2);
	REG_FUNC(cellMusic, cellMusicGetContentsId);
	REG_FUNC(cellMusic, cellMusicSetSelectionContext);
	REG_FUNC(cellMusic, cellMusicInitialize2SystemWorkload);
	REG_FUNC(cellMusic, cellMusicGetPlaybackStatus2);
	REG_FUNC(cellMusic, cellMusicGetContentsId2);
	REG_FUNC(cellMusic, cellMusicFinalize);
	REG_FUNC(cellMusic, cellMusicInitializeSystemWorkload);
	REG_FUNC(cellMusic, cellMusicInitialize);
	REG_FUNC(cellMusic, cellMusicFinalize2);
	REG_FUNC(cellMusic, cellMusicGetSelectionContext2);
	REG_FUNC(cellMusic, cellMusicGetVolume);
	REG_FUNC(cellMusic, cellMusicGetPlaybackStatus);
	REG_FUNC(cellMusic, cellMusicSetPlaybackCommand2);
	REG_FUNC(cellMusic, cellMusicSetPlaybackCommand);
	REG_FUNC(cellMusic, cellMusicSelectContents2);
	REG_FUNC(cellMusic, cellMusicSelectContents);
	REG_FUNC(cellMusic, cellMusicInitialize2);
	REG_FUNC(cellMusic, cellMusicSetVolume);
	REG_FUNC(cellMusic, cellMusicGetVolume2);
});
