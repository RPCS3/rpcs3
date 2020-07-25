#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_fs.h"
#include "cellSysutil.h"



LOG_CHANNEL(cellPhotoImportUtil);

// Return Codes
enum CellPhotoImportError : u32
{
	CELL_PHOTO_IMPORT_ERROR_BUSY         = 0x8002c701,
	CELL_PHOTO_IMPORT_ERROR_INTERNAL     = 0x8002c702,
	CELL_PHOTO_IMPORT_ERROR_PARAM        = 0x8002c703,
	CELL_PHOTO_IMPORT_ERROR_ACCESS_ERROR = 0x8002c704,
	CELL_PHOTO_IMPORT_ERROR_COPY         = 0x8002c705,
	CELL_PHOTO_IMPORT_ERROR_INITIALIZE   = 0x8002c706,
};

template<>
void fmt_class_string<CellPhotoImportError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_BUSY);
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_INTERNAL);
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_PARAM);
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_COPY);
			STR_CASE(CELL_PHOTO_IMPORT_ERROR_INITIALIZE);
		}

		return unknown;
	});
}

enum
{
	CELL_PHOTO_IMPORT_HDD_PATH_MAX           = 1055,
	CELL_PHOTO_IMPORT_PHOTO_TITLE_MAX_LENGTH = 64,
	CELL_PHOTO_IMPORT_GAME_TITLE_MAX_SIZE    = 128,
	CELL_PHOTO_IMPORT_GAME_COMMENT_MAX_SIZE  = 1024
};

enum CellPhotoImportFormatType
{
	CELL_PHOTO_IMPORT_FT_UNKNOWN = 0,
	CELL_PHOTO_IMPORT_FT_JPEG,
	CELL_PHOTO_IMPORT_FT_PNG,
	CELL_PHOTO_IMPORT_FT_GIF,
	CELL_PHOTO_IMPORT_FT_BMP,
	CELL_PHOTO_IMPORT_FT_TIFF,
	CELL_PHOTO_IMPORT_FT_MPO,
};

enum CellPhotoImportTexRot
{
	CELL_PHOTO_IMPORT_TEX_ROT_0 = 0,
	CELL_PHOTO_IMPORT_TEX_ROT_90,
	CELL_PHOTO_IMPORT_TEX_ROT_180,
	CELL_PHOTO_IMPORT_TEX_ROT_270,
};

struct CellPhotoImportFileDataSub
{
	be_t<s32> width;
	be_t<s32> height;
	be_t<CellPhotoImportFormatType> format;
	be_t<CellPhotoImportTexRot> rotate;
};

struct CellPhotoImportFileData
{
	char dstFileName[CELL_FS_MAX_FS_FILE_NAME_LENGTH];
	char photo_title[CELL_PHOTO_IMPORT_PHOTO_TITLE_MAX_LENGTH * 3];
	char game_title[CELL_PHOTO_IMPORT_GAME_TITLE_MAX_SIZE];
	char game_comment[CELL_PHOTO_IMPORT_GAME_COMMENT_MAX_SIZE];
	char padding;
	vm::bptr<CellPhotoImportFileDataSub> data_sub;
	vm::bptr<void> reserved;
};

struct CellPhotoImportSetParam
{
	be_t<u32> fileSizeMax;
	vm::bptr<void> reserved1;
	vm::bptr<void> reserved2;
};

using CellPhotoImportFinishCallback = void(s32 result, vm::ptr<CellPhotoImportFileData> filedata, vm::ptr<void> userdata);

error_code cellPhotoImport(u32 version, vm::cptr<char> dstHddPath, vm::ptr<CellPhotoImportSetParam> param, u32 container, vm::ptr<CellPhotoImportFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoImportUtil.todo("cellPhotoImport(version=0x%x, dstHddPath=%s, param=*0x%x, container=0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, dstHddPath, param, container, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellPhotoImportFileData> filedata;
		vm::var<CellPhotoImportFileDataSub> sub;
		filedata->data_sub = sub;
		funcFinish(ppu, CELL_OK, filedata, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellPhotoImport2(u32 version, vm::cptr<char> dstHddPath, vm::ptr<CellPhotoImportSetParam> param, vm::ptr<CellPhotoImportFinishCallback> funcFinish, vm::ptr<void> userdata)
{
	cellPhotoImportUtil.todo("cellPhotoImport2(version=0x%x, dstHddPath=%s, param=*0x%x, funcFinish=*0x%x, userdata=*0x%x)", version, dstHddPath, param, funcFinish, userdata);

	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		vm::var<CellPhotoImportFileData> filedata;
		vm::var<CellPhotoImportFileDataSub> sub;
		filedata->data_sub = sub;
		funcFinish(ppu, CELL_OK, filedata, userdata);
		return CELL_OK;
	});

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPhotoImportUtil)("cellPhotoImportUtil", []()
{
	REG_FUNC(cellPhotoImportUtil, cellPhotoImport);
	REG_FUNC(cellPhotoImportUtil, cellPhotoImport2);
});
