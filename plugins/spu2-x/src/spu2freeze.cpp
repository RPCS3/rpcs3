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
#include "PS2E-spu2.h"		// hopefully temporary, until I resolve lClocks depdendency

namespace Savestate
{
	// Arbitrary ID to identify SPU2-X saves.
	static const u32 SAVE_ID = 0x1227521;

	// versioning for saves.
	// Increment this when changes to the savestate system are made.
	static const u32 SAVE_VERSION = 0x000a;

	static void wipe_the_cache()
	{
		memset( pcm_cache_data, 0, pcm_BlockCount * sizeof(PcmCacheEntry) );
	}
}

struct Savestate::DataBlock
{
	u32		spu2id;				// SPU2-X state identifier lets ZeroGS/PeopsSPU2 know this isn't their state)
	u8		unkregs[0x10000];	// SPU2 raw register memory
	u8		mem[0x200000];		// SPU2 raw sample memory

	u32		version;			// SPU2-X version identifier
	V_Core	Cores[2];
	V_SPDIF	Spdif;
	s16		OutPos;
	s16		InputPos;
	u32		Cycles;
	u32		lClocks;
	int		PlayMode;
};

s32 __fastcall Savestate::FreezeIt( DataBlock& spud )
{
	spud.spu2id		= SAVE_ID;
	spud.version	= SAVE_VERSION;

	pxAssumeMsg( spu2regs && _spu2mem, "Looks like PCSX2 is trying to savestate while pluigns are shut down.  That's a no-no! It shouldn't crash, but the savestate will probably be corrupted." );

	if( spu2regs != NULL )	memcpy(spud.unkregs,	spu2regs,	sizeof(spud.unkregs));
	if( _spu2mem != NULL )	memcpy(spud.mem,		_spu2mem,	sizeof(spud.mem));

	memcpy(spud.Cores,		Cores,		sizeof(Cores));
	memcpy(&spud.Spdif,		&Spdif,		sizeof(Spdif));

	spud.OutPos		= OutPos;
	spud.InputPos	= InputPos;
	spud.Cycles		= Cycles;
	spud.lClocks    = lClocks;
	spud.PlayMode	= PlayMode;

	// note: Don't save the cache.  PCSX2 doesn't offer a safe method of predicting
	// the required size of the savestate prior to saving, plus this is just too
	// "implementation specific" for the intended spec of a savestate.  Let's just
	// force the user to rebuild their cache instead.

	return 0;
}

s32 __fastcall Savestate::ThawIt( DataBlock& spud )
{
	if( spud.spu2id != SAVE_ID || spud.version < SAVE_VERSION )
	{
		fprintf(stderr, "\n*** SPU2-X Warning:\n");
		if( spud.spu2id == SAVE_ID )
			fprintf(stderr, "\tSavestate version is from an older version of this plugin.\n");
		else
			fprintf(stderr, "\tThe savestate you are trying to load was not made with this plugin.\n");

		fprintf(stderr,
			"\tAudio may not recover correctly.  Save your game to memorycard, reset,\n\n"
			"\tand then continue from there.\n\n"
		);

		// Do *not* reset the cores.
		// We'll need some "hints" as to how the cores should be initialized, and the
		// only way to get that is to use the game's existing core settings and hope
		// they kinda match the settings for the savestate (IRQ enables and such).

		// adpcm cache : Clear all the cache flags and buffers.

		wipe_the_cache();
	}
	else
	{
		SndBuffer::ClearContents();

		pxAssumeMsg( spu2regs && _spu2mem, "Looks like PCSX2 is trying to loadstate while pluigns are shut down.  That's a no-no!  It shouldn't crash, but the savestate will probably be corrupted." );

		// base stuff
		if( spu2regs )	memcpy(spu2regs, spud.unkregs,	sizeof(spud.unkregs));
		if( _spu2mem )	memcpy(_spu2mem, spud.mem,		sizeof(spud.mem));

		memcpy(Cores,	spud.Cores,		sizeof(Cores));
		memcpy(&Spdif,	&spud.Spdif,	sizeof(Spdif));

		OutPos		= spud.OutPos;
		InputPos	= spud.InputPos;
		Cycles		= spud.Cycles;
		lClocks     = spud.lClocks;
		PlayMode	= spud.PlayMode;

		wipe_the_cache();

		// Go through the V_Voice structs and recalculate SBuffer pointer from
		// the NextA setting.

		for( int c=0; c<2; c++ )
		{
			for( int v=0; v<24; v++ )
			{
				const int cacheIdx = Cores[c].Voices[v].NextA / pcm_WordsPerBlock;
				Cores[c].Voices[v].SBuffer = pcm_cache_data[cacheIdx].Sampledata;
			}
		}

		// HACKFIX!! DMAPtr can be invalid after a savestate load, so force it to NULL and
		// ignore it on any pending ADMA writes.  (the DMAPtr concept used to work in old VM
		// editions of PCSX2 with fixed addressing, but new PCSX2s have dynamic memory
		// addressing).

		Cores[0].DMAPtr = Cores[1].DMAPtr = NULL;
	}
	return 0;
}

s32 __fastcall Savestate::SizeIt()
{
	return sizeof(DataBlock);
}
