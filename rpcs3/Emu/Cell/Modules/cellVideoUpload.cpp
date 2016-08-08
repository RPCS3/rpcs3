#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

logs::channel cellVideoUpload("cellVideoUpload", logs::level::notice);

s32 cellVideoUploadInitialize()
{
	fmt::throw_exception("Unimplemented" HERE);
}

DECLARE(ppu_module_manager::cellVideoUpload)("cellVideoUpload", []()
{
	REG_FUNC(cellVideoUpload, cellVideoUploadInitialize);
});
