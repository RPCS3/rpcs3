#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "gcm_enums.h"
#include "gcm_printing.h"
#include "util/atomic.hpp"


struct CellGcmControl
{
	atomic_be_t<u32> put;
	atomic_be_t<u32> get;
	atomic_be_t<u32> ref;
};

struct CellGcmConfig
{
	be_t<u32> localAddress;
	be_t<u32> ioAddress;
	be_t<u32> localSize;
	be_t<u32> ioSize;
	be_t<u32> memoryFrequency;
	be_t<u32> coreFrequency;
};

struct CellGcmContextData;

using CellGcmContextCallback = s32 (vm::ptr<CellGcmContextData>, u32);

struct CellGcmContextData
{
	vm::bptr<u32> begin;
	vm::bptr<u32> end;
	vm::bptr<u32> current;
	vm::bptr<CellGcmContextCallback> callback;
};

struct gcmInfo
{
	u32 config_addr;
	u32 context_addr;
	u32 control_addr;
	u32 label_addr;
	u32 command_size = 0x400;
	u32 segment_size = 0x100;
};

struct CellGcmSurface
{
	u8 type;
	u8 antialias;
	u8 colorFormat;
	u8 colorTarget;
	u8 colorLocation[4];
	be_t<u32> colorOffset[4];
	be_t<u32> colorPitch[4];
	u8 depthFormat;
	u8 depthLocation;
	u8 _padding[2];
	be_t<u32> depthOffset;
	be_t<u32> depthPitch;
	be_t<u16> width;
	be_t<u16> height;
	be_t<u16> x;
	be_t<u16> y;
};

struct alignas(16) CellGcmReportData
{
	be_t<u64> timer;
	be_t<u32> value;
	be_t<u32> padding;
};

struct CellGcmZcullInfo
{
	be_t<u32> region;
	be_t<u32> size;
	be_t<u32> start;
	be_t<u32> offset;
	be_t<u32> status0;
	be_t<u32> status1;
};

struct CellGcmDisplayInfo
{
	be_t<u32> offset;
	be_t<u32> pitch;
	be_t<u32> width;
	be_t<u32> height;
};

struct CellGcmTileInfo
{
	be_t<u32> tile;
	be_t<u32> limit;
	be_t<u32> pitch;
	be_t<u32> format;
};

struct GcmZcullInfo
{
	u32 offset;
	u32 width;
	u32 height;
	u32 cullStart;
	u32 zFormat;
	u32 aaFormat;
	u32 zcullDir;
	u32 zcullFormat;
	u32 sFunc;
	u32 sRef;
	u32 sMask;
	bool bound = false;

	CellGcmZcullInfo pack() const
	{
		CellGcmZcullInfo ret;

		ret.region = (1<<0) | (zFormat<<4) | (aaFormat<<8);
		ret.size = ((width>>6)<<22) | ((height>>6)<<6);
		ret.start = cullStart&(~0xFFF);
		ret.offset = offset;
		ret.status0 = (zcullDir<<1) | (zcullFormat<<2) | ((sFunc&0xF)<<12) | (sRef<<16) | (sMask<<24);
		ret.status1 = (0x2000<<0) | (0x20<<16);

		return ret;
	}
};

struct GcmTileInfo
{
	u32 location;
	u32 offset;
	u32 size;
	u32 pitch;
	u32 comp;
	u32 base;
	u32 bank;
	bool bound = false;

	CellGcmTileInfo pack() const
	{
		CellGcmTileInfo ret;

		ret.tile = (location + 1) | (bank << 4) | ((offset / 0x10000) << 16) | (location << 31);
		ret.limit = ((offset + size - 1) / 0x10000) << 16 | (location << 31);
		ret.pitch = (pitch / 0x100) << 8;
		ret.format = base | ((base + ((size - 1) / 0x10000)) << 13) | (comp << 26) | (1 << 30);

		return ret;
	}
};

namespace rsx
{
	template<typename AT>
	static inline u32 make_command(vm::_ptr_base<be_t<u32>, AT>& dst, u32 start_register, std::initializer_list<any32> values)
	{
		*dst++ = start_register << 2 | static_cast<u32>(values.size()) << 18;

		for (const any32& cmd : values)
		{
			*dst++ = cmd.as<u32>();
		}

		return u32{sizeof(u32)} * (static_cast<u32>(values.size()) + 1);
	}

	template<typename AT>
	static inline u32 make_jump(vm::_ptr_base<be_t<u32>, AT>& dst, u32 offset)
	{
		*dst++ = RSX_METHOD_OLD_JUMP_CMD | offset;

		return sizeof(u32);
	}
}
