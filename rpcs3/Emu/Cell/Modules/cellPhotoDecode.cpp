#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/VFS.h"
#include "Emu/System.h"
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

enum
{
	CELL_PHOTO_DECODE_VERSION_CURRENT = 0
};

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
	cellPhotoDecode.warning("cellPhotoDecodeInitialize(version=0x%x, container1=0x%x, container2=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container1, container2, funcFinish, userdata);

	if (version != CELL_PHOTO_DECODE_VERSION_CURRENT || !funcFinish)
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	if (container1 != 0xffffffff && false) // TODO: size < 0x300000
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	if (container2 != 0xffffffff && false) // TODO: size depends on image type, width and height
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeInitialize2(u32 version, u32 container2, vm::ptr<CellPhotoDecodeFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoDecode.warning("cellPhotoDecodeInitialize2(version=0x%x, container2=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, container2, funcFinish, userdata);

	if (version != CELL_PHOTO_DECODE_VERSION_CURRENT || !funcFinish)
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	if (container2 != 0xffffffff && false) // TODO: size depends on image type, width and height
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeFinalize(vm::ptr<CellPhotoDecodeFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoDecode.warning("cellPhotoDecodeFinalize(funcFinish=*0x%x, userdata=*0x%x)", funcFinish, userdata);

	if (!funcFinish)
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		funcFinish(ppu, CELL_OK, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoDecodeFromFile(vm::cptr<char> srcHddDir, vm::cptr<char> srcHddFile, vm::ptr<CellPhotoDecodeSetParam> set_param, vm::ptr<CellPhotoDecodeReturnParam> return_param)
{
	cellPhotoDecode.warning("cellPhotoDecodeFromFile(srcHddDir=%s, srcHddFile=%s, set_param=*0x%x, return_param=*0x%x)", srcHddDir, srcHddFile, set_param, return_param);

	if (!srcHddDir || !srcHddFile || !set_param || !return_param)
	{
		return CELL_PHOTO_DECODE_ERROR_PARAM;
	}

	*return_param = {};

	const std::string vpath = fmt::format("%s/%s", srcHddDir.get_ptr(), srcHddFile.get_ptr());
	const std::string path = vfs::get(vpath);

	if (!vpath.starts_with("/dev_hdd0") && !vpath.starts_with("/dev_hdd1") && !vpath.starts_with("/dev_bdvd"))
	{
		cellPhotoDecode.error("Source '%s' is not inside dev_hdd0 or dev_hdd1 or dev_bdvd", vpath);
		return CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR; // TODO: is this correct?
	}

	if (!fs::is_file(path))
	{
		cellPhotoDecode.error("Source '%s' is not a file (vfs='%s')", path, vpath);
		return CELL_PHOTO_DECODE_ERROR_ACCESS_ERROR; // TODO: is this correct?
	}

	cellPhotoDecode.notice("About to decode '%s' (set_param: width=%d, height=%d, dstBuffer=*0x%x)", path, set_param->width, set_param->height, set_param->dstBuffer);

	s32 width{};
	s32 height{};

	if (!Emu.GetCallbacks().get_scaled_image(path, set_param->width, set_param->height, width, height, static_cast<u8*>(set_param->dstBuffer.get_ptr()), false))
	{
		cellPhotoDecode.error("Failed to decode '%s'", path);
		return CELL_PHOTO_DECODE_ERROR_DECODE;
	}

	return_param->width = width;
	return_param->height = height;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPhotoDecode)("cellPhotoDecodeUtil", []()
{
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeInitialize);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeInitialize2);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeFinalize);
	REG_FUNC(cellPhotoDecodeUtil, cellPhotoDecodeFromFile);
});
