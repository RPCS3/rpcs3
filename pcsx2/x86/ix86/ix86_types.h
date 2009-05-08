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
struct CAPABILITIES
{
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
   u32 hasStreamingSIMD4Extensions2;

   // AMD-specific CPU Features
   u32 hasMultimediaExtensionsExt;
   u32 hasAMD64BitArchitecture;
   u32 has3DNOWInstructionExtensionsExt;
   u32 has3DNOWInstructionExtensions;
   u32 hasStreamingSIMD4ExtensionsA;
};

extern CAPABILITIES cpucaps;

struct CPUINFO
{
   u32 x86Family;	   // Processor Family
   u32 x86Model;	   // Processor Model
   u32 x86PType;	   // Processor Type
   u32 x86StepID;	   // Stepping ID
   u32 x86Flags;	   // Feature Flags
   u32 x86Flags2;	   // More Feature Flags
   u32 x86EFlags;	   // Extended Feature Flags
   u32 x86EFlags2;	   // Extended Feature Flags pg2

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

namespace x86Emitter
{

extern void xWrite8( u8 val );
extern void xWrite16( u16 val );
extern void xWrite32( u32 val );
extern void xWrite64( u64 val );

//------------------------------------------------------------------
// templated version of is_s8 is required, so that u16's get correct sign extension treatment.
template< typename T >
static __forceinline bool is_s8( T imm ) { return (s8)imm == (s32)imm; }

template< typename T >
__forceinline void xWrite( T val )
{
	*(T*)x86Ptr = val;
	x86Ptr += sizeof(T); 
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

	class xAddressInfo;
	class ModSibBase;

	extern void xSetPtr( void* ptr );
	extern u8* xGetPtr();
	extern void xAlignPtr( uint bytes );
	extern void xAdvancePtr( uint bytes );

	//////////////////////////////////////////////////////////////////////////////////////////
	// xRegisterBase
	// Unless templating some fancy stuff, use the friendly xRegister32/16/8 typedefs instead.
	//
	template< typename OperandType >
	class xRegisterBase
	{
	public:
		static const uint OperandSize = sizeof( OperandType );
		static const xRegisterBase Empty;		// defined as an empty/unused value (-1)

		int Id;

		xRegisterBase( const xRegisterBase<OperandType>& src ) : Id( src.Id ) {}
		xRegisterBase(): Id( -1 ) {}
		explicit xRegisterBase( int regId ) : Id( regId ) { jASSUME( Id >= -1 && Id < 8 ); }

		bool IsEmpty() const { return Id < 0; }

		// Returns true if the register is a valid accumulator: Eax, Ax, Al.
		bool IsAccumulator() const { return Id == 0; }
		
		// returns true if the register is a valid MMX or XMM register.
		bool IsSIMD() const { return OperandSize == 8 || OperandSize == 16; }

		bool operator==( const xRegisterBase<OperandType>& src ) const	{ return (Id == src.Id); }
		bool operator!=( const xRegisterBase<OperandType>& src ) const	{ return (Id != src.Id); }

		xRegisterBase<OperandType>& operator=( const xRegisterBase<OperandType>& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	template< typename OperandType >
	class xRegister : public xRegisterBase<OperandType>
	{
	public:
		static const xRegister Empty;		// defined as an empty/unused value (-1)

	public:
		xRegister(): xRegisterBase<OperandType>() {}
		xRegister( const xRegister& src ) : xRegisterBase<OperandType>( src.Id ) {}
		explicit xRegister( const xRegisterBase<OperandType>& src ) : xRegisterBase<OperandType>( src ) {}
		explicit xRegister( int regId ) : xRegisterBase<OperandType>( regId ) {}
		
		bool operator==( const xRegister<OperandType>& src ) const	{ return this->Id == src.Id; }
		bool operator!=( const xRegister<OperandType>& src ) const	{ return this->Id != src.Id; }

		xRegister<OperandType>& operator=( const xRegister<OperandType>& src )
		{
			this->Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	template< typename OperandType >
	class xRegisterSIMD : public xRegisterBase<OperandType>
	{
	public:
		static const xRegisterSIMD Empty;		// defined as an empty/unused value (-1)

	public:
		xRegisterSIMD(): xRegisterBase<OperandType>() {}
		xRegisterSIMD( const xRegisterSIMD& src ) : xRegisterBase<OperandType>( src.Id ) {}
		explicit xRegisterSIMD( const xRegisterBase<OperandType>& src ) : xRegisterBase<OperandType>( src ) {}
		explicit xRegisterSIMD( int regId ) : xRegisterBase<OperandType>( regId ) {}
		
		bool operator==( const xRegisterSIMD<OperandType>& src ) const	{ return this->Id == src.Id; }
		bool operator!=( const xRegisterSIMD<OperandType>& src ) const	{ return this->Id != src.Id; }

		xRegisterSIMD<OperandType>& operator=( const xRegisterSIMD<OperandType>& src )
		{
			this->Id = src.Id;
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

	typedef xRegisterSIMD<u128> xRegisterSSE;
	typedef xRegisterSIMD<u64>  xRegisterMMX;
	typedef xRegister<u32>  xRegister32;
	typedef xRegister<u16>  xRegister16;
	typedef xRegister<u8>   xRegister8;

	class xRegisterCL : public xRegister8
	{
	public:
		xRegisterCL(): xRegister8( 1 ) {}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Use 32 bit registers as out index register (for ModSib memory address calculations)
	// Only xAddressReg provides operators for constructing xAddressInfo types.
	//
	class xAddressReg : public xRegister32
	{
	public:
		static const xAddressReg Empty;		// defined as an empty/unused value (-1)
	
	public:
		xAddressReg(): xRegister32() {}
		xAddressReg( const xAddressReg& src ) : xRegister32( src.Id ) {}
		xAddressReg( const xRegister32& src ) : xRegister32( src ) {}
		explicit xAddressReg( int regId ) : xRegister32( regId ) {}

		// Returns true if the register is the stack pointer: ESP.
		bool IsStackPointer() const { return Id == 4; }

		xAddressInfo operator+( const xAddressReg& right ) const;
		xAddressInfo operator+( const xAddressInfo& right ) const;
		xAddressInfo operator+( s32 right ) const;
		xAddressInfo operator+( const void* right ) const;

		xAddressInfo operator*( u32 factor ) const;
		xAddressInfo operator<<( u32 shift ) const;
		
		xAddressReg& operator=( const xRegister32& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class xAddressInfo
	{
	public:
		xAddressReg Base;		// base register (no scale)
		xAddressReg Index;		// index reg gets multiplied by the scale
		int Factor;				// scale applied to the index register, in factor form (not a shift!)
		s32 Displacement;		// address displacement

	public:
		__forceinline xAddressInfo( const xAddressReg& base, const xAddressReg& index, int factor=1, s32 displacement=0 ) :
			Base( base ),
			Index( index ),
			Factor( factor ),
			Displacement( displacement )
		{
		}

		__forceinline explicit xAddressInfo( const xAddressReg& index, int displacement=0 ) :
			Base(),
			Index( index ),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		__forceinline explicit xAddressInfo( s32 displacement ) :
			Base(),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		static xAddressInfo FromIndexReg( const xAddressReg& index, int scale=0, s32 displacement=0 );

	public:
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline xAddressInfo& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}
		
		__forceinline xAddressInfo& Add( const xAddressReg& src );
		__forceinline xAddressInfo& Add( const xAddressInfo& src );

		__forceinline xAddressInfo operator+( const xAddressReg& right ) const { return xAddressInfo( *this ).Add( right ); }
		__forceinline xAddressInfo operator+( const xAddressInfo& right ) const { return xAddressInfo( *this ).Add( right ); }
		__forceinline xAddressInfo operator+( s32 imm ) const { return xAddressInfo( *this ).Add( imm ); }
		__forceinline xAddressInfo operator-( s32 imm ) const { return xAddressInfo( *this ).Add( -imm ); }
		__forceinline xAddressInfo operator+( const void* addr ) const { return xAddressInfo( *this ).Add( (uptr)addr ); }
	};

	extern const xRegisterSSE
		xmm0, xmm1, xmm2, xmm3,
		xmm4, xmm5, xmm6, xmm7;

	extern const xRegisterMMX
		mm0, mm1, mm2, mm3,
		mm4, mm5, mm6, mm7;

	extern const xAddressReg
		eax, ebx, ecx, edx,
		esi, edi, ebp, esp;

	extern const xRegister16
		ax, bx, cx, dx,
		si, di, bp, sp;

	extern const xRegister8
		al, dl, bl,
		ah, ch, dh, bh;

	extern const xRegisterCL cl;		// I'm special!

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib - Internal low-level representation of the ModRM/SIB information.
	//
	// This class serves two purposes:  It houses 'reduced' ModRM/SIB info only, which means
	// that the Base, Index, Scale, and Displacement values are all in the correct arrange-
	// ments, and it serves as a type-safe layer between the xRegister's operators (which
	// generate xAddressInfo types) and the emitter's ModSib instruction forms.  Without this,
	// the xRegister would pass as a ModSib type implicitly, and that would cause ambiguity
	// on a number of instructions.
	//
	// End users should always use xAddressInfo instead.
	//
	class ModSibBase
	{
	public:
		xAddressReg Base;		// base register (no scale)
		xAddressReg Index;		// index reg gets multiplied by the scale
		uint Scale;				// scale applied to the index register, in scale/shift form
		s32 Displacement;		// offset applied to the Base/Index registers.

	public:
		explicit ModSibBase( const xAddressInfo& src );
		explicit ModSibBase( s32 disp );
		ModSibBase( xAddressReg base, xAddressReg index, int scale=0, s32 displacement=0 );
		
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

		__forceinline explicit ModSibStrict( const xAddressInfo& src ) : ModSibBase( src ) {}
		__forceinline explicit ModSibStrict( s32 disp ) : ModSibBase( disp ) {}
		__forceinline ModSibStrict( xAddressReg base, xAddressReg index, int scale=0, s32 displacement=0 ) :
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
	// xAddressIndexerBase - This is a static class which provisions our ptr[] syntax.
	//
	struct xAddressIndexerBase
	{
		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibBase& operator[]( const ModSibBase& src ) const { return src; }

		__forceinline ModSibBase operator[]( xAddressReg src ) const
		{
			return ModSibBase( src, xAddressReg::Empty );
		}

		__forceinline ModSibBase operator[]( const xAddressInfo& src ) const
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
		
		xAddressIndexerBase() {}			// appease the GCC gods
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Explicit version of ptr[], in the form of ptr32[], ptr16[], etc. which allows
	// specification of the operand size for ImmToMem operations.
	//
	template< typename OperandType >
	struct xAddressIndexer
	{
		static const uint OperandSize = sizeof( OperandType );

		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibStrict<OperandType>& operator[]( const ModSibStrict<OperandType>& src ) const { return src; }

		__forceinline ModSibStrict<OperandType> operator[]( xAddressReg src ) const
		{
			return ModSibStrict<OperandType>( src, xAddressReg::Empty );
		}

		__forceinline ModSibStrict<OperandType> operator[]( const xAddressInfo& src ) const
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
		
		xAddressIndexer() {}  // GCC initialization dummy
	};

	// ptr[] - use this form for instructions which can resolve the address operand size from
	// the other register operand sizes.
	extern const xAddressIndexerBase ptr;
	extern const xAddressIndexer<u128> ptr128;
	extern const xAddressIndexer<u64> ptr64;
	extern const xAddressIndexer<u32> ptr32;	// explicitly typed addressing, usually needed for '[dest],imm' instruction forms
	extern const xAddressIndexer<u16> ptr16;	// explicitly typed addressing, usually needed for '[dest],imm' instruction forms
	extern const xAddressIndexer<u8> ptr8;		// explicitly typed addressing, usually needed for '[dest],imm' instruction forms

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
	// SSE2_ComparisonType - enumerated possibilities for SIMD data comparison!
	//
	enum SSE2_ComparisonType
	{
		SSE2_Equal = 0,
		SSE2_Less,
		SSE2_LessOrEqual,
		SSE2_Unordered,
		SSE2_NotEqual,
		SSE2_NotLess,
		SSE2_NotLessOrEqual,
		SSE2_Ordered
	};


	//////////////////////////////////////////////////////////////////////////////////////////
	// xSmartJump
	// This class provides an interface for generating forward-based j8's or j32's "smartly"
	// as per the measured displacement distance.  If the displacement is a valid s8, then
	// a j8 is inserted, else a j32.
	// 
	// Note: This class is inherently unsafe, and so it's recommended to use xForwardJump8/32
	// whenever it is known that the jump destination is (or is not) short.  Only use
	// xSmartJump in cases where it's unknown what jump encoding will be ideal.
	//
	// Important: Use this tool with caution!  xSmartJump cannot be used in cases where jump
	// targets overlap, since the writeback of the second target will alter the position of
	// the first target (which breaks the relative addressing).  To assist in avoiding such
	// errors, xSmartJump works based on C++ block scope, where the destruction of the
	// xSmartJump object (invoked by a '}') signals the target of the jump.  Example:
	//
	// {
	//     iCMP( EAX, ECX );
	//     xSmartJump jumpTo( Jcc_Above );
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
	class xSmartJump : public NoncopyableObject
	{
	protected:
		u8* m_baseptr;				// base address of the instruction (passed to the instruction emitter)
		JccComparisonType m_cc;		// comparison type of the instruction

	public:
		int GetMaxInstructionSize() const
		{
			jASSUME( m_cc != Jcc_Unknown );
			return ( m_cc == Jcc_Unconditional ) ? 5 : 6;
		}

		JccComparisonType GetCondition() const	{ return m_cc; }
		virtual ~xSmartJump();

		// ------------------------------------------------------------------------
		// ccType - Comparison type to be written back to the jump instruction position.
		//
		xSmartJump( JccComparisonType ccType )
		{
			jASSUME( ccType != Jcc_Unknown );
			m_baseptr = xGetPtr();
			m_cc = ccType;
			xAdvancePtr( GetMaxInstructionSize() );
		}
		
	protected:
		void SetTarget();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// xForwardJump
	// Primary use of this class is through the various xForwardJA8/xForwardJLE32/etc. helpers
	// defined later in this header. :)
	//
	template< typename OperandType >
	class xForwardJump
	{
	public:
		static const uint OperandSize = sizeof( OperandType );

		// pointer to base of the instruction *Following* the jump.  The jump address will be
		// relative to this address.
		s8* const BasePtr;

		// The jump instruction is emitted at the point of object construction.  The conditional
		// type must be valid (Jcc_Unknown generates an assertion).
		xForwardJump( JccComparisonType cctype = Jcc_Unconditional );
		
		// Sets the jump target by writing back the current x86Ptr to the jump instruction.
		// This method can be called multiple times, re-writing the jump instruction's target
		// in each case. (the the last call is the one that takes effect).
		void SetTarget() const;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//	
	namespace Internal
	{
		extern void SimdPrefix( u8 prefix, u16 opcode );
		extern void EmitSibMagic( uint regfield, const void* address );
		extern void EmitSibMagic( uint regfield, const ModSibBase& info );
		extern void xJccKnownTarget( JccComparisonType comparison, const void* target, bool slideForward );


		// Writes a ModRM byte for "Direct" register access forms, which is used for all
		// instructions taking a form of [reg,reg].
		template< typename T > __emitinline
		void EmitSibMagic( uint reg1, const xRegisterBase<T>& reg2 )
		{
			xWrite8( (Mod_Direct << 6) | (reg1 << 3) | reg2.Id );
		}

		template< typename T1, typename T2 > __emitinline
		void EmitSibMagic( const xRegisterBase<T1> reg1, const xRegisterBase<T2>& reg2 )
		{
			xWrite8( (Mod_Direct << 6) | (reg1.Id << 3) | reg2.Id );
		}

		template< typename T1 > __emitinline
		void EmitSibMagic( const xRegisterBase<T1> reg1, const void* src )			{ EmitSibMagic( reg1.Id, src ); }

		template< typename T1 > __emitinline
		void EmitSibMagic( const xRegisterBase<T1> reg1, const ModSibBase& sib )	{ EmitSibMagic( reg1.Id, sib ); }

		// ------------------------------------------------------------------------
		template< typename T1, typename T2 > __emitinline
		void xOpWrite( u8 prefix, u8 opcode, const T1& param1, const T2& param2 )
		{
			if( prefix != 0 )
				xWrite16( (opcode<<8) | prefix );
			else
				xWrite8( opcode );

			EmitSibMagic( param1, param2 );
		}

		// ------------------------------------------------------------------------
		template< typename T1, typename T2 > __emitinline
		void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2 )
		{
			SimdPrefix( prefix, opcode );
			EmitSibMagic( param1, param2 );
		}

		template< typename T1, typename T2 > __emitinline
		void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2, u8 imm8 )
		{
			xOpWrite0F( prefix, opcode, param1, param2 );
			xWrite8( imm8 );
		}

		template< typename T1, typename T2 > __emitinline
		void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2 )			{ xOpWrite0F( 0, opcode, param1, param2 ); }

		template< typename T1, typename T2 > __emitinline
		void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2, u8 imm8 )	{ xOpWrite0F( 0, opcode, param1, param2, imm8 ); }

		// ------------------------------------------------------------------------

		template< typename T > bool Is8BitOp() { return sizeof(T) == 1; }
		template< typename T > void prefix16() { if( sizeof(T) == 2 ) xWrite8( 0x66 ); }

		#include "implement/xmm/basehelpers.h"
		#include "implement/xmm/moremovs.h"
		#include "implement/xmm/arithmetic.h"
		#include "implement/xmm/comparisons.h"
		#include "implement/xmm/shufflepack.h"
		#include "implement/group1.h"
		#include "implement/group2.h"
		#include "implement/group3.h"
		#include "implement/movs.h"		// cmov and movsx/zx
		#include "implement/dwshift.h"	// doubleword shifts!
		#include "implement/incdec.h"
		#include "implement/test.h"
		#include "implement/jmpcall.h"
	}
}

#include "ix86_inlines.inl"
