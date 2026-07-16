#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/RSX/Core/RSXIOMap.hpp"

class cpu_thread;

struct RsxDriverInfo
{
	be_t<u32> version_driver;     // 0x0
	be_t<u32> version_gpu;        // 0x4
	be_t<u32> memory_size;        // 0x8
	be_t<u32> hardware_channel;   // 0xC
	be_t<u32> nvcore_frequency;   // 0x10
	be_t<u32> memory_frequency;   // 0x14
	be_t<u32> unk1[4];            // 0x18 - 0x24
	be_t<u32> unk2;               // 0x28 -- pgraph stuff
	be_t<u32> reportsNotifyOffset;// 0x2C offset to notify memory
	be_t<u32> reportsOffset;      // 0x30 offset to reports memory
	be_t<u32> reportsReportOffset;// 0x34 offset to reports in reports memory
	be_t<u32> unk3[6];            // 0x38-0x54
	be_t<u32> systemModeFlags;    // 0x54
	u8 unk4[0x1064];              // 0x10B8

	struct Head
	{
		be_t<u64> lastFlipTime;    // 0x0 last flip time
		atomic_be_t<u32> flipFlags; // 0x8 flags to handle flip/queue
		be_t<u32> offset;          // 0xC
		be_t<u32> flipBufferId;    // 0x10
		be_t<u32> lastQueuedBufferId; // 0x14 todo: this is definately not this variable but its 'unused' so im using it for queueId to pass to flip handler
		be_t<u32> unk3;            // 0x18
		be_t<u32> lastVTimeLow;    // 0x1C last time for first vhandler freq (low 32-bits)
		atomic_be_t<u64> lastSecondVTime; // 0x20 last time for second vhandler freq
		be_t<u64> unk4;            // 0x28
		atomic_be_t<u64> vBlankCount; // 0x30
		be_t<u32> unk;             // 0x38 possible u32, 'flip field', top/bottom for interlaced
		be_t<u32> lastVTimeHigh;   // 0x3C last time for first vhandler freq (high 32-bits)
	} head[8]; // size = 0x40, 0x200

	be_t<u32> unk7;          // 0x12B8
	be_t<u32> unk8;          // 0x12BC
	atomic_be_t<u32> handlers; // 0x12C0 -- flags showing which handlers are set
	be_t<u32> unk9;          // 0x12C4
	be_t<u32> unk10;         // 0x12C8
	be_t<u32> userCmdParam;  // 0x12CC
	be_t<u32> handler_queue; // 0x12D0
	be_t<u32> unk11;         // 0x12D4
	be_t<u32> unk12;         // 0x12D8
	be_t<u32> unk13;         // 0x12DC
	be_t<u32> unk14;         // 0x12E0
	be_t<u32> unk15;         // 0x12E4
	be_t<u32> unk16;         // 0x12E8
	be_t<u32> unk17;         // 0x12F0
	be_t<u32> lastError;     // 0x12F4 error param for cellGcmSetGraphicsHandler
							 // todo: theres more to this
};

static_assert(sizeof(RsxDriverInfo) == 0x12F8, "rsxSizeTest");
static_assert(sizeof(RsxDriverInfo::Head) == 0x40, "rsxHeadSizeTest");

enum : u64
{
	// Unused
	SYS_RSX_IO_MAP_IS_STRICT = 1ull << 60
};

// Unofficial event names
enum : u64
{
	//SYS_RSX_EVENT_GRAPHICS_ERROR = 1 << 0,
	SYS_RSX_EVENT_VBLANK = 1 << 1,
	SYS_RSX_EVENT_FLIP_BASE = 1 << 3,
	SYS_RSX_EVENT_QUEUE_BASE = 1 << 5,
	SYS_RSX_EVENT_USER_CMD = 1 << 7,
	SYS_RSX_EVENT_SECOND_VBLANK_BASE = 1 << 10,
	SYS_RSX_EVENT_UNMAPPED_BASE = 1ull << 32,
};

struct RsxDmaControl
{
	u8 resv[0x40];
	atomic_be_t<u32> put;
	atomic_be_t<u32> get;
	atomic_be_t<u32> ref;
	be_t<u32> unk[2];
	be_t<u32> unk1;
};

using RsxSemaphore = be_t<u32>;

