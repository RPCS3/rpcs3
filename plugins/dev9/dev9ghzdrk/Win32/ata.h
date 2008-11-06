#pragma once
#include "dev9.h"

void ata_init();
void ata_term();

template<int sz>
void CALLBACK ata_write(u32 addr, u32 value);
template<int sz>
u8 CALLBACK ata_read(u32 addr);

void CALLBACK ata_readDMA8Mem(u32 *pMem, int size);
void CALLBACK ata_writeDMA8Mem(u32 *pMem, int size);