#pragma once

namespace vm { using namespace ps3; }

// Error Codes
enum
{
	CELL_PRINT_ERROR_INTERNAL = 0x8002c401,
	CELL_PRINT_ERROR_NO_MEMORY = 0x8002c402,
	CELL_PRINT_ERROR_PRINTER_NOT_FOUND = 0x8002c403,
	CELL_PRINT_ERROR_INVALID_PARAM = 0x8002c404,
	CELL_PRINT_ERROR_INVALID_FUNCTION = 0x8002c405,
	CELL_PRINT_ERROR_NOT_SUPPORT = 0x8002c406,
	CELL_PRINT_ERROR_OCCURRED = 0x8002c407,
	CELL_PRINT_ERROR_CANCELED_BY_PRINTER = 0x8002c408,
};

using CellPrintCallback = void(s32 result, vm::ptr<u32> userdata);

struct CellPrintLoadParam
{
	be_t<u32> mode;
	u8 reserved[32];
};

struct CellPrintStatus
{
	be_t<s32> status;
	be_t<s32> errorStatus;
	be_t<s32> continueEnabled;
	u8 reserved[32];
};