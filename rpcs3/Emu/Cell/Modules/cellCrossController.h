#pragma once

enum CellCrossControllerError : u32
{
	CELL_CROSS_CONTROLLER_ERROR_CANCEL                = 0x8002cd80,
	CELL_CROSS_CONTROLLER_ERROR_NETWORK               = 0x8002cd81,
	CELL_CROSS_CONTROLLER_ERROR_OUT_OF_MEMORY         = 0x8002cd90,
	CELL_CROSS_CONTROLLER_ERROR_FATAL                 = 0x8002cd91,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILENAME  = 0x8002cd92,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_SIG_FILENAME  = 0x8002cd93,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_ICON_FILENAME = 0x8002cd94,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_VALUE         = 0x8002cd95,
	CELL_CROSS_CONTROLLER_ERROR_PKG_FILE_OPEN         = 0x8002cd96,
	CELL_CROSS_CONTROLLER_ERROR_SIG_FILE_OPEN         = 0x8002cd97,
	CELL_CROSS_CONTROLLER_ERROR_ICON_FILE_OPEN        = 0x8002cd98,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_STATE         = 0x8002cd99,
	CELL_CROSS_CONTROLLER_ERROR_INVALID_PKG_FILE      = 0x8002cd9a,
	CELL_CROSS_CONTROLLER_ERROR_INTERNAL              = 0x8002cda0,
};

enum
{
	CELL_CROSS_CONTROLLER_STATUS_INITIALIZED = 1,
	CELL_CROSS_CONTROLLER_STATUS_FINALIZED   = 2
};

enum
{
	CELL_CROSS_CONTROLLER_PKG_APP_VER_LEN  = 6,  // e.g. 01.00
	CELL_CROSS_CONTROLLER_PKG_TITLE_ID_LEN = 10, // e.g. NEKO12345
	CELL_CROSS_CONTROLLER_PKG_TITLE_LEN    = 52, // e.g. Cat Simulator 5

	// Undefined helper value
	CELL_CROSS_CONTROLLER_PARAM_FILE_NAME_LEN = 255,
};

struct CellCrossControllerParam
{
	vm::bcptr<char> pPackageFileName;
	vm::bcptr<char> pSignatureFileName;
	vm::bcptr<char> pIconFileName;
	vm::bptr<void> option;
};

struct CellCrossControllerPackageInfo
{
	vm::bcptr<char> pTitle;
	vm::bcptr<char> pTitleId;
	vm::bcptr<char> pAppVer;
};

using CellCrossControllerCallback = void(s32 status, s32 errorCode, vm::ptr<void> option, vm::ptr<void> userdata);
