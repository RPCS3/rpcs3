/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "Rec.h"
#include "Page.h"
#include "Transfer.h"
#include "Cache.h"

#ifdef __MSCW32__
#pragma warning(disable:4244)

#endif

int primTableC[8] = {1, 2, 2, 3, 3, 3, 2, 0};
void (*primTable[8])(Vertex *v);

int pmode=0;

void CSRwrite(u32 value) {
	gs.CSRw = value;
	if (value & 0x1) gs.CSRr&= ~0x1;
	if (value & 0x2) gs.CSRr&= ~0x2;
	if (value & 0x200) { // reset
		GSreset();
	}
}

void IMRwrite(u32 value) {
	gs.IMR = value;
}

void writePMODE(u32 *data) {
#ifdef PREG_LOG
	PREG_LOG("GSwrite   PMODE value %8.8lx_%8.8lx\n", data[1], data[0]);
#endif

	gs.PMODE.en[0] = (data[0]      ) & 0x1;
	gs.PMODE.en[1] = (data[0] >>  1) & 0x1;
	gs.PMODE.crtmd = (data[0] >>  2) & 0x7;
	gs.PMODE.mmod  = (data[0] >>  5) & 0x1;
	gs.PMODE.amod  = (data[0] >>  6) & 0x1;
	gs.PMODE.slbg  = (data[0] >>  7) & 0x1;
	gs.PMODE.alp   = (data[0] >>  8) & 0xff;
}

