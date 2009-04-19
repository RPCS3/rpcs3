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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "vtlb.h"

#include "iCore.h"
#include "iR5900.h"

using namespace vtlb_private;
using namespace x86Emitter;

// NOTICE: This function *destroys* EAX!!
// Moves 128 bits of memory from the source register ptr to the dest register ptr.
// (used as an equivalent to movaps, when a free XMM register is unavailable for some reason)
void MOV128_MtoM( x86IntRegType destRm, x86IntRegType srcRm )
{
	// (this is one of my test cases for the new emitter --air)

	iAddressReg src( srcRm );
	iAddressReg dest( destRm );

	iMOV( eax, ptr[src] );
	iMOV( ptr[dest], eax );

	iMOV( eax, ptr[src+4] );
	iMOV( ptr[dest+4], eax );

	iMOV( eax, ptr[src+8] );
	iMOV( ptr[dest+8], eax );

	iMOV( eax, ptr[src+12] );
	iMOV( ptr[dest+12], eax );
}

/*
	// Pseudo-Code For the following Dynarec Implementations -->

	u32 vmv=vmap[addr>>VTLB_PAGE_BITS];
	s32 ppf=addr+vmv;
	if (!(ppf<0))
	{
		data[0]=*reinterpret_cast<DataType*>(ppf);
		if (DataSize==128)
			data[1]=*reinterpret_cast<DataType*>(ppf+8);
		return 0;
	}
	else
	{	
		//has to: translate, find function, call function
		u32 hand=(u8)vmv;
		u32 paddr=ppf-hand+0x80000000;
		//Console::WriteLn("Translated 0x%08X to 0x%08X",params addr,paddr);
		return reinterpret_cast<TemplateHelper<DataSize,false>::HandlerType*>(RWFT[TemplateHelper<DataSize,false>::sidx][0][hand])(paddr,data);
	}

	// And in ASM it looks something like this -->

	mov eax,ecx;
	shr eax,VTLB_PAGE_BITS;
	mov eax,[eax*4+vmap];
	add ecx,eax;
	js _fullread;

	//these are wrong order, just an example ...
	mov [eax],ecx;
	mov ecx,[edx];
	mov [eax+4],ecx;
	mov ecx,[edx+4];
	mov [eax+4+4],ecx;
	mov ecx,[edx+4+4];
	mov [eax+4+4+4+4],ecx;
	mov ecx,[edx+4+4+4+4];
	///....

	jmp cont;
	_fullread:
	movzx eax,al;
	sub   ecx,eax;
	sub   ecx,0x80000000;
	call [eax+stuff];
	cont:
	........

	*/

//////////////////////////////////////////////////////////////////////////////////////////
//                            Dynarec Load Implementations

static void _vtlb_DynGen_DirectRead( u32 bits, bool sign )
{
	switch( bits )
	{
		case 8:
			if( sign )
				MOVSX32Rm8toR(EAX,ECX);
			else
				MOVZX32Rm8toR(EAX,ECX);
		break;

		case 16:
			if( sign )
				MOVSX32Rm16toR(EAX,ECX);
			else
				MOVZX32Rm16toR(EAX,ECX);
		break;

		case 32:
			MOV32RmtoR(EAX,ECX);
		break;

		case 64:
			if( _hasFreeMMXreg() )
			{
				const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRmtoR(freereg,ECX);
				MOVQRtoRm(EDX,freereg);
				_freeMMXreg(freereg);
			}
			else
			{
				MOV32RmtoR(EAX,ECX);
				MOV32RtoRm(EDX,EAX);

				MOV32RmtoR(EAX,ECX,4);
				MOV32RtoRm(EDX,EAX,4);
			}
		break;

		case 128:
			if( _hasFreeXMMreg() )
			{
				const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
				SSE2_MOVDQARmtoR(freereg,ECX);
				SSE2_MOVDQARtoRm(EDX,freereg);
				_freeXMMreg(freereg);
			}
			else
			{
				// Could put in an MMX optimization here as well, but no point really.
				// It's almost never used since there's almost always a free XMM reg.

				MOV128_MtoM( EDX, ECX );		// dest <- src!
			}
		break;

		jNO_DEFAULT
	}
}

// ------------------------------------------------------------------------
static void _vtlb_DynGen_IndirectRead( u32 bits )
{
	int szidx;

	switch( bits )
	{
		case 8:  szidx=0;	break;
		case 16: szidx=1;	break;
		case 32: szidx=2;	break;
		case 64: szidx=3;	break;
		case 128: szidx=4;	break;
		jNO_DEFAULT
	}

	MOVZX32R8toR(EAX,EAX);
	SUB32RtoR(ECX,EAX);
	//eax=[funct+eax]
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.RWFT[szidx][0],2);
	SUB32ItoR(ECX,0x80000000);
	CALL32R(EAX);
}

