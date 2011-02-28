/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSFunctionMap.h"

void GSCodeGenerator::enter(uint32 size, bool align)
{
	#ifdef _M_AMD64

	push(r15);
	mov(r15, rsp);
	if(size > 0) sub(rsp, size);
	if(align) and(rsp, 0xfffffffffffffff0);

	#else

	push(ebp);
	mov(ebp, esp);
	if(size > 0) sub(esp, size);
	if(align) and(esp, 0xfffffff0);

	#endif
}

void GSCodeGenerator::leave()
{
	#ifdef _M_AMD64

	mov(rsp, r15);
	pop(r15);

	#else

	mov(esp, ebp);
	pop(ebp);

	#endif
}
