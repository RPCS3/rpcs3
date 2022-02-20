#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

enum : u32
{
	SYS_RSXAUDIO_SERIAL_STREAM_CNT        = 4,
	SYS_RSXAUDIO_STREAM_SIZE              = 1024,
	SYS_RSXAUDIO_DATA_BLK_SIZE            = 256,

	SYS_RSXAUDIO_RINGBUF_BLK_SZ_SERIAL    = 0x1000,
	SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF     = 0x400,

	SYS_RSXAUDIO_RINGBUF_SZ	              = 16,

	SYS_RSXAUDIO_AVPORT_CNT               = 5,

	SYS_RSXAUDIO_FREQ_BASE_384K           = 384000,
	SYS_RSXAUDIO_FREQ_BASE_352K           = 352800,

	SYS_RSXAUDIO_PORT_SERIAL              = 0,
	SYS_RSXAUDIO_PORT_SPDIF_0             = 1,
	SYS_RSXAUDIO_PORT_SPDIF_1             = 2,
	SYS_RSXAUDIO_PORT_INVALID             = 0xFF,
	SYS_RSXAUDIO_PORT_CNT                 = 3,

	SYS_RSXAUDIO_SPDIF_CNT                = 2,
};

enum class RsxaudioAvportIdx : u8
{
	HDMI_0  = 0,
	HDMI_1  = 1,
	AVMULTI = 2,
	SPDIF_0 = 3,
	SPDIF_1 = 4,
};

enum class RsxaudioSampleSize : u8
{
	_16BIT = 2,
	_32BIT = 4,
};


// SysCalls

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle);
error_code sys_rsxaudio_finalize(u32 handle);
error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> addr);
error_code sys_rsxaudio_unimport_shared_memory(u32 handle, vm::ptr<u64> addr);
