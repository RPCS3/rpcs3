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
// debugging).
//
// Note: I use __forceinline directly for most single-line class members, when needed.
// There's no point in using __emitline in these cases since the debugger can't trace into
// single-line functions anyway.
//
#ifdef PCSX2_DEVBUILD
#	define __emitinline
#else
#	define __emitinline __forceinline
#endif

#ifdef _MSC_VER
#	define __noinline __declspec(noinline) 
#else
#	define __noinline
#endif

	static const int ModRm_Direct = 3;		// when used as the first parameter, specifies direct register operation (no mem)
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
	//
	template< int OperandSize >
	class iRegister
	{
	public:
		static const iRegister Empty;		// defined as an empty/unused value (-1)

		int Id;

		iRegister( const iRegister<OperandSize>& src ) : Id( src.Id ) {}
		iRegister(): Id( -1 ) {}
		explicit iRegister( int regId ) : Id( regId ) { jASSUME( Id >= -1 && Id < 8 ); }

		bool IsEmpty() const { return Id < 0; }

		// Returns true if the register is a valid accumulator: Eax, Ax, Al.
		bool IsAccumulator() const { return Id == 0; }

		bool operator==( const iRegister<OperandSize>& src ) const
		{
			return (Id == src.Id);
		}

		bool operator!=( const iRegister<OperandSize>& src ) const
		{
			return (Id != src.Id);
		}

		iRegister<OperandSize>& operator=( const iRegister<OperandSize>& src )
		{
			Id = src.Id;
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

	typedef iRegister<4> iRegister32;
	typedef iRegister<2> iRegister16;
	typedef iRegister<1> iRegister8;

	class iRegisterCL : public iRegister8
	{
	public:
		iRegisterCL(): iRegister8( 1 ) {}
	};

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
	// Only x86IndexReg provides operators for constructing iAddressInfo types.
	class x86IndexReg : public iRegister32
	{
	public:
		static const x86IndexReg Empty;		// defined as an empty/unused value (-1)
	
	public:
		x86IndexReg(): iRegister32() {}
		x86IndexReg( const x86IndexReg& src ) : iRegister32( src.Id ) {}
		x86IndexReg( const iRegister32& src ) : iRegister32( src ) {}
		explicit x86IndexReg( int regId ) : iRegister32( regId ) {}

		// Returns true if the register is the stack pointer: ESP.
		bool IsStackPointer() const { return Id == 4; }

		iAddressInfo operator+( const x86IndexReg& right ) const;
		iAddressInfo operator+( const iAddressInfo& right ) const;
		iAddressInfo operator+( s32 right ) const;

		iAddressInfo operator*( u32 factor ) const;
		iAddressInfo operator<<( u32 shift ) const;
		
		x86IndexReg& operator=( const iRegister32& src )
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
		x86IndexReg Base;		// base register (no scale)
		x86IndexReg Index;		// index reg gets multiplied by the scale
		int Factor;				// scale applied to the index register, in factor form (not a shift!)
		s32 Displacement;		// address displacement

	public:
		__forceinline iAddressInfo( const x86IndexReg& base, const x86IndexReg& index, int factor=1, s32 displacement=0 ) :
			Base( base ),
			Index( index ),
			Factor( factor ),
			Displacement( displacement )
		{
		}

		__forceinline explicit iAddressInfo( const x86IndexReg& base, int displacement=0 ) :
			Base( base ),
			Index(),
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
		
		static iAddressInfo FromIndexReg( const x86IndexReg& index, int scale=0, s32 displacement=0 );

	public:
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline iAddressInfo& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}
		
		__forceinline iAddressInfo& Add( const x86IndexReg& src );
		__forceinline iAddressInfo& Add( const iAddressInfo& src );

		__forceinline iAddressInfo operator+( const x86IndexReg& right ) const { return iAddressInfo( *this ).Add( right ); }
		__forceinline iAddressInfo operator+( const iAddressInfo& right ) const { return iAddressInfo( *this ).Add( right ); }
		__forceinline iAddressInfo operator+( s32 imm ) const { return iAddressInfo( *this ).Add( imm ); }
		__forceinline iAddressInfo operator-( s32 imm ) const { return iAddressInfo( *this ).Add( -imm ); }
	};

	enum OperandSizeType
	{
		OpSize_8 = 1,
		OpSize_16 = 2,
		OpSize_32 = 4,
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
		x86IndexReg Base;		// base register (no scale)
		x86IndexReg Index;		// index reg gets multiplied by the scale
		uint Scale;				// scale applied to the index register, in scale/shift form
		s32 Displacement;		// offset applied to the Base/Index registers.

	public:
		explicit ModSibBase( const iAddressInfo& src );
		explicit ModSibBase( s32 disp );
		ModSibBase( x86IndexReg base, x86IndexReg index, int scale=0, s32 displacement=0 );
		
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline ModSibBase& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibBase operator+( const s32 imm ) const { return ModSibBase( *this ).Add( imm ); }
		__forceinline ModSibBase operator-( const s32 imm ) const { return ModSibBase( *this ).Add( -imm ); }

	protected:
		__forceinline void Reduce();
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class ModSibSized : public ModSibBase
	{
	public:
		int OperandSize;
		
		ModSibSized( int opsize, const iAddressInfo& src ) :
			ModSibBase( src ),
			OperandSize( opsize )
		{
			jASSUME( OperandSize == 1 || OperandSize == 2 || OperandSize == 4 );
		}

		ModSibSized( int opsize, s32 disp ) :
			ModSibBase( disp ),
			OperandSize( opsize )
		{
			jASSUME( OperandSize == 1 || OperandSize == 2 || OperandSize == 4 );
		}
		
		ModSibSized( int opsize, x86IndexReg base, x86IndexReg index, int scale=0, s32 displacement=0 ) :
			ModSibBase( base, index, scale, displacement ),
			OperandSize( opsize )
		{
			jASSUME( OperandSize == 1 || OperandSize == 2 || OperandSize == 4 );
		}
		
		__forceinline ModSibSized& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibSized operator+( const s32 imm ) const { return ModSibSized( *this ).Add( imm ); }
		__forceinline ModSibSized operator-( const s32 imm ) const { return ModSibSized( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Strictly-typed version of ModSibBase, which is used to apply operand size information
	// to ImmToMem operations.
	//
	template< int OpSize >
	class ModSibStrict : public ModSibSized
	{
	public:
		__forceinline explicit ModSibStrict( const iAddressInfo& src ) : ModSibSized( OpSize, src ) {}
		__forceinline explicit ModSibStrict( s32 disp ) : ModSibSized( OpSize, disp ) {}
		__forceinline ModSibStrict( x86IndexReg base, x86IndexReg index, int scale=0, s32 displacement=0 ) :
			ModSibSized( OpSize, base, index, scale, displacement ) {}
		
		__forceinline ModSibStrict<OpSize>& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibStrict<OpSize> operator+( const s32 imm ) const { return ModSibStrict<OpSize>( *this ).Add( imm ); }
		__forceinline ModSibStrict<OpSize> operator-( const s32 imm ) const { return ModSibStrict<OpSize>( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// x86IndexerType - This is a static class which provisions our ptr[] syntax.
	//
	struct x86IndexerType
	{
		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibBase& operator[]( const ModSibBase& src ) const { return src; }

		__forceinline ModSibBase operator[]( x86IndexReg src ) const
		{
			return ModSibBase( src, x86IndexReg::Empty );
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
		
		x86IndexerType() {}			// applease the GCC gods
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Explicit version of ptr[], in the form of ptr32[], ptr16[], etc. which allows
	// specification of the operand size for ImmToMem operations.
	//
	template< int OperandSize >
	struct x86IndexerTypeExplicit
	{
		// passthrough instruction, allows ModSib to pass silently through ptr translation
		// without doing anything and without compiler error.
		const ModSibStrict<OperandSize>& operator[]( const ModSibStrict<OperandSize>& src ) const { return src; }

		__forceinline ModSibStrict<OperandSize> operator[]( x86IndexReg src ) const
		{
			return ModSibStrict<OperandSize>( src, x86IndexReg::Empty );
		}

		__forceinline ModSibStrict<OperandSize> operator[]( const iAddressInfo& src ) const
		{
			return ModSibStrict<OperandSize>( src );
		}

		__forceinline ModSibStrict<OperandSize> operator[]( uptr src ) const
		{
			return ModSibStrict<OperandSize>( src );
		}

		__forceinline ModSibStrict<OperandSize> operator[]( const void* src ) const
		{
			return ModSibStrict<OperandSize>( (uptr)src );
		}
		
		x86IndexerTypeExplicit() {}  // GCC initialization dummy
	};

	extern const x86IndexerType ptr;
	extern const x86IndexerTypeExplicit<4> ptr32;
	extern const x86IndexerTypeExplicit<2> ptr16;
	extern const x86IndexerTypeExplicit<1> ptr8;	

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
	// Performance Analysis:  j8's use 4 less byes per opcode, and thus can provide
	// minor speed benefits in the form of L1/L2 cache clutter.  They're also notably faster
	// on P4's, and mildly faster on AMDs.  (Core2's and i7's don't care)
	//
	class iSmartJump
	{
	protected:
		u8* m_target;				// x86Ptr target address of this label
		u8* m_baseptr;				// base address of the instruction (passed to the instruction emitter)
		JccComparisonType m_cc;		// comparison type of the instruction
		bool m_written;				// set true when the jump is written (at which point the object becomes invalid)

	public:

		const int GetMaxInstructionSize() const
		{
			jASSUME( m_cc != Jcc_Unknown );
			return ( m_cc == Jcc_Unconditional ) ? 5 : 6;
		}

		// Creates a backward jump label which will be passed into a Jxx instruction (or few!)
		// later on, and the current x86Ptr is recorded as the target [thus making the class
		// creation point the jump target].
		iSmartJump()
		{
			m_target = iGetPtr();
			m_baseptr = NULL;
			m_cc = Jcc_Unknown;
			m_written = false;
		}

		// ccType - Comparison type to be written back to the jump instruction position.
		//
		iSmartJump( JccComparisonType ccType )
		{
			jASSUME( ccType != Jcc_Unknown );
			m_target = NULL;
			m_baseptr = iGetPtr();
			m_cc = ccType;
			m_written = false;
			iAdvancePtr( GetMaxInstructionSize() );
		}

		JccComparisonType GetCondition() const
		{
			return m_cc;
		}

		u8* GetTarget() const
		{
			return m_target;
		}

		void SetTarget();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// 
	template< typename OperandType >
	class iForwardJump
	{
	public:
		static const uint OperandSize = sizeof( OperandType );

		// pointer to base of the instruction *Following* the jump.  The jump address will be
		// relative to this address.
		s8* const BasePtr;

	public:	
		iForwardJump( JccComparisonType cctype = Jcc_Unconditional );
		void SetTarget() const;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//	
	namespace Internal
	{
		extern void ModRM( uint mod, uint reg, uint rm );
		extern void SibSB( u32 ss, u32 index, u32 base );
		extern void iWriteDisp( int regfield, s32 displacement );
		extern void iWriteDisp( int regfield, const void* address );

		extern void EmitSibMagic( uint regfield, const ModSibBase& info );

		#include "ix86_impl_group1.h"
		#include "ix86_impl_group2.h"
		#include "ix86_impl_movs.h"		// cmov and movsx/zx
		#include "ix86_impl_dwshift.h"	// dowubleword shifts!

		// if the immediate is zero, we can replace the instruction, or ignore it
		// entirely, depending on the instruction being issued.  That's what we do here.
		//  (returns FALSE if no optimization is performed)
		// [TODO] : Work-in-progress!
		//template< G1Type InstType, typename RegType >
		//static __forceinline void _optimize_imm0( RegType to );

		/*template< G1Type InstType, typename RegType >
		static __forceinline void _optimize_imm0( const RegType& to )
		{
			switch( InstType )
			{
				// ADD, SUB, and OR can be ignored if the imm is zero..
				case G1Type_ADD:
				case G1Type_SUB:
				case G1Type_OR:
					return true;

				// ADC and SBB can never be ignored (could have carry bits)
				// XOR behavior is distinct as well [or is it the same as NEG or NOT?]
				case G1Type_ADC:
				case G1Type_SBB:
				case G1Type_XOR:
					return false;

				// replace AND with XOR (or SUB works too.. whatever!)
				case G1Type_AND:
					iXOR( to, to );
				return true;

				// replace CMP with OR reg,reg:
				case G1Type_CMP:
					iOR( to, to );
				return true;

				jNO_DEFAULT
			}
			return false;
		}*/

	}

	// ------------------------------------------------------------------------

	// ----- Group 1 Instruction Class -----

	extern const Internal::Group1ImplAll<Internal::G1Type_ADD> iADD;
	extern const Internal::Group1ImplAll<Internal::G1Type_OR>  iOR;
	extern const Internal::Group1ImplAll<Internal::G1Type_ADC> iADC;
	extern const Internal::Group1ImplAll<Internal::G1Type_SBB> iSBB;
	extern const Internal::Group1ImplAll<Internal::G1Type_AND> iAND;
	extern const Internal::Group1ImplAll<Internal::G1Type_SUB> iSUB;
	extern const Internal::Group1ImplAll<Internal::G1Type_XOR> iXOR;
	extern const Internal::Group1ImplAll<Internal::G1Type_CMP> iCMP;

	// ----- Group 2 Instruction Class -----
	// Optimization Note: For Imm forms, we ignore the instruction if the shift count is
	// zero.  This is a safe optimization since any zero-value shift does not affect any
	// flags.

	extern const Internal::Group2ImplAll<Internal::G2Type_ROL> iROL;
	extern const Internal::Group2ImplAll<Internal::G2Type_ROR> iROR;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCL> iRCL;
	extern const Internal::Group2ImplAll<Internal::G2Type_RCR> iRCR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHL> iSHL;
	extern const Internal::Group2ImplAll<Internal::G2Type_SHR> iSHR;
	extern const Internal::Group2ImplAll<Internal::G2Type_SAR> iSAR;

	extern const Internal::MovExtendImplAll<false> iMOVZX;
	extern const Internal::MovExtendImplAll<true>  iMOVSX;

	extern const Internal::DwordShiftImplAll<false> iSHLD;
	extern const Internal::DwordShiftImplAll<true>  iSHRD;

	extern const Internal::CMovImplGeneric iCMOV;

	extern const Internal::CMovImplAll<Jcc_Above>			iCMOVA;
	extern const Internal::CMovImplAll<Jcc_AboveOrEqual>	iCMOVAE;
	extern const Internal::CMovImplAll<Jcc_Below>			iCMOVB;
	extern const Internal::CMovImplAll<Jcc_BelowOrEqual>	iCMOVBE;

	extern const Internal::CMovImplAll<Jcc_Greater>			iCMOVG;
	extern const Internal::CMovImplAll<Jcc_GreaterOrEqual>	iCMOVGE;
	extern const Internal::CMovImplAll<Jcc_Less>			iCMOVL;
	extern const Internal::CMovImplAll<Jcc_LessOrEqual>		iCMOVLE;

	extern const Internal::CMovImplAll<Jcc_Zero>			iCMOVZ;
	extern const Internal::CMovImplAll<Jcc_Equal>			iCMOVE;
	extern const Internal::CMovImplAll<Jcc_NotZero>			iCMOVNZ;
	extern const Internal::CMovImplAll<Jcc_NotEqual>		iCMOVNE;

	extern const Internal::CMovImplAll<Jcc_Overflow>		iCMOVO;
	extern const Internal::CMovImplAll<Jcc_NotOverflow>		iCMOVNO;
	extern const Internal::CMovImplAll<Jcc_Carry>			iCMOVC;
	extern const Internal::CMovImplAll<Jcc_NotCarry>		iCMOVNC;

	extern const Internal::CMovImplAll<Jcc_Signed>			iCMOVS;
	extern const Internal::CMovImplAll<Jcc_Unsigned>		iCMOVNS;
	extern const Internal::CMovImplAll<Jcc_ParityEven>		iCMOVPE;
	extern const Internal::CMovImplAll<Jcc_ParityOdd>		iCMOVPO;

}

#include "ix86_inlines.inl"
