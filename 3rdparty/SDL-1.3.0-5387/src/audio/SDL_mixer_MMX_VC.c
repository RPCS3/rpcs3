/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#include "SDL_mixer_MMX_VC.h"

#if defined(SDL_BUGGY_MMX_MIXERS) /* buggy, so we're disabling them. --ryan. */
#if ((defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)) && defined(SDL_ASSEMBLY_ROUTINES)
// MMX assembler version of SDL_MixAudio for signed little endian 16 bit samples and signed 8 bit samples
// Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
// Converted to Intel ASM notation by Cth
// This code is licensed under the LGPL (see COPYING for details)
// 
// Assumes buffer size in bytes is a multiple of 16
// Assumes SDL_MIX_MAXVOLUME = 128


////////////////////////////////////////////////
// Mixing for 16 bit signed buffers
////////////////////////////////////////////////

void
SDL_MixAudio_MMX_S16_VC(char *dst, char *src, unsigned int nSize, int volume)
{
    /* *INDENT-OFF* */
	__asm
	{

		push	edi
		push	esi
		push	ebx
		
		mov		edi, dst		// edi = dst
		mov		esi, src		// esi = src
		mov		eax, volume		// eax = volume
		mov		ebx, nSize		// ebx = size
		shr		ebx, 4			// process 16 bytes per iteration = 8 samples
		jz		endS16
		
		pxor	mm0, mm0
		movd	mm0, eax		//%%eax,%%mm0
		movq	mm1, mm0		//%%mm0,%%mm1
		psllq	mm0, 16			//$16,%%mm0
		por		mm0, mm1		//%%mm1,%%mm0
		psllq	mm0, 16			//$16,%%mm0
		por		mm0, mm1		//%%mm1,%%mm0
		psllq	mm0, 16			//$16,%%mm0
		por		mm0, mm1		//%%mm1,%%mm0			// mm0 = vol|vol|vol|vol

		#ifndef __WATCOMC__
		align	16
		#endif
mixloopS16:
		movq	mm1, [esi]		//(%%esi),%%mm1\n" // mm1 = a|b|c|d
		movq	mm2, mm1		//%%mm1,%%mm2\n" // mm2 = a|b|c|d
		movq	mm4, [esi + 8]	//8(%%esi),%%mm4\n" // mm4 = e|f|g|h
		// pre charger le buffer dst dans mm7
		movq	mm7, [edi]		//(%%edi),%%mm7\n" // mm7 = dst[0]"
		// multiplier par le volume
		pmullw	mm1, mm0		//%%mm0,%%mm1\n" // mm1 = l(a*v)|l(b*v)|l(c*v)|l(d*v)
		pmulhw	mm2, mm0		//%%mm0,%%mm2\n" // mm2 = h(a*v)|h(b*v)|h(c*v)|h(d*v)
		movq	mm5, mm4		//%%mm4,%%mm5\n" // mm5 = e|f|g|h
		pmullw	mm4, mm0		//%%mm0,%%mm4\n" // mm4 = l(e*v)|l(f*v)|l(g*v)|l(h*v)
		pmulhw	mm5, mm0		//%%mm0,%%mm5\n" // mm5 = h(e*v)|h(f*v)|h(g*v)|h(h*v)
		movq	mm3, mm1		//%%mm1,%%mm3\n" // mm3 = l(a*v)|l(b*v)|l(c*v)|l(d*v)
		punpckhwd	mm1, mm2	//%%mm2,%%mm1\n" // mm1 = a*v|b*v
		movq		mm6, mm4	//%%mm4,%%mm6\n" // mm6 = l(e*v)|l(f*v)|l(g*v)|l(h*v)
		punpcklwd	mm3, mm2	//%%mm2,%%mm3\n" // mm3 = c*v|d*v
		punpckhwd	mm4, mm5	//%%mm5,%%mm4\n" // mm4 = e*f|f*v
		punpcklwd	mm6, mm5	//%%mm5,%%mm6\n" // mm6 = g*v|h*v
		// pre charger le buffer dst dans mm5
		movq	mm5, [edi + 8]	//8(%%edi),%%mm5\n" // mm5 = dst[1]
		// diviser par 128
		psrad	mm1, 7			//$7,%%mm1\n" // mm1 = a*v/128|b*v/128 , 128 = SDL_MIX_MAXVOLUME
		add		esi, 16			//$16,%%esi\n"
		psrad	mm3, 7			//$7,%%mm3\n" // mm3 = c*v/128|d*v/128
		psrad	mm4, 7			//$7,%%mm4\n" // mm4 = e*v/128|f*v/128
		// mm1 = le sample avec le volume modifie
		packssdw	mm3, mm1	//%%mm1,%%mm3\n" // mm3 = s(a*v|b*v|c*v|d*v)
		psrad	mm6, 7			//$7,%%mm6\n" // mm6= g*v/128|h*v/128
		paddsw	mm3, mm7		//%%mm7,%%mm3\n" // mm3 = adjust_volume(src)+dst
		// mm4 = le sample avec le volume modifie
		packssdw	mm6, mm4	//%%mm4,%%mm6\n" // mm6 = s(e*v|f*v|g*v|h*v)
		movq	[edi], mm3		//%%mm3,(%%edi)\n"
		paddsw	mm6, mm5		//%%mm5,%%mm6\n" // mm6 = adjust_volume(src)+dst
		movq	[edi + 8], mm6	//%%mm6,8(%%edi)\n"
		add		edi, 16			//$16,%%edi\n"
		dec		ebx				//%%ebx\n"
		jnz mixloopS16

endS16:
		emms
		
		pop		ebx
		pop		esi
		pop		edi
	}
    /* *INDENT-ON* */
}

