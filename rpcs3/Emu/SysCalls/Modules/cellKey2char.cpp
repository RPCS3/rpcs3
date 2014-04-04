#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellKey2char_init();
Module cellKey2char(0x0021, cellKey2char_init);

// Return Codes
enum
{
	CELL_K2C_OK                        = 0x00000000,
	CELL_K2C_ERROR_FATAL               = 0x80121301,
	CELL_K2C_ERROR_INVALID_HANDLE      = 0x80121302,
	CELL_K2C_ERROR_INVALID_PARAMETER   = 0x80121303,
	CELL_K2C_ERROR_ALREADY_INITIALIZED = 0x80121304,
	CELL_K2C_ERROR_UNINITIALIZED       = 0x80121305,
	CELL_K2C_ERROR_OTHER               = 0x80121306,
};

int cellKey2CharOpen()
{
	UNIMPLEMENTED_FUNC(cellKey2char);
	return CELL_OK;
}

int cellKey2CharClose()
{
	UNIMPLEMENTED_FUNC(cellKey2char);
	return CELL_OK;
}

int cellKey2CharGetChar()
{
	UNIMPLEMENTED_FUNC(cellKey2char);
	return CELL_OK;
}

int cellKey2CharSetMode()
{
	UNIMPLEMENTED_FUNC(cellKey2char);
	return CELL_OK;
}

int cellKey2CharSetArrangement()
{
	UNIMPLEMENTED_FUNC(cellKey2char);
	return CELL_OK;
}

void cellKey2char_init()
{
	cellKey2char.AddFunc(0xabf629c1, cellKey2CharOpen);
	cellKey2char.AddFunc(0x14bf2dc1, cellKey2CharClose);
	cellKey2char.AddFunc(0x56776c0d, cellKey2CharGetChar);
	cellKey2char.AddFunc(0xbfc03768, cellKey2CharSetMode);
	cellKey2char.AddFunc(0x0dfbadfa, cellKey2CharSetArrangement);
}