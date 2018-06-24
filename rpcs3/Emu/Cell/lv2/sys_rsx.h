#pragma once

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
		be_t<u32> flipFlags;       // 0x8 flags to handle flip/queue
		be_t<u32> unk1;            // 0xC
		be_t<u32> flipBufferId;    // 0x10
		be_t<u32> queuedBufferId;  // 0x14 todo: this is definately not this variable but its 'unused' so im using it for queueId to pass to flip handler
		be_t<u32> unk3;            // 0x18
		be_t<u32> unk6;            // 0x18 possible low bits of time stamp?  used in getlastVBlankTime
		be_t<u64> lastSecondVTime; // 0x20 last time for second vhandler freq
		be_t<u64> unk4;            // 0x28
		be_t<u64> vBlankCount;     // 0x30
		be_t<u32> unk;             // 0x38 possible u32, 'flip field', top/bottom for interlaced
		be_t<u32> unk5;            // 0x3C possible high bits of time stamp? used in getlastVBlankTime
	} head[8]; // size = 0x40, 0x200

	be_t<u32> unk7;          // 0x12B8
	be_t<u32> unk8;          // 0x12BC
	be_t<u32> handlers;      // 0x12C0 -- flags showing which handlers are set
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

struct RsxDmaControl
{
	u8 resv[0x40];
	atomic_be_t<u32> put;
	atomic_be_t<u32> get;
	atomic_be_t<u32> ref;
	be_t<u32> unk[2];
	be_t<u32> unk1;
};

struct RsxSemaphore
{
	be_t<u32> val;
	be_t<u32> pad;
	be_t<u64> timestamp;
};

struct RsxNotify
{
	be_t<u64> timestamp;
	be_t<u64> zero;
};

struct RsxReport
{
	be_t<u64> timestamp;
	be_t<u32> val;
	be_t<u32> pad;
};

struct RsxReports
{
	RsxSemaphore semaphore[0x100];
	RsxNotify notify[64];
	RsxReport report[2048];
};

struct RsxDisplayInfo
{
	be_t<u32> offset;
	be_t<u32> pitch;
	be_t<u32> width;
	be_t<u32> height;
};

struct SysRsxConfig
{
	be_t<u32> rsx_event_port{ 0 };
	u32 driverInfo{ 0 };
	u32 rsx_context_addr{ 0 }; 
};

// SysCalls
s32 sys_rsx_device_open();
s32 sys_rsx_device_close();
s32 sys_rsx_memory_allocate(vm::ptr<u32> mem_handle, vm::ptr<u64> mem_addr, u32 size, u64 flags, u64 a5, u64 a6, u64 a7);
s32 sys_rsx_memory_free(u32 mem_handle);
s32 sys_rsx_context_allocate(vm::ptr<u32> context_id, vm::ptr<u64> lpar_dma_control, vm::ptr<u64> lpar_driver_info, vm::ptr<u64> lpar_reports, u64 mem_ctx, u64 system_mode);
s32 sys_rsx_context_free(u32 context_id);
s32 sys_rsx_context_iomap(u32 context_id, u32 io, u32 ea, u32 size, u64 flags);
s32 sys_rsx_context_iounmap(u32 context_id, u32 a2, u32 io_addr, u32 size);
s32 sys_rsx_context_attribute(s32 context_id, u32 package_id, u64 a3, u64 a4, u64 a5, u64 a6);
s32 sys_rsx_device_map(vm::ptr<u64> dev_addr, vm::ptr<u64> a2, u32 dev_id);
s32 sys_rsx_device_unmap(u32 dev_id);
s32 sys_rsx_attribute(u32 a1, u32 a2, u32 a3, u32 a4, u32 a5);