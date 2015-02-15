// Copyright (C) 2015 AlexAltea (https://github.com/AlexAltea/nucleus)
#pragma once

enum {
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0 = 0x66604200, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY1 = 0x66604201, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY2 = 0x66604202, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY3 = 0x66604203, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY4 = 0x66604204, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY5 = 0x66604205, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY6 = 0x66604206, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY7 = 0x66604207, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY8 = 0x66604208, // Target: lpar_reports[0x1000 : 0x????]
	RSX_CONTEXT_DMA_NOTIFY_MAIN_0 = 0x6660420F,
	RSX_CONTEXT_DMA_SEMAPHORE_RW = 0x66606660, // Target: lpar_reports[0x0000 : 0x1000] (Read/Write)
	RSX_CONTEXT_DMA_SEMAPHORE_R = 0x66616661, // Target: lpar_reports[0x0000 : 0x1000] (Read)
	RSX_CONTEXT_DMA_REPORT_LOCATION_LOCAL = 0x66626660, // Target: lpar_reports[0x1400 : 0x9400]
	RSX_CONTEXT_DMA_REPORT_LOCATION_MAIN = 0xBAD68000,
	RSX_CONTEXT_DMA_DEVICE_RW = 0x56616660,
	RSX_CONTEXT_DMA_DEVICE_R = 0x56616661,
};

struct DMAObject {
	// Flags
	enum {
		READ = 1 << 0,
		WRITE = 1 << 1,
		READWRITE = READ | WRITE,
	};
	u32 addr;
	u32 size;
	u32 flags;
};

// RSX Direct Memory Access
DMAObject dma_address(u32 dma_object);

u8 dma_read8(u32 dma_object, u32 offset);
u16 dma_read16(u32 dma_object, u32 offset);
u32 dma_read32(u32 dma_object, u32 offset);
u64 dma_read64(u32 dma_object, u32 offset);

void dma_write8(u32 dma_object, u32 offset, u8 value);
void dma_write16(u32 dma_object, u32 offset, u16 value);
void dma_write32(u32 dma_object, u32 offset, u32 value);
void dma_write64(u32 dma_object, u32 offset, u64 value);