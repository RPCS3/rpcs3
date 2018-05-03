#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/ErrorCodes.h"

#include "sys_rsxaudio.h"

logs::channel sys_rsxaudio("sys_rsxaudio");

s32 sys_rsxaudio_initialize(vm::ptr<u8> args)
{
	sys_rsxaudio.warning("sys_rsxaudio_initialize(args=*0x%x)", args);
	return CELL_OK;
}

s32 sys_rsxaudio_finalize(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_finalize(unk=0x%x)", unk);
	return CELL_OK;
}

s32 sys_rsxaudio_import_shared_memory(u32 unk0, u32 unk1)
{
	sys_rsxaudio.warning("sys_rsxaudio_import_shared_memory(unk0=0x%x, unk1=0x%x)", unk0, unk1);
	return CELL_OK;
}

s32 sys_rsxaudio_unimport_shared_memory(u32 unk0, u32 unk1)
{
	sys_rsxaudio.warning("sys_rsxaudio_import_shared_memory(unk0=0x%x, unk1=0x%x)", unk0, unk1);
	return CELL_OK;
}

s32 sys_rsxaudio_create_connection(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_create_connection(unk=0x%x)", unk);
	return CELL_OK;
}

u32 sys_rsxaudio_close_connection(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_close_connection(unk=0x%x)", unk);
	return CELL_OK;
}

u32 sys_rsxaudio_prepare_process(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_prepare_process(unk=0x%x)", unk);
	return CELL_OK;
}

u32 sys_rsxaudio_start_process(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_start_process(unk=0x%x)", unk);
	return CELL_OK;
}

u32 sys_rsxaudio_stop_process(u32 unk)
{
	sys_rsxaudio.warning("sys_rsxaudio_stop_process(unk=0x%x)", unk);
	return CELL_OK;
}

u32 sys_rsxaudio_()
{
	sys_rsxaudio.warning("sys_rsxaudio_()");
	return CELL_OK;
}