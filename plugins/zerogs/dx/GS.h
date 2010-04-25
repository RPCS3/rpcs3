/*  ZeroGS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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

#ifndef __GS_H__
#define __GS_H__

#include <stdio.h>
#include <malloc.h>

#define GSdefs
#include "PS2Edefs.h"

#ifdef _WIN32

#include <windows.h>
#include <windowsx.h>

extern HWND GShwnd;

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <gtk/gtk.h>

#define __inline inline

typedef int BOOL;

#define max(a,b)			(((a) > (b)) ? (a) : (b))
#define min(a,b)			(((a) < (b)) ? (a) : (b))

#endif

#if defined(_MSC_VER)
#define FASTCALL(fn) __fastcall fn
#else

#ifdef __x86_64
#define FASTCALL(fn) fn
#else
#define FASTCALL(fn) __attribute__((fastcall)) fn
#endif
#endif

struct Vector_16F
{
	u16 x, y, z, w;
};


/////////////////////
// define when releasing
// The only code that uses it is commented out!
//#define ZEROGS_CACHEDCLEAR // much better performance
//#define RELEASE_TO_PUBLIC

#if defined(ZEROGS_DEVBUILD)
#define GS_LOG __Log
#else
#define GS_LOG 0&&
#endif

#define ERROR_LOG __LogToConsole
#define DEBUG_LOG printf

#ifdef RELEASE_TO_PUBLIC
#define WARN_LOG 0&&
#define PRIM_LOG 0&&
#else
#define WARN_LOG printf
#define PRIM_LOG if (conf.log & 0x00000010) GS_LOG
#endif

#ifndef GREG_LOG
#define GREG_LOG 0&&
#endif
#ifndef PRIM_LOG
#define PRIM_LOG 0&&
#endif
#ifndef WARN_LOG
#define WARN_LOG 0&&
#endif

#define REG64(name) \
union name			\
{					\
	u64 i64;		\
	u32 ai32[2];	\
	struct {		\

#define REG128(name)\
union name			\
{					\
	u64 ai64[2];	\
	u32 ai32[4];	\
	struct {		\

#define REG64_(prefix, name) REG64(prefix##name)
#define REG128_(prefix, name) REG128(prefix##name)

#define REG_END }; };
#define REG_END2 };

#define REG64_SET(name) \
union name			\
{					\
	u64 i64;		\
	u32 ai32[2];	\

#define REG128_SET(name)\
union name			\
{					\
	u64 ai64[2];	\
	u32 ai32[4];	\

#define REG_SET_END };

REG64_(GSReg, BGCOLOR)
	u32 R:8;
	u32 G:8;
	u32 B:8;
	u32 _PAD1:8;
	u32 _PAD2:32;
REG_END

REG64_(GSReg, BUSDIR)
	u32 DIR:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GSReg, CSR)
	u32 SIGNAL:1;
	u32 FINISH:1;
	u32 HSINT:1;
	u32 VSINT:1;
	u32 EDWINT:1;
	u32 ZERO1:1;
	u32 ZERO2:1;
	u32 _PAD1:1;
	u32 FLUSH:1;
	u32 RESET:1;
	u32 _PAD2:2;
	u32 NFIELD:1;
	u32 FIELD:1;
	u32 FIFO:2;
	u32 REV:8;
	u32 ID:8;
	u32 _PAD3:32;
REG_END

REG64_(GSReg, DISPFB) // (-1/2)
	u32 FBP:9;
	u32 FBW:6;
	u32 PSM:5;
	u32 _PAD:12;
	u32 DBX:11;
	u32 DBY:11;
	u32 _PAD2:10;
REG_END

REG64_(GSReg, DISPLAY) // (-1/2)
	u32 DX:12;
	u32 DY:11;
	u32 MAGH:4;
	u32 MAGV:2;
	u32 _PAD:3;
	u32 DW:12;
	u32 DH:11;
	u32 _PAD2:9;
REG_END

REG64_(GSReg, EXTBUF)
	u32 EXBP:14;
	u32 EXBW:6;
	u32 FBIN:2;
	u32 WFFMD:1;
	u32 EMODA:2;
	u32 EMODC:2;
	u32 _PAD1:5;
	u32 WDX:11;
	u32 WDY:11;
	u32 _PAD2:10;
REG_END

REG64_(GSReg, EXTDATA)
	u32 SX:12;
	u32 SY:11;
	u32 SMPH:4;
	u32 SMPV:2;
	u32 _PAD1:3;
	u32 WW:12;
	u32 WH:11;
	u32 _PAD2:9;
REG_END

REG64_(GSReg, EXTWRITE)
	u32 WRITE;
	u32 _PAD2:32;
REG_END

REG64_(GSReg, IMR)
	u32 _PAD1:8;
	u32 SIGMSK:1;
	u32 FINISHMSK:1;
	u32 HSMSK:1;
	u32 VSMSK:1;
	u32 EDWMSK:1;
	u32 _PAD2:19;
	u32 _PAD3:32;
REG_END

REG64_(GSReg, PMODE)
	u32 EN1:1;
	u32 EN2:1;
	u32 CRTMD:3;
	u32 MMOD:1;
	u32 AMOD:1;
	u32 SLBG:1;
	u32 ALP:8;
	u32 _PAD:16;
	u32 _PAD1:32;
REG_END

REG64_(GSReg, SIGLBLID)
	u32 SIGID:32;
	u32 LBLID:32;
REG_END

REG64_(GSReg, SMODE1)
	u32 RC:3;
	u32 LC:7;
	u32 T1248:2;
	u32 SLCK:1;
	u32 CMOD:2;
	u32 EX:1;
	u32 PRST:1;
	u32 SINT:1;
	u32 XPCK:1;
	u32 PCK2:2;
	u32 SPML:4;
	u32 GCONT:1;
	u32 PHS:1;
	u32 PVS:1;
	u32 PEHS:1;
	u32 PEVS:1;
	u32 CLKSEL:2;
	u32 NVCK:1;
	u32 SLCK2:1;
	u32 VCKSEL:2;
	u32 VHP:1;
	u32 _PAD1:27;
REG_END

REG64_(GSReg, SMODE2)
	u32 INT:1;
	u32 FFMD:1;
	u32 DPMS:2;
	u32 _PAD2:28;
	u32 _PAD3:32;
REG_END

REG64_(GSReg, SIGBLID)
	u32 SIGID;
	u32 LBLID;
REG_END

extern int g_LastCRC;
extern u8* g_pBasePS2Mem;
#define PMODE ((GSRegPMODE*)(g_pBasePS2Mem+0x0000))
#define SMODE1 ((GSRegSMODE1*)(g_pBasePS2Mem+0x0010))
#define SMODE2 ((GSRegSMODE2*)(g_pBasePS2Mem+0x0020))
// SRFSH
#define SYNCH1 ((GSRegSYNCH1*)(g_pBasePS2Mem+0x0040))
#define SYNCH2 ((GSRegSYNCH2*)(g_pBasePS2Mem+0x0050))
#define SYNCV ((GSRegSYNCV*)(g_pBasePS2Mem+0x0060))
#define DISPFB1 ((GSRegDISPFB*)(g_pBasePS2Mem+0x0070))
#define DISPLAY1 ((GSRegDISPLAY*)(g_pBasePS2Mem+0x0080))
#define DISPFB2 ((GSRegDISPFB*)(g_pBasePS2Mem+0x0090))
#define DISPLAY2 ((GSRegDISPLAY*)(g_pBasePS2Mem+0x00a0))
#define EXTBUF ((GSRegEXTBUF*)(g_pBasePS2Mem+0x00b0))
#define EXTDATA ((GSRegEXTDATA*)(g_pBasePS2Mem+0x00c0))
#define EXTWRITE ((GSRegEXTWRITE*)(g_pBasePS2Mem+0x00d0))
#define BGCOLOR ((GSRegBGCOLOR*)(g_pBasePS2Mem+0x00e0))
#define CSR ((GSRegCSR*)(g_pBasePS2Mem+0x1000))
#define IMR ((GSRegIMR*)(g_pBasePS2Mem+0x1010))
#define BUSDIR ((GSRegBUSDIR*)(g_pBasePS2Mem+0x1040))
#define SIGLBLID ((GSRegSIGBLID*)(g_pBasePS2Mem+0x1080))

#define GET_GSFPS (((SMODE1->CMOD&1) ? 50 : 60) / (SMODE2->INT ? 1 : 2))

//
// sps2tags.h
//
#ifdef _M_AMD64
#define GET_GIF_REG(tag, reg) \
	(((tag).ai64[1] >> ((reg) << 2)) & 0xf)
#else
#define GET_GIF_REG(tag, reg) \
	(((tag).ai32[2 + ((reg) >> 3)] >> (((reg) & 7) << 2)) & 0xf)
#endif

//
// GIFTag
REG128(GIFTag)
	u32 NLOOP:15;
	u32 EOP:1;
	u32 _PAD1:16;
	u32 _PAD2:14;
	u32 PRE:1;
	u32 PRIM:11;
	u32 FLG:2; // enum GIF_FLG
	u32 NREG:4;
	u64 REGS:64;
REG_END

typedef struct {
	int x, y, w, h;
} Rect;

typedef struct {
	int x, y;
} Point;

typedef struct {
	int x0, y0;
	int x1, y1;
} Rect2;

typedef struct {
	int x, y, c;
} PointC;

#define GSOPTION_FULLSCREEN 0x2
#define GSOPTION_BMPSNAP 0x4
#define GSOPTION_CAPTUREAVI 0x8

#define GSOPTION_WINDIMS	0x70
#define GSOPTION_WIN640		0x00
#define GSOPTION_WIN800		0x10
#define GSOPTION_WIN1024	0x20
#define GSOPTION_WIN1280	0x30
#define GSOPTION_WIN960W	0x40
#define GSOPTION_WIN1280W	0x50
#define GSOPTION_WIN1920W	0x60

#define GSOPTION_WIREFRAME	0x100
#define GSOPTION_WIDESCREEN	0x200
#define GSOPTION_LOADED		0x8000

typedef struct {
	u8 mrtdepth; // write color in render target
	u8 interlace;
	u8 aa; // antialiasing 0 - off, 1 - 2x, 2 - 4x
	u8 bilinear; // set to enable bilinear support
	u32 options;
	u32 gamesettings; // default game settings
	int width, height;
	int winstyle; // window style before full screen
#ifdef GS_LOG
	u32 log;
#endif
} GSconf;

#define VERTEXGPU \
	union \
	{ \
		struct \
		{ \
		u16 x, y, f, resv0;		/* note: xy is 12d3*/ \
		u32 rgba; \
		u32 z; \
		}; \
	}; \
	\
	float s, t, q; \

typedef struct {
	s16 x, y, f, resv0;		/* note: xy is 12d3*/ \
	u32 rgba;
	u32 z;
	float s, t, q;
} VertexGPU;

typedef struct {
	VERTEXGPU
	u16 u, v;
} Vertex;

extern int g_GameSettings;
extern GSconf conf;
extern int ppf;

#define PSMCT32		0
#define PSMCT24		1
#define PSMCT16		2
#define PSMCT16S	10
#define PSMT8		19
#define PSMT4		20
#define PSMT8H		27
#define PSMT4HL		36
#define PSMT4HH		44
#define PSMT32Z		48
#define PSMT24Z		49
#define PSMT16Z		50
#define PSMT16SZ	58

#define PSMT_ISCLUT(psm) (((psm)&0x7)>2)
#define PSMT_IS16BIT(psm) (((psm)&7)==2||((psm)&7)==10)

typedef struct {
	int nloop;
	int eop;
	int nreg;
} tagInfo;

typedef union {
	s64 SD;
	u64 UD;
	s32 SL[2];
	u32 UL[2];
	s16 SS[4];
	u16 US[4];
	s8  SC[8];
	u8  UC[8];
} reg64;

/* general purpose regs structs */
typedef struct {
	int fbp;
	int fbw;
	int fbh;
	int psm;
	u32 fbm;
} frameInfo;

