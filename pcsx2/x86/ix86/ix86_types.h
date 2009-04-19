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

extern void cpudetectInit();//this is all that needs to be called and will fill up the below structs

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
#ifdef _MSC_VER
#define __threadlocal __declspec(thread)
#else
#define __threadlocal __thread
#endif

// Register counts for x86/32 mode:
static const uint iREGCNT_XMM = 8;
static const uint iREGCNT_GPR = 8;
static const uint iREGCNT_MMX = 8;

enum XMMSSEType
{
	XMMT_INT = 0, // integer (sse2 only)
	XMMT_FPS = 1, // floating point
	//XMMT_FPD = 3, // double
};

extern __threadlocal u8  *x86Ptr;
extern __threadlocal u8  *j8Ptr[32];		// depreciated item.  use local u8* vars instead.
extern __threadlocal u32 *j32Ptr[32];		// depreciated item.  use local u32* vars instead.

extern __threadlocal XMMSSEType g_xmmtypes[iREGCNT_XMM];

//------------------------------------------------------------------
// templated version of is_s8 is required, so that u16's get correct sign extension treatment.
template< typename T >
static __forceinline bool is_s8( T imm ) { return (s8)imm == (s32)imm; }

template< typename T >
static __forceinline void iWrite( T val )
{
	*(T*)x86Ptr = val;
	x86Ptr += sizeof(T); 
}

namespace x86Emitter
{

/////////////////////////////////////////////////////////////////////////////////////////////
// __emitline - preprocessors definition
// 
// This is configured to inline emitter functions appropriately for release builds, and 
// disables some of the more aggressive inlines for dev builds (which can be helpful when
// debugging).  Additionally,  I've set up the inlining to be as practical and intelligent
// as possible with regard to constant propagation.  Namely this involves forcing inlining
// for (void*) forms of ModRM, which (thanks to constprop) reduce to virtually no code, and
// force-disabling inlining on complicated SibSB forms [since MSVC would sometimes inline
// despite being a generally bad idea].
//
// In the case of (Reg, Imm) forms, the inlining is up to the discreation of the compiler.
//
// Note: I *intentionally* use __forceinline directly for most single-line class members,
// when needed.  There's no point in using __emitline in these cases since the debugger
// can't trace into single-line functions anyway.
//
#ifdef PCSX2_DEVBUILD
#	define __emitinline
#else
#	define __emitinline __forceinline
#endif

	// ModRM 'mod' field enumeration.   Provided mostly for reference:
	enum ModRm_ModField
	{
		Mod_NoDisp = 0,		// effective address operation with no displacement, in the form of [reg] (or uses special Disp32-only encoding in the case of [ebp] form)
		Mod_Disp8,			// effective address operation with 8 bit displacement, in the form of [reg+disp8]
		Mod_Disp32,			// effective address operation with 32 bit displacement, in the form of [reg+disp32],
		Mod_Direct,			// direct reg/reg operation
	};

	static const int ModRm_UseSib = 4;		// same index value as ESP (used in RM field)
	static const int ModRm_UseDisp32 = 5;	// same index value as EBP (used in Mod field)

	class iAddressInfo;
	class ModSibBase;

	extern void iSetPtr( void* ptr );
	extern u8* iGetPtr();
	extern void iAlignPtr( uint bytes );
	extern void iAdvancePtr( uint bytes );


	static __forceinline void write8( u8 val )
	{
		iWrite( val );
	}

	static __forceinline void write16( u16 val )
	{ 
		iWrite( val );
	} 

	static __forceinline void write24( u32 val )
	{ 
		*(u32*)x86Ptr = val;
		x86Ptr += 3;
	} 

	static __forceinline void write32( u32 val )
	{ 
		iWrite( val );
	} 

