#pragma once
#include "dev9.h"

u8 CALLBACK smap_read8(u32 addr);
u16 CALLBACK smap_read16(u32 addr);
u32 CALLBACK smap_read32(u32 addr);

void CALLBACK smap_write8(u32 addr, u8 value);
void CALLBACK smap_write16(u32 addr, u16 value);
void CALLBACK smap_write32(u32 addr, u32 value);

void CALLBACK smap_readDMA8Mem(u32 *pMem, int size);
void CALLBACK smap_writeDMA8Mem(u32 *pMem, int size);