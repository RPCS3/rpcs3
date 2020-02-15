#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_rsxaudio.h"

LOG_CHANNEL(sys_rsxaudio);

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle)
{
	sys_rsxaudio.todo("sys_rsxaudio_initialize(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code sys_rsxaudio_finalize(u32 handle)
{
	sys_rsxaudio.todo("sys_rsxaudio_finalize(handle=0x%x)", handle);

	return CELL_OK;
}

error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> addr)
{
	sys_rsxaudio.todo("sys_rsxaudio_import_shared_memory(handle=0x%x, addr=*0x%x)", handle, addr);

	return CELL_OK;
}

error_code sys_rsxaudio_unimport_shared_memory(u32 handle, vm::ptr<u64> addr)
{
	sys_rsxaudio.todo("sys_rsxaudio_unimport_shared_memory(handle=0x%x, addr=*0x%x)", handle, addr);

	return CELL_OK;
}