typedef struct {
	u16 prim;

	union {
		struct {
			u16 iip : 1;
			u16 tme : 1;
			u16 fge : 1;
			u16 abe : 1;
			u16 aa1 : 1;
			u16 fst : 1;
			u16 ctxt : 1;
			u16 fix : 1;
			u16 resv : 8;
		};
		u16 _val;
	};
} primInfo;

extern primInfo *prim;

typedef union {
	struct {
		u32 ate : 1;
		u32 atst : 3;
		u32 aref : 8;
		u32 afail : 2;
		u32 date : 1;
		u32 datm : 1;
		u32 zte : 1;
		u32 ztst : 2;
		u32 resv : 13;
	};
	u32 _val;
} pixTest;

typedef struct {
	int bp;
	int bw;
	int psm;
} bufInfo;

typedef struct {
	int tbp0;
	int tbw;
	int cbp;
	u16 tw, th;
	u8 psm;
	u8 tcc;
	u8 tfx;
	u8 cpsm;
	u8 csm;
	u8 csa;
	u8 cld;
} tex0Info;

#define TEX_MODULATE 0
#define TEX_DECAL 1
#define TEX_HIGHLIGHT 2
#define TEX_HIGHLIGHT2 3

typedef struct {
	int lcm;
	int mxl;
	int mmag;
	int mmin;
	int mtba;
	int l;
	int k;
} tex1Info;

