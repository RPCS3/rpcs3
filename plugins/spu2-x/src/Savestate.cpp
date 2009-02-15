/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "SPU2.h"

namespace Savestate {

struct SPU2freezeData
{
	u32 version;
	u8 unkregs[0x10000];
	u8 mem[0x200000];

	u32 id;
	V_Core Cores[2];
	V_SPDIF Spdif;
	s16 OutPos;
	s16 InputPos;
	u32 Cycles;
	s32 uTicks;
	int PlayMode;

	PcmCacheEntry cacheData;
};

// Arbitrary ID to identify SPU2-X saves.
static const u32 SAVE_ID = 0x1227521;

// versioning for saves.
// Increment this when changes to the savestate system are made.

static const u32 SAVE_VERSION = 0x0001;

static void wipe_the_cache()
{
	memset( pcm_cache_data, 0, pcm_BlockCount * sizeof(PcmCacheEntry) );
}


static s16 old_state_sBuffer[pcm_DecodedSamplesPerBlock] = {0};

typedef s32 __fastcall FreezeHandlerFunction( SPU2freezeData& data );

s32 __fastcall FreezeIt( SPU2freezeData& spud )
{
	if( disableFreezes )
	{
		// No point in making a save state since the SPU2
		// state is completely bogus anyway... Let's just
		// give this some random ID that no one will recognize.

		strcpy( (char*)&spud, "invalid" );
		return 0;
	}


	spud.id		 = SAVE_ID;
	spud.version = SAVE_VERSION;

	memcpy(spud.unkregs, spu2regs, 0x010000);
	memcpy(spud.mem,     _spu2mem, 0x200000);
	memcpy(spud.Cores, Cores, sizeof(Cores));
	memcpy(&spud.Spdif, &Spdif, sizeof(Spdif));
	spud.OutPos		= OutPos;
	spud.InputPos	= InputPos;
	spud.Cycles		= Cycles;
	spud.uTicks		= uTicks;
	spud.PlayMode	= PlayMode;
	
	// Save our cache:
	//   We could just force the user to rebuild the cache when loading
	//   from stavestates, but for most games the cache is pretty
	//   small and compresses well.
	//
	// Potential Alternative:
	//   If the cache is not saved then it is necessary to save the
	//   decoded blocks currently in use by active voices.  This allows
	//   voices to resume seamlessly on load.

	PcmCacheEntry* pcmDst = &spud.cacheData;
	int blksSaved=0;

	for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
	{
		if( pcm_cache_data[bidx].Validated )
		{
			// save a cache block!
			memcpy( pcmDst, &pcm_cache_data[bidx], sizeof(PcmCacheEntry) );
			pcmDst++;
			blksSaved++;
		}
	}

	//printf( " * SPU2 > FreezeSave > Saved %d cache blocks.\n", blksSaved++ );
	
	return 0;
}

s32 __fastcall ThawIt( SPU2freezeData& spud )
{
	if( spud.id != SAVE_ID || spud.version < SAVE_VERSION )
	{
		printf("\n*** SPU2-X Warning:\n");
		if( spud.id == SAVE_ID )
			printf("\tSavestate version is from an older version of this plugin.\n");
		else
			printf("\tThe savestate you are trying to load was not made with this plugin.\n");

		printf("\tAudio may not recover correctly.  Save your game to memorycard, reset,\n\n");
		printf("  and then continue from there.\n\n");

		disableFreezes=true;
		resetClock = true;

		// Do *not* reset the cores.
		// We'll need some "hints" as to how the cores should be initialized,
		// and the only way to get that is to use the game's existing core settings
		// and hope they kinda match the settings for the savestate (IRQ enables and such).
		//

		//CoreReset( 0 );
		//CoreReset( 1 );

		// adpcm cache : Clear all the cache flags and buffers.

		wipe_the_cache();
	}
	else
	{
		disableFreezes=false;

		// base stuff
		memcpy(spu2regs, spud.unkregs, 0x010000);
		memcpy(_spu2mem, spud.mem,     0x200000);

		memcpy(Cores, spud.Cores, sizeof(Cores));
		memcpy(&Spdif, &spud.Spdif, sizeof(Spdif));
		OutPos		= spud.OutPos;
		InputPos	= spud.InputPos;
		Cycles		= spud.Cycles;
		uTicks		= spud.uTicks;
		PlayMode	= spud.PlayMode;

		// Load the ADPCM cache:

		wipe_the_cache();

		const PcmCacheEntry* pcmSrc = &spud.cacheData;
		int blksLoaded=0;

		for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
		{
			if( pcm_cache_data[bidx].Validated )
			{
				// load a cache block!
				memcpy( &pcm_cache_data[bidx], pcmSrc, sizeof(PcmCacheEntry) );
				pcmSrc++;
				blksLoaded++;
			}
		}

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
	}
	return 0;
}

s32 __fastcall SizeIt()
{
	if( disableFreezes ) return 8;	// length of the string id "invalid" (plus a zero!)

	int size = sizeof(SPU2freezeData);

	// calculate the amount of memory consumed by our cache:

	for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
	{
		if( pcm_cache_data[bidx].Validated )
			size += pcm_DecodedSamplesPerBlock*sizeof(PcmCacheEntry);
	}
	return size;
}

}

using namespace Savestate;

EXPORT_C_(s32) SPU2freeze(int mode, freezeData *data)
{
	if( mode == FREEZE_SIZE )
		return SizeIt();

	jASSUME( mode == FREEZE_LOAD || mode == FREEZE_SAVE );
	jASSUME( data != NULL );

	if( data->data == NULL ) return -1;
	SPU2freezeData& spud = (SPU2freezeData&)*(data->data);

	switch( mode )
	{
		case FREEZE_LOAD: return ThawIt( spud );
		case FREEZE_SAVE: return FreezeIt( spud );
		
		jNO_DEFAULT;
	}
}
