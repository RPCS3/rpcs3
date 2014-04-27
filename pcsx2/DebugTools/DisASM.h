/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

//DECODE PROCUDURES

//cop0
#define DECODE_FS           (DECODE_RD)
#define DECODE_FT           (DECODE_RT)
#define DECODE_FD           (DECODE_SA)
///********

#define DECODE_FUNCTION     ((disasmOpcode) & 0x3F)
#define DECODE_RD     ((disasmOpcode >> 11) & 0x1F) // The rd part of the instruction register
#define DECODE_RT     ((disasmOpcode >> 16) & 0x1F) // The rt part of the instruction register
#define DECODE_RS     ((disasmOpcode >> 21) & 0x1F) // The rs part of the instruction register
#define DECODE_SA     ((disasmOpcode >>  6) & 0x1F) // The sa part of the instruction register
#define DECODE_IMMED     ( disasmOpcode & 0xFFFF)      // The immediate part of the instruction register
#define DECODE_OFFSET  ((((short)DECODE_IMMED * 4) + opcode_addr + 4))
#define DECODE_JUMP     (opcode_addr & 0xf0000000)|((disasmOpcode&0x3ffffff)<<2)
#define DECODE_SYSCALL      ((opcode_addr & 0x03FFFFFF) >> 6)
#define DECODE_BREAK        (DECODE_SYSCALL)
#define DECODE_C0BC         ((disasmOpcode >> 16) & 0x03)
#define DECODE_C1BC         ((disasmOpcode >> 16) & 0x03)
#define DECODE_C2BC         ((disasmOpcode >> 16) & 0x03)

//IOP

#define DECODE_RD_IOP     ((psxRegs.code >> 11) & 0x1F)
#define DECODE_RT_IOP     ((psxRegs.code >> 16) & 0x1F)
#define DECODE_RS_IOP     ((psxRegs.code >> 21) & 0x1F)
#define DECODE_IMMED_IOP   ( psxRegs.code & 0xFFFF)
#define DECODE_SA_IOP    ((psxRegs.code >>  6) & 0x1F)
#define DECODE_FS_IOP           (DECODE_RD_IOP)