void writeSMODE1(u32 *data) {
	int cmod;

#ifdef PREG_LOG
	PREG_LOG("GSwrite   SMODE1 value %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
	cmod = (data[0] >> 13) & 0x3;
	if (gs.SMODE1.cmod != cmod) {
		gs.SMODE1.cmod = cmod;
		if (conf.record) recOpen();
	}

	gs.SMODE1.rc    = (data[0]      ) & 0x7;
	gs.SMODE1.lc    = (data[0] >>  3) & 0x1f;
	gs.SMODE1.t1248 = (data[0] >> 10) & 0x3;
	gs.SMODE1.slck  = (data[0] >> 12) & 0x1;
//	gs.SMODE1.cmod  = (data[0] >> 13) & 0x3;
	gs.SMODE1.ex    = (data[0] >> 15) & 0x1;
	gs.SMODE1.prst  = (data[0] >> 16) & 0x1;
	gs.SMODE1.sint  = (data[0] >> 17) & 0x1;
	gs.SMODE1.xpck  = (data[0] >> 18) & 0x1;
	gs.SMODE1.pck2  = (data[0] >> 19) & 0x1;
	gs.SMODE1.spml  = (data[0] >> 21) & 0xf;
	gs.SMODE1.gcont = (data[0] >> 25) & 0x1;
	gs.SMODE1.phs   = (data[0] >> 26) & 0x1;
	gs.SMODE1.pvs   = (data[0] >> 27) & 0x1;
	gs.SMODE1.pehs  = (data[0] >> 28) & 0x1;
	gs.SMODE1.pevs  = (data[0] >> 29) & 0x1;
	gs.SMODE1.clksel= (data[0] >> 30) & 0x3;
	gs.SMODE1.nvck  = (data[1]      ) & 0x1;
	gs.SMODE1.slck2 = (data[1] >>  1) & 0x1;
	gs.SMODE1.vcksel= (data[1] >>  2) & 0x3;
	gs.SMODE1.vhp   = (data[1] >>  4);// & 0x7;
							      
	DXsetGSmode(640, gs.SYNCV.vfp & 1 ? gs.SYNCV.vdp : gs.SYNCV.vdp/2);
}

void writeSMODE2(u32 *data) {
#ifdef PREG_LOG
	PREG_LOG("GSwrite   SMODE2 value %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
	gs.SMODE2.inter = (data[0]     ) & 0x1;
	gs.SMODE2.ffmd  = (data[0] >> 1) & 0x1;
	gs.SMODE2.dpms  = (data[0] >> 2) & 0x3;
}

void writeSYNCH1(u32 *data) {
#ifdef PREG_LOG
	PREG_LOG("GSwrite   SYNCH1 value %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
	gs.SYNCH1.hfp  = (data[0]      ) & 0x7ff;
	gs.SYNCH1.fbp  = (data[0] >> 11) & 0x7ff;
	gs.SYNCH1.hseq = (data[0] >> 22) & 0x3ff;
	gs.SYNCH1.hsvs = (data[1]      ) & 0x7ff;
	gs.SYNCH1.hs   = (data[1] >> 11);// & 0x3ff;
}

void writeSYNCH2(u32 *data) {
#ifdef PREG_LOG
	PREG_LOG("GSwrite   SYNCH2 value %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
	gs.SYNCH2.hf   = (data[0]      ) & 0x7ff;
	gs.SYNCH2.hb   = (data[0] >> 11);//\ & 0x7ff;
}

void writeSYNCHV(u32 *data) {
	gs.SYNCV.vfp  = (data[0]      ) & 0x3ff;
	gs.SYNCV.vfpe = (data[0] >> 10) & 0x3ff;
	gs.SYNCV.vbp  = (data[0] >> 20) & 0xfff;
	gs.SYNCV.vbpe = (data[1]      ) & 0x3ff;
	gs.SYNCV.vdp  = (data[1] >> 10) & 0x7ff;
	gs.SYNCV.vs   = (data[1] >> 21);//\ & 0x7ff;

#ifdef PREG_LOG
	PREG_LOG("GSwrite   SYNCV value %8.8lx_%8.8lx: vfp=0x%x vfpe=0x%x vbp=0x%x vbpe=0x%x vdp=0x%x vs=0x%x\n", data[1], data[0],
		gs.SYNCV.vfp, gs.SYNCV.vfpe, gs.SYNCV.vbp, gs.SYNCV.vbpe, gs.SYNCV.vdp, gs.SYNCV.vs);
#endif
}

void writeDISPFB(u32 *data, int i) {
#ifdef PREG_LOG
	PREG_LOG("GSwrite   DISPFB%d value %8.8lx_%8.8lx\n", i+1, data[1], data[0]);
#endif
	gs.DISPFB[i].fbp =((data[0]      ) & 0x1ff) * 32;
	gs.DISPFB[i].fbw =((data[0] >>  9) & 0x3f) * 64;
	gs.DISPFB[i].psm = (data[0] >> 15) & 0x1f;
	gs.DISPFB[i].dbx = (data[1]      ) & 0x7ff;
	gs.DISPFB[i].dby = (data[1] >> 11) & 0x7ff;
	if (gs.DISPFB[i].fbw > 0) {
		gs.DISPFB[i].fbh = ((4/TextureSizeGS(1, 1, gs.DISPFB[i].psm))*1024*1024) / gs.DISPFB[i].fbw;
	} else {
		gs.DISPFB[i].fbh = 1024*1024;
	}
#ifdef PREG_LOG
	PREG_LOG("dispfb%d fbp=0x%x fbw=%d fbh=%d psm=0x%x db %d,%d\n", i+1,
		   gs.DISPFB[i].fbp, gs.DISPFB[i].fbw, gs.DISPFB[i].fbh,
		   gs.DISPFB[i].psm, gs.DISPFB[i].dbx, gs.DISPFB[i].dby);
#endif
}

void writeDISPLAY(u32 *data, int i) {
	int magh, magv;

	magh = ((data[0] >> 23) & 0xf) + 1;
	magv = ((data[0] >> 27) & 0x3) + 1;
//	gs.DISPLAY[i].x = ((data[0]      ) & 0xfff) / magh;
//	gs.DISPLAY[i].y = ((data[0] >> 12) & 0x7ff) / magv;
	gs.DISPLAY[i].w = (((data[1]      ) & 0xfff) / magh) + 1;
	gs.DISPLAY[i].h = (((data[1] >> 12) & 0x7ff) / magv) + 1;
	
//	if(((gs.SMODE2.inter == 0) || (gs.SMODE2.ffmd == 1)) && (gs.DISPLAY[1].h > 256))
	if (gs.SMODE2.inter && gs.SMODE2.ffmd)
		gs.DISPLAY[i].h >>= 1;

#ifdef PREG_LOG
	PREG_LOG("GSwrite   DISPLAY%d value %8.8lx_%8.8lx: mode = %dx%d %dx%d, magh=%d, magv=%d\n",
			 i+1, data[1], data[0],
			 gs.DISPLAY[i].x, gs.DISPLAY[i].y, gs.DISPLAY[i].w, 
			 gs.DISPLAY[i].h, magh, magv);
#endif
}

void BUSDIRwrite(u32 value) {
	gs.BUSDIR = value;
}

void CALLBACK GSwrite8(u32 mem, u8 value) {
}
	
void CALLBACK GSwrite16(u32 mem, u16 value) {
}

void CALLBACK GSwrite32(u32 mem, u32 value) {
	u32 data[2];

	data[0] = value; data[1] = 0;
	switch (mem) {
		case 0x12000000: // PMODE
			writePMODE(data);
			break;

		case 0x12000070: // DISPFB1
			writeDISPFB(data, 0);
			break;

		case 0x12000090: // DISPFB2
			writeDISPFB(data, 1);
			break;

		case 0x12001000: // CSR
#ifdef PREG_LOG
			PREG_LOG("GSwrite32 CSR (%x) value %8.8lx\n", mem, value);
#endif
			CSRwrite(value);
			break;

		case 0x12001010: // IMR
#ifdef PREG_LOG
			PREG_LOG("GSwrite32 IMR (%x) value %8.8lx\n", mem, value);
#endif
			IMRwrite(value);
			break;

		case 0x12001040: // BUSDIR
			BUSDIRwrite(value);
			break;

		case 0x12001080: // SIGLBLID
#ifdef PREG_LOG
			PREG_LOG("GSwrite32 SIGLBLID (%x) value %8.8lx\n", mem, value);
#endif
			gs.SIGLBLID.sigid = value;
			break;

		default:
#ifdef PREG_LOG
			PREG_LOG("GSwrite32 unknown mem %x value %8.8lx\n", mem, value);
#endif
			break;
	}
}

void CALLBACK GSwrite64(u32 mem, u64 value) {
	u32 data[2];

	*(u64*)data = value;
	switch (mem) {
		case 0x12000000: // PMODE
			writePMODE(data);
			break;

		case 0x12000010: // SMODE1
			writeSMODE1(data);
			break;

		case 0x12000020: // SMODE2
			writeSMODE2(data);
			break;

		case 0x12000030: // SRFSH
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 SRFSH (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;

		case 0x12000040: // SYNCH1
			writeSYNCH1(data);
			break;

		case 0x12000050: // SYNCH2
			writeSYNCH2(data);
			break;

		case 0x12000060: // SYNCV
			writeSYNCHV(data);
			break;

		case 0x12000070: // DISPFB1
			writeDISPFB(data, 0);
			break;

		case 0x12000080: // DISPLAY1
			writeDISPLAY(data, 0);
			break;

		case 0x12000090: // DISPFB2
			writeDISPFB(data, 1);
			break;

		case 0x120000A0: // DISPLAY2
			writeDISPLAY(data, 1);
			break;

		case 0x120000B0: // EXTBUF
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 EXTBUF (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;

		case 0x120000C0: // EXTDATA
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 EXTDATA (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;

		case 0x120000D0: // EXTWRITE
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 EXTWRITE (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;

		case 0x120000E0: // BGCOLOR
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 BGCOLOR (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;

		case 0x12001000: // CSR
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 CSR (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			CSRwrite(*data);
			break;

		case 0x12001010: // IMR
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 IMR (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			IMRwrite(*data);
			break;

		case 0x12001040: // BUSDIR
			BUSDIRwrite(*data);
			break;

		case 0x12001080: // SIGLBLID
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 SIGLBLID (%x) value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			gs.SIGLBLID.sigid = data[0];
			gs.SIGLBLID.lblid = data[1];
			break;

		default:
#ifdef PREG_LOG
			PREG_LOG("GSwrite64 unknown mem %x value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
			break;
	}
}

u32 CALLBACK GSread32(u32 mem) {
	u32 ret;

	switch (mem) {
		case 0x12001000: // CSR
//			gs.interlace = 1 - gs.interlace;
			ret = gs.interlace << 13;
			ret|= gs.CSRr;
			return ret;

		case 0x12001080: // SIGLBLID
			ret = gs.SIGLBLID.sigid;
			break;

		default:
			ret = 0;
			break;
	}

#ifdef PREG_LOG
	PREG_LOG("GSread32 mem %x ret %8.8lx\n", mem, ret);
#endif
	return ret;
}

u8 CALLBACK GSread8(u32 mem) {
	u8 ret;

	ret = GSread32(mem & ~3) >> ((mem & 3) * 8);
/*	switch (mem) {
		case 0x12001000: // CSR
			ret = gs.interlace << 13;
			ret|= gs.CSRr;
			return ret;

		case 0x12001080: // SIGLBLID
			ret = gs.SIGLBLID.sigid;
			break;

		default:
			ret = 0;
			break;
	}*/

#ifdef PREG_LOG
	PREG_LOG("GSread8 mem %x ret %8.8lx\n", mem, ret);
#endif
	return ret;
}

u16 CALLBACK GSread16(u32 mem) {
	u16 ret;

	ret = GSread32(mem & ~3) >> ((mem & 3) * 8);
/*	switch (mem) {
		case 0x12001000: // CSR
			ret = gs.interlace << 13;
			ret|= gs.CSRr;
			return ret;

		case 0x12001080: // SIGLBLID
			ret = gs.SIGLBLID.sigid;
			break;

		default:
			ret = 0;
			break;
	}*/

#ifdef PREG_LOG
	PREG_LOG("GSread16 mem %x ret %8.8lx\n", mem, ret);
#endif
	return ret;
}

u64 CALLBACK GSread64(u32 mem) {
	u64 ret;

	switch (mem) {
		case 0x12000400: // ???
//			gs.interlace = 1 - gs.interlace;
			ret = gs.interlace << 13;
			break;

		case 0x12001000: // CSR
//			gs.interlace = 1 - gs.interlace;
			ret = gs.interlace << 13;
			ret|= gs.CSRr;
			return ret;

		case 0x12001080: // SIGLBLID
			ret = (u64)gs.SIGLBLID.sigid | 
				  ((u64)gs.SIGLBLID.lblid << 32);
			break;

		default:
			ret = 0;
			break;
	}
#ifdef PREG_LOG
	PREG_LOG("GSread64 mem %x ret %8.8lx_%8.8lx\n", mem, (u32)(ret>>32), (u32)ret);
#endif
	return ret;
}


void primWrite(u32 *data) {
	if (data[1] || data[0] & ~0x3ff) {
#ifdef WARN_LOG
		WARN_LOG("warning: unknown bits in prim %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
	}

	prim->prim = (data[0]      ) & 0x7;
	if (gs.prac == 0) return;
//	prim->prim = (data[0]      ) & 0x7;
	prim->iip  = (data[0] >>  3) & 0x1;
	prim->tme  = (data[0] >>  4) & 0x1;
	prim->fge  = (data[0] >>  5) & 0x1;
	prim->abe  = (data[0] >>  6) & 0x1;
	prim->aa1  = (data[0] >>  7) & 0x1;
	prim->fst  = (data[0] >>  8) & 0x1;
	prim->ctxt = (data[0] >>  9) & 0x1;
	prim->fix  = (data[0] >> 10) & 0x1;
	gsSetCtxt(prim->ctxt);
	gs.primC = 0;

#ifdef GREG_LOG
	GREG_LOG("prim       %8.8lx_%8.8lx prim=%x iip=%x tme=%x fge=%x abe=%x aa1=%x fst=%x ctxt=%x fix=%x\n",
			 data[1], data[0], prim->prim, prim->iip, prim->tme, prim->fge, prim->abe, prim->aa1, prim->fst, prim->ctxt, prim->fix);
#endif
}

void tex0Write(int i, u32 *data) {
	gs._tex0[i].tbp0 =  (data[0]        & 0x3fff);
	gs._tex0[i].tbw  = ((data[0] >> 14) & 0x3f) * 64;
	gs._tex0[i].psm  =  (data[0] >> 20) & 0x3f;
	gs._tex0[i].tw   =  (data[0] >> 26) & 0xf;
	if (gs._tex0[i].tw > 10) gs._tex0[i].tw = 10;
	gs._tex0[i].tw   = (int)pow(2, (double)gs._tex0[i].tw);
	gs._tex0[i].th   = ((data[0] >> 30) & 0x3) | ((data[1] & 0x3) << 2);
	if (gs._tex0[i].th > 10) gs._tex0[i].th = 10;
	gs._tex0[i].th   = (int)pow(2, (double)gs._tex0[i].th);
	gs._tex0[i].tcc  =  (data[1] >>  2) & 0x1;
	gs._tex0[i].tfx  =  (data[1] >>  3) & 0x3;
	gs._tex0[i].cbp  = ((data[1] >>  5) & 0x3fff);
	gs._tex0[i].cpsm =  (data[1] >> 19) & 0xf;
	gs._tex0[i].csm  =  (data[1] >> 23) & 0x1;
	gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
	GREG_LOG("tex0_%d    %8.8lx_%8.8lx tbp0=0x%x, tbw=%d, psm=0x%x, tw=%d, th=%d, tcc=%d, tfx=%d, cbp=0x%x, cpsm=0x%x, csm=%d\n", 
			 i+1, data[1], data[0],
			 gs._tex0[i].tbp0, gs._tex0[i].tbw, gs._tex0[i].psm, gs._tex0[i].tw, 
			 gs._tex0[i].th, gs._tex0[i].tcc, gs._tex0[i].tfx, gs._tex0[i].cbp,
		 	 gs._tex0[i].cpsm, gs._tex0[i].csm);
#endif

/*	if (conf.log) {
		SetTexture();
		DumpTexture();
	}*/
	if (gs._tex0[i].tbw == 0) gs._tex0[i].tbw = 64;
}

void frameWrite(int i, u32 *data) {
	gs._gsfb[i].fbp = ((data[0]      ) & 0x1ff) * 32;
	gs._gsfb[i].fbw = ((data[0] >> 16) & 0x3f) * 64;
	gs._gsfb[i].psm =  (data[0] >> 24) & 0x3f;
	gs._gsfb[i].fbm = data[1];
	if (gs._gsfb[i].fbw > 0) gs._gsfb[i].fbh = (1024*1024) / gs._gsfb[i].fbw;
	else gs._gsfb[i].fbh = 1024*1024;
	gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
	GREG_LOG("frame_%d    %8.8lx_%8.8lx: fbp=0x%x fbw=%d psm=0x%x fbm=0x%x\n", 
			 i+1, data[1], data[0], 
			 gs._gsfb[i].fbp, gs._gsfb[i].fbw, gs._gsfb[i].psm, gs._gsfb[i].fbm);
#endif
}

void testWrite(int i, u32 *data) {
	gs._test[i].ate   = (data[0]      ) & 0x1;
	gs._test[i].atst  = (data[0] >>  1) & 0x7;
	gs._test[i].aref  = (data[0] >>  4) & 0xff;
	gs._test[i].afail = (data[0] >> 12) & 0x3;
	gs._test[i].date  = (data[0] >> 14) & 0x1;
	gs._test[i].datm  = (data[0] >> 15) & 0x1;
	gs._test[i].zte   = (data[0] >> 16) & 0x1;
	gs._test[i].ztst  = (data[0] >> 17) & 0x3;

#ifdef GREG_LOG
	GREG_LOG("test_%d     %8.8lx_%8.8lx: ate=%x atst=%x aref=%x afail=%x date=%x datm%x zte=%x ztst=%x\n", 
			 i+1, data[1], data[0],
			 gs._test[0].ate, gs._test[0].atst, gs._test[0].aref,
			 gs._test[0].afail, gs._test[0].date, gs._test[0].datm,
			 gs._test[0].zte, gs._test[0].ztst);
#endif
}

void clampWrite(int i, u32 *data) {
	gs._clamp[i].wms  = (data[0]      ) & 0x2;
	gs._clamp[i].wmt  = (data[0] >>  2) & 0x2;
	gs._clamp[i].minu = (data[0] >>  4) & 0x3ff;
	gs._clamp[i].maxu = (data[0] >> 14) & 0x3ff;
	gs._clamp[i].minv =((data[0] >> 24) & 0xff) | ((data[1] & 0x3) << 8);
	gs._clamp[i].maxv = (data[1] >> 2) & 0x3ff;

#ifdef GREG_LOG
	GREG_LOG("clamp_%d    %8.8lx_%8.8lx: wms=%x wmt=%x minu=%x maxu=%x minv=%x maxv=%x\n", i, data[1], data[0],
			 gs._clamp[i].wms, gs._clamp[i].wmt, 
			 gs._clamp[i].minu, gs._clamp[i].maxu, 
			 gs._clamp[i].minv, gs._clamp[i].maxv);
#endif
}

/*
 * GSwrite: 128bit wide
 */
void GSwrite(u32 *data, int reg) {
	int i;

	switch (reg) {
		case 0x00: // prim
			primWrite(data);
			break;

		case 0x01: // rgbaq
			gs.gsvertex[gs.primC].rgba = data[0];
			gs.gsvertex[gs.primC].q    = data[1];
			gs.rgba = data[0];
			gs.q = data[1];

#ifdef GREG_LOG
			GREG_LOG("rgbaq %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x02: // st
			gs.gsvertex[gs.primC].s = data[0];
			gs.gsvertex[gs.primC].t = data[1];
			

#ifdef GREG_LOG
			GREG_LOG("st %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x03: // uv
			gs.gsvertex[gs.primC].u = (data[0] >> 4) & 0x7ff;
			gs.gsvertex[gs.primC].v = (data[0] >> (16+4)) & 0x7ff;

#ifdef GREG_LOG
			GREG_LOG("uv %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x04: // xyzf2
			gs.gsvertex[gs.primC].x = (data[0] >> 4) & 0xfff;
			gs.gsvertex[gs.primC].y = (data[0] >> (16+4)) & 0xfff;
			gs.gsvertex[gs.primC].z = data[1] & 0xffffff;
			gs.gsvertex[gs.primC].f = data[1] >> 24;
			gs.primC++;

#ifdef GREG_LOG
			GREG_LOG("xyzf2 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif

			if (gs.primC >= primTableC[prim->prim]) {
#ifdef GREG_LOG
				GREG_LOG("prim %x\n", prim->prim);
#endif
				primTable[prim->prim](gs.gsvertex);
				ppf++;
			}
			break;

		case 0x05: // xyz2
			gs.gsvertex[gs.primC].x = (data[0] >> 4) & 0xfff;
			gs.gsvertex[gs.primC].y = (data[0] >> (16+4)) & 0xfff;
			gs.gsvertex[gs.primC].z = data[1];
			gs.primC++;

#ifdef GREG_LOG
			GREG_LOG("xyz2 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif

			if (gs.primC >= primTableC[prim->prim]) {
#ifdef GREG_LOG
				GREG_LOG("prim %x\n", prim->prim);
#endif
				primTable[prim->prim](gs.gsvertex);
				ppf++;
			}
			break;

		case 0x06: // tex0_1
			tex0Write(0, data);
			break;

		case 0x07: // tex0_2
			tex0Write(1, data);
			break;

		case 0x08: // clamp_1
			clampWrite(0, data);
			break;

		case 0x09: // clamp_2
			clampWrite(1, data);
			break;

		case 0x0a: // fog
			gs.fogf = data[1] >> 24;

#ifdef GREG_LOG
			GREG_LOG("fogH       %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x0c: // xyzf3
			gs.gsvertex[gs.primC].x = (data[0] >> 4) & 0xfff;
			gs.gsvertex[gs.primC].y = (data[0] >> (16+4)) & 0xfff;
			gs.gsvertex[gs.primC].z = data[1] & 0xffffff;
			gs.gsvertex[gs.primC].f = data[1] >> 24;
			gs.primC++;

#ifdef GREG_LOG
			GREG_LOG("xyzf3      %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			if (gs.primC >= primTableC[prim->prim]) {
				primTable[prim->prim](NULL);
			}
			break;

		case 0x0d: // xyz3
			gs.gsvertex[gs.primC].x = (data[0] >> 4) & 0xfff;
			gs.gsvertex[gs.primC].y = (data[0] >> (16+4)) & 0xfff;
			gs.gsvertex[gs.primC].z = data[1];
			gs.primC++;

#ifdef GREG_LOG
			GREG_LOG("xyz3       %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			if (gs.primC >= primTableC[prim->prim]) {
				primTable[prim->prim](NULL);
			}
			break;

		case 0x14: // tex1_1
			gs._tex1[0].lcm  = (data[0]      ) & 0x1; 
			gs._tex1[0].mxl  = (data[0] >>  2) & 0x7;
			gs._tex1[0].mmag = (data[0] >>  5) & 0x1;
			gs._tex1[0].mmin = (data[0] >>  6) & 0x7;
			gs._tex1[0].mtba = (data[0] >>  9) & 0x1;
			gs._tex1[0].l    = (data[0] >> 19) & 0x3;
			gs._tex1[0].k    = (data[1] >> 4) & 0xff;

#ifdef GREG_LOG
			GREG_LOG("tex1_1     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x15: // tex1_2
			gs._tex1[1].lcm  = (data[0]      ) & 0x1; 
			gs._tex1[1].mxl  = (data[0] >>  2) & 0x7;
			gs._tex1[1].mmag = (data[0] >>  5) & 0x1;
			gs._tex1[1].mmin = (data[0] >>  6) & 0x7;
			gs._tex1[1].mtba = (data[0] >>  9) & 0x1;
			gs._tex1[1].l    = (data[0] >> 19) & 0x3;
			gs._tex1[1].k    = (data[1] >> 4) & 0xff;

#ifdef GREG_LOG
			GREG_LOG("tex1_2     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x16: // tex2_1
			gs._tex2[0].psm  = (data[0] >> 20) & 0x3f; 
			gs._tex2[0].cbp  = (data[1] >>  5) & 0x3fff;
			gs._tex2[0].cpsm = (data[1] >> 19) & 0xf;
			gs._tex2[0].csm  = (data[1] >> 23) & 0x1;
			gs._tex2[0].csa  = (data[1] >> 24) & 0x1f;
			gs._tex2[0].cld  = (data[1] >> 29) & 0x7;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("tex2_1     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x17: // tex2_2
			gs._tex2[1].psm  = (data[0] >> 20) & 0x3f; 
			gs._tex2[1].cbp  = (data[1] >>  5) & 0x3fff;
			gs._tex2[1].cpsm = (data[1] >> 19) & 0xf;
			gs._tex2[1].csm  = (data[1] >> 23) & 0x1;
			gs._tex2[1].csa  = (data[1] >> 24) & 0x1f;
			gs._tex2[1].cld  = (data[1] >> 29) & 0x7;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("tex2_2     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x18: // xyoffset_1
			gs._offset[0].x = (data[0] >> 4) & 0xfff;
			gs._offset[0].y = (data[1] >> 4) & 0xfff;

#ifdef GREG_LOG
			GREG_LOG("xyoffset_1 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x19: // xyoffset_2
			gs._offset[1].x = (data[0] >> 4) & 0xfff;
			gs._offset[1].y = (data[1] >> 4) & 0xfff;

#ifdef GREG_LOG
			GREG_LOG("xyoffset_2 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x1a: // prmodecont
			gs.prac = data[0] & 0x1;
			prim = &gs._prim[gs.prac];

#ifdef GREG_LOG
			GREG_LOG("prmodecont %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x1b: // prmode
			if (gs.prac == 1) break;
			prim->iip  = (data[0] >>  3) & 0x1;
			prim->tme  = (data[0] >>  4) & 0x1;
			prim->fge  = (data[0] >>  5) & 0x1;
			prim->abe  = (data[0] >>  6) & 0x1;
			prim->aa1  = (data[0] >>  7) & 0x1;
			prim->fst  = (data[0] >>  8) & 0x1;
			prim->ctxt = (data[0] >>  9) & 0x1;
			prim->fix  = (data[0] >> 10) & 0x1;
			gsSetCtxt(prim->ctxt);
			gs.primC = 0;

#ifdef GREG_LOG
			GREG_LOG("prmode     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x1c: // texclut
			gs.clut.cbw = ((data[0]      ) & 0x3f) * 64;
			gs.clut.cou = ((data[0] >>  6) & 0x3f) * 16;
			gs.clut.cov = (data[0] >> 12) & 0x3ff;

#ifdef GREG_LOG
			GREG_LOG("texclut    %8.8lx_%8.8lx: cbw=%x, cou=%d, cov=%d\n", 
					 data[1], data[0], gs.clut.cbw, gs.clut.cou, gs.clut.cov);
#endif
			break;

		case 0x22: // scanmsk
			gs.smask = data[0] & 0x3;

#ifdef GREG_LOG
			GREG_LOG("scanmsk    %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x34: // miptbp1_1
			gs._miptbp0[0].tbp[0] = (data[0]      ) & 0x3fff;
			gs._miptbp0[0].tbw[0] = (data[0] >> 14) & 0x3f;
			gs._miptbp0[0].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			gs._miptbp0[0].tbw[1] = (data[1] >>  2) & 0x3f;
			gs._miptbp0[0].tbp[2] = (data[1] >>  8) & 0x3fff;
			gs._miptbp0[0].tbw[2] = (data[1] >> 22) & 0x3f;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("miptbp1_1  %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x35: // miptbp1_2
			gs._miptbp0[1].tbp[0] = (data[0]      ) & 0x3fff;
			gs._miptbp0[1].tbw[0] = (data[0] >> 14) & 0x3f;
			gs._miptbp0[1].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			gs._miptbp0[1].tbw[1] = (data[1] >>  2) & 0x3f;
			gs._miptbp0[1].tbp[2] = (data[1] >>  8) & 0x3fff;
			gs._miptbp0[1].tbw[2] = (data[1] >> 22) & 0x3f;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("miptbp1_2  %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x36: // miptbp2_1
			gs._miptbp1[0].tbp[0] = (data[0]      ) & 0x3fff;
			gs._miptbp1[0].tbw[0] = (data[0] >> 14) & 0x3f;
			gs._miptbp1[0].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			gs._miptbp1[0].tbw[1] = (data[1] >>  2) & 0x3f;
			gs._miptbp1[0].tbp[2] = (data[1] >>  8) & 0x3fff;
			gs._miptbp1[0].tbw[2] = (data[1] >> 22) & 0x3f;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("miptbp2_1  %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x37: // miptbp2_2
			gs._miptbp1[1].tbp[0] = (data[0]      ) & 0x3fff;
			gs._miptbp1[1].tbw[0] = (data[0] >> 14) & 0x3f;
			gs._miptbp1[1].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			gs._miptbp1[1].tbw[1] = (data[1] >>  2) & 0x3f;
			gs._miptbp1[1].tbp[2] = (data[1] >>  8) & 0x3fff;
			gs._miptbp1[1].tbw[2] = (data[1] >> 22) & 0x3f;
			gsSetCtxt(prim->ctxt);

#ifdef GREG_LOG
			GREG_LOG("miptbp2_2  %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x3b: // texa
			gs.texa.ta[0] = data[0] & 0xff;
			gs.texa.aem   = (data[0] >> 15) & 0x1;
			gs.texa.ta[1] = data[1] & 0xff;

#ifdef GREG_LOG
			GREG_LOG("texa       %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x3d: // fogcol
			gs.fogcol = data[0] & 0xffffff;

#ifdef GREG_LOG
			GREG_LOG("fogcol     %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x3f: // texflush
#ifdef GREG_LOG
			GREG_LOG("texflush   %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x40: // scissor_1
			gs._scissor[0].x0 = (data[0]      ) & 0x7ff;
			gs._scissor[0].x1 = (data[0] >> 16) & 0x7ff;
			gs._scissor[0].y0 = (data[1]      ) & 0x7ff;
			gs._scissor[0].y1 = (data[1] >> 16) & 0x7ff;
#ifdef GREG_LOG
			GREG_LOG("scissor_1  %8.8lx_%8.8lx: %dx%d - %dx%d\n", 
				   data[1], data[0], 
				   gs._scissor[0].x0, gs._scissor[0].y0, gs._scissor[0].x1, gs._scissor[0].y1);
#endif
			break;

		case 0x41: // scissor_2
			gs._scissor[1].x0 = (data[0]      ) & 0x7ff;
			gs._scissor[1].x1 = (data[0] >> 16) & 0x7ff;
			gs._scissor[1].y0 = (data[1]      ) & 0x7ff;
			gs._scissor[1].y1 = (data[1] >> 16) & 0x7ff;
#ifdef GREG_LOG
			GREG_LOG("scissor_2  %8.8lx_%8.8lx: %dx%d - %dx%d\n", 
				   data[1], data[0], 
				   gs._scissor[1].x0, gs._scissor[1].y0, gs._scissor[1].x1, gs._scissor[1].y1);
#endif
			break;

		case 0x42: // alpha_1
			gs._alpha[0].a   = (data[0]     ) & 0x3;
			gs._alpha[0].b   = (data[0] >> 2) & 0x3;
			gs._alpha[0].c   = (data[0] >> 4) & 0x3;
			gs._alpha[0].d   = (data[0] >> 6) & 0x3;
			gs._alpha[0].fix = (data[1]     ) & 0xff;

#ifdef GREG_LOG
			GREG_LOG("alpha_1 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x43: // alpha_2
			gs._alpha[1].a   = (data[0]     ) & 0x3;
			gs._alpha[1].b   = (data[0] >> 2) & 0x3;
			gs._alpha[1].c   = (data[0] >> 4) & 0x3;
			gs._alpha[1].d   = (data[0] >> 6) & 0x3;
			gs._alpha[1].fix = (data[1]     ) & 0xff;

#ifdef GREG_LOG
			GREG_LOG("alpha_2 %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x44: // dimx
#ifdef GREG_LOG
			GREG_LOG("dimx %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x45: // dthe
			gs.dthe = data[0] & 0x1;

#ifdef GREG_LOG
			GREG_LOG("dthe %8.8lx_%8.8lx: dthe = %x\n", data[1], data[0], gs.dthe);
#endif
			break;

		case 0x46: // colclamp
			gs.colclamp = data[0] & 0x1;

#ifdef GREG_LOG
			GREG_LOG("colclamp %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x47: // test_1
			testWrite(0, data);
			break;

		case 0x48: // test_2
			testWrite(1, data);
			break;

		case 0x49: // pabe
			gs.pabe = *data & 0x1;

#ifdef GREG_LOG
			GREG_LOG("pabe       %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x4a: // fba_1
			gs._fba[0].fba = *data & 0x1;

#ifdef GREG_LOG
			GREG_LOG("fba_1      %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x4b: // fba_2
			gs._fba[1].fba = *data & 0x1;

#ifdef GREG_LOG
			GREG_LOG("fba_1      %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x4c: // frame_1
			frameWrite(0, data);
			break;

		case 0x4d: // frame_2
			frameWrite(1, data);
			break;

		case 0x4e: // zbuf_1
			gs._zbuf[0].zbp = (data[0] & 0x1ff) * 32;
			gs._zbuf[0].psm = (data[0] >> 24) & 0xf;
			gs._zbuf[0].zmsk = data[1] & 0x1;

#ifdef GREG_LOG
			GREG_LOG("zbuf_1     %8.8lx_%8.8lx: zbp=%x psm=%x zmsk=%x\n",
					 data[1], data[0],
					 gs._zbuf[0].zbp, gs._zbuf[0].psm, gs._zbuf[0].zmsk);
#endif
			break;

		case 0x4f: // zbuf_2
			gs._zbuf[1].zbp = (data[0] & 0x1ff) * 32;
			gs._zbuf[1].psm = (data[0] >> 24) & 0xf;
			gs._zbuf[1].zmsk = data[1] & 0x1;

#ifdef GREG_LOG
			GREG_LOG("zbuf_2     %8.8lx_%8.8lx: zbp=%x psm=%x zmsk=%x\n",
					 data[1], data[0],
					 gs._zbuf[1].zbp, gs._zbuf[1].psm, gs._zbuf[1].zmsk);
#endif
 			break;

		case 0x50: // bitbltbuf
			gs.srcbuf.bp  = ((data[0]      ) & 0x3fff);// * 64;
			gs.srcbuf.bw  = ((data[0] >> 16) & 0x3f) * 64;
			gs.srcbuf.psm =  (data[0] >> 24) & 0x3f;
			if (gs.srcbuf.bw > 0) gs.srcbuf.bh = (1024*1024) / gs.srcbuf.bw;
			else gs.srcbuf.bh = 1024*1024;
			gs.dstbuf.bp  = ((data[1]      ) & 0x3fff);// * 64;
			gs.dstbuf.bw  = ((data[1] >> 14) & 0x1ff) * 16;
			gs.dstbuf.psm =  (data[1] >> 24) & 0x3f;
			if (gs.dstbuf.bw > 0) gs.dstbuf.bh = (1024*1024) / gs.dstbuf.bw;
			else gs.dstbuf.bh = 1024*1024;

#ifdef GREG_LOG
			GREG_LOG("bitbltbuf %8.8lx_%8.8lx: sbp = %x, sbw = %d, spsm = %x, sbh = %d; dbp = %x, dbw = %d, dpsm = %x, dbh = %d\n", data[1], data[0],
				   gs.srcbuf.bp, gs.srcbuf.bw, gs.srcbuf.psm, gs.srcbuf.bh,
				   gs.dstbuf.bp, gs.dstbuf.bw, gs.dstbuf.psm, gs.dstbuf.bh);
#endif

			if (gs.dstbuf.bw == 0) gs.dstbuf.bw = 64;
			break;

		case 0x51: // trxpos
			gs.trxpos.sx  = (data[0]      ) & 0x7ff;
			gs.trxpos.sy  = (data[0] >> 16) & 0x7ff;
			gs.trxpos.dx  = (data[1]      ) & 0x7ff;
			gs.trxpos.dy  = (data[1] >> 16) & 0x7ff;
			gs.trxpos.dir = (data[1] >> 27) & 0x3;
#ifdef GREG_LOG
			GREG_LOG("trxpos %8.8lx_%8.8lx: %dx%d %dx%d : dir=%d\n", data[1], data[0],
				   gs.trxpos.sx, gs.trxpos.sy, gs.trxpos.dx, gs.trxpos.dy, gs.trxpos.dir);
#endif
			break;

		case 0x52: // trxreg
			gs.imageW = data[0];
			gs.imageH = data[1];

#ifdef GREG_LOG
			GREG_LOG("trxreg %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			break;

		case 0x53: // trxdir
			gs.imageTransfer = data[0] & 0x3;

#ifdef GREG_LOG
			GREG_LOG("trxdir %8.8lx_%8.8lx: gs.imageTransfer = %x\n", data[1], data[0], gs.imageTransfer);
#endif
			if (gs.imageTransfer == 0x2) {
#ifdef GREG_LOG
				GREG_LOG("moveImage %dx%d %dx%d %dx%d (dir=%d)\n",
					   gs.trxpos.sx, gs.trxpos.sy, gs.trxpos.dx, gs.trxpos.dy, gs.imageW, gs.imageH, gs.trxpos.dir);
#endif
				FBmoveImage();
				break;
			}
			if (gs.imageTransfer == 1) {
#ifdef GREG_LOG
//				GREG_LOG("gs.imageTransferSrc size %lx, %dx%d %dx%d (psm=%x, bp=%x)\n",
//					   gs.gtag.nloop, gs.trxpos.sx, gs.trxpos.sy, gs.imageW, gs.imageH, gs.srcbuf.psm, gs.srcbuf.bp);
#endif
				gs.imageX = gs.trxpos.sx;
				gs.imageY = gs.trxpos.sy;
			} else {
				gs.imageX = gs.trxpos.dx;
				gs.imageY = gs.trxpos.dy;
			}
			
			break;

		case 0x60: // signal
#ifdef GREG_LOG
			GREG_LOG("signal %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			for (i=0; i<32; i++) {
				if (data[1] & (1<<i)) {
					if (data[0] & (1<<i)) {
						((u32*)&gs.SIGLBLID)[0]|=  1<<i;
					} else {
						((u32*)&gs.SIGLBLID)[0]&=~(1<<i);
					}
				}
			}
			if (gs.CSRw & 0x1) gs.CSRr|= 0x1;
			if (!(gs.IMR & 0x100)) GSirq();
			break;

		case 0x61: // finish
#ifdef GREG_LOG
			GREG_LOG("finish %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			if (gs.CSRw & 0x2) gs.CSRr|= 0x2;
			if (!(gs.IMR & 0x200)) GSirq();
			break;

		case 0x62: // label
#ifdef GREG_LOG
			GREG_LOG("label %8.8lx_%8.8lx\n", data[1], data[0]);
#endif
			for (i=0; i<32; i++) {
				if (data[1] & (1<<i)) {
					if (data[0] & (1<<i)) {
						((u32*)&gs.SIGLBLID)[1]|=  1<<i;
					} else {
						((u32*)&gs.SIGLBLID)[1]&=~(1<<i);
					}
				}
			}
			break;

		default:
#ifdef GREG_LOG
			GREG_LOG("unhandled %x: %8.8lx_%8.8lx\n", reg, data[1], data[0]);
#endif
			break;
	}
}

