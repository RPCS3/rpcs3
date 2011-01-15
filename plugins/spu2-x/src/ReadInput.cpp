/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "Dma.h"

#include "PS2E-spu2.h"	// required for ENABLE_NEW_IOPDMA_SPU2 define

// Core 0 Input is "SPDIF mode" - Source audio is AC3 compressed.

// Core 1 Input is "CDDA mode" - Source audio data is 32 bits.
// PS2 note:  Very! few PS2 games use this mode.  Some PSX games used it, however no
// *known* PS2 game does since it was likely only available if the game was recorded to CD
// media (ie, not available in DVD mode, which almost all PS2 games use).  Plus PS2 games
// generally prefer to use ADPCM streaming audio since they need as much storage space as
// possible for FMVs and high-def textures.
//
StereoOut32 V_Core::ReadInput_HiFi()
{
	InputPosRead &= ~1;
//
//#ifdef PCM24_S1_INTERLEAVE
//	StereoOut32 retval(
//		*((s32*)(ADMATempBuffer+(InputPosRead<<1))),
//		*((s32*)(ADMATempBuffer+(InputPosRead<<1)+2))
//	);
//#else
//	StereoOut32 retval(
//		(s32&)(ADMATempBuffer[InputPosRead]),
//		(s32&)(ADMATempBuffer[InputPosRead+0x200])
//	);
//#endif

	StereoOut32 retval(
		(s32&)(*GetMemPtr(0x2000 + (Index<<10) + InputPosRead)),
		(s32&)(*GetMemPtr(0x2200 + (Index<<10) + InputPosRead))
	);

	if( Index == 1 )
	{
		// CDDA Mode:
		// give 30 bit data (SndOut downsamples the rest of the way)
		// HACKFIX: 28 bits seems better according to rama.  I should take some time and do some
		//    bitcounting on this one.  --air
		retval.Left		>>= 4;
		retval.Right	>>= 4;
	}

	InputPosRead += 2;

	// Why does CDDA mode check for InputPos == 0x100? In the old code, SPDIF mode did not but CDDA did.
	//  One of these seems wrong, they should be the same.  Since standard ADMA checks too I'm assuming that as default. -- air

	if( (InputPosRead==0x100) || (InputPosRead>=0x200) )
	{
#ifdef ENABLE_NEW_IOPDMA_SPU2
		// WARNING: Assumes this to be in the same thread as the dmas
		AutoDmaFree += 0x200;
#else
		AdmaInProgress = 0;
		if(InputDataLeft >= 0x200)
		{
#ifdef PCM24_S1_INTERLEAVE
			AutoDMAReadBuffer(1);
#else
			AutoDMAReadBuffer(0);
#endif
			AdmaInProgress = 1;

			TSA = (Index<<10) + InputPosRead;

			if (InputDataLeft < 0x200)
			{
				FileLog("[%10d] %s AutoDMA%c block end.\n", (Index==1) ? "CDDA" : "SPDIF", Cycles, GetDmaIndexChar());

				if( IsDevBuild )
				{
					if(InputDataLeft > 0)
					{
						if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
					}
				}
				InputDataLeft	= 0;
				// Hack, kinda. We call the interrupt early here, since PCSX2 doesn't like them delayed.
				//DMAICounter		= 1;
				if(Index == 0)	{ if(dma4callback) dma4callback(); }
				else			{ if(dma7callback) dma7callback(); }
			}
		}
#endif
		InputPosRead &= 0x1ff;
	}
	return retval;
}

StereoOut32 V_Core::ReadInput()
{
	if((AutoDMACtrl&(Index+1)) != (Index+1))
		return StereoOut32();

	StereoOut32 retval;

	if( (Index!=1) || ((PlayMode&2)==0) )
	{
		//retval = StereoOut32(
		//	(s32)ADMATempBuffer[InputPosRead],
		//	(s32)ADMATempBuffer[InputPosRead+0x200]
		//);
		retval = StereoOut32(
			(s32)(*GetMemPtr(0x2000 + (Index<<10) + InputPosRead)),
			(s32)(*GetMemPtr(0x2200 + (Index<<10) + InputPosRead))
		);
	}

#ifdef PCSX2_DEVBUILD
	DebugCores[Index].admaWaveformL[InputPosRead%0x100]=retval.Left;
	DebugCores[Index].admaWaveformR[InputPosRead%0x100]=retval.Right;
#endif

	InputPosRead++;
	if( (InputPosRead==0x100) || (InputPosRead>=0x200) )
	{
#ifdef ENABLE_NEW_IOPDMA_SPU2
		// WARNING: Assumes this to be in the same thread as the dmas
		AutoDmaFree += 0x200;
#else
		AdmaInProgress = 0;
		if(InputDataLeft >= 0x200)
		{
			//u8 k=InputDataLeft>=InputDataProgress;

			AutoDMAReadBuffer(0);

			AdmaInProgress	= 1;
			TSA			= (Index<<10) + InputPosRead;

			if (InputDataLeft < 0x200)
			{
				AutoDMACtrl |= ~3;

				if( IsDevBuild )
				{
					FileLog("[%10d] AutoDMA%c block end.\n",Cycles, GetDmaIndexChar());
					if(InputDataLeft>0)
					{
						if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
					}
				}

				InputDataLeft = 0;
				// Hack, kinda. We call the interrupt early here, since PCSX2 doesn't like them delayed.
				//DMAICounter   = 1;
				if(Index == 0)	{ if(dma4callback) dma4callback(); }
				else			{ if(dma7callback) dma7callback(); }
			}
		}
#endif
		InputPosRead&=0x1ff;
	}
	return retval;
}
