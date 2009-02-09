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

#pragma once

#include "GS.h"

#pragma pack(push, 1)

__declspec(align(16)) class GSDrawingEnvironment
{
public:
	GIFRegPRIM			PRIM;
	GIFRegPRMODE		PRMODE;
	GIFRegPRMODECONT	PRMODECONT;
	GIFRegTEXCLUT		TEXCLUT;
	GIFRegSCANMSK		SCANMSK;
	GIFRegTEXA			TEXA;
	GIFRegFOGCOL		FOGCOL;
	GIFRegDIMX			DIMX;
	GIFRegDTHE			DTHE;
	GIFRegCOLCLAMP		COLCLAMP;
	GIFRegPABE			PABE;
	GIFRegBITBLTBUF		BITBLTBUF;
	GIFRegTRXDIR		TRXDIR;
	GIFRegTRXPOS		TRXPOS;
	GIFRegTRXREG		TRXREG;
	GIFRegTRXREG		TRXREG2;
	GSDrawingContext	CTXT[2];

	GSDrawingEnvironment() 
	{
	}

	void Reset()
	{
		memset(&PRIM, 0, sizeof(PRIM));
		memset(&PRMODE, 0, sizeof(PRMODE));
		memset(&PRMODECONT, 0, sizeof(PRMODECONT));
		memset(&TEXCLUT, 0, sizeof(TEXCLUT));
		memset(&SCANMSK, 0, sizeof(SCANMSK));
		memset(&TEXA, 0, sizeof(TEXA));
		memset(&FOGCOL, 0, sizeof(FOGCOL));
		memset(&DIMX, 0, sizeof(DIMX));
		memset(&DTHE, 0, sizeof(DTHE));
		memset(&COLCLAMP, 0, sizeof(COLCLAMP));
		memset(&PABE, 0, sizeof(PABE));
		memset(&BITBLTBUF, 0, sizeof(BITBLTBUF));
		memset(&TRXDIR, 0, sizeof(TRXDIR));
		memset(&TRXPOS, 0, sizeof(TRXPOS));
		memset(&TRXREG, 0, sizeof(TRXREG));
		memset(&TRXREG2, 0, sizeof(TRXREG2));

		CTXT[0].Reset();
		CTXT[1].Reset();
	}
};

#pragma pack(pop)
