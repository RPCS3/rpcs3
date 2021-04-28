#pragma once

enum CellStorageError : u32
{
	CELL_STORAGEDATA_ERROR_BUSY         = 0x8002be01,
	CELL_STORAGEDATA_ERROR_INTERNAL     = 0x8002be02,
	CELL_STORAGEDATA_ERROR_PARAM        = 0x8002be03,
	CELL_STORAGEDATA_ERROR_ACCESS_ERROR = 0x8002be04,
	CELL_STORAGEDATA_ERROR_FAILURE      = 0x8002be05
};

enum CellStorageDataVersion
{
	CELL_STORAGEDATA_VERSION_CURRENT      = 0,
	CELL_STORAGEDATA_VERSION_DST_FILENAME = 1,
};

enum CellStorageDataParamSize
{
	CELL_STORAGEDATA_HDD_PATH_MAX   = 1055,
	CELL_STORAGEDATA_MEDIA_PATH_MAX = 1024,
	CELL_STORAGEDATA_FILENAME_MAX   = 64,
	CELL_STORAGEDATA_FILESIZE_MAX   = 1024 * 1024 * 1024,
	CELL_STORAGEDATA_TITLE_MAX      = 256
};

inline const char* CELL_STORAGEDATA_IMPORT_FILENAME = "IMPORT.BIN";
inline const char* CELL_STORAGEDATA_EXPORT_FILENAME = "EXPORT.BIN";

struct CellStorageDataSetParam
{
	be_t<u32> fileSizeMax;
	vm::bptr<char> title;
	vm::bptr<void> reserved;
};

using CellStorageDataFinishCallback = void(s32 result, vm::ptr<void> userdata);
