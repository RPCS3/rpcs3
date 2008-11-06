/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __PSXDMA_H__
#define __PSXDMA_H__

void psxDma2(u32 madr, u32 bcr, u32 chcr);
void psxDma3(u32 madr, u32 bcr, u32 chcr);
void psxDma4(u32 madr, u32 bcr, u32 chcr);
void psxDma6(u32 madr, u32 bcr, u32 chcr);
void psxDma7(u32 madr, u32 bcr, u32 chcr);
void psxDma8(u32 madr, u32 bcr, u32 chcr);
void psxDma9(u32 madr, u32 bcr, u32 chcr);
void psxDma10(u32 madr, u32 bcr, u32 chcr);

int  psxDma4Interrupt();
int  psxDma7Interrupt();
void  dev9Interrupt();
DEV9handler dev9Handler;
void dev9Irq(int cycles);
void  usbInterrupt();
USBhandler usbHandler;
void usbIrq(int cycles);
void fwIrq();
void spu2Irq();

#endif /* __PSXDMA_H__ */