	static __forceinline void write64( u64 val )
	{ 
		iWrite( val );
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// iRegister
	// Unless templating some fancy stuff, use the friendly iRegister32/16/8 typedefs instead.
	//
	template< typename OperandType >
	class iRegister
	{
	public:
		static const uint OperandSize = sizeof( OperandType );
		static const iRegister Empty;		// defined as an empty/unused value (-1)

		int Id;

		iRegister( const iRegister<OperandType>& src ) : Id( src.Id ) {}
		iRegister(): Id( -1 ) {}
		explicit iRegister( int regId ) : Id( regId ) { jASSUME( Id >= -1 && Id < 8 ); }

		bool IsEmpty() const { return Id < 0; }

		// Returns true if the register is a valid accumulator: Eax, Ax, Al.
		bool IsAccumulator() const { return Id == 0; }
		
		// returns true if the register is a valid MMX or XMM register.
		bool IsSIMD() const { return OperandSize == 8 || OperandSize == 16; }

		bool operator==( const iRegister<OperandType>& src ) const
		{
			return (Id == src.Id);
		}

		bool operator!=( const iRegister<OperandType>& src ) const
		{
			return (Id != src.Id);
		}

		iRegister<OperandType>& operator=( const iRegister<OperandType>& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	template< typename OperandType >
	class iRegisterSIMD : public iRegister<OperandType>
	{
	public:
		static const iRegisterSIMD Empty;		// defined as an empty/unused value (-1)

	public:
		iRegisterSIMD(): iRegister<OperandType>() {}
		iRegisterSIMD( const iRegisterSIMD& src ) : iRegister<OperandType>( src.Id ) {}
		iRegisterSIMD( const iRegister<OperandType>& src ) : iRegister<OperandType>( src ) {}
		explicit iRegisterSIMD( int regId ) : iRegister<OperandType>( regId ) {}

		iRegisterSIMD<OperandType>& operator=( const iRegisterSIMD<OperandType>& src )
		{
			iRegister<OperandType>::Id = src.Id;
			return *this;
		}
	};

	
	// ------------------------------------------------------------------------
	// Note: GCC parses templates ahead of time apparently as a 'favor' to the programmer, which
	// means it finds undeclared variables when MSVC does not (Since MSVC compiles templates
	// when they are actually used).  In practice this sucks since it means we have to move all
	// our variable and function prototypes from a nicely/neatly unified location to being strewn
	// all about the the templated code in haphazard fashion.  Yay.. >_<
	//

	typedef iRegisterSIMD<u128> iRegisterSSE;
	typedef iRegisterSIMD<u64>  iRegisterMMX;
	typedef iRegister<u32>  iRegister32;
	typedef iRegister<u16>  iRegister16;
	typedef iRegister<u8>   iRegister8;

	class iRegisterCL : public iRegister8
	{
	public:
		iRegisterCL(): iRegister8( 1 ) {}
	};

	extern const iRegisterSSE
		xmm0, xmm1, xmm2, xmm3,
		xmm4, xmm5, xmm6, xmm7;

	extern const iRegisterMMX
		mm0, mm1, mm2, mm3,
		mm4, mm5, mm6, mm7;

	extern const iRegister32
		eax, ebx, ecx, edx,
		esi, edi, ebp, esp;

	extern const iRegister16
		ax, bx, cx, dx,
		si, di, bp, sp;

	extern const iRegister8
		al, dl, bl,
		ah, ch, dh, bh;

	extern const iRegisterCL cl;		// I'm special!

	//////////////////////////////////////////////////////////////////////////////////////////
	// Use 32 bit registers as out index register (for ModSib memory address calculations)
	// Only iAddressReg provides operators for constructing iAddressInfo types.
	//
	class iAddressReg : public iRegister32
	{
	public:
		static const iAddressReg Empty;		// defined as an empty/unused value (-1)
	
	public:
		iAddressReg(): iRegister32() {}
		iAddressReg( const iAddressReg& src ) : iRegister32( src.Id ) {}
		iAddressReg( const iRegister32& src ) : iRegister32( src ) {}
		explicit iAddressReg( int regId ) : iRegister32( regId ) {}

		// Returns true if the register is the stack pointer: ESP.
		bool IsStackPointer() const { return Id == 4; }

		iAddressInfo operator+( const iAddressReg& right ) const;
		iAddressInfo operator+( const iAddressInfo& right ) const;
		iAddressInfo operator+( s32 right ) const;

		iAddressInfo operator*( u32 factor ) const;
		iAddressInfo operator<<( u32 shift ) const;
		
		iAddressReg& operator=( const iRegister32& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class iAddressInfo
	{
	public:
		iAddressReg Base;		// base register (no scale)
		iAddressReg Index;		// index reg gets multiplied by the scale
		int Factor;				// scale applied to the index register, in factor form (not a shift!)
		s32 Displacement;		// address displacement

	public:
		__forceinline iAddressInfo( const iAddressReg& base, const iAddressReg& index, int factor=1, s32 displacement=0 ) :
			Base( base ),
			Index( index ),
			Factor( factor ),
			Displacement( displacement )
		{
		}

		__forceinline explicit iAddressInfo( const iAddressReg& index, int displacement=0 ) :
			Base(),
			Index( index ),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		__forceinline explicit iAddressInfo( s32 displacement ) :
			Base(),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		static iAddressInfo FromIndexReg( const iAddressReg& index, int scale=0, s32 displacement=0 );

	public:
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline iAddressInfo& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}
		
		__forceinline iAddressInfo& Add( const iAddressReg& src );
		__forceinline iAddressInfo& Add( const iAddressInfo& src );

		__forceinline iAddressInfo operator+( const iAddressReg& right ) const { return iAddressInfo( *this ).Add( right ); }
		__forceinline iAddressInfo operator+( const iAddressInfo& right ) const { return iAddressInfo( *this ).Add( right ); }
		__forceinline iAddressInfo operator+( s32 imm ) const { return iAddressInfo( *this ).Add( imm ); }
		__forceinline iAddressInfo operator-( s32 imm ) const { return iAddressInfo( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib - Internal low-level representation of the ModRM/SIB information.
	//
	// This class serves two purposes:  It houses 'reduced' ModRM/SIB info only, which means
	// that the Base, Index, Scale, and Displacement values are all in the correct arrange-
	// ments, and it serves as a type-safe layer between the iRegister's operators (which
	// generate iAddressInfo types) and the emitter's ModSib instruction forms.  Without this,
	// the iRegister would pass as a ModSib type implicitly, and that would cause ambiguity
	// on a number of instructions.
	//
	// End users should always use iAddressInfo instead.
	//
	class ModSibBase
	{
	public:
		iAddressReg Base;		// base register (no scale)
		iAddressReg Index;		// index reg gets multiplied by the scale
		uint Scale;				// scale applied to the index register, in scale/shift form
		s32 Displacement;		// offset applied to the Base/Index registers.

	public:
		explicit ModSibBase( const iAddressInfo& src );
		explicit ModSibBase( s32 disp );
		ModSibBase( iAddressReg base, iAddressReg index, int scale=0, s32 displacement=0 );
		
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline ModSibBase& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibBase operator+( const s32 imm ) const { return ModSibBase( *this ).Add( imm ); }
		__forceinline ModSibBase operator-( const s32 imm ) const { return ModSibBase( *this ).Add( -imm ); }

	protected:
		void Reduce();
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	// Strictly-typed version of ModSibBase, which is used to apply operand size information
	// to ImmToMem operations.
	//
	template< typename OperandType >
	class ModSibStrict : public ModSibBase
	{
	public:
		static const uint OperandSize = sizeof( OperandType );

		__forceinline explicit ModSibStrict( const iAddressInfo& src ) : ModSibBase( src ) {}
		__forceinline explicit ModSibStrict( s32 disp ) : ModSibBase( disp ) {}
		__forceinline ModSibStrict( iAddressReg base, iAddressReg index, int scale=0, s32 displacement=0 ) :
			ModSibBase( base, index, scale, displacement ) {}
		
		__forceinline ModSibStrict<OperandType>& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibStrict<OperandType> operator+( const s32 imm ) const { return ModSibStrict<OperandType>( *this ).Add( imm ); }
		__forceinline ModSibStrict<OperandType> operator-( const s32 imm ) const { return ModSibStrict<OperandType>( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// iAddressIndexerBase - This is a static class which provisions our ptr[] syntax.
	//
	struct iAddressIndexerBase
	{
		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibBase& operator[]( const ModSibBase& src ) const { return src; }

		__forceinline ModSibBase operator[]( iAddressReg src ) const
		{
			return ModSibBase( src, iAddressReg::Empty );
		}

		__forceinline ModSibBase operator[]( const iAddressInfo& src ) const
		{
			return ModSibBase( src );
		}

		__forceinline ModSibBase operator[]( uptr src ) const
		{
			return ModSibBase( src );
		}

		__forceinline ModSibBase operator[]( const void* src ) const
		{
			return ModSibBase( (uptr)src );
		}
		
		iAddressIndexerBase() {}			// appease the GCC gods
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Explicit version of ptr[], in the form of ptr32[], ptr16[], etc. which allows
	// specification of the operand size for ImmToMem operations.
	//
	template< typename OperandType >
	struct iAddressIndexer
	{
		static const uint OperandSize = sizeof( OperandType );

		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibStrict<OperandType>& operator[]( const ModSibStrict<OperandType>& src ) const { return src; }

		__forceinline ModSibStrict<OperandType> operator[]( iAddressReg src ) const
		{
			return ModSibStrict<OperandType>( src, iAddressReg::Empty );
		}

		__forceinline ModSibStrict<OperandType> operator[]( const iAddressInfo& src ) const
		{
			return ModSibStrict<OperandType>( src );
		}

		__forceinline ModSibStrict<OperandType> operator[]( uptr src ) const
		{
			return ModSibStrict<OperandType>( src );
		}

		__forceinline ModSibStrict<OperandType> operator[]( const void* src ) const
		{
			return ModSibStrict<OperandType>( (uptr)src );
		}
		
		iAddressIndexer() {}  // GCC initialization dummy
	};

	// ptr[] - use this form for instructions which can resolve the address operand size from
	// the other register operand sizes.
	extern const iAddressIndexerBase ptr;
	extern const iAddressIndexer<u128> ptr128;
	extern const iAddressIndexer<u64> ptr64;
	extern const iAddressIndexer<u32> ptr32;	// explicitly typed addressing, usually needed for '[dest],imm' instruction forms
	extern const iAddressIndexer<u16> ptr16;	// explicitly typed addressing, usually needed for '[dest],imm' instruction forms
	extern const iAddressIndexer<u8> ptr8;		// explicitly typed addressing, usually needed for '[dest],imm' instruction forms

	//////////////////////////////////////////////////////////////////////////////////////////
	// JccComparisonType - enumerated possibilities for inspired code branching!
	//
	enum JccComparisonType
	{
		Jcc_Unknown			= -2,
		Jcc_Unconditional	= -1,
		Jcc_Overflow		= 0x0,
		Jcc_NotOverflow		= 0x1,
		Jcc_Below			= 0x2,
		Jcc_Carry			= 0x2,
		Jcc_AboveOrEqual	= 0x3,
		Jcc_NotCarry		= 0x3,
		Jcc_Zero			= 0x4,
		Jcc_Equal			= 0x4,
		Jcc_NotZero			= 0x5,
		Jcc_NotEqual		= 0x5,
		Jcc_BelowOrEqual	= 0x6,
		Jcc_Above			= 0x7,
		Jcc_Signed			= 0x8,
		Jcc_Unsigned		= 0x9,
		Jcc_ParityEven		= 0xa,
		Jcc_ParityOdd		= 0xb,
		Jcc_Less			= 0xc,
		Jcc_GreaterOrEqual	= 0xd,
		Jcc_LessOrEqual		= 0xe,
		Jcc_Greater			= 0xf,
	};
	
	// Not supported yet:
	//E3 cb 	JECXZ rel8 	Jump short if ECX register is 0.


	//////////////////////////////////////////////////////////////////////////////////////////
	// iSmartJump
	// This class provides an interface for generating forward-based j8's or j32's "smartly"
	// as per the measured displacement distance.  If the displacement is a valid s8, then
	// a j8 is inserted, else a j32.
	// 
	// Note: This class is inherently unsafe, and so it's recommended to use iForwardJump8/32
	// whenever it is known that the jump destination is (or is not) short.  Only use
	// iSmartJump in cases where it's unknown what jump encoding will be ideal.
	//
	// Important: Use this tool with caution!  iSmartJump cannot be used in cases where jump
	// targets overlap, since the writeback of the second target will alter the position of
	// the first target (which breaks the relative addressing).  To assist in avoiding such
	// errors, iSmartJump works based on C++ block scope, where the destruction of the
	// iSmartJump object (invoked by a '}') signals the target of the jump.  Example:
	//
	// {
	//     iCMP( EAX, ECX );
	//     iSmartJump jumpTo( Jcc_Above );
	//     [... conditional code ...]
	// }  // smartjump targets this spot.
	//
	// No code inside the scope can attempt to jump outside the scoped block (unless the jump
	// uses an immediate addressing method, such as Register or Mod/RM forms of JMP/CALL).
	// Multiple SmartJumps can be safely nested inside scopes, as long as they are properly
	// scoped themselves.
	//
	// Performance Analysis:  j8's use 4 less byes per opcode, and thus can provide minor
	// speed benefits in the form of L1/L2 cache clutter, on any CPU.  They're also notably
	// faster on P4's, and mildly faster on AMDs.  (Core2's and i7's don't care)
	//
	class iSmartJump : public NoncopyableObject
	{
	protected:
		u8* m_baseptr;				// base address of the instruction (passed to the instruction emitter)
		JccComparisonType m_cc;		// comparison type of the instruction

	public:
		const int GetMaxInstructionSize() const
		{
			jASSUME( m_cc != Jcc_Unknown );
			return ( m_cc == Jcc_Unconditional ) ? 5 : 6;
		}

		JccComparisonType GetCondition() const	{ return m_cc; }
		virtual ~iSmartJump();

		// ------------------------------------------------------------------------
		// ccType - Comparison type to be written back to the jump instruction position.
		//
		iSmartJump( JccComparisonType ccType )
		{
			jASSUME( ccType != Jcc_Unknown );
			m_baseptr = iGetPtr();
			m_cc = ccType;
			iAdvancePtr( GetMaxInstructionSize() );
		}
		
	protected:
		void SetTarget();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// iForwardJump
	// Primary use of this class is through the various iForwardJA8/iForwardJLE32/etc. helpers
	// defined later in this header. :)
	//
	template< typename OperandType >
	class iForwardJump
	{
	public:
		static const uint OperandSize = sizeof( OperandType );

		// pointer to base of the instruction *Following* the jump.  The jump address will be
		// relative to this address.
		s8* const BasePtr;

		// The jump instruction is emitted at the point of object construction.  The conditional
		// type must be valid (Jcc_Unknown generates an assertion).
		iForwardJump( JccComparisonType cctype = Jcc_Unconditional );
		
		// Sets the jump target by writing back the current x86Ptr to the jump instruction.
		// This method can be called multiple times, re-writing the jump instruction's target
		// in each case. (the the last call is the one that takes effect).
		void SetTarget() const;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//	
	namespace Internal
	{
		extern void ModRM( uint mod, uint reg, uint rm );
		extern void ModRM_Direct( uint reg, uint rm );
		extern void SibSB( u32 ss, u32 index, u32 base );
		extern void iWriteDisp( int regfield, s32 displacement );
		extern void iWriteDisp( int regfield, const void* address );

		extern void EmitSibMagic( uint regfield, const ModSibBase& info );

		// ------------------------------------------------------------------------
		#include "implement/group1.h"
		#include "implement/group2.h"
		#include "implement/group3.h"
		#include "implement/movs.h"		// cmov and movsx/zx
		#include "implement/dwshift.h"	// doubleword shifts!
		#include "implement/incdec.h"
		#include "implement/bittest.h"
		#include "implement/test.h"
		#include "implement/jmpcall.h"
		#include "implement/xmm/movqss.h"
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// ALWAYS_USE_MOVAPS [define] / AlwaysUseMovaps [const]
	//
	// This tells the recompiler's emitter to always use movaps instead of movdqa.  Both instructions
	// do the exact same thing, but movaps is 1 byte shorter, and thus results in a cleaner L1 cache
	// and some marginal speed gains as a result.  (it's possible someday in the future the per-
	// formance of the two instructions could change, so this constant is provided to restore MOVDQA
	// use easily at a later time, if needed).
	#define ALWAYS_USE_MOVAPS
	
	#ifdef ALWAYS_USE_MOVAPS
	static const bool AlwaysUseMovaps = true;
	#else
	static const bool AlwaysUseMovaps = false;
	#endif

	extern const Internal::MovapsImplAll<0, 0x28, 0x29> iMOVAPS;
	extern const Internal::MovapsImplAll<0, 0x10, 0x11> iMOVUPS;

	extern const Internal::MovapsImplAll<0x66, 0x28, 0x29> iMOVAPD;
	extern const Internal::MovapsImplAll<0x66, 0x10, 0x11> iMOVUPD;

	#ifdef ALWAYS_USE_MOVAPS
	extern const Internal::MovapsImplAll<0x66, 0x6f, 0x7f> iMOVDQA;
	extern const Internal::MovapsImplAll<0xf3, 0x6f, 0x7f> iMOVDQU;
	#else
	extern const Internal::MovapsImplAll<0, 0x28, 0x29> iMOVDQA;
	extern const Internal::MovapsImplAll<0, 0x10, 0x11> iMOVDQU;
	#endif
		
	extern const Internal::MovhlImplAll<0, 0x16> iMOVHPS;
	extern const Internal::MovhlImplAll<0, 0x12> iMOVLPS;
	extern const Internal::MovhlImplAll<0x66, 0x16> iMOVHPD;
	extern const Internal::MovhlImplAll<0x66, 0x12> iMOVLPD;

	extern const Internal::PLogicImplAll<0xdb> iPAND;
	extern const Internal::PLogicImplAll<0xdf> iPANDN;
	extern const Internal::PLogicImplAll<0xeb> iPOR;
	extern const Internal::PLogicImplAll<0xef> iPXOR;
	
	extern const Internal::PLogicImplSSE<0x00,0x54> iANDPS;
	extern const Internal::PLogicImplSSE<0x66,0x54> iANDPD;
	extern const Internal::PLogicImplSSE<0x00,0x55> iANDNPS;
	extern const Internal::PLogicImplSSE<0x66,0x55> iANDNPD;
	extern const Internal::PLogicImplSSE<0x00,0x56> iORPS;
	extern const Internal::PLogicImplSSE<0x66,0x56> iORPD;
	extern const Internal::PLogicImplSSE<0x00,0x57> iXORPS;
	extern const Internal::PLogicImplSSE<0x66,0x57> iXORPD;

	extern const Internal::PLogicImplSSE<0x00,0x5c> iSUBPS;
	extern const Internal::PLogicImplSSE<0x66,0x5c> iSUBPD;
	extern const Internal::PLogicImplSSE<0xf3,0x5c> iSUBSS;
	extern const Internal::PLogicImplSSE<0xf2,0x5c> iSUBSD;

	extern const Internal::PLogicImplSSE<0x00,0x58> iADDPS;
	extern const Internal::PLogicImplSSE<0x66,0x58> iADDPD;
	extern const Internal::PLogicImplSSE<0xf3,0x58> iADDSS;
	extern const Internal::PLogicImplSSE<0xf2,0x58> iADDSD;

	extern const Internal::PLogicImplSSE<0x00,0x59> iMULPS;
	extern const Internal::PLogicImplSSE<0x66,0x59> iMULPD;
	extern const Internal::PLogicImplSSE<0xf3,0x59> iMULSS;
	extern const Internal::PLogicImplSSE<0xf2,0x59> iMULSD;

	extern const Internal::PLogicImplSSE<0x00,0x5e> iDIVPS;
	extern const Internal::PLogicImplSSE<0x66,0x5e> iDIVPD;
	extern const Internal::PLogicImplSSE<0xf3,0x5e> iDIVSS;
	extern const Internal::PLogicImplSSE<0xf2,0x5e> iDIVSD;



	extern const Internal::PLogicImplSSE<0,0x53> iRCPPS;
	extern const Internal::PLogicImplSSE<0xf3,0x53> iRCPSS;
	
}

#include "ix86_inlines.inl"
