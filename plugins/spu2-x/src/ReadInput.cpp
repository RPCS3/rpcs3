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

#include "Spu2.h"

void __fastcall ReadInput( uint core, StereoOut32& PData ) 
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
					AutoDMAReadBuffer(core,1);
#else
					AutoDMAReadBuffer(core,0);
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

					AutoDMAReadBuffer(core,0);

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

					AutoDMAReadBuffer(core,0);

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