////////////////////////////////////////////////
// Mixing for 8 bit signed buffers
////////////////////////////////////////////////

void
SDL_MixAudio_MMX_S8_VC(char *dst, char *src, unsigned int nSize, int volume)
{
    /* *INDENT-OFF* */
	_asm
	{

		push	edi
		push	esi
		push	ebx
		
		mov		edi, dst	//movl	%0,%%edi	// edi = dst
		mov		esi, src	//%1,%%esi	// esi = src
		mov		eax, volume	//%3,%%eax	// eax = volume

		movd	mm0, eax	//%%eax,%%mm0
		movq	mm1, mm0	//%%mm0,%%mm1
		psllq	mm0, 16		//$16,%%mm0
		por		mm0, mm1	//%%mm1,%%mm0
		psllq	mm0, 16		//$16,%%mm0
		por		mm0, mm1	//%%mm1,%%mm0
		psllq	mm0, 16		//$16,%%mm0
		por		mm0, mm1	//%%mm1,%%mm0
		
		mov		ebx, nSize	//%2,%%ebx	// ebx = size
		shr		ebx, 3		//$3,%%ebx	// process 8 bytes per iteration = 8 samples
		cmp		ebx, 0		//$0,%%ebx
		je		endS8

		#ifndef __WATCOMC__
		align 16
		#endif
mixloopS8:
		pxor	mm2, mm2	//%%mm2,%%mm2		// mm2 = 0
		movq	mm1, [esi]	//(%%esi),%%mm1	// mm1 = a|b|c|d|e|f|g|h
		movq	mm3, mm1	//%%mm1,%%mm3 	// mm3 = a|b|c|d|e|f|g|h
		// on va faire le "sign extension" en faisant un cmp avec 0 qui retourne 1 si <0, 0 si >0
		pcmpgtb		mm2, mm1	//%%mm1,%%mm2	// mm2 = 11111111|00000000|00000000....
		punpckhbw	mm1, mm2	//%%mm2,%%mm1	// mm1 = 0|a|0|b|0|c|0|d
		punpcklbw	mm3, mm2	//%%mm2,%%mm3	// mm3 = 0|e|0|f|0|g|0|h
		movq	mm2, [edi]	//(%%edi),%%mm2	// mm2 = destination
		pmullw	mm1, mm0	//%%mm0,%%mm1	// mm1 = v*a|v*b|v*c|v*d
		add		esi, 8		//$8,%%esi
		pmullw	mm3, mm0	//%%mm0,%%mm3	// mm3 = v*e|v*f|v*g|v*h
		psraw	mm1, 7		//$7,%%mm1		// mm1 = v*a/128|v*b/128|v*c/128|v*d/128 
		psraw	mm3, 7		//$7,%%mm3		// mm3 = v*e/128|v*f/128|v*g/128|v*h/128
		packsswb mm3, mm1	//%%mm1,%%mm3	// mm1 = v*a/128|v*b/128|v*c/128|v*d/128|v*e/128|v*f/128|v*g/128|v*h/128
		paddsb	mm3, mm2	//%%mm2,%%mm3	// add to destination buffer
		movq	[edi], mm3	//%%mm3,(%%edi)	// store back to ram
		add		edi, 8		//$8,%%edi
		dec		ebx			//%%ebx
		jnz		mixloopS8
		
endS8:
		emms
		
		pop		ebx
		pop		esi
		pop		edi
	}
    /* *INDENT-ON* */
}

#endif /* SDL_ASSEMBLY_ROUTINES */
#endif /* SDL_BUGGY_MMX_MIXERS */

/* vi: set ts=4 sw=4 expandtab: */
