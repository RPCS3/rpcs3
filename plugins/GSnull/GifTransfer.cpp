/*  GSnull
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


 // Processes a GIFtag & packet, and throws out some gsIRQs as needed.
// Used to keep interrupts in sync with the EE, while the GS itself
// runs potentially several frames behind.
// size - size of the packet in simd128's

#include "GS.h"
#include "GifTransfer.h"

using namespace std;

u32 CSRw;
GIFPath m_path[3];
bool Path3transfer;

PCSX2_ALIGNED16( u8 g_RealGSMem[0x2000] );
#define GSCSRr *((u64*)(g_RealGSMem+0x1000))
#define GSIMR *((u32*)(g_RealGSMem+0x1010))
#define GSSIGLBLID ((GSRegSIGBLID*)(g_RealGSMem+0x1080))

static void RegHandlerSIGNAL(const u32* data)
{
	GSSIGLBLID->SIGID = (GSSIGLBLID->SIGID&~data[1])|(data[0]&data[1]);

	if ((CSRw & 0x1))  GSCSRr |= 1; // signal

	if (!(GSIMR & 0x100) ) GSirq();
}

static void RegHandlerFINISH(const u32* data)
{
	if ((CSRw & 0x2))  GSCSRr |= 2; // finish

	if (!(GSIMR & 0x200) ) GSirq();

}

static void RegHandlerLABEL(const u32* data)
{
	GSSIGLBLID->LBLID = (GSSIGLBLID->LBLID&~data[1])|(data[0]&data[1]);
}

typedef void (*GIFRegHandler)(const u32* data);
static GIFRegHandler s_GSHandlers[3] =
{
	RegHandlerSIGNAL, RegHandlerFINISH, RegHandlerLABEL
};

__forceinline void GIFPath::PrepRegs()
{
	if( tag.nreg == 0 )
	{
		u32 tempreg = tag.regs[0];
		for(u32 i=0; i<16; ++i, tempreg >>= 4)
		{
			if( i == 8 ) tempreg = tag.regs[1];
			assert( (tempreg&0xf) < 0x64 );
			regs[i] = tempreg & 0xf;
		}
	}
	else
	{
		u32 tempreg = tag.regs[0];
		for(u32 i=0; i<tag.nreg; ++i, tempreg >>= 4)
		{
			assert( (tempreg&0xf) < 0x64 );
			regs[i] = tempreg & 0xf;
		}
	}
}

void GIFPath::SetTag(const void* mem)
{
	tag = *((GIFTAG*)mem);
	curreg = 0;

	PrepRegs();
}

u32 GIFPath::GetReg()
{
	return regs[curreg];
}

bool GIFPath::StepReg()
{
	if ((++curreg & 0xf) == tag.nreg) {
		curreg = 0;
		if (--tag.nloop == 0) {
			return false;
		}
	}
	return true;
}

__forceinline u32 _gifTransfer( GIF_PATH pathidx, const u8* pMem, u32 size )
{	GIFPath& path = m_path[pathidx];

	while(size > 0)
	{
		if(path.tag.nloop == 0)
		{
			path.SetTag( pMem );

			pMem += sizeof(GIFTAG);
			--size;

			if(pathidx == 2 && path.tag.eop)
			{
				Path3transfer = FALSE;
			}

			if( pathidx == 0 )
			{
				// hack: if too much data for VU1, just ignore.

				// The GIF is evil : if nreg is 0, it's really 16.  Otherwise it's the value in nreg.
				const int numregs = ((path.tag.nreg-1)&15)+1;

				if((path.tag.nloop * numregs) > (size * ((path.tag.flg == 1) ? 2 : 1)))
				{
					path.tag.nloop = 0;
					return ++size;
				}
			}
		}
		else
		{
			// NOTE: size > 0 => do {} while(size > 0); should be faster than while(size > 0) {}

			switch(path.tag.flg)
			{
			case GIF_FLG_PACKED:

				do
				{
					if( path.GetReg() == 0xe )
					{
						const int handler = pMem[8];
						if(handler >= 0x60 && handler < 0x63)
							s_GSHandlers[handler&0x3]((const u32*)pMem);
					}

					size--;
					pMem += 16; // 128 bits! //sizeof(GIFPackedReg);
				}
				while(path.StepReg() && size > 0);

			break;

			case GIF_FLG_REGLIST:

				size *= 2;

				do
				{
					const int handler = path.GetReg();
					if(handler >= 0x60 && handler < 0x63)
						s_GSHandlers[handler&0x3]((const u32*)pMem);

					size--;
					pMem += 8; //sizeof(GIFReg); -- 64 bits!
				}
				while(path.StepReg() && size > 0);

				if(size & 1) pMem += 8; //sizeof(GIFReg);

				size /= 2;

			break;

			case GIF_FLG_IMAGE2: // hmmm
				assert(0);
				path.tag.nloop = 0;

			break;

			case GIF_FLG_IMAGE:
			{
				int len = (int)min(size, path.tag.nloop);

				pMem += len * 16;
				path.tag.nloop -= len;
				size -= len;
			}
			break;

			jNO_DEFAULT;

			}
		}

		if(pathidx == 0)
		{
			if(path.tag.eop && path.tag.nloop == 0)
			{
				break;
			}
		}
	}

	if(pathidx == 0)
	{
		if(size == 0 && path.tag.nloop > 0)
		{
			path.tag.nloop = 0;
			SysPrintf( "path1 hack! \n" );

			// This means that the giftag data got screwly somewhere
			// along the way (often means curreg was in a bad state or something)
		}
	}

	return size;
}

// This currently segfaults in the beginning of KH1 if defined.
//#define DO_GIF_TRANSFERS

void _GSgifTransfer1(u32 *pMem, u32 addr)
{
#ifdef DO_GIF_TRANSFERS
	/* This needs looking at, since I quickly grabbed it from ZeroGS. */
	addr &= 0x3fff;
	_gifTransfer( GIF_PATH_1, ((u8*)pMem+(u8)addr), (0x4000-addr)/16);
#endif
}

void _GSgifTransfer2(u32 *pMem, u32 size)
{
#ifdef DO_GIF_TRANSFERS
	_gifTransfer( GIF_PATH_2, (u8*)pMem, size);
#endif
}

void _GSgifTransfer3(u32 *pMem, u32 size)
{
#ifdef DO_GIF_TRANSFERS
	_gifTransfer( GIF_PATH_3, (u8*)pMem, size);
#endif
}