typedef struct {
	int wms;
	int wmt;
	int minu;
	int maxu;
	int minv;
	int maxv;
} clampInfo;

typedef struct {
	int cbw;
	int cou;
	int cov;
} clutInfo;

typedef struct {
	int tbp[3];
	int tbw[3];
} miptbpInfo;

typedef struct {
	u16 aem;
	u8 ta[2];
	float fta[2];
} texaInfo;

typedef struct {
	int sx;
	int sy;
	int dx;
	int dy;
	int dir;
} trxposInfo;

typedef struct {
	union {
		struct {
			u8 a : 2;
			u8 b : 2;
			u8 c : 2;
			u8 d : 2;
		};
		u8 abcd;
	};

	u8 fix : 8;
} alphaInfo;

typedef struct {
	u16 zbp;		// word address / 64
	u8 psm;
	u8 zmsk;
} zbufInfo;

typedef struct {
	int fba;
} fbaInfo;

typedef struct {
	int mode;
	int regn;
	u64 regs;
	tagInfo tag;
} pathInfo;

typedef struct {
	Vertex gsvertex[3];
	u32 rgba;
	float q;
	Vertex vertexregs;

	int primC;		// number of verts current storing
	int primIndex;	// current prim index
	int nTriFanVert;

	int prac;
	int dthe;
	int colclamp;
	int fogcol;
	int smask;
	int pabe;
	u64 buff[2];
	int buffsize;
	int cbp[2];		// internal cbp registers

	u32 CSRw;

	primInfo _prim[2];
	bufInfo srcbuf, srcbufnew;
	bufInfo dstbuf, dstbufnew;

	clutInfo clut;

	texaInfo texa;
	trxposInfo trxpos, trxposnew;

	int imageWtemp, imageHtemp;

	int imageTransfer;
	int imageWnew, imageHnew, imageX, imageY, imageEndX, imageEndY;

	pathInfo path1;
	pathInfo path2;
	pathInfo path3;

} GSinternal;

extern GSinternal gs;

extern FILE *gsLog;

void __Log(const char *fmt, ...);
void __LogToConsole(const char *fmt, ...);

void LoadConfig();
void SaveConfig();

extern void (*GSirq)();

void *SysLoadLibrary(char *lib);		// Loads Library
void *SysLoadSym(void *lib, char *sym);	// Loads Symbol from Library
char *SysLibError();					// Gets previous error loading sysbols
void SysCloseLibrary(void *lib);		// Closes Library
void SysMessage(char *fmt, ...);

extern "C" void * memcpy_amd(void *dest, const void *src, size_t n);
extern "C" u8 memcmp_mmx(const void *dest, const void *src, int n);

