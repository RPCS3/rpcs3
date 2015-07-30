#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellVideoUpload;

s32 cellVideoUploadInitialize()
{
	throw EXCEPTION("");
}

Module cellVideoUpload("cellVideoUpload", []()
{
	REG_FUNC(cellVideoUpload, cellVideoUploadInitialize);
});
