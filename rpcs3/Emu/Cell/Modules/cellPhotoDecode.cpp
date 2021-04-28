#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellSysutil.h"



LOG_CHANNEL(cellPhotoDecode);

// Return Codes
enum CellPhotoDecodeError : u32
{
	CELL_PHOTO_DECODE_ERROR_BUSY         = 0x8002c901,
	CELL_PHOTO_DECODE_ERROR_INTERNAL     = 0x8002c902,
	CELL_PHOTO_DECODE_ERROR_PARAM        = 0x8002c903,
	CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR = 0x8002c904,
	CELL_PHOTO_DECODE_ERROR_INITIALIZE   = 0x8002c905,
	CELL_PHOTO_DECODE_ERROR_DECODE       = 0x8002c906,
};

template<>
void fmt_class_string<CellPhotoDecodeError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PHOTO_DECODE_ERROR_BUSY);
			STR_CASE(CELL_PHOTO_DECODE_ERROR_INTERNAL);
			STR_CASE(CELL_PHOTO_DECODE_ERROR_PARAM);
			STR_CASE(CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_PHOTO_DECODE_ERROR_INITIALIZE);
			STR_CASE(CELL_PHOTO_DECODE_ERROR_DECODE);
		}

		return unknown;
	});
}

struct CellPhotoDecodeSetParam
{
	vm::bptr<void> dstBuffer;
	be_t<u16> width;
	be_t<u16> height;
	vm::bptr<void> reserved1;
	vm::bptr<void> reserved2;
};

struct CellPhotoDecodeReturnParam
{
	be_t<u16> width;
	be_t<u16> height;
	vm::bptr<void> reserved1;
	vm::bptr<void> reserved2;
};

using CellPhotoDecodeFinishCallback = void(s32 result, vm::ptr<void> userdata);

error_code cellPhotoDecodeInitialize(u32 version, u32 container1, u32 container2, vm::ptr<CellPhotoDecodeFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoDecode.todo("cellPhotoDecodeInitialize(version=0x%x, container1=0x%x, container2=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container1, container2, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeInitialize2(u32 version, u32 container2, vm::ptr<CellPhotoDecodeFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoDecode.todo("cellPhotoDecodeInitialize2(version=0x%x, container2=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container2, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeFinalize(vm::ptr<CellPhotoDecodeFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoDecode.todo("cellPhotoDecodeFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoDecodeSetParam> set_param, vm::ptr<CellPhotoDecodeReturnParam> return_param)
{
	cellPhotoDecode.todo("cellPhotoDecodeFromFile(srcHddDir=%s, srcHddFile=%s, set_param=*0x%x, return_param=*0x%x)", srcHddDir, srcHddFile, set_param, return_param);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPhotoDecode)("cellPhotoDecodeUtil", []()
{
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeInitialize);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeInitialize2);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeFinalize);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeFromFile);
});
