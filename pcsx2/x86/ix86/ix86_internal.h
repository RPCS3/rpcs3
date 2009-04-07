
#pragma once
#include "ix86.h"

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define MEMADDR(addr, oplen)	(addr)

#define Rex(w,r,x,b) assert(0)
#define RexR(w, reg) assert( !(w || (reg)>=8) )
#define RexB(w, base) assert( !(w || (base)>=8) )
#define RexRB(w, reg, base) assert( !(w || (reg) >= 8 || (base)>=8) )
#define RexRXB(w, reg, index, base) assert( !(w || (reg) >= 8 || (index) >= 8 || (base) >= 8) )

#define _MM_MK_INSERTPS_NDX(srcField, dstField, zeroMask) (((srcField)<<6) | ((dstField)<<4) | (zeroMask))

static const int ModRm_UseSib = 4;		// same index value as ESP (used in RM field)
static const int ModRm_UseDisp32 = 5;	// same index value as EBP (used in Mod field)


//------------------------------------------------------------------
// General Emitter Helper functions
//------------------------------------------------------------------

namespace x86Emitter
{
	extern void EmitSibMagic( int regfield, const ModSib& info );
	extern void EmitSibMagic( x86Register regfield, const ModSib& info );
	extern bool NeedsSibMagic( const ModSib& info );
}

// From here out are the legacy (old) emitter functions...

extern void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset);
extern void ModRM( int mod, int reg, int rm );
extern void SibSB( int ss, int index, int base );
extern void SET8R( int cc, int to );
extern u8*  J8Rel( int cc, int to );
extern u32* J32Rel( int cc, u32 to );
extern u64  GetCPUTick( void );
//------------------------------------------------------------------
