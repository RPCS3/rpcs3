/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

// x86 opcode descriptors
#define XMMREGS 8
#define X86REGS 8
#define MMXREGS 8

#define SIB 4
#define SIBDISP 5
#define DISP32 5

// general types
typedef int x86IntRegType;

#define EAX 0
#define EBX 3
#define ECX 1
#define EDX 2
#define ESI 6
#define EDI 7
#define EBP 5
#define ESP 4

#define X86ARG1 EAX
#define X86ARG2 ECX
#define X86ARG3 EDX
#define X86ARG4 EBX

#define MM0 0
#define MM1 1
#define MM2 2
#define MM3 3
#define MM4 4
#define MM5 5
#define MM6 6
#define MM7 7

typedef int x86MMXRegType;

#define XMM0 0 
#define XMM1 1 
#define XMM2 2 
#define XMM3 3 
#define XMM4 4 
#define XMM5 5 
#define XMM6 6 
#define XMM7 7 
#define XMM8 8 
#define XMM9 9 
#define XMM10 10 
#define XMM11 11 
#define XMM12 12 
#define XMM13 13 
#define XMM14 14 
#define XMM15 15

typedef int x86SSERegType;

enum XMMSSEType
{
	XMMT_INT = 0, // integer (sse2 only)
	XMMT_FPS = 1, // floating point
	//XMMT_FPD = 3, // double
};

extern XMMSSEType g_xmmtypes[XMMREGS];

extern void cpudetectInit( void );//this is all that needs to be called and will fill up the below structs

typedef struct CAPABILITIES CAPABILITIES;
//cpu capabilities structure
struct CAPABILITIES {
   u32 hasFloatingPointUnit;
   u32 hasVirtual8086ModeEnhancements;
   u32 hasDebuggingExtensions;
   u32 hasPageSizeExtensions;
   u32 hasTimeStampCounter;
   u32 hasModelSpecificRegisters;
   u32 hasPhysicalAddressExtension;
   u32 hasCOMPXCHG8BInstruction;
   u32 hasAdvancedProgrammableInterruptController;
   u32 hasSEPFastSystemCall;
   u32 hasMemoryTypeRangeRegisters;
   u32 hasPTEGlobalFlag;
   u32 hasMachineCheckArchitecture;
   u32 hasConditionalMoveAndCompareInstructions;
   u32 hasFGPageAttributeTable;
   u32 has36bitPageSizeExtension;
   u32 hasProcessorSerialNumber;
   u32 hasCFLUSHInstruction;
   u32 hasDebugStore;
   u32 hasACPIThermalMonitorAndClockControl;
   u32 hasMultimediaExtensions;
   u32 hasFastStreamingSIMDExtensionsSaveRestore;
   u32 hasStreamingSIMDExtensions;
   u32 hasStreamingSIMD2Extensions;
   u32 hasSelfSnoop;
   u32 hasMultiThreading;			// is TRUE for both mutli-core and Hyperthreaded CPUs.
   u32 hasThermalMonitor;
   u32 hasIntel64BitArchitecture;
   u32 hasStreamingSIMD3Extensions;
   u32 hasSupplementalStreamingSIMD3Extensions;
   u32 hasStreamingSIMD4Extensions;

   // AMD-specific CPU Features
   u32 hasMultimediaExtensionsExt;
   u32 hasAMD64BitArchitecture;
   u32 has3DNOWInstructionExtensionsExt;
   u32 has3DNOWInstructionExtensions;
};

extern CAPABILITIES cpucaps;

struct CPUINFO{
   
   u32 x86Family;	   // Processor Family
   u32 x86Model;	   // Processor Model
   u32 x86PType;	   // Processor Type
   u32 x86StepID;	   // Stepping ID
   u32 x86Flags;	   // Feature Flags
   u32 x86Flags2;	   // More Feature Flags
   u32 x86EFlags;	   // Extended Feature Flags

   u32 PhysicalCores;
   u32 LogicalCores;

   char x86ID[16];	   // Vendor ID  //the vendor creator (in %s)
   char x86Type[20];   //cpu type in char format //the cpu type (in %s)
   char x86Fam[50];    // family in char format //the original cpu name string (in %s)
   u32 cpuspeed;      // speed of cpu //this will give cpu speed (in %d)
};

extern CPUINFO cpuinfo;
//------------------------------------------------------------------

static __forceinline bool is_s8( u32 imm ) { return (s8)imm == (s32)imm; }

