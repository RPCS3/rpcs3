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
#include "IopCommon.h"

#include <ctype.h>

namespace R3000A {

//#define r0 (psxRegs.GPR.n.r0)
#define at (psxRegs.GPR.n.at)
#define v0 (psxRegs.GPR.n.v0)
#define v1 (psxRegs.GPR.n.v1)
#define a0 (psxRegs.GPR.n.a0)
#define a1 (psxRegs.GPR.n.a1)
#define a2 (psxRegs.GPR.n.a2)
#define a3 (psxRegs.GPR.n.a3)
#define t0 (psxRegs.GPR.n.t0)
#define t1 (psxRegs.GPR.n.t1)
#define t2 (psxRegs.GPR.n.t2)
#define t3 (psxRegs.GPR.n.t3)
#define t4 (psxRegs.GPR.n.t4)
#define t5 (psxRegs.GPR.n.t5)
#define t6 (psxRegs.GPR.n.t6)
#define t7 (psxRegs.GPR.n.t7)
#define s0 (psxRegs.GPR.n.s0)
#define s1 (psxRegs.GPR.n.s1)
#define s2 (psxRegs.GPR.n.s2)
#define s3 (psxRegs.GPR.n.s3)
#define s4 (psxRegs.GPR.n.s4)
#define s5 (psxRegs.GPR.n.s5)
#define s6 (psxRegs.GPR.n.s6)
#define s7 (psxRegs.GPR.n.s7)
#define t8 (psxRegs.GPR.n.t6)
#define t9 (psxRegs.GPR.n.t7)
#define k0 (psxRegs.GPR.n.k0)
#define k1 (psxRegs.GPR.n.k1)
#define gp (psxRegs.GPR.n.gp)
#define sp (psxRegs.GPR.n.sp)
#define fp (psxRegs.GPR.n.s8)
#define ra (psxRegs.GPR.n.ra)
#define pc (psxRegs.pc)

#define Ra0 (iopVirtMemR<char>(a0))
#define Ra1 (iopVirtMemR<char>(a1))
#define Ra2 (iopVirtMemR<char>(a2))
#define Ra3 (iopVirtMemR<char>(a3))
#define Rv0 (iopVirtMemR<char>(v0))
#define Rsp (iopVirtMemR<char>(sp))

int ioman_write()
{
	if (a0 == 1) // stdout
	{
		Console.Write( ConColor_IOP, L"%s", ShiftJIS_ConvertString(Ra1, a2).c_str() );
	}
	else
	{
		return 0;
	}

	pc = ra;
	return 1;
}

static u32 vararg_get(unsigned i)
{
	return iopVirtMemR<u32>(sp)[i];
}

int sysmem_Kprintf() // 3f
{
	char tmp[1024], tmp2[1024];
	char *ptmp = tmp;
	int n=1, i=0, j = 0;

	iopMemWrite32(sp, a0);
	iopMemWrite32(sp + 4, a1);
	iopMemWrite32(sp + 8, a2);
	iopMemWrite32(sp + 12, a3);

	while (Ra0[i])
	{
		switch (Ra0[i])
		{
			case '%':
				j = 0;
				tmp2[j++] = '%';

_start:
				switch (Ra0[++i])
				{
					case '.':
					case 'l':
						tmp2[j++] = Ra0[i];
						goto _start;
					default:
						if (Ra0[i] >= '0' && Ra0[i] <= '9')
						{
							tmp2[j++] = Ra0[i];
							goto _start;
						}
						break;
				}

				tmp2[j++] = Ra0[i];
				tmp2[j] = 0;

				switch (Ra0[i])
				{
					case 'f': case 'F':
						ptmp+= sprintf(ptmp, tmp2, (float)iopMemRead32(sp + n * 4));
						n++;
						break;

					case 'a': case 'A':
					case 'e': case 'E':
					case 'g': case 'G':
						ptmp+= sprintf(ptmp, tmp2, (double)iopMemRead32(sp + n * 4));
						n++;
						break;

					case 'p':
					case 'i':
					case 'd': case 'D':
					case 'o': case 'O':
					case 'x': case 'X':
						ptmp+= sprintf(ptmp, tmp2, (u32)iopMemRead32(sp + n * 4));
						n++;
						break;

					case 'c':
						ptmp+= sprintf(ptmp, tmp2, (u8)iopMemRead32(sp + n * 4));
						n++;
						break;

					case 's':
						ptmp+= sprintf(ptmp, tmp2, iopVirtMemR<char>(iopMemRead32(sp + n * 4)));
						n++;
						break;

					case '%':
						*ptmp++ = Ra0[i];
						break;

					default:
						break;
				}
				i++;
				break;

			default:
				*ptmp++ = Ra0[i++];
				break;
		}
	}
	*ptmp = 0;

	// Use "%s" even though it seems indirect: this avoids chaos if/when the IOP decides
	// to feed us strings that contain percentages or other printf formatting control chars.
	Console.Write( ConColor_IOP, L"%s", ShiftJIS_ConvertString(tmp).c_str(), 1023 );
	
	pc = ra;
	return 1;
}

}	// end namespace R3000A
