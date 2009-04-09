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

u8* execohax_pos=0;
u8* execohax_start=0;
u32 execohx_sz;

u8* code_pos=0;
u8* code_start=0;
u32 code_sz;

using namespace vtlb_private;
#include <windows.h>

void execuCode(bool set)
{
	u32 used=code_pos-code_start;
	u32 free=2*1024*1024-used;

	if (code_pos == 0 || free<128)
	{
		SysPrintf("Leaking 2 megabytes of ram\n");
		code_start=code_pos=(u8*)VirtualAlloc(0,2*1024*1024,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
		code_sz+=2*1024*1024;
		int i=0;
		while(i<code_sz)
		{
			//UD2 is 0xF 0xB.Fill the stream with it so that the cpu don't try to execute past branches ..
			code_start[i]=0xF;i++;
			code_start[i]=0xB;i++;
		}
	}

	static u8* old;

	if (set)
	{
		old=x86SetPtr(code_pos);
	}
	else
	{
		code_pos=x86SetPtr(old);
		u32 tt=execohx_sz-2*1024*1024+(execohax_pos-execohax_start);
		u32 tc=code_sz-free;
		SysPrintf("%d code, %d pot, %.2f%%\n",tc,tt,tc/(float)tt*100);
	}
}

u32* execohaxme(bool set)
{
	u32 used=execohax_pos-execohax_start;
	u32 free=2*1024*1024-used;

	if (execohax_pos == 0 || free<128)
	{
		SysPrintf("Leaking 2 megabytes of ram\n");
		execohax_start=execohax_pos=(u8*)VirtualAlloc(0,2*1024*1024,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
		execohx_sz+=2*1024*1024;
	}
	static u8* saved;
	static u8* mod;
	if (set)
	{
		write8<_EmitterId_>( 0x81 ); 
		ModRM<_EmitterId_>( 3, 0, EAX );
		write32<_EmitterId_>( (uptr)execohax_pos );

		saved=x86SetPtr(execohax_pos);
		mod=execohax_pos;
		write8<_EmitterId_>(0);	//size, in bytes
		write32<_EmitterId_>(0); //return address
	}
	else
	{
		//x86AlignExecutable(4);
		//x86Align(64);
		execohax_pos=x86SetPtr(mod);
		write8<_EmitterId_>(execohax_pos-mod-5);
		return (u32*)x86SetPtr(saved);
	}

	return 0;
}

uptr _vtlb_HandleRewrite(uptr block)
{
	u8 size=*(u8*)block;
	u32 ra=*(u32*)(block+1);
	u8* pcode=(u8*)(block+5);
	execuCode(true);
	uptr rv=(uptr)code_pos;

	while(size--)
	{
		write8<_EmitterId_>(*pcode++);
	}
	JMP32(ra-(uptr)x86Ptr[_EmitterId_]-5);
	
	execuCode(false);
	//do magic
	return rv;
}

PCSX2_ALIGNED16( static u64 g_globalXMMData[2*XMMREGS] );
void MOVx_SSE( x86IntRegType destRm, x86IntRegType srcRm,u32 srcAddr=0,u32 dstAddr=0,bool half=false )
{
	int reg;
	bool free_reg=false;
	if( _hasFreeXMMreg() )
	{
		free_reg=true;
		reg=_allocTempXMMreg( XMMT_INT, -1 );
	}
	else
	{
		SSE2_MOVDQA_XMM_to_M128((uptr)g_globalXMMData,XMM0);
		reg=XMM0;
	}

	if (half)
	{
		if (srcAddr)
			SSE_MOVLPS_M64_to_XMM(reg,srcAddr);
		else
			SSE_MOVLPS_RmOffset_to_XMM(reg,srcRm,0);

		if (dstAddr)
			SSE_MOVLPS_XMM_to_M64(dstAddr,reg);
		else
			SSE_MOVLPS_XMM_to_RmOffset(destRm,reg,0);
	}
	else
	{
		if (srcAddr)
			SSE2_MOVDQA_M128_to_XMM(reg,srcAddr);
		else
			SSE2_MOVDQARmtoROffset(reg,srcRm,0);

		if (dstAddr)
			SSE2_MOVDQA_XMM_to_M128(dstAddr,reg);
		else
			SSE2_MOVDQARtoRmOffset(destRm,reg,0);
	}


	if (free_reg)
		_freeXMMreg(reg);
	else
	{
		SSE2_MOVDQA_M128_to_XMM(XMM0,(uptr)g_globalXMMData);
	}
}
void MOV64_MMX( x86IntRegType destRm, x86IntRegType srcRm,u32 srcAddr=0,u32 dstAddr=0)
{
	//if free xmm && fpu state then we use the SSE version.
	if( !(_hasFreeXMMreg() && (x86FpuState ==  FPU_STATE)) &&  _hasFreeMMXreg() )
	{
		const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
		if (srcAddr)
			MOVQMtoR(freereg,srcAddr);
		else
			MOVQRmtoROffset(freereg,srcRm,0);

		if (dstAddr)
			MOVQRtoM(dstAddr,freereg);
		else
			MOVQRtoRmOffset(destRm,freereg,0);

		_freeMMXreg(freereg);
	}
	else
	{
		MOVx_SSE(destRm,srcRm,srcAddr,dstAddr,true);
	}
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
			MOV64_MMX(EDX,ECX);
		break;

		case 128:
			MOVx_SSE(EDX,ECX);
		break;

		jNO_DEFAULT
	}
}

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
	//u8* _direct = JMP8(0);
	execohaxme(true);

	_vtlb_DynGen_IndirectRead( bits );
	
	u32* patch=execohaxme(false);
	
	_vtlb_DynGen_DirectRead( bits, false );
	
	*patch=(uptr)x86Ptr[_EmitterId_];
}

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
	//u8* _direct = JMP8(0);
	execohaxme(true);

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

	u32* patch=execohaxme(false);

	_vtlb_DynGen_DirectRead( bits, sign );
	
	*patch=(uptr)x86Ptr[_EmitterId_];
}

//
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
				MOV64_MMX( EDX, ECX,ppf );		// dest <- src!
			break;

			case 128:
				MOVx_SSE( EDX, ECX,ppf );		// dest <- src!
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
			MOV64_MMX( ECX, EDX );
		break;

		case 128:
			MOVx_SSE( ECX, EDX );
		break;
	}

//	SHR32ItoR(ECX,4);// do /16
//	BTS_wtf(asdasd,ECX);
}

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

void vtlb_DynGenWrite(u32 sz)
{
	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vtlbdata.vmap,2);
	ADD32RtoR(ECX,EAX);

	//u8* _direct=JMP8(0);

	execohaxme(true);
	
	_vtlb_DynGen_IndirectWrite( sz );
	
	u32* patch=execohaxme(false);
	_vtlb_DynGen_DirectWrite( sz );
	
	*patch=(uptr)x86Ptr[_EmitterId_];
}


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
				MOV64_MMX( ECX, EDX,0,ppf);	// dest <- src!
			break;

			case 128:
				MOVx_SSE( ECX, EDX,0,ppf);	// dest <- src!
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
