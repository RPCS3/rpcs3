/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "x86/ix86/ix86.h"
#include "iCore.h"
#include "iR5900.h"

using namespace vtlb_private;

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
		//SysPrintf("Translted 0x%08X to 0x%08X\n",addr,paddr);
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


//ecx = addr
//edx = ptr
void vtlb_DynGenRead64(u32 bits)
{
	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vmap,2);
	ADD32RtoR(ECX,EAX);
	u8* _fullread=JS8(0);
	switch(bits)
	{
		case 64:
			if( _hasFreeMMXreg() )
			{
				const int freereg = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQRmtoROffset(freereg,ECX,0);
				MOVQRtoRmOffset(EDX,freereg,0);
				_freeMMXreg(freereg);
			}
			else
			{
				MOV32RmtoR(EAX,ECX);
				MOV32RtoRm(EDX,EAX);

				MOV32RmtoROffset(EAX,ECX,4);
				MOV32RtoRmOffset(EDX,EAX,4);
			}
			break;

		case 128:
			if( _hasFreeXMMreg() )
			{
				const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
				SSE2_MOVDQARmtoROffset(freereg,ECX,0);
				SSE2_MOVDQARtoRmOffset(EDX,freereg,0);
				_freeXMMreg(freereg);
			}
			else
			{
				// Could put in an MMX optimization here as well, but no point really.
				// It's almost never used since there's almost always a free XMM reg.

				MOV32RmtoR(EAX,ECX);
				MOV32RtoRm(EDX,EAX);

				MOV32RmtoROffset(EAX,ECX,4);
				MOV32RtoRmOffset(EDX,EAX,4);

				MOV32RmtoROffset(EAX,ECX,8);
				MOV32RtoRmOffset(EDX,EAX,8);

				MOV32RmtoROffset(EAX,ECX,12);
				MOV32RtoRmOffset(EDX,EAX,12);
			}
			break;

		jNO_DEFAULT
	}

	u8* cont=JMP8(0);
	x86SetJ8(_fullread);
	int szidx;

	switch(bits)
	{
		case 64:   szidx=3;	break;
		case 128:  szidx=4; break;
		jNO_DEFAULT
	}

	MOVZX32R8toR(EAX,EAX);
	SUB32RtoR(ECX,EAX);
	//eax=[funct+eax]
	MOV32RmSOffsettoR(EAX,EAX,(int)RWFT[szidx][0],2);
	SUB32ItoR(ECX,0x80000000);
	CALL32R(EAX);

	x86SetJ8(cont);
}

// ecx - source address to read from
// Returns read value in eax.
void vtlb_DynGenRead32(u32 bits, bool sign)
{
	jASSUME( bits <= 32 );

	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vmap,2);
	ADD32RtoR(ECX,EAX);
	u8* _fullread=JS8(0);

	switch(bits)
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

		jNO_DEFAULT
	}

	u8* cont=JMP8(0);
	x86SetJ8(_fullread);
	int szidx;

	switch(bits)
	{
		case 8:  szidx=0;	break;
		case 16: szidx=1;	break;
		case 32: szidx=2;	break;
		jNO_DEFAULT
	}

	MOVZX32R8toR(EAX,EAX);
	SUB32RtoR(ECX,EAX);
	//eax=[funct+eax]
	MOV32RmSOffsettoR(EAX,EAX,(int)RWFT[szidx][0],2);
	SUB32ItoR(ECX,0x80000000);
	CALL32R(EAX);

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

	x86SetJ8(cont);
}

void vtlb_DynGenWrite(u32 sz)
{
	MOV32RtoR(EAX,ECX);
	SHR32ItoR(EAX,VTLB_PAGE_BITS);
	MOV32RmSOffsettoR(EAX,EAX,(int)vmap,2);
	ADD32RtoR(ECX,EAX);
	u8* _full=JS8(0);
	switch(sz)
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
				MOVQRmtoROffset(freereg,EDX,0);
				MOVQRtoRmOffset(ECX,freereg,0);
				_freeMMXreg( freereg );
			}
			else
			{
				MOV32RmtoR(EAX,EDX);
				MOV32RtoRm(ECX,EAX);

				MOV32RmtoROffset(EAX,EDX,4);
				MOV32RtoRmOffset(ECX,EAX,4);
			}
			break;

		case 128:
			if( _hasFreeXMMreg() )
			{
				const int freereg = _allocTempXMMreg( XMMT_INT, -1 );
				SSE2_MOVDQARmtoROffset(freereg,EDX,0);
				SSE2_MOVDQARtoRmOffset(ECX,freereg,0);
				_freeXMMreg( freereg );
			}
			else
			{
				// Could put in an MMX optimization here as well, but no point really.
				// It's almost never used since there's almost always a free XMM reg.

				MOV32RmtoR(EAX,EDX);
				MOV32RtoRm(ECX,EAX);
				MOV32RmtoROffset(EAX,EDX,4);
				MOV32RtoRmOffset(ECX,EAX,4);
				MOV32RmtoROffset(EAX,EDX,8);
				MOV32RtoRmOffset(ECX,EAX,8);
				MOV32RmtoROffset(EAX,EDX,12);
				MOV32RtoRmOffset(ECX,EAX,12);
			}
			break;
	}
	u8* cont=JMP8(0);

	x86SetJ8(_full);

	int szidx=0;
	switch(sz)
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
	MOV32RmSOffsettoR(EAX,EAX,(int)RWFT[szidx][1],2);
	SUB32ItoR(ECX,0x80000000);
	CALL32R(EAX);

	x86SetJ8(cont);
}