template <typename T>
class CInterfacePtr
{
public:
	inline CInterfacePtr() : ptr(NULL) {}
	inline explicit CInterfacePtr(T* newptr) : ptr(newptr) { if ( ptr != NULL ) ptr->AddRef(); }
	inline ~CInterfacePtr() { if( ptr != NULL ) ptr->Release(); }

	inline T* operator* () { assert( ptr != NULL); return *ptr; }
	inline T* operator->() { return ptr; }
	inline T* get() { return ptr; }

	inline void release() {
		if( ptr != NULL ) { ptr->Release(); ptr = NULL; }
	}

	inline operator T*() { return ptr; }

	inline bool operator==(T* rhs) { return ptr == rhs; }
	inline bool operator!=(T* rhs) { return ptr != rhs; }

	inline CInterfacePtr& operator= (T* newptr) {
		if( ptr != NULL ) ptr->Release();
		ptr = newptr;

		if( ptr != NULL ) ptr->AddRef();
		return *this;
	}

private:
	T* ptr;
};

#define RGBA32to16(c) \
	(u16)((((c) & 0x000000f8) >>  3) | \
	(((c) & 0x0000f800) >>  6) | \
	(((c) & 0x00f80000) >>  9) | \
	(((c) & 0x80000000) >> 16)) \

#define RGBA16to32(c) \
	(((c) & 0x001f) <<  3) | \
	(((c) & 0x03e0) <<  6) | \
	(((c) & 0x7c00) <<  9) | \
	(((c) & 0x8000) ? 0xff000000 : 0) \

// converts float16 [0,1] to BYTE [0,255] (assumes value is in range, otherwise will take lower 8bits)
// f is a u16
__forceinline u16 Float16ToBYTE(u16 f) {
	//assert( !(f & 0x8000) );
	if( f & 0x8000 ) return 0;

	u16 d = ((((f&0x3ff)|0x400)*255)>>(10-((f>>10)&0x1f)+15));
	return d > 255 ? 255 : d;
}

__forceinline u16 Float16ToALPHA(u16 f) {
	//assert( !(f & 0x8000) );
	if( f & 0x8000 ) return 0;

	// round up instead of down (crash and burn), too much and charlie breaks
	u16 d = (((((f&0x3ff)|0x400))*255)>>(10-((f>>10)&0x1f)+15));
	d = (d)>>1;
	return d > 255 ? 255 : d;
}

#ifndef COLOR_ARGB
#define COLOR_ARGB(a,r,g,b) \
	((u32)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#endif

// assumes that positive in [1,2] (then extracts fraction by just looking at the specified bits)
#define Float16ToBYTE_2(f) ((u8)(*(u16*)&f>>2))
#define Float16To5BIT(f) (Float16ToBYTE(f)>>3)

#define Float16Alpha(f) (((*(u16*)&f&0x7c00)>=0x3900)?0x8000:0) // alpha is >= 1

// converts an array of 4 u16s to a u32 color
// f is a pointer to a u16
#define Float16ToARGB(f) COLOR_ARGB(Float16ToALPHA(f.w), Float16ToBYTE(f.x), Float16ToBYTE(f.y), Float16ToBYTE(f.z));

#define Float16ToARGB16(f) (Float16Alpha(f.w)|(Float16To5BIT(f.x)<<10)|(Float16To5BIT(f.y)<<5)|Float16To5BIT(f.z))

// used for Z values
#define Float16ToARGB_Z(f) COLOR_ARGB((u32)Float16ToBYTE_2(f.w), Float16ToBYTE_2(f.x), Float16ToBYTE_2(f.y), Float16ToBYTE_2(f.z))
#define Float16ToARGB16_Z(f) ((Float16ToBYTE_2(f.y)<<8)|Float16ToBYTE_2(f.z))


inline float Clamp(float fx, float fmin, float fmax)
{
	if( fx < fmin ) return fmin;
	return fx > fmax ? fmax : fx;
}

#include <string>

extern std::string s_strIniPath;

#endif
