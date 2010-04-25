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

static const s32 ADSR_MAX_VOL = 0x7fffffff;

static const int InvExpOffsets[] = { 0,4,6,8,9,10,11,12 };
static u32 PsxRates[160];


void InitADSR()                                    // INIT ADSR
{
	for (int i=0; i<(32+128); i++)
	{
		int shift=(i-32)>>2;
		s64 rate=(i&3)+4;
		if (shift<0)
			rate >>= -shift;
		else
			rate <<= shift;

		PsxRates[i] = (int)std::min( rate, 0x3fffffffLL );
	}
}

#define VOL(x) (((s32)x)) //24.8 volume

// Returns the linear slide value for AR and SR inputs.
// (currently not used, it's buggy)
static int GetLinearSrAr( uint SrAr )
{
	// The Sr/Ar settings work in quarter steps, which means
	// the bottom 2 bits go on the left side of the shift, and
	// the right side of the shift gets divided by 4:

	const uint newSr = 0x7f - SrAr;
	return ((1|(newSr&3)) << (newSr>>2));
}

bool V_ADSR::Calculate()
{
	jASSUME( Phase != 0 );

	if(Releasing && (Phase < 5))
		Phase = 5;

	switch (Phase)
	{
		case 1: // attack
			if( Value == ADSR_MAX_VOL )
			{
				// Already maxed out.  Progress phase and nothing more:
				Phase++;
				break;
			}

			// Case 1 below is for pseudo exponential below 75%.
			// Pseudo Exp > 75% and Linear are the same.

			if( AttackMode && (Value>=0x60000000) )
				Value += PsxRates[(AttackRate^0x7f)-0x18+32];
			else
				Value += PsxRates[(AttackRate^0x7f)-0x10+32];

			if( Value < 0 )
			{
				// We hit the ceiling.
				Phase++;
				Value = ADSR_MAX_VOL;
			}
		break;

		case 2: // decay
		{
			u32 off = InvExpOffsets[(Value>>28)&7];
			Value -= PsxRates[((DecayRate^0x1f)*4)-0x18+off+32];

			// calculate sustain level as a factor of the ADSR maximum volume.

			s32 suslev = ((0x80000000 / 0x10 ) * (SustainLevel+1)) - 1;

			if( Value <= suslev )
			{
				if (Value < 0)
					Value = 0;
				Phase++;
			}
		}
		break;

		case 3: // sustain
		{
			// 0x7f disables sustain (infinite sustain)
			if( SustainRate == 0x7f ) return true;

			if (SustainMode&2) // decreasing
			{
				if (SustainMode&4) // exponential
				{
					u32 off = InvExpOffsets[(Value>>28)&7];
					Value -= PsxRates[(SustainRate^0x7f)-0x1b+off+32];
				}
				else // linear
					Value -= PsxRates[(SustainRate^0x7f)-0xf+32];

				if( Value <= 0 )
				{
					Value = 0;
					Phase++;
				}
			}
			else // increasing
			{
				if( (SustainMode&4) && (Value>=0x60000000) )
					Value += PsxRates[(SustainRate^0x7f)-0x18+32];
				else
					// linear / Pseudo below 75% (they're the same)
					Value += PsxRates[(SustainRate^0x7f)-0x10+32];

				if( Value < 0 )
				{
					Value = ADSR_MAX_VOL;
					Phase++;
				}
			}
		}
		break;

		case 4: // sustain end
			Value = (SustainMode&2) ? 0 : ADSR_MAX_VOL;
			if(Value==0)
				Phase=6;
		break;

		case 5: // release
			if (ReleaseMode) // exponential
			{
				u32 off=InvExpOffsets[(Value>>28)&7];
				Value-=PsxRates[((ReleaseRate^0x1f)*4)-0x18+off+32];
			}
			else // linear
			{
				//Value-=PsxRates[((ReleaseRate^0x1f)*4)-0xc+32];
				if( ReleaseRate != 0x1f )
					Value -= (1 << (0x1f-ReleaseRate));
			}

			if( Value <= 0 )
			{
				Value=0;
				Phase++;
			}
		break;

		case 6: // release end
			Value=0;
		break;

		jNO_DEFAULT
	}

	// returns true if the voice is active, or false if it's stopping.
	return Phase != 6;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //

#define VOLFLAG_REVERSE_PHASE	(1ul<<0)
#define VOLFLAG_DECREMENT		(1ul<<1)
#define VOLFLAG_EXPONENTIAL		(1ul<<2)
#define VOLFLAG_SLIDE_ENABLE	(1ul<<3)

void V_VolumeSlide::Update()
{
	if( !(Mode & VOLFLAG_SLIDE_ENABLE) ) return;

	// Volume slides use the same basic logic as ADSR, but simplified (single-stage
	// instead of multi-stage)

	if( Increment == 0x7f ) return;

	if (Mode & VOLFLAG_DECREMENT)
	{
		// Decrement

		if(Mode & VOLFLAG_EXPONENTIAL)
		{
			u32 off = InvExpOffsets[(Value>>28)&7];
			Value -= PsxRates[(Increment^0x7f)-0x1b+off+32];
		}
		else
			Value -= PsxRates[(Increment^0x7f)-0xf+32];

		if (Value < 0)
		{
			Value = 0;
			Mode = 0;	// disable slide
		}
	}
	else
	{
		// Increment
		// Pseudo-exponential increments, as done by the SPU2 (really!)
		// Above 75% slides slow, below 75% slides fast.  It's exponential, pseudo'ly speaking.

		if( (Mode & VOLFLAG_EXPONENTIAL) && (Value>=0x60000000))
			Value += PsxRates[(Increment^0x7f)-0x18+32];
		else
			// linear / Pseudo below 75% (they're the same)
			Value += PsxRates[(Increment^0x7f)-0x10+32];

		if( Value < 0 )		// wrapped around the "top"?
		{
			Value = 0x7fffffff;
			Mode = 0; // disable slide
		}
	}
}
