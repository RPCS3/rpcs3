/*  PCSX2 - PS2 Emulator for PCs
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

#pragma once

// This shouldn't break Win compiles, but it does.
#ifdef __LINUX__
#include "PS2Edefs.h"
#endif
#include "System.h"

// Savestate Versioning!
//  If you make changes to the savestate version, please increment the value below.
//  If the change is minor and compatibility with old states is retained, increment
//  the lower 16 bit value.  IF the change is breaking of all compatibility with old
//  states, increment the upper 16 bit value, and clear the lower 16 bits to 0.

static const u32 g_SaveVersion = 0x8b410002;

// this function is meant to be used in the place of GSfreeze, and provides a safe layer
// between the GS saving function and the MTGS's needs. :)
extern s32 CALLBACK gsSafeFreeze( int mode, freezeData *data );


enum FreezeSectionId
{
	FreezeId_End,

	FreezeId_Memory,
	FreezeId_Registers,

	// A BIOS tag should always be saved in conjunction with Memory or Registers tags,
	// but can be skipped if the savestate has only plugins.
	FreezeId_Bios,

	FreezeId_Plugin,

	// anything here and beyond we can skip, with a warning
	FreezeId_Unknown,
};

// --------------------------------------------------------------------------------------
//  SaveStateBase class
// --------------------------------------------------------------------------------------
// Provides the base API for both loading and saving savestates.  Normally you'll want to
// use one of the four "functional" derived classes rather than this class directly: gzLoadingState, gzSavingState (gzipped disk-saved
// states), and memLoadingState, memSavingState (uncompressed memory states).
class SaveStateBase
{
protected:
	SafeArray<u8>& m_memory;
	char m_tagspace[32];

	u32 m_version;		// version of the savestate being loaded.

	int m_idx;		// current read/write index of the allocation
	int m_sectid;
	int m_pid;
	
	bool m_DidBios;

public:
	SaveStateBase( SafeArray<u8>& memblock );
	virtual ~SaveStateBase() { }

	static wxString GetFilename( int slot );

	// Gets the version of savestate that this object is acting on.
	// The version refers to the low 16 bits only (high 16 bits classifies Pcsx2 build types)
	u32 GetVersion() const
	{
		return (m_version & 0xffff);
	}

	// Loads or saves the entire emulation state.
	// Note: The Cpu state must be reset, and plugins *open*, prior to Defrosting
	// (loading) a state!
	void FreezeAll();

	// Loads or saves an arbitrary data type.  Usable on atomic types, structs, and arrays.  
	// For dynamically allocated pointers use FreezeMem instead.
	template<typename T>
	void Freeze( T& data )
	{
		FreezeMem( &data, sizeof( T ) );
	}

	// FreezeLegacy can be used to load structures short of their full size, which is
	// useful for loading structures that have had new stuff added since a previous version.
	template<typename T>
	void FreezeLegacy( T& data, int sizeOfNewStuff )
	{
		FreezeMem( &data, sizeof( T ) - sizeOfNewStuff );
	}

	void PrepBlock( int size );

	u8* GetBlockPtr()
	{
		return &m_memory[m_idx];
	}

	void CommitBlock( int size )
	{
		m_idx += size;
	}

	bool FreezeSection();

	// Freezes an identifier value into the savestate for troubleshooting purposes.
	// Identifiers can be used to determine where in a savestate that data has become
	// skewed (if the value does not match then the error occurs somewhere prior to that
	// position).
	void FreezeTag( const char* src );

	// Returns true if this object is a StateLoading type object.
	bool IsLoading() const { return !IsSaving(); }

	// Loads or saves a memory block.
	virtual void FreezeMem( void* data, int size )=0;

	// Returns true if this object is a StateSaving type object.
	virtual bool IsSaving() const=0;

public:
	// note: gsFreeze() needs to be public because of the GSState recorder.
	void gsFreeze();

protected:

	// Load/Save functions for the various components of our glorious emulator!

	void FreezeBios();
	void FreezeMainMemory();
	void FreezeRegisters();

	void rcntFreeze();
	void vuMicroFreeze();
	void vif0Freeze();
	void vif1Freeze();
	void sifFreeze();
	void ipuFreeze();
	void gifFreeze();
	void sprFreeze();

	void sioFreeze();
	void cdrFreeze();
	void cdvdFreeze();
	void psxRcntFreeze();
	void sio2Freeze();

	// called by gsFreeze automatically.
	void mtgsFreeze();

};

// --------------------------------------------------------------------------------------
//  Saving and Loading Specialized Implementations...
// --------------------------------------------------------------------------------------

class memSavingState : public SaveStateBase
{
protected:
	static const int ReallocThreshold = 0x200000;	// 256k reallocation block size.
	static const int MemoryBaseAllocSize = 0x02a00000;  // 42 meg base alloc

public:
	virtual ~memSavingState() { }
	memSavingState( SafeArray<u8>& save_to );
	
	// Saving of state data to a memory buffer
	void FreezeMem( void* data, int size );
	bool IsSaving() const { return true; }
};

class memLoadingState : public SaveStateBase
{
public:
	virtual ~memLoadingState();
	memLoadingState( const SafeArray<u8>& load_from );

	// Loading of state data from a memory buffer...
	void FreezeMem( void* data, int size );
	bool IsSaving() const { return false; }
	bool IsFinished() const { return m_idx >= m_memory.GetSizeInBytes(); }
};

namespace StateRecovery
{
	extern bool HasState();
	extern void Recover();
	extern void SaveToFile( const wxString& file );
	extern void SaveToSlot( uint num );
	extern void MakeFull();
	extern void Clear();
}