namespace x86Emitter
{
	class x86ModRm;

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	struct x86Register
	{
		static const x86Register Empty;		// defined as an empty/unused value (-1)
		
		int Id;

		x86Register( const x86Register& src ) : Id( src.Id ) {}
		x86Register() : Id( -1 ) {}
		explicit x86Register( int regId ) : Id( regId ) { }

		bool IsEmpty() const { return Id == -1; }

		bool operator==( const x86Register& src ) const { return Id == src.Id; }
		bool operator!=( const x86Register& src ) const { return Id != src.Id; }
		
		x86ModRm operator+( const x86Register& right ) const;
		x86ModRm operator+( const x86ModRm& right ) const;
		
		x86Register& operator=( const x86Register& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Similar to x86Register, but without the ability to add/combine them with ModSib.
	//
	class x86Register16
	{
	public:
		static const x86Register16 Empty;

		int Id;

		x86Register16( const x86Register16& src ) : Id( src.Id ) {}
		x86Register16() : Id( -1 ) {}
		explicit x86Register16( int regId ) : Id( regId ) { }

		bool IsEmpty() const { return Id == -1; }

		bool operator==( const x86Register16& src ) const { return Id == src.Id; }
		bool operator!=( const x86Register16& src ) const { return Id != src.Id; }

		x86Register16& operator=( const x86Register16& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Similar to x86Register, but without the ability to add/combine them with ModSib.
	//
	class x86Register8
	{
	public:
		static const x86Register8 Empty;

		int Id;

		x86Register8( const x86Register16& src ) : Id( src.Id ) {}
		x86Register8() : Id( -1 ) {}
		explicit x86Register8( int regId ) : Id( regId ) { }

		bool IsEmpty() const { return Id == -1; }

		bool operator==( const x86Register8& src ) const { return Id == src.Id; }
		bool operator!=( const x86Register8& src ) const { return Id != src.Id; }

		x86Register8& operator=( const x86Register8& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class x86ModRm
	{
	public:
		x86Register Base;		// base register (no scale)
		x86Register Index;		// index reg gets multiplied by the scale
		int Factor;				// scale applied to the index register, in factor form (not a shift!)
		s32 Displacement;		// address displacement

	public:
		x86ModRm( x86Register base, x86Register index, int factor=1, s32 displacement=0 ) :
			Base( base ),
			Index( index ),
			Factor( factor ),
			Displacement( displacement )
		{
		}

		explicit x86ModRm( x86Register base, int displacement=0 ) :
			Base( base ),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		explicit x86ModRm( s32 displacement ) :
			Base(),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		static x86ModRm FromIndexReg( x86Register index, int scale=0, s32 displacement=0 );

	public:
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }
		x86Register GetEitherReg() const;

		x86ModRm& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}
		
		x86ModRm& Add( const x86Register& src );
		x86ModRm& Add( const x86ModRm& src );

		x86ModRm operator+( const x86Register& right ) const { return x86ModRm( *this ).Add( right ); }
		x86ModRm operator+( const x86ModRm& right ) const { return x86ModRm( *this ).Add( right ); }
		x86ModRm operator+( const s32 imm ) const { return x86ModRm( *this ).Add( imm ); }
		x86ModRm operator-( const s32 imm ) const { return x86ModRm( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib - Internal low-level representation of the ModRM/SIB information.
	//
	// This class serves two purposes:  It houses 'reduced' ModRM/SIB info only, which means that
	// the Base, Index, Scale, and Displacement values are all valid, and it serves as a type-
	// safe layer between the x86Register's operators (which generate x86ModRm types) and the
	// emitter's ModSib instruction forms.  Without this, the x86Register would pass as a
	// ModSib type implicitly, and that would cause ambiguity on a number of instructions.
	//
	class ModSib
	{
	public:
		x86Register Base;		// base register (no scale)
		x86Register Index;		// index reg gets multiplied by the scale
		int Scale;				// scale applied to the index register, in scale/shift form
		s32 Displacement;		// offset applied to the Base/Index registers.

		ModSib( const x86ModRm& src );
		ModSib( x86Register base, x86Register index, int scale=0, s32 displacement=0 );
		ModSib( s32 disp );
		
		x86Register GetEitherReg() const;
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

	protected:
		void Reduce();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// x86IndexerType - This is a static class which provisions our ptr[] syntax.
	//
	struct x86IndexerType
	{
		ModSib operator[]( x86Register src ) const
		{
			return ModSib( src, x86Register::Empty );
		}

		ModSib operator[]( const x86ModRm& src ) const
		{
			return ModSib( src );
		}

		ModSib operator[]( uptr src ) const
		{
			return ModSib( src );
		}

		ModSib operator[]( void* src ) const
		{
			return ModSib( (uptr)src );
		}
	};

	// ------------------------------------------------------------------------
	extern const x86Register eax;
	extern const x86Register ebx;
	extern const x86Register ecx;
	extern const x86Register edx;
	extern const x86Register esi;
	extern const x86Register edi;
	extern const x86Register ebp;
	extern const x86Register esp;

	extern const x86IndexerType ptr;
}