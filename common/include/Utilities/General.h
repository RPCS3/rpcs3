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

#pragma once

// This macro is actually useful for about any and every possible application of C++
// equality operators.
#define OpEqu( field )		(field == right.field)

// Macro used for removing some of the redtape involved in defining bitfield/union helpers.
//
#define BITFIELD32()	\
	union {				\
		u32 bitset;		\
		struct {

#define BITFIELD_END	}; };


// ----------------------------------------------------------------------------------------
//  RecursionGuard  -  Basic protection against function recursion
// ----------------------------------------------------------------------------------------
// Thread safety note: If used in a threaded environment, you shoud use a handle to a __threadlocal
// storage variable (protects aaginst race conditions and, in *most* cases, is more desirable
// behavior as well.
//
// Rationale: wxWidgets has its own wxRecursionGuard, but it has a sloppy implementation with
// entirely unnecessary assertion checks.
//
class RecursionGuard
{
public:
	int& Counter;

	RecursionGuard( int& counter ) : Counter( counter )
	{ ++Counter; }

	virtual ~RecursionGuard() throw()
	{ --Counter; }

	bool IsReentrant() const { return Counter > 1; }
};

// --------------------------------------------------------------------------------------
//  ICloneable / IActionInvocation / IDeletableObject
// --------------------------------------------------------------------------------------
class IActionInvocation
{
public:
	virtual ~IActionInvocation() throw() {}
	virtual void InvokeAction()=0;
};

class ICloneable
{
public:
	virtual ICloneable* Clone() const=0;
};

class IDeletableObject
{
public:
	virtual ~IDeletableObject() throw() {}

	virtual void DeleteSelf()=0;
	virtual bool IsBeingDeleted()=0;

protected:
	// This function is GUI implementation dependent!  It's implemented by PCSX2's AppHost,
	// but if the SysCore is being linked to another front end, you'll need to implement this
	// yourself.  Most GUIs have built in message pumps.  If a platform lacks one then you'll
	// need to implement one yourself (yay?).
	virtual void DoDeletion()=0;
};

// --------------------------------------------------------------------------------------
//  BaseDeletableObject
// --------------------------------------------------------------------------------------
// Oh the fruits and joys of multithreaded C++ coding conundrums!  This class provides a way
// to be deleted from arbitraty threads, or to delete themselves (which is considered unsafe
// in C++, though it does typically work).  It also gives objects a second recourse for
// doing fully virtualized cleanup, something C++ also makes impossible because of how it
// implements it's destructor hierarchy.
//
// To utilize virtual destruction, override DoDeletion() and be sure to invoke the base class
// implementation of DoDeletion().
//
// Assertions:
//   This class generates an assertion of the destructor is called from anything other than
//   the main/gui thread.
//
// Rationale:
//   wxWidgets provides a pending deletion feature, but it's specific to wxCore (not wxBase)
//   which means it requires wxApp and all that, which is bad for plugins and the possibility
//   of linking PCSX2 core against a non-WX gui in the future.  It's also not thread safe
//   (sigh).  And, finally, it requires quite a bit of red tape to implement wxObjects because
//   of the wx-custom runtime type information.  So I made my own.
//
class BaseDeletableObject : public virtual IDeletableObject
{
protected:
	volatile long	m_IsBeingDeleted;

public:
	BaseDeletableObject();
	virtual ~BaseDeletableObject() throw();

	void DeleteSelf();
	bool IsBeingDeleted() { return !!m_IsBeingDeleted; }

	// Returns FALSE if the object is already marked for deletion, or TRUE if the app
	// should schedule the object for deletion.  Only schedule if TRUE is returned, otherwise
	// the object could get deleted twice if two threads try to schedule it at the same time.
	bool MarkForDeletion();

protected:
	// This function is GUI implementation dependent!  It's implemented by PCSX2's AppHost,
	// but if the SysCore is being linked to another front end, you'll need to implement this
	// yourself.  Most GUIs have built in message pumps.  If a platform lacks one then you'll
	// need to implement one yourself (yay?).
	virtual void DoDeletion();
};


enum PageProtectionMode
{
	Protect_NoAccess = 0,
	Protect_ReadOnly,
	Protect_ReadWrite
};

// --------------------------------------------------------------------------------------
//  HostSys
// --------------------------------------------------------------------------------------
// (this namespace name sucks, and is a throw-back to an older attempt to make things cross
// platform prior to wxWidgets .. it should prolly be removed -- air)
namespace HostSys
{
	// Maps a block of memory for use as a recompiled code buffer.
	// The allocated block has code execution privileges.
	// Returns NULL on allocation failure.
	extern void *Mmap(uptr base, u32 size);

	// Unmaps a block allocated by SysMmap
	extern void Munmap(uptr base, u32 size);

	extern void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution=false );

	extern void Munmap( void* base, u32 size );

	template< uint size >
	void MemProtectStatic( u8 (&arr)[size], PageProtectionMode mode, bool allowExecution=false )
	{
		MemProtect( arr, size, mode, allowExecution );
	}
}

// Safe version of Munmap -- NULLs the pointer variable immediately after free'ing it.
#define SafeSysMunmap( ptr, size ) \
	((void) ( HostSys::Munmap( (uptr)(ptr), size ), (ptr) = NULL ))

extern void InitCPUTicks();
extern u64  GetTickFrequency();
extern u64  GetCPUTicks();

extern wxString GetOSVersionString();
