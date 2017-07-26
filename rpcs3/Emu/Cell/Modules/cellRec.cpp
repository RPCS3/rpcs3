#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

namespace vm { using namespace ps3; }

logs::channel cellRec("cellRec");

struct CellRecSpursParam
{
	vm::bptr<struct CellSpurs> pSpurs;
	be_t<s32> spu_usage_rate;
	u8 priority[8];
};

struct CellRecOption
{
	be_t<s32> option;
	union
	{
		be_t<s32> ppu_thread_priority;
		be_t<s32> spu_thread_priority;
		be_t<s32> capture_priority;
		be_t<s32> use_system_spu;
		be_t<s32> fit_to_youtube;
		be_t<s32> xmb_bgm;
		be_t<s32> mpeg4_fast_encode;
		be_t<u32> ring_sec;
		be_t<s32> video_input;
		be_t<s32> audio_input;
		be_t<s32> audio_input_mix_vol;
		be_t<s32> reduce_memsize;
		be_t<s32> show_xmb;
		vm::bptr<char> metadata_filename;
		vm::bptr<CellRecSpursParam> pSpursParam;
		be_t<u64> dummy;
	} value;
};

struct CellRecParam
{
	be_t<s32> videoFmt;
	be_t<s32> audioFmt;
	be_t<s32> numOfOpt;
	vm::bptr<CellRecOption> pOpt;
};

using CellRecCallback = void(s32 recStatus, s32 recError, vm::ptr<void> userdata);

s32 cellRecOpen(vm::cptr<char> pDirName, vm::cptr<char> pFileName, vm::cptr<CellRecParam> pParam, u32 container, vm::ptr<CellRecCallback> cb, vm::ptr<void> cbUserData)
{
	cellRec.todo("cellRecOpen(pDirName=%s, pFileName=%s, pParam=*0x%x, container=0x%x, cb=*0x%x, cbUserData=*0x%x)", pDirName, pFileName, pParam, container, cb, cbUserData);
	return CELL_OK;
}

s32 cellRecClose(s32 isDiscard)
{
	cellRec.todo("cellRecClose(isDiscard=0x%x)", isDiscard);
	return CELL_OK;
}

void cellRecGetInfo(s32 info, vm::ptr<u64> pValue)
{
	cellRec.todo("cellRecGetInfo(info=0x%x, pValue=*0x%x)", info, pValue);
}

s32 cellRecStop()
{
	cellRec.todo("cellRecStop()");
	return CELL_OK;
}

s32 cellRecStart()
{
	cellRec.todo("cellRecStart()");
	return CELL_OK;
}

u32 cellRecQueryMemSize(vm::cptr<CellRecParam> pParam)
{
	cellRec.todo("cellRecQueryMemSize(pParam=*0x%x)", pParam);
	return 1 * 1024 * 1024; // dummy memory size
}

s32 cellRecSetInfo(s32 setInfo, u64 value)
{
	cellRec.todo("cellRecSetInfo(setInfo=0x%x, value=0x%x)", setInfo, value);
	return CELL_OK;
}


DECLARE(ppu_module_manager::cellRec)("cellRec", []()
{
	REG_FUNC(cellRec, cellRecOpen);
	REG_FUNC(cellRec, cellRecClose);
	REG_FUNC(cellRec, cellRecGetInfo);
	REG_FUNC(cellRec, cellRecStop);
	REG_FUNC(cellRec, cellRecStart);
	REG_FUNC(cellRec, cellRecQueryMemSize);
	REG_FUNC(cellRec, cellRecSetInfo);
});
