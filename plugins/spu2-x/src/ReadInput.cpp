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
#include "dma.h"

#if 0
static void __fastcall __ReadInput( uint core, StereoOut32& PData ) 
{
	V_Core& thiscore( Cores[core] );

	if((thiscore.AutoDMACtrl&(core+1))==(core+1))
	{
		s32 tl,tr;

		if((core==1)&&((PlayMode&8)==8))
		{
			thiscore.InputPos&=~1;

			// CDDA mode
			// Source audio data is 32 bits.
			// We don't yet have the capability to handle this high res input data
			// so we just downgrade it to 16 bits for now.

#ifdef PCM24_S1_INTERLEAVE
			*PData.Left=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1))));
			*PData.Right=*(((s32*)(thiscore.ADMATempBuffer+(thiscore.InputPos<<1)+2)));
#else
			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PData.Left = *pl;
			PData.Right = *pr;
#endif

			PData.Left >>= 2; //give 30 bit data (SndOut downsamples the rest of the way)
			PData.Right >>= 2;

			thiscore.InputPos+=2;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

#ifdef PCM24_S1_INTERLEAVE
					thiscore.AutoDMAReadBuffer(1);
#else
					thiscore.AutoDMAReadBuffer(0);
#endif
					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else if((core==0)&&((PlayMode&4)==4))
		{
			thiscore.InputPos&=~1;

			s32 *pl=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos]);
			s32 *pr=(s32*)&(thiscore.ADMATempBuffer[thiscore.InputPos+0x200]);
			PData.Left  = *pl;
			PData.Right = *pr;

			thiscore.InputPos+=2;
			if(thiscore.InputPos>=0x200) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					thiscore.AutoDMAReadBuffer(0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						FileLog("[%10d] Spdif AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');

						if( IsDevBuild )
						{
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}
						thiscore.InputDataLeft=0;
						thiscore.DMAICounter=1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}

		}
		else
		{
			if((core==1)&&((PlayMode&2)!=0))
			{
				tl=0;
				tr=0;
			}
			else
			{
				// Using the temporary buffer because this area gets overwritten by some other code.
				//*PData.Left  = (s32)*(s16*)(spu2mem+0x2000+(core<<10)+thiscore.InputPos);
				//*PData.Right = (s32)*(s16*)(spu2mem+0x2200+(core<<10)+thiscore.InputPos);

				tl = (s32)thiscore.ADMATempBuffer[thiscore.InputPos];
				tr = (s32)thiscore.ADMATempBuffer[thiscore.InputPos+0x200];

			}

			PData.Left  = tl;
			PData.Right = tr;

			thiscore.InputPos++;
			if((thiscore.InputPos==0x100)||(thiscore.InputPos>=0x200)) {
				thiscore.AdmaInProgress=0;
				if(thiscore.InputDataLeft>=0x200)
				{
					u8 k=thiscore.InputDataLeft>=thiscore.InputDataProgress;

					thiscore.AutoDMAReadBuffer(0);

					thiscore.AdmaInProgress=1;

					thiscore.TSA=(core<<10)+thiscore.InputPos;

					if (thiscore.InputDataLeft<0x200) 
					{
						thiscore.AutoDMACtrl |= ~3;

						if( IsDevBuild )
						{
							FileLog("[%10d] AutoDMA%c block end.\n",Cycles, (core==0)?'4':'7');
							if(thiscore.InputDataLeft>0)
							{
								if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
							}
						}

						thiscore.InputDataLeft = 0;
						thiscore.DMAICounter   = 1;
					}
				}
				thiscore.InputPos&=0x1ff;
			}
		}
	}
	else
	{
		PData.Left  = 0;
		PData.Right = 0;
	}
}
#endif


// CDDA mode - Source audio data is 32 bits.
// PS2 note:  Very! few PS2 games use this mode.  Some PSX games used it, however no
// *known* PS2 game does since it was likely only available if the game was recorded to CD
// media (ie, not available in DVD mode, which almost all PS2 games use).  Plus PS2 games
// generally prefer to use ADPCM streaming audio since they need as much storage space as
// possible for FMVs and high-def textures.
//
__forceinline StereoOut32 V_Core::ReadInput_HiFi( bool isCDDA )
{
	InputPos &= ~1;

#ifdef PCM24_S1_INTERLEAVE
	StereoOut32 retval(
		*((s32*)(ADMATempBuffer+(InputPos<<1))),
		*((s32*)(ADMATempBuffer+(InputPos<<1)+2))
	);
#else
	StereoOut32 retval( 
		(s32&)(ADMATempBuffer[InputPos]),
		(s32&)(ADMATempBuffer[InputPos+0x200])
	);
#endif

	if( isCDDA )
	{
		//give 30 bit data (SndOut downsamples the rest of the way)
		retval.Left		>>= 2;
		retval.Right	>>= 2;
	}
	
	InputPos += 2;
	if( (InputPos==0x100) || (InputPos>=0x200) )	// CDDA mode?
	//if( InputPos >= 0x200 )
	{
		AdmaInProgress = 0;
		if(InputDataLeft >= 0x200)
		{
			u8 k = (InputDataLeft >= InputDataProgress);

#ifdef PCM24_S1_INTERLEAVE
			AutoDMAReadBuffer(1);
#else
			AutoDMAReadBuffer(0);
#endif
			AdmaInProgress = 1;

			TSA = (Index<<10) + InputPos;

			if (InputDataLeft < 0x200) 
			{
				FileLog("[%10d] %s AutoDMA%c block end.\n", isCDDA ? "CDDA" : "SPDIF", Cycles, GetDmaIndexChar());

				if( IsDevBuild )
				{
					if(InputDataLeft > 0)
					{
						if(MsgAutoDMA()) ConLog("WARNING: adma buffer didn't finish with a whole block!!\n");
					}
				}
				InputDataLeft	= 0;
				DMAICounter		= 1;
			}
		}
		InputPos &= 0x1ff;
	}
	return retval;
}

StereoOut32 V_Core::ReadInput()
{
	/*StereoOut32 retval2;
	__ReadInput( Index, retval2 );
	return retval2;*/

	if((AutoDMACtrl&(Index+1)) != (Index+1))
		return StereoOut32();

	if( (Index==1) && ((PlayMode&8)==8) )
	{
		return ReadInput_HiFi( false );
	}
	else if( (Index==0) && ((PlayMode&4)==4) )
	{
		return ReadInput_HiFi( true );
	}
	else
	{
		StereoOut32 retval;
		
		if( (Index!=1) || ((PlayMode&2)==0) )
		{
			// Using the temporary buffer because this area gets overwritten by some other code.
			//*PData.Left  = (s32)*(s16*)(spu2mem+0x2000+(core<<10)+InputPos);
			//*PData.Right = (s32)*(s16*)(spu2mem+0x2200+(core<<10)+InputPos);

			retval = StereoOut32(
				(s32)ADMATempBuffer[InputPos],
				(s32)ADMATempBuffer[InputPos+0x200]
			);
		}

		InputPos++;
		if( (InputPos==0x100) || (InputPos>=0x200) )
		{
			AdmaInProgress = 0;
			if(InputDataLeft >= 0x200)
			{
				u8 k=InputDataLeft>=InputDataProgress;

				AutoDMAReadBuffer(0);

				AdmaInProgress	= 1;
				TSA			= (Index<<10) + InputPos;

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
					DMAICounter   = 1;
				}
			}
			InputPos&=0x1ff;
		}
		return retval;
	}
}