// ------------------------------------------------------------------------
// Recompiled input registers:
//   ecx = source addr to read from
//   edx = ptr to dest to write to
void vtlb_DynGenRead64(u32 bits)
{
	jASSUME( bits == 64 || bits == 128 );

	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.vmap,2);
	ADD32RtoR(ECX,EAX);
	iForwardJS8 _fullread;

	_vtlb_DynGen_DirectRead( bits, false );
	iForwardJump8 cont;

	_fullread.SetTarget();
	
	_vtlb_DynGen_IndirectRead( bits );
	cont.SetTarget();
}

// ------------------------------------------------------------------------
// Recompiled input registers:
//   ecx - source address to read from
//   Returns read value in eax.
void vtlb_DynGenRead32(u32 bits, bool sign)
{
	jASSUME( bits <= 32 );

	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.vmap,2);
	ADD32RtoR(ECX,EAX);
	iForwardJS8 _fullread;

	_vtlb_DynGen_DirectRead( bits, sign );
	iForwardJump8 cont;

	_fullread.SetTarget();
	_vtlb_DynGen_IndirectRead( bits );

	// perform sign extension on the result:

	if( bits==8 )
	{
		if( sign )
			MOVSX32R8toR(EAX,EAX);
		else
			MOVZX32R8toR(EAX,EAX);
	}
	else if( bits==16 )
	{
		if( sign )
			MOVSX32R16toR(EAX,EAX);
		else
			MOVZX32R16toR(EAX,EAX);
	}
	cont.SetTarget();
}

// ------------------------------------------------------------------------
// TLB lookup is performed in const, with the assumption that the COP0/TLB will clear the
// recompiler if the TLB is changed.
void vtlb_DynGenRead64_Const( u32 bits, u32 addr_const )
{
	u32 vmv_ptr = vtlbdata.vmap[addr_const>>VTLB_PAGE_BITS];
	s32 ppf = addr_const + vmv_ptr;
	if( ppf >= 0 )
	{
		switch( bits )
		{
			case 64:
				if( _hasFreeMMXreg() )
				{
					const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
					MOVQMtoR(freereg,ppf);
					MOVQRtoRm(EDX,freereg);
					_freeMMXreg(freereg);
				}
				else
				{
					MOV32MtoR(EAX,ppf);
					MOV32RtoRm(EDX,EAX);

					MOV32MtoR(EAX,ppf+4);
					MOV32RtoRm(EDX,EAX,4);
				}
			break;

			case 128:
				if( _hasFreeXMMreg() )
				{
					const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
					SSE2_MOVDQA_M128_to_XMM( freereg, ppf );
					SSE2_MOVDQARtoRm(EDX,freereg);
					_freeXMMreg(freereg);
				}
				else
				{
					// Could put in an MMX optimization here as well, but no point really.
					// It's almost never used since there's almost always a free XMM reg.

					MOV32ItoR( ECX, ppf );
					MOV128_MtoM( EDX, ECX );		// dest <- src!
				}
			break;

			jNO_DEFAULT
		}
	}
	else
	{
		// has to: translate, find function, call function
		u32 handler = (u8)vmv_ptr;
		u32 paddr = ppf - handler + 0x80000000;

		int szidx = 0;
		switch( bits )
		{
			case 64:	szidx=3;	break;
			case 128:	szidx=4;	break;
		}

		MOV32ItoR( ECX, paddr );
		CALLFunc( (int)vtlbdata.RWFT[szidx][0][handler] );
	}
}

