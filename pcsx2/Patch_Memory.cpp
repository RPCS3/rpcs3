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

#include "PrecompiledHeader.h"

#define _PC_	// disables MIPS opcode macros.

#include "IopCommon.h"
#include "Patch.h"

u32 SkipCount = 0, IterationCount = 0;
u32 IterationIncrement = 0, ValueIncrement = 0;
u32 PrevCheatType = 0, PrevCheatAddr = 0,LastType = 0;

void writeCheat()
{
	switch (LastType)
	{
		case 0x0:
			memWrite8(PrevCheatAddr,IterationIncrement & 0xFF);
			break;
		case 0x1:
			memWrite16(PrevCheatAddr,IterationIncrement & 0xFFFF);
			break;
		case 0x2:
			memWrite32(PrevCheatAddr,IterationIncrement);
			break;
		default:
			break;
	}
}

void handle_extended_t( IniPatch *p)
{
	if (SkipCount > 0)
	{
		SkipCount--;
	}
	else switch (PrevCheatType)
	{
		case 0x3040: // vvvvvvvv  00000000  Inc
		{
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem + (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x3050: // vvvvvvvv  00000000  Dec
		{
			u32 mem = memRead32(PrevCheatAddr);
			memWrite32(PrevCheatAddr, mem - (p->addr));
			PrevCheatType = 0;
			break;
		}

		case 0x4000: // vvvvvvvv  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				memWrite32((u32)(PrevCheatAddr + (i * IterationIncrement)),(u32)(p->addr + ((u32)p->data * i)));
			}
			PrevCheatType = 0;
			break;

		case 0x5000: // dddddddd  iiiiiiii
			for(u32 i = 0; i < IterationCount; i++)
			{
				u8 mem = memRead8(PrevCheatAddr + i);
				memWrite8(((u32)p->data) + i, mem);
			}
			PrevCheatType = 0;
			break;

		case 0x6000: // 000Xnnnn  iiiiiiii
			if (IterationIncrement == 0x0)
			{
				//LastType = ((u32)p->addr & 0x000F0000) >> 16;
				u32 mem = memRead32(PrevCheatAddr);
				if ((u32)p->addr < 0x100)
				{
					LastType = 0x0;
					PrevCheatAddr = mem + ((u32)p->addr);
				}
				else if ((u32)p->addr < 0x1000)
				{
					LastType = 0x1;
					PrevCheatAddr = mem + ((u32)p->addr * 2);
				}
				else
				{
					LastType = 0x2;
					PrevCheatAddr = mem + ((u32)p->addr * 4);
				}

				// Check if needed to read another pointer
				PrevCheatType = 0;
				if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0)
				{
				    switch(LastType)
				    {
				        case 0x0:
                            memWrite8(PrevCheatAddr,(u8)p->data & 0xFF);
                            break;
				        case 0x1:
                            memWrite16(PrevCheatAddr,(u16)p->data & 0x0FFFF);
                            break;
				        case 0x2:
                            memWrite32(PrevCheatAddr,(u32)p->data);
                            break;
				        default:
                            break;
				    }
				}
			}
			else
			{
				// Get Number of pointers
				if (((u32)p->addr & 0x0000FFFF) == 0)
					IterationCount = 1;
				else
					IterationCount = (u32)p->addr & 0x0000FFFF;

				// Read first pointer
				LastType = ((u32)p->addr & 0x000F0000) >> 16;
				u32 mem = memRead32(PrevCheatAddr);

				PrevCheatAddr = mem + (u32)p->data;
				IterationCount--;

				// Check if needed to read another pointer
				if (IterationCount == 0)
				{
					PrevCheatType = 0;
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
				}
				else
				{
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0)
						PrevCheatType = 0;
					else
						PrevCheatType = 0x6001;
				}
			}
			break;

		case 0x6001: // 000Xnnnn  iiiiiiii
		{
			// Read first pointer
			u32 mem = memRead32(PrevCheatAddr & 0x0FFFFFFF);

			PrevCheatAddr = mem + (u32)p->addr;
			IterationCount--;

			// Check if needed to read another pointer
			if (IterationCount == 0)
			{
				PrevCheatType = 0;
				if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
			}
			else
			{
				mem = memRead32(PrevCheatAddr);

				PrevCheatAddr = mem + (u32)p->data;
				IterationCount--;
				if (IterationCount == 0)
				{
					PrevCheatType = 0;
					if (((mem & 0x0FFFFFFF) & 0x3FFFFFFC) != 0) writeCheat();
				}
			}
		}
		break;

		default:
            if ((p->addr & 0xF0000000) == 0x00000000) // 0aaaaaaa 0000000vv
            {
                memWrite8(p->addr & 0x0FFFFFFF, (u8)p->data & 0x000000FF);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xF0000000) == 0x10000000) // 0aaaaaaa 0000vvvv
            {
                memWrite16(p->addr & 0x0FFFFFFF, (u16)p->data & 0x0000FFFF);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xF0000000) == 0x20000000) // 0aaaaaaa vvvvvvvv
            {
                memWrite32(p->addr & 0x0FFFFFFF, (u32)p->data);
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30000000) // 300000vv 0aaaaaaa  Inc
            {
                u8 mem = memRead8((u32)p->data);
                memWrite8((u32)p->data, mem + (p->addr & 0x000000FF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30100000) // 301000vv 0aaaaaaa  Dec
            {
                u8 mem = memRead8((u32)p->data);
                memWrite8((u32)p->data, mem - (p->addr & 0x000000FF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30200000) // 3020vvvv 0aaaaaaa Inc
            {
                u16 mem = memRead16((u32)p->data);
                memWrite16((u32)p->data, mem + (p->addr & 0x0000FFFF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30300000) // 3030vvvv 0aaaaaaa Dec
            {
                u16 mem = memRead16((u32)p->data);
                memWrite16((u32)p->data, mem - (p->addr & 0x0000FFFF));
                PrevCheatType = 0;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30400000) // 30400000 0aaaaaaa Inc	+ Another line
            {
                PrevCheatType = 0x3040;
                PrevCheatAddr = (u32)p->data;
            }
            else if ((p->addr & 0xFFFF0000) == 0x30500000) // 30500000 0aaaaaaa Inc	+ Another line
            {
                PrevCheatType = 0x3050;
                PrevCheatAddr = (u32)p->data;
            }
            else if ((p->addr & 0xF0000000) == 0x40000000) // 4aaaaaaa nnnnssss + Another line
            {
                IterationCount = ((u32)p->data & 0xFFFF0000) / 0x10000;
                IterationIncrement = ((u32)p->data & 0x0000FFFF) * 4;
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                PrevCheatType = 0x4000;
            }
            else if ((p->addr & 0xF0000000) == 0x50000000) // 5sssssss nnnnnnnn + Another line
            {
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                IterationCount = ((u32)p->data);
                PrevCheatType = 0x5000;
            }
            else if ((p->addr & 0xF0000000) == 0x60000000) // 6aaaaaaa 000000vv + Another line/s
            {
                PrevCheatAddr = (u32)p->addr & 0x0FFFFFFF;
                IterationIncrement = ((u32)p->data);
                IterationCount = 0;
                PrevCheatType = 0x6000;
            }
            else if ((p->addr & 0xF0000000) == 0x70000000)
            {
                if ((p->data & 0x00F00000) == 0x00000000) // 7aaaaaaa 000000vv
                {
                    u8 mem = memRead8((u32)p->addr & 0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem | (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00100000) // 7aaaaaaa 0010vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem | (p->data & 0x0000FFFF)));
                }
                else if ((p->data & 0x00F00000) == 0x00200000) // 7aaaaaaa 002000vv
                {
                    u8 mem = memRead8((u32)p->addr&0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem & (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00300000) // 7aaaaaaa 0030vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem & (p->data & 0x0000FFFF)));
                }
                else if ((p->data & 0x00F00000) == 0x00400000) // 7aaaaaaa 004000vv
                {
                    u8 mem = memRead8((u32)p->addr & 0x0FFFFFFF);
                    memWrite8((u32)p->addr & 0x0FFFFFFF,(u8)(mem ^ (p->data & 0x000000FF)));
                }
                else if ((p->data & 0x00F00000) == 0x00500000) // 7aaaaaaa 0050vvvv
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    memWrite16((u32)p->addr & 0x0FFFFFFF,(u16)(mem ^ (p->data & 0x0000FFFF)));
                }
            }
            else if (p->addr < 0xE0000000)
            {
                if ((((u32)p->data & 0xFFFF0000) == 0x00000000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00100000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00200000) ||
                    (((u32)p->data & 0xFFFF0000) == 0x00300000))
                {
                    u16 mem = memRead16((u32)p->addr & 0x0FFFFFFF);
                    if (mem != (0x0000FFFF & (u32)p->data)) SkipCount = 1;
                    PrevCheatType = 0;
                }
            }
            else if (p->addr < 0xF0000000)
            {
                if (((u32)p->data & 0xC0000000) == 0x00000000)
                if ((((u32)p->data & 0xF0000000) == 0x00000000) ||
                    (((u32)p->data & 0xF0000000) == 0x10000000) ||
                    (((u32)p->data & 0xF0000000) == 0x20000000) ||
                    (((u32)p->data & 0xF0000000) == 0x30000000))
                {
                    u16 mem = memRead16((u32)p->data & 0x0FFFFFFF);
                    if (mem != (0x0000FFFF & (u32)p->addr)) SkipCount = ((u32)p->addr & 0xFFF0000) / 0x10000;
                    PrevCheatType = 0;
                }
            }
	}
}

void _ApplyPatch(IniPatch *p)
{
	if (p->enabled == 0) return;

	switch (p->cpu)
	{
		case CPU_EE:
			switch (p->type)
			{
				case BYTE_T:
					if (memRead8(p->addr) != (u8)p->data)
						memWrite8(p->addr, (u8)p->data);
					break;

				case SHORT_T:
					if (memRead16(p->addr) != (u16)p->data)
						memWrite16(p->addr, (u16)p->data);
					break;

				case WORD_T:
					if (memRead32(p->addr) != (u32)p->data)
						memWrite32(p->addr, (u32)p->data);
					break;

				case DOUBLE_T:
					u64 mem;
					memRead64(p->addr, &mem);
					if (mem != p->data)
						memWrite64(p->addr, &p->data);
					break;

				case EXTENDED_T:
					handle_extended_t(p);
					break;

				default:
					break;
			}
			break;

		case CPU_IOP:
			switch (p->type)
			{
				case BYTE_T:
					if (iopMemRead8(p->addr) != (u8)p->data)
						iopMemWrite8(p->addr, (u8)p->data);
					break;
				case SHORT_T:
					if (iopMemRead16(p->addr) != (u16)p->data)
						iopMemWrite16(p->addr, (u16)p->data);
					break;
				case WORD_T:
					if (iopMemRead32(p->addr) != (u32)p->data)
						iopMemWrite32(p->addr, (u32)p->data);
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
}
