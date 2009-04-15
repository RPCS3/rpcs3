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

// x86 opcode descriptors
#define XMMREGS 8
#define X86REGS 8
#define MMXREGS 8

enum XMMSSEType
{
	XMMT_INT = 0, // integer (sse2 only)
	XMMT_FPS = 1, // floating point
	//XMMT_FPD = 3, // double
};

extern __threadlocal u8  *x86Ptr;
extern __threadlocal u8  *j8Ptr[32];
extern __threadlocal u32 *j32Ptr[32];

extern __threadlocal XMMSSEType g_xmmtypes[XMMREGS];

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

	static const int ModRm_UseSib = 4;		// same index value as ESP (used in RM field)
	static const int ModRm_UseDisp32 = 5;	// same index value as EBP (used in Mod field)

	class x86AddressInfo;
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
	class x86Register
	{
	public:
		static const x86Register Empty;		// defined as an empty/unused value (-1)

		int Id;

		x86Register( const x86Register<OperandSize>& src ) : Id( src.Id ) {}
		x86Register(): Id( -1 ) {}
		explicit x86Register( int regId ) : Id( regId ) { jASSUME( Id >= -1 && Id < 8 ); }

		bool IsEmpty() const { return Id < 0; }

		// Returns true if the register is a valid accumulator: Eax, Ax, Al.
		bool IsAccumulator() const { return Id == 0; }

		bool operator==( const x86Register<OperandSize>& src ) const
		{
			return (Id == src.Id);
		}

		bool operator!=( const x86Register<OperandSize>& src ) const
		{
			return (Id != src.Id);
		}

		x86Register<OperandSize>& operator=( const x86Register<OperandSize>& src )
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

	typedef x86Register<4> x86Register32;
	typedef x86Register<2> x86Register16;
	typedef x86Register<1> x86Register8;

	extern const x86Register32 eax;
	extern const x86Register32 ebx;
	extern const x86Register32 ecx;
	extern const x86Register32 edx;
	extern const x86Register32 esi;
	extern const x86Register32 edi;
	extern const x86Register32 ebp;
	extern const x86Register32 esp;

	extern const x86Register16 ax;
	extern const x86Register16 bx;
	extern const x86Register16 cx;
	extern const x86Register16 dx;
	extern const x86Register16 si;
	extern const x86Register16 di;
	extern const x86Register16 bp;
	extern const x86Register16 sp;

	extern const x86Register8 al;
	extern const x86Register8 cl;
	extern const x86Register8 dl;
	extern const x86Register8 bl;
	extern const x86Register8 ah;
	extern const x86Register8 ch;
	extern const x86Register8 dh;
	extern const x86Register8 bh;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Use 32 bit registers as out index register (for ModSib memory address calculations)
	// Only x86IndexReg provides operators for constructing x86AddressInfo types.
	class x86IndexReg : public x86Register32
	{
	public:
		static const x86IndexReg Empty;		// defined as an empty/unused value (-1)
	
	public:
		x86IndexReg(): x86Register32() {}
		x86IndexReg( const x86IndexReg& src ) : x86Register32( src.Id ) {}
		x86IndexReg( const x86Register32& src ) : x86Register32( src ) {}
		explicit x86IndexReg( int regId ) : x86Register32( regId ) {}

		// Returns true if the register is the stack pointer: ESP.
		bool IsStackPointer() const { return Id == 4; }

		x86AddressInfo operator+( const x86IndexReg& right ) const;
		x86AddressInfo operator+( const x86AddressInfo& right ) const;
		x86AddressInfo operator+( s32 right ) const;

		x86AddressInfo operator*( u32 factor ) const;
		x86AddressInfo operator<<( u32 shift ) const;
		
		x86IndexReg& operator=( const x86Register32& src )
		{
			Id = src.Id;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class x86AddressInfo
	{
	public:
		x86IndexReg Base;		// base register (no scale)
		x86IndexReg Index;		// index reg gets multiplied by the scale
		int Factor;				// scale applied to the index register, in factor form (not a shift!)
		s32 Displacement;		// address displacement

	public:
		__forceinline x86AddressInfo( const x86IndexReg& base, const x86IndexReg& index, int factor=1, s32 displacement=0 ) :
			Base( base ),
			Index( index ),
			Factor( factor ),
			Displacement( displacement )
		{
		}

		__forceinline explicit x86AddressInfo( const x86IndexReg& base, int displacement=0 ) :
			Base( base ),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		__forceinline explicit x86AddressInfo( s32 displacement ) :
			Base(),
			Index(),
			Factor(0),
			Displacement( displacement )
		{
		}
		
		static x86AddressInfo FromIndexReg( const x86IndexReg& index, int scale=0, s32 displacement=0 );

	public:
		bool IsByteSizeDisp() const { return is_s8( Displacement ); }

		__forceinline x86AddressInfo& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}
		
		__forceinline x86AddressInfo& Add( const x86IndexReg& src );
		__forceinline x86AddressInfo& Add( const x86AddressInfo& src );

		__forceinline x86AddressInfo operator+( const x86IndexReg& right ) const { return x86AddressInfo( *this ).Add( right ); }
		__forceinline x86AddressInfo operator+( const x86AddressInfo& right ) const { return x86AddressInfo( *this ).Add( right ); }
		__forceinline x86AddressInfo operator+( s32 imm ) const { return x86AddressInfo( *this ).Add( imm ); }
		__forceinline x86AddressInfo operator-( s32 imm ) const { return x86AddressInfo( *this ).Add( -imm ); }
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// ModSib - Internal low-level representation of the ModRM/SIB information.
	//
	// This class serves two purposes:  It houses 'reduced' ModRM/SIB info only, which means
	// that the Base, Index, Scale, and Displacement values are all in the correct arrange-
	// ments, and it serves as a type-safe layer between the x86Register's operators (which
	// generate x86AddressInfo types) and the emitter's ModSib instruction forms.  Without this,
	// the x86Register would pass as a ModSib type implicitly, and that would cause ambiguity
	// on a number of instructions.
	//
	// End users should always use x86AddressInfo instead.
	//
	class ModSibBase
	{
	public:
		x86IndexReg Base;		// base register (no scale)
		x86IndexReg Index;		// index reg gets multiplied by the scale
		uint Scale;				// scale applied to the index register, in scale/shift form
		s32 Displacement;		// offset applied to the Base/Index registers.

	public:
		explicit ModSibBase( const x86AddressInfo& src );
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
	// Strictly-typed version of ModSibBase, which is used to apply operand size information
	// to ImmToMem operations.
	//
	template< int OperandSize >
	class ModSibStrict : public ModSibBase
	{
	public:
		__forceinline explicit ModSibStrict( const x86AddressInfo& src ) : ModSibBase( src ) {}
		__forceinline explicit ModSibStrict( s32 disp ) : ModSibBase( disp ) {}
		__forceinline ModSibStrict( x86IndexReg base, x86IndexReg index, int scale=0, s32 displacement=0 ) :
			ModSibBase( base, index, scale, displacement ) {}
		
		__forceinline ModSibStrict<OperandSize>& Add( s32 imm )
		{
			Displacement += imm;
			return *this;
		}

		__forceinline ModSibStrict<OperandSize> operator+( const s32 imm ) const { return ModSibStrict<OperandSize>( *this ).Add( imm ); }
		__forceinline ModSibStrict<OperandSize> operator-( const s32 imm ) const { return ModSibStrict<OperandSize>( *this ).Add( -imm ); }
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

		__forceinline ModSibBase operator[]( const x86AddressInfo& src ) const
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

		__forceinline ModSibStrict<OperandSize> operator[]( const x86AddressInfo& src ) const
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
		extern void EmitSibMagic( uint regfield, const ModSibBase& info );

		struct SibMagic
		{
			static void Emit( uint regfield, const ModSibBase& info )
			{
				EmitSibMagic( regfield, info );
			}
		};

		struct SibMagicInline
		{
			static __forceinline void Emit( uint regfield, const ModSibBase& info )
			{
				EmitSibMagic( regfield, info );
			}
		};	

	
		enum G1Type
		{
			G1Type_ADD=0,
			G1Type_OR,
			G1Type_ADC,
			G1Type_SBB,
			G1Type_AND,
			G1Type_SUB,
			G1Type_XOR,
			G1Type_CMP
		};

		enum G2Type
		{
			G2Type_ROL=0,
			G2Type_ROR,
			G2Type_RCL,
			G2Type_RCR,
			G2Type_SHL,
			G2Type_SHR,
			G2Type_Unused,
			G2Type_SAR
		};

		// -------------------------------------------------------------------
		template< typename ImmType, G1Type InstType, typename SibMagicType >
		class Group1Impl
		{
		public: 
			static const uint OperandSize = sizeof(ImmType);

			Group1Impl() {}		// because GCC doesn't like static classes

		protected:
			static bool Is8BitOperand()	{ return OperandSize == 1; }
			static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

		public:
			static __emitinline void Emit( const x86Register<OperandSize>& to, const x86Register<OperandSize>& from ) 
			{
				prefix16();
				iWrite<u8>( (Is8BitOperand() ? 0 : 1) | (InstType<<3) ); 
				ModRM( 3, from.Id, to.Id );
			}
			
			static __emitinline void Emit( const ModSibBase& sibdest, const x86Register<OperandSize>& from ) 
			{
				prefix16();
				iWrite<u8>( (Is8BitOperand() ? 0 : 1) | (InstType<<3) ); 
				SibMagicType::Emit( from.Id, sibdest );
			}

			static __emitinline void Emit( const x86Register<OperandSize>& to, const ModSibBase& sibsrc ) 
			{
				prefix16();
				iWrite<u8>( (Is8BitOperand() ? 2 : 3) | (InstType<<3) );
				SibMagicType::Emit( to.Id, sibsrc );
			}

			static __emitinline void Emit( const x86Register<OperandSize>& to, ImmType imm ) 
			{
				prefix16();
				if( !Is8BitOperand() && is_s8( imm ) )
				{
					iWrite<u8>( 0x83 );
					ModRM( 3, InstType, to.Id );
					iWrite<s8>( imm );
				}
				else
				{
					if( to.IsAccumulator() )
						iWrite<u8>( (Is8BitOperand() ? 4 : 5) | (InstType<<3) );
					else
					{
						iWrite<u8>( Is8BitOperand() ? 0x80 : 0x81 );
						ModRM( 3, InstType, to.Id );
					}
					iWrite<ImmType>( imm );
				}
			}

			static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest, ImmType imm ) 
			{
				if( Is8BitOperand() )
				{
					iWrite<u8>( 0x80 );
					SibMagicType::Emit( InstType, sibdest );
					iWrite<ImmType>( imm );
				}
				else
				{		
					prefix16();
					iWrite<u8>( is_s8( imm ) ? 0x83 : 0x81 );
					SibMagicType::Emit( InstType, sibdest );
					if( is_s8( imm ) )
						iWrite<s8>( imm );
					else
						iWrite<ImmType>( imm );
				}
			}
		};

		// -------------------------------------------------------------------
		// Group 2 (shift) instructions have no Sib/ModRM forms.
		// Note: For Imm forms, we ignore the instruction if the shift count is zero.  This
		// is a safe optimization since any zero-value shift does not affect any flags.
		//
		template< typename ImmType, G2Type InstType, typename SibMagicType >
		class Group2Impl
		{
		public: 
			static const uint OperandSize = sizeof(ImmType);

			Group2Impl() {}		// For the love of GCC.

		protected:
			static bool Is8BitOperand()	{ return OperandSize == 1; }
			static void prefix16()		{ if( OperandSize == 2 ) iWrite<u8>( 0x66 ); }

		public:
			static __emitinline void Emit( const x86Register<OperandSize>& to, const x86Register8& from ) 
			{
				jASSUME( from == cl );	// cl is the only valid shift register.  (turn this into a compile time check?)

				prefix16();
				iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
				ModRM( 3, InstType, to.Id );
			}

			static __emitinline void Emit( const x86Register<OperandSize>& to, u8 imm ) 
			{
				if( imm == 0 ) return;

				prefix16();
				if( imm == 1 )
				{
					// special encoding of 1's
					iWrite<u8>( Is8BitOperand() ? 0xd0 : 0xd1 );
					ModRM( 3, InstType, to.Id );
				}
				else
				{
					iWrite<u8>( Is8BitOperand() ? 0xc0 : 0xc1 );
					ModRM( 3, InstType, to.Id );
					iWrite<u8>( imm );
				}
			}

			static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest, const x86Register8& from ) 
			{
				jASSUME( from == cl );	// cl is the only valid shift register.  (turn this into a compile time check?)

				prefix16();
				iWrite<u8>( Is8BitOperand() ? 0xd2 : 0xd3 );
				SibMagicType::Emit( from.Id, sibdest );
			}

			static __emitinline void Emit( const ModSibStrict<OperandSize>& sibdest, u8 imm ) 
			{
				if( imm == 0 ) return;

				prefix16();
				if( imm == 1 )
				{
					// special encoding of 1's
					iWrite<u8>( Is8BitOperand() ? 0xd0 : 0xd1 );
					SibMagicType::Emit( InstType, sibdest );
				}
				else
				{
					iWrite<u8>( Is8BitOperand() ? 0xc0 : 0xc1 );
					SibMagicType::Emit( InstType, sibdest );
					iWrite<u8>( imm );
				}
			}
		};

		// -------------------------------------------------------------------
		//
		template< G1Type InstType >
		class Group1ImplAll
		{
		protected:
			typedef Group1Impl<u32, InstType, SibMagic> m_32;
			typedef Group1Impl<u16, InstType, SibMagic> m_16;
			typedef Group1Impl<u8, InstType, SibMagic>  m_8;

			typedef Group1Impl<u32, InstType, SibMagicInline> m_32i;
			typedef Group1Impl<u16, InstType, SibMagicInline> m_16i;
			typedef Group1Impl<u8, InstType, SibMagicInline>  m_8i;

			// Inlining Notes:
			//   I've set up the inlining to be as practical and intelligent as possible, which means
			//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
			//   virtually no code.  In the case of (Reg, Imm) forms, the inlining is up to the dis-
			//   creation of the compiler.
			// 

			// (Note: I'm not going to macro this since it would likely clobber intellisense parameter resolution)

		public:
			// ---------- 32 Bit Interface -----------
			__forceinline void operator()( const x86Register32& to,	const x86Register32& from ) const	{ m_32i::Emit( to, from ); }
			__forceinline void operator()( const x86Register32& to,	const void* src ) const				{ m_32i::Emit( to, ptr32[src] ); }
			__forceinline void operator()( const void* dest,		const x86Register32& from ) const	{ m_32i::Emit( ptr32[dest], from ); }
			__noinline void operator()( const ModSibBase& sibdest,	const x86Register32& from ) const	{ m_32::Emit( sibdest, from ); }
			__noinline void operator()( const x86Register32& to,	const ModSibBase& sibsrc ) const	{ m_32::Emit( to, sibsrc ); }
			__noinline void operator()( const ModSibStrict<4>& sibdest, u32 imm ) const					{ m_32::Emit( sibdest, imm ); }

			void operator()( const x86Register32& to, u32 imm, bool needs_flags=false ) const
			{
				//if( needs_flags || (imm != 0) || !_optimize_imm0() )
				m_32i::Emit( to, imm );
			}

			// ---------- 16 Bit Interface -----------
			__forceinline void operator()( const x86Register16& to,	const x86Register16& from ) const	{ m_16i::Emit( to, from ); }
			__forceinline void operator()( const x86Register16& to,	const void* src ) const				{ m_16i::Emit( to, ptr16[src] ); }
			__forceinline void operator()( const void* dest,		const x86Register16& from ) const	{ m_16i::Emit( ptr16[dest], from ); }
			__noinline void operator()( const ModSibBase& sibdest,	const x86Register16& from ) const	{ m_16::Emit( sibdest, from ); }
			__noinline void operator()( const x86Register16& to,	const ModSibBase& sibsrc ) const	{ m_16::Emit( to, sibsrc ); }
			__noinline void operator()( const ModSibStrict<2>& sibdest, u16 imm ) const					{ m_16::Emit( sibdest, imm ); }

			void operator()( const x86Register16& to, u16 imm, bool needs_flags=false ) const			{ m_16i::Emit( to, imm ); }

			// ---------- 8 Bit Interface -----------
			__forceinline void operator()( const x86Register8& to,	const x86Register8& from ) const	{ m_8i::Emit( to, from ); }
			__forceinline void operator()( const x86Register8& to,	const void* src ) const				{ m_8i::Emit( to, ptr8[src] ); }
			__forceinline void operator()( const void* dest,		const x86Register8& from ) const	{ m_8i::Emit( ptr8[dest], from ); }
			__noinline void operator()( const ModSibBase& sibdest,	const x86Register8& from ) const	{ m_8::Emit( sibdest, from ); }
			__noinline void operator()( const x86Register8& to,		const ModSibBase& sibsrc ) const	{ m_8::Emit( to, sibsrc ); }
			__noinline void operator()( const ModSibStrict<1>& sibdest, u8 imm ) const					{ m_8::Emit( sibdest, imm ); }

			void operator()( const x86Register8& to, u8 imm, bool needs_flags=false ) const				{ m_8i::Emit( to, imm ); }

			Group1ImplAll() {}		// Why does GCC need these?
		};


		// -------------------------------------------------------------------
		//
		template< G2Type InstType >
		class Group2ImplAll
		{
		protected:
			typedef Group2Impl<u32, InstType, SibMagic> m_32;
			typedef Group2Impl<u16, InstType, SibMagic> m_16;
			typedef Group2Impl<u8, InstType, SibMagic>  m_8;

			typedef Group2Impl<u32, InstType, SibMagicInline> m_32i;
			typedef Group2Impl<u16, InstType, SibMagicInline> m_16i;
			typedef Group2Impl<u8, InstType, SibMagicInline>  m_8i;

			// Inlining Notes:
			//   I've set up the inlining to be as practical and intelligent as possible, which means
			//   forcing inlining for (void*) forms of ModRM, which thanks to constprop reduce to
			//   virtually no code.  In the case of (Reg, Imm) forms, the inlining is up to the dis-
			//   creation of the compiler.
			// 

			// (Note: I'm not going to macro this since it would likely clobber intellisense parameter resolution)

		public:
			// ---------- 32 Bit Interface -----------
			__forceinline void operator()( const x86Register32& to,		const x86Register8& from ) const{ m_32i::Emit( to, from ); }
			__noinline void operator()( const ModSibStrict<4>& sibdest,	const x86Register8& from ) const{ m_32::Emit( sibdest, from ); }
			__noinline void operator()( const ModSibStrict<4>& sibdest, u8 imm ) const					{ m_32::Emit( sibdest, imm ); }
			void operator()( const x86Register32& to, u8 imm ) const									{ m_32i::Emit( to, imm ); }

			// ---------- 16 Bit Interface -----------
			__forceinline void operator()( const x86Register16& to,		const x86Register8& from ) const{ m_16i::Emit( to, from ); }
			__noinline void operator()( const ModSibStrict<2>& sibdest,	const x86Register8& from ) const{ m_16::Emit( sibdest, from ); }
			__noinline void operator()( const ModSibStrict<2>& sibdest, u8 imm ) const					{ m_16::Emit( sibdest, imm ); }
			void operator()( const x86Register16& to, u8 imm ) const									{ m_16i::Emit( to, imm ); }

			// ---------- 8 Bit Interface -----------
			__forceinline void operator()( const x86Register8& to,		const x86Register8& from ) const{ m_8i::Emit( to, from ); }
			__noinline void operator()( const ModSibStrict<1>& sibdest,	const x86Register8& from ) const{ m_8::Emit( sibdest, from ); }
			__noinline void operator()( const ModSibStrict<1>& sibdest, u8 imm ) const					{ m_8::Emit( sibdest, imm ); }
			void operator()( const x86Register8& to, u8 imm ) const										{ m_8i::Emit( to, imm ); }

			Group2ImplAll() {}		// I am a class with no members, so I need an explicit constructor!  Sense abounds.
		};

		// Define the externals for Group1/2 instructions here (inside the Internal namespace).
		// and then import then into the x86Emitter namespace later.  Done because it saves a
		// lot of Internal:: namespace resolution mess, and is better than the alternative of
		// importing Internal into x86Emitter, which done at the header file level would defeat
		// the purpose!)

		extern const Group1ImplAll<G1Type_ADD> iADD;
		extern const Group1ImplAll<G1Type_OR>  iOR;
		extern const Group1ImplAll<G1Type_ADC> iADC;
		extern const Group1ImplAll<G1Type_SBB> iSBB;
		extern const Group1ImplAll<G1Type_AND> iAND;
		extern const Group1ImplAll<G1Type_SUB> iSUB;
		extern const Group1ImplAll<G1Type_XOR> iXOR;
		extern const Group1ImplAll<G1Type_CMP> iCMP;

		extern const Group2ImplAll<G2Type_ROL> iROL;
		extern const Group2ImplAll<G2Type_ROR> iROR;
		extern const Group2ImplAll<G2Type_RCL> iRCL;
		extern const Group2ImplAll<G2Type_RCR> iRCR;
		extern const Group2ImplAll<G2Type_SHL> iSHL;
		extern const Group2ImplAll<G2Type_SHR> iSHR;
		extern const Group2ImplAll<G2Type_SAR> iSAR;

		//////////////////////////////////////////////////////////////////////////////////////////
		// Mov with sign/zero extension implementations:
		//
		template< int DestOperandSize, int SrcOperandSize >
		class MovExtendImpl
		{
		protected:
			static bool Is8BitOperand()	{ return SrcOperandSize == 1; }
			static void prefix16()		{ if( DestOperandSize == 2 ) iWrite<u8>( 0x66 ); }
			static __forceinline void emit_base( bool SignExtend )
			{
				prefix16();
				iWrite<u8>( 0x0f );
				iWrite<u8>( 0xb6 | (Is8BitOperand() ? 0 : 1) | (SignExtend ? 8 : 0 ) );
			}

		public: 
			MovExtendImpl() {}		// For the love of GCC.

			static __emitinline void Emit( const x86Register<DestOperandSize>& to, const x86Register<SrcOperandSize>& from, bool SignExtend )
			{
				emit_base( SignExtend );
				ModRM( 3, from.Id, to.Id );
			}

			static __emitinline void Emit( const x86Register<DestOperandSize>& to, const ModSibStrict<SrcOperandSize>& sibsrc, bool SignExtend )
			{
				emit_base( SignExtend );
				EmitSibMagic( to.Id, sibsrc );
			}
		};

		// ------------------------------------------------------------------------
		template< bool SignExtend >
		class MovExtendImplAll
		{
		protected:
			typedef MovExtendImpl<4, 2> m_16to32;
			typedef MovExtendImpl<4, 1> m_8to32;

		public:
			__forceinline void operator()( const x86Register32& to, const x86Register16& from )	const	{ m_16to32::Emit( to, from, SignExtend ); }
			__noinline void operator()( const x86Register32& to, const ModSibStrict<2>& sibsrc ) const	{ m_16to32::Emit( to, sibsrc, SignExtend ); }

			__forceinline void operator()( const x86Register32& to, const x86Register8& from ) const	{ m_8to32::Emit( to, from, SignExtend ); }
			__noinline void operator()( const x86Register32& to, const ModSibStrict<1>& sibsrc ) const	{ m_8to32::Emit( to, sibsrc, SignExtend ); }

			MovExtendImplAll() {}		// don't ask.
		};

		// ------------------------------------------------------------------------
		
		extern const MovExtendImplAll<true>  iMOVSX;
		extern const MovExtendImplAll<false> iMOVZX;


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
}

#include "ix86_inlines.inl"