// ------------------------------------------------------------------------
// Recompiled input registers:
//   ecx - source address to read from
//   Returns read value in eax.
//
// TLB lookup is performed in const, with the assumption that the COP0/TLB will clear the
// recompiler if the TLB is changed.
void vtlb_DynGenRead32_Const( u32 bits, bool sign, u32 addr_const )
{
	u32 vmv_ptr = vtlbdata.vmap[addr_const>>VTLB_PAGE_BITS];
	s32 ppf = addr_const + vmv_ptr;
	if( ppf >= 0 )
	{
		switch( bits )
		{
			case 8:
				if( sign )
					MOVSX32M8toR(EAX,ppf);
				else
					MOVZX32M8toR(EAX,ppf);
			break;

			case 16:
				if( sign )
					MOVSX32M16toR(EAX,ppf);
				else
					MOVZX32M16toR(EAX,ppf);
			break;

			case 32:
				MOV32MtoR(EAX,ppf);
			break;
		}
	}
	else
	{
		// has to: translate, find function, call function
		u32 handler = (u8)vmv_ptr;
		u32 paddr = ppf - handler + 0x80000000;
		
		int szidx = 0;
		switch( bits )
		{
			case 8:		szidx=0;	break;
			case 16:	szidx=1;	break;
			case 32:	szidx=2;	break;
		}

		// Shortcut for the INTC_STAT register, which many games like to spin on heavily.
		if( (bits == 32) && !CHECK_INTC_STAT_HACK && (paddr == INTC_STAT) )
		{
			MOV32MtoR( EAX, (uptr)&psHu32( INTC_STAT ) );
		}
		else
		{
			MOV32ItoR( ECX, paddr );
			CALLFunc( (int)vtlbdata.RWFT[szidx][0][handler] );

			// perform sign extension on the result:

			if( bits==8 )
			{
				if( sign )
					MOVSX32R8toR(EAX,EAX);
				else
					MOVZX32R8toR(EAX,EAX);
			}
			else if( bits==16 )
			{
				if( sign )
					MOVSX32R16toR(EAX,EAX);
				else
					MOVZX32R16toR(EAX,EAX);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//                            Dynarec Store Implementations

static void _vtlb_DynGen_DirectWrite( u32 bits )
{
	switch(bits)
	{
		//8 , 16, 32 : data on EDX
		case 8:
			MOV8RtoRm(ECX,EDX);
		break;
		case 16:
			MOV16RtoRm(ECX,EDX);
		break;
		case 32:
			MOV32RtoRm(ECX,EDX);
		break;

		case 64:
			if( _hasFreeMMXreg() )
			{
				const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRmtoR(freereg,EDX);
				MOVQRtoRm(ECX,freereg);
				_freeMMXreg( freereg );
			}
			else
			{
				MOV32RmtoR(EAX,EDX);
				MOV32RtoRm(ECX,EAX);

				MOV32RmtoR(EAX,EDX,4);
				MOV32RtoRm(ECX,EAX,4);
			}
		break;

		case 128:
			if( _hasFreeXMMreg() )
			{
				const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
				SSE2_MOVDQARmtoR(freereg,EDX);
				SSE2_MOVDQARtoRm(ECX,freereg);
				_freeXMMreg( freereg );
			}
			else
			{
				// Could put in an MMX optimization here as well, but no point really.
				// It's almost never used since there's almost always a free XMM reg.

				MOV128_MtoM( ECX, EDX );	// dest <- src!
			}
		break;
	}
}

// ------------------------------------------------------------------------
static void _vtlb_DynGen_IndirectWrite( u32 bits )
{
	int szidx=0;
	switch( bits )
	{
		case 8:  szidx=0;	break;
		case 16:   szidx=1;	break;
		case 32:   szidx=2;	break;
		case 64:   szidx=3;	break;
		case 128:   szidx=4; break;
	}
	MOVZX32R8toR(EAX,EAX);
	SUB32RtoR(ECX,EAX);
	//eax=[funct+eax]
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.RWFT[szidx][1],2);
	SUB32ItoR(ECX,0x80000000);
	CALL32R(EAX);
}

// ------------------------------------------------------------------------
void vtlb_DynGenWrite(u32 sz)
{
	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.vmap,2);
	ADD32RtoR(ECX,EAX);
	iForwardJS8 _full;

	_vtlb_DynGen_DirectWrite( sz );
	iForwardJump8 cont;

	_full.SetTarget();
	_vtlb_DynGen_IndirectWrite( sz );

	cont.SetTarget();
}


// ------------------------------------------------------------------------
// Generates code for a store instruction, where the address is a known constant.
// TLB lookup is performed in const, with the assumption that the COP0/TLB will clear the
// recompiler if the TLB is changed.
void vtlb_DynGenWrite_Const( u32 bits, u32 addr_const )
{
	u32 vmv_ptr = vtlbdata.vmap[addr_const>>VTLB_PAGE_BITS];
	s32 ppf = addr_const + vmv_ptr;
	if( ppf >= 0 )
	{
		switch(bits)
		{
			//8 , 16, 32 : data on EDX
			case 8:
				MOV8RtoM(ppf,EDX);
			break;
			case 16:
				MOV16RtoM(ppf,EDX);
			break;
			case 32:
				MOV32RtoM(ppf,EDX);
			break;

			case 64:
				if( _hasFreeMMXreg() )
				{
					const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
					MOVQRmtoR(freereg,EDX);
					MOVQRtoM(ppf,freereg);
					_freeMMXreg( freereg );
				}
				else
				{
					MOV32RmtoR(EAX,EDX);
					MOV32RtoM(ppf,EAX);

					MOV32RmtoR(EAX,EDX,4);
					MOV32RtoM(ppf+4,EAX);
				}
			break;

			case 128:
				if( _hasFreeXMMreg() )
				{
					const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
					SSE2_MOVDQARmtoR(freereg,EDX);
					SSE2_MOVDQA_XMM_to_M128(ppf,freereg);
					_freeXMMreg( freereg );
				}
				else
				{
					// Could put in an MMX optimization here as well, but no point really.
					// It's almost never used since there's almost always a free XMM reg.

					MOV32ItoR( ECX, ppf );
					MOV128_MtoM( ECX, EDX );	// dest <- src!
				}
			break;
		}

	}
	else
	{	
		// has to: translate, find function, call function
		u32 handler = (u8)vmv_ptr;
		u32 paddr = ppf - handler + 0x80000000;
		
		int szidx = 0;
		switch( bits )
		{
			case 8:  szidx=0;	break;
			case 16:   szidx=1;	break;
			case 32:   szidx=2;	break;
			case 64:   szidx=3;	break;
			case 128:   szidx=4; break;
		}

		MOV32ItoR( ECX, paddr );
		CALLFunc( (int)vtlbdata.RWFT[szidx][1][handler] );
	}
}