struct alignas(16) RsxNotify
{
	be_t<u64> timestamp;
	be_t<u64> zero;
};

struct alignas(16) RsxReport
{
	be_t<u64> timestamp;
	be_t<u32> val;
	be_t<u32> pad;
};

struct RsxReports
{
	RsxSemaphore semaphore[1024];
	RsxNotify notify[64];
	RsxReport report[2048];
};

struct RsxDisplayInfo
{
	be_t<u32> offset{0};
	be_t<u32> pitch{0};
	be_t<u32> width{0};
	be_t<u32> height{0};

	ENABLE_BITWISE_SERIALIZATION;

	bool valid() const
	{
		return height != 0u && width != 0u;
	}
};

struct GcmTileInfo;
struct GcmZcullInfo;

namespace rsx
{
	struct GCM_tile_reference;
}

namespace utils
{
	template <typename T>
	class address_range;

	using address_range32 = address_range<u32>;
}

struct lv2_rsx_context
{
	// TODO: LV1 does 0x55555555 XOR (internal index) for ID creation
	// So the first context is 0x55555555, then 0x55555554, then 0x55555557 until 0x5555555A lastly
	// Whereas IDM cannot do that yet
	static constexpr u32 id_base = 0x55555550;
	static constexpr u32 id_count = 16;
	static constexpr u32 id_step = 1;

	SAVESTATE_INIT_POS(56);

	rsx::rsx_iomap_table iomap_table;
	
	// Vector for incomplete typename
	std::vector<struct GcmTileInfo> tiles;
	std::vector<struct GcmZcullInfo> zculls;
	
	RsxDisplayInfo display_buffers[8];
	u32 display_buffers_count{ 0 };
	u32 current_display_buffer{ 0 };
	
	u32 dma_address{ 0 };
	u32 label_addr{ 0 };
	u32 main_mem_size{ 0 };
	u32 rsx_event_port{ 0 };
	u32 driver_info{ 0 };

	//atomic_t<bool> requested_vsync = false;
	atomic_t<bool> enable_second_vhandler = false;
	atomic_t<u64> unsent_gcm_events = 0; // Unsent event bits when aborting RSX/VBLANK thread (will be sent on savestate load)

	lv2_rsx_context() noexcept;
	lv2_rsx_context(utils::serial& ar) noexcept;
	void save(utils::serial& ar) noexcept;

	rsx::GCM_tile_reference get_tiled_memory_region(const utils::address_range32& range) const;
};

// Process-specific RSX information (each process has one and it is not realy regulated by ID by CellOS)
struct lv2_rsx_process_info
{
	u32 device_addr[16]{ 0 };
	u32 local_mem_size{ 0 };

	shared_mutex mutex;
};



// SysCalls
error_code sys_rsx_device_open(cpu_thread& cpu);
error_code sys_rsx_device_close(cpu_thread& cpu);
error_code sys_rsx_memory_allocate(cpu_thread& cpu, vm::ptr<u32> mem_handle, vm::ptr<u64> mem_addr, u64 size, u64 flags, u64 a5, u64 a6, u64 a7);
error_code sys_rsx_memory_free(cpu_thread& cpu, u32 mem_handle);
error_code sys_rsx_context_allocate(cpu_thread& cpu, vm::ptr<u32> context_id, vm::ptr<u64> lpar_dma_control, vm::ptr<u64> lpar_driver_info, vm::ptr<u64> lpar_reports, u64 mem_ctx, u64 system_mode);
error_code sys_rsx_context_free(ppu_thread& ppu, u32 context_id);
error_code sys_rsx_context_iomap(cpu_thread& cpu, u32 context_id, u64 io, u64 ea, u64 size, u64 flags);
error_code sys_rsx_context_iounmap(cpu_thread& cpu, u32 context_id, u64 io, u64 size);
error_code sys_rsx_context_attribute(u32 context_id, u32 package_id, u64 a3, u64 a4, u64 a5, u64 a6);
error_code sys_rsx_device_map(cpu_thread& cpu, vm::ptr<u64> dev_addr, vm::ptr<u64> a2, u32 dev_id);
error_code sys_rsx_device_unmap(cpu_thread& cpu, u32 dev_id);
error_code sys_rsx_attribute(cpu_thread& cpu, u32 packageId, u32 a2, u32 a3, u32 a4, u32 a5);
