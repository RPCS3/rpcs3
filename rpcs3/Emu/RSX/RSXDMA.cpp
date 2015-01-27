#include "stdafx.h"
#include "RSXDMA.h"
#include "Emu/Memory/Memory.h"
#include "Utilities/Log.h"

DMAObject dma_address(u32 dma_object)
{
	// NOTE: RAMIN is not emulated, therefore DMA Objects are hardcoded in this function
	switch (dma_object) {
	case RSX_CONTEXT_DMA_REPORT_LOCATION_LOCAL:
		return DMAObject{ 0x40300000, 0x8000, DMAObject::READWRITE }; // TODO: Inconsistency: Gitbrew says R starting at 0x1400, test says RW starting at 0x0.
	case RSX_CONTEXT_DMA_DEVICE_RW:
		return DMAObject{ 0x40000000, 0x1000, DMAObject::READWRITE };
	case RSX_CONTEXT_DMA_DEVICE_R:
		return DMAObject{ 0x40000000, 0x1000, DMAObject::READWRITE }; // TODO: Inconsistency: Gitbrew says R, test says RW
	case RSX_CONTEXT_DMA_SEMAPHORE_RW:
		return DMAObject{ 0x40100000, 0x1000, DMAObject::READWRITE };
	case RSX_CONTEXT_DMA_SEMAPHORE_R:
		return DMAObject{ 0x40100000, 0x1000, DMAObject::READWRITE }; // TODO: Inconsistency: Gitbrew says R, test says RW
	default:
		LOG_WARNING(RSX, "Unknown DMA object (0x%08X)", dma_object);
		return DMAObject{};
	}
}

u8 dma_read8(u32 dma_object, u8 offset)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::READ)
	{
		return vm::read8(dma.addr + offset);
	}

	LOG_WARNING(RSX, "Illegal DMA 8-bit read");
	return 0;
}

u16 dma_read16(u32 dma_object, u16 offset)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::READ)
	{
		return vm::read16(dma.addr + offset);
	}

	LOG_WARNING(RSX, "Illegal DMA 16-bit read");
	return 0;
}

u32 dma_read32(u32 dma_object, u32 offset)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::READ)
	{
		return vm::read32(dma.addr + offset);
	}

	LOG_WARNING(RSX, "Illegal DMA 32-bit read");
	return 0;
}

u64 dma_read64(u32 dma_object, u64 offset)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::READ)
	{
		return vm::read64(dma.addr + offset);
	}

	LOG_WARNING(RSX, "Illegal DMA 64-bit read");
	return 0;
}

void dma_write8(u32 dma_object, u32 offset, u8 value)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::WRITE)
	{
		return vm::write8(dma.addr + offset, value);
	}

	LOG_WARNING(RSX, "Illegal DMA 32-bit write");
}

void dma_write16(u32 dma_object, u32 offset, u16 value)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::WRITE)
	{
		return vm::write16(dma.addr + offset, value);
	}

	LOG_WARNING(RSX, "Illegal DMA 32-bit write");
}

void dma_write32(u32 dma_object, u32 offset, u32 value)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::WRITE)
	{
		return vm::write32(dma.addr + offset, value);
	}

	LOG_WARNING(RSX, "Illegal DMA 32-bit write");
}

void dma_write64(u32 dma_object, u32 offset, u64 value)
{
	const DMAObject& dma = dma_address(dma_object);

	if (dma.addr && dma.flags & DMAObject::WRITE)
	{
		return vm::write64(dma.addr + offset, value);
	}

	LOG_WARNING(RSX, "Illegal DMA 64-bit write");
}