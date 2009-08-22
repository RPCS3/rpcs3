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

// Super VU recompiler - author: zerofrog(@gmail.com)

#include "PrecompiledHeader.h"

#include <float.h>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#ifndef _WIN32
#include <sys/types.h>
#endif

#include "Common.h"

#include "GS.h"
#include "R5900.h"
#include "VU.h"
#include "iR5900.h"

#include "sVU_zerorec.h"
#include "SamplProf.h"
#include "NakedAsm.h"

using namespace std;

// temporary externs
extern void iDumpVU0Registers();
extern void iDumpVU1Registers();

// SuperVURec optimization options, uncomment only for debugging purposes
#define SUPERVU_CACHING			// vu programs are saved and queried via memcompare (should be no reason to disable this)
#define SUPERVU_WRITEBACKS			// don't flush the writebacks after every block
#define SUPERVU_X86CACHING			// use x86reg caching (faster) (not really. rather lots slower :p (rama) )
#define SUPERVU_VIBRANCHDELAY  		 // when integers are modified right before a branch that uses the integer,
								// the old integer value is used in the branch, fixes kh2

#define SUPERVU_PROPAGATEFLAGS  // the correct behavior of VUs, for some reason superman breaks gfx with it on...

// registers won't be flushed at block boundaries (faster) (nothing noticable speed-wise, causes SPS in Ratchet and clank (Nneeve) )
#ifndef PCSX2_DEBUG
//#define SUPERVU_INTERCACHING	
#endif

#define SUPERVU_CHECKCONDITION 0 // has to be 0!!

#define VU_EXESIZE 0x00800000

#define _Imm11_ 	(s32)( (vucode & 0x400) ? (0xfffffc00 | (vucode & 0x3ff)) : (vucode & 0x3ff) )
#define _UImm11_	(s32)(vucode & 0x7ff)

#define _Ft_ ((VU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((VU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((VU->code >>  6) & 0x1F)  // The sa part of the instruction register
#define _It_ (_Ft_ & 15)
#define _Is_ (_Fs_ & 15)
#define _Id_ (_Fd_ & 15)

static const u32 QWaitTimes[] = { 6, 12 };
static const u32 PWaitTimes[] = { 53, 43, 28, 23, 17, 11, 10 };

static u32 s_vuInfo; // info passed into rec insts

static const u32 s_MemSize[2] = {VU0_MEMSIZE, VU1_MEMSIZE};
static u8* s_recVUMem = NULL, *s_recVUPtr = NULL;

// tables which are defined at the bottom of this massive file.
extern void (*recVU_UPPER_OPCODE[64])(VURegs* VU, s32 info);
extern void (*recVU_LOWER_OPCODE[128])(VURegs* VU, s32 info);

#define INST_Q_READ			0x0001 // flush Q
#define INST_P_READ			0x0002 // flush P
#define INST_BRANCH_DELAY	0x0004
#define INST_CLIP_WRITE		0x0040 // inst writes CLIP in the future
#define INST_STATUS_WRITE	0x0080
#define INST_MAC_WRITE		0x0100
#define INST_Q_WRITE		0x0200
#define INST_CACHE_VI       0x0400 // write old vi value to s_VIBranchDelay

// Let's tempt fate by defining two different constants with almost identical names
#define INST_DUMMY_			0x8000
#define INST_DUMMY				0x83c0

#define VFFREE_INVALID0     0x80000000 // (vffree[i]&0xf) is invalid

//#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))

#ifdef PCSX2_DEBUG
u32 s_vucount = 0;

static u32 g_vu1lastrec = 0, skipparent = -1;
static u32 s_svulast = 0, s_vufnheader;
static u32 badaddrs[][2] = {0, 0xffff};
#endif

union VURecRegs
{
	struct
	{
		u16 reg;
		u16 type;
	};
	u32 id;
};

#define SUPERVU_XGKICKDELAY 1 // yes this is needed as default (wipeout)

class VuBaseBlock;

struct VuFunctionHeader
{
	struct RANGE
	{
		RANGE() : pmem(NULL) {}

		u16 start, size;
		void* pmem; // all the mem
	};

	VuFunctionHeader() : pprogfunc(NULL), startpc(0xffffffff) {}
	~VuFunctionHeader()
	{
		for (vector<RANGE>::iterator it = ranges.begin(); it != ranges.end(); ++it)
		{
			free(it->pmem);
		}
	}

	// returns true if the checksum for the current mem is the same as this fn
	bool IsSame(void* pmem);

	u32 startpc;
	void* pprogfunc;

	vector<RANGE> ranges;
};

struct VuBlockHeader
{
	VuBaseBlock* pblock;
	u32 delay;
};

// one vu inst (lower and upper)
class VuInstruction
{
	public:
		VuInstruction()
		{
			memzero_obj(*this);
			nParentPc = -1;
			vicached = -1;
		}

		int nParentPc; // used for syncing with flag writes, -1 for no parent

		_vuopinfo info;

		_VURegsNum regs[2]; // [0] - lower, [1] - upper
		u32 livevars[2]; // live variables right before this inst, [0] - inst, [1] - float
		u32 addvars[2]; // live variables to add
		u32 usedvars[2]; // set if var is used in the future including vars used in this inst
		u32 keepvars[2];
		u16 pqcycles; // the number of cycles to stall if function writes to the regs
		u16 type; // INST_

		u32 pClipWrite, pMACWrite, pStatusWrite; // addrs to write the flags
		u32 vffree[2];
		s8 vfwrite[2], vfread0[2], vfread1[2], vfacc[2];
		s8 vfflush[2]; // extra flush regs
		s8 vicached; // if >= 0, then use the cached integer s_VIBranchDelay
		VuInstruction *pPrevInst;

		int SetCachedRegs(int upper, u32 vuxyz);
		void Recompile(list<VuInstruction>::iterator& itinst, u32 vuxyz);
};

enum BlockType
{
	BLOCKTYPE_EOP = 0x01, // at least one of the children of the block contains eop (or the block itself)
	BLOCKTYPE_FUNCTION = 0x02,
	BLOCKTYPE_HASEOP = 0x04, // last inst of block is an eop
	BLOCKTYPE_MACFLAGS = 0x08,
	BLOCKTYPE_ANALYZED = 0x40,
	BLOCKTYPE_IGNORE = 0x80, // special for recursive fns
	BLOCKTYPE_ANALYZEDPARENT = 0x100
};

// base block used when recompiling
class VuBaseBlock
{
	public:
		typedef list<VuBaseBlock*> LISTBLOCKS;

		VuBaseBlock();

		// returns true if the leads to a EOP (ALL VU blocks must ret true)
		void AssignVFRegs();
		void AssignVIRegs(int parent);

		list<VuInstruction>::iterator GetInstIterAtPc(int instpc);
		void GetInstsAtPc(int instpc, list<VuInstruction*>& listinsts);

		void Recompile();

		u16 type; // BLOCKTYPE_
		u16 id;
		u16 startpc;
		u16 endpc; // first inst not in block
		void* pcode; // x86 code pointer
		void* pendcode; // end of the x86 code pointer
		int cycles;
		list<VuInstruction> insts;
		list<VuBaseBlock*> parents;
		LISTBLOCKS blocks; // blocks branches to
		u32* pChildJumps[4]; // addrs that need to be filled with the children's start addrs
		// if highest bit is set, addr needs to be relational
		u32 vuxyz; // corresponding bit is set if reg's xyz channels are used only
		u32 vuxy; // corresponding bit is set if reg's xyz channels are used only

		_xmmregs startregs[iREGCNT_XMM], endregs[iREGCNT_XMM];
		int nStartx86, nEndx86; // indices into s_vecRegArray

		int allocX86Regs;
		int prevFlagsOutOfBlock;
};

struct WRITEBACK
{
	WRITEBACK() : nParentPc(0), cycle(0) //, pStatusWrite(NULL), pMACWrite(NULL)
	{
		viwrite[0] = viwrite[1] = 0;
		viread[0] = viread[1] = 0;
	}

	void InitInst(VuInstruction* pinst, int cycle) const
	{
		u32 write = viwrite[0] | viwrite[1];
		pinst->type = ((write & (1 << REG_CLIP_FLAG)) ? INST_CLIP_WRITE : 0) |
		              ((write & (1 << REG_MAC_FLAG)) ? INST_MAC_WRITE : 0) |
		              ((write & (1 << REG_STATUS_FLAG)) ? INST_STATUS_WRITE : 0) |
		              ((write & (1 << REG_Q)) ? INST_Q_WRITE : 0);
		pinst->nParentPc = nParentPc;
		pinst->info.cycle = cycle;
		for (int i = 0; i < 2; ++i)
		{
			pinst->regs[i].VIwrite = viwrite[i];
			pinst->regs[i].VIread = viread[i];
		}
	}

	static int SortWritebacks(const WRITEBACK& w1, const WRITEBACK& w2)
	{
		return w1.cycle < w2.cycle;
	}

	int nParentPc;
	int cycle;
	u32 viwrite[2];
	u32 viread[2];
};

struct VUPIPELINES
{
	fmacPipe fmac[8];
	fdivPipe fdiv;
	efuPipe efu;
	ialuPipe ialu[8];
	list< WRITEBACK > listWritebacks;
};

VuBaseBlock::VuBaseBlock()
{
	type = 0;
	endpc = 0;
	cycles = 0;
	pcode = NULL;
	id = 0;
	memzero_obj(pChildJumps);
	memzero_obj(startregs);
	memzero_obj(endregs);
	allocX86Regs = nStartx86 = nEndx86 = -1;
	prevFlagsOutOfBlock = 0;
}

#define SUPERVU_STACKSIZE 0x1000

static list<VuFunctionHeader*> s_listVUHeaders[2];
static list<VuFunctionHeader*>* s_plistCachedHeaders[2] = {NULL, NULL};
static VuFunctionHeader** recVUHeaders[2] = {NULL, NULL};
static VuBlockHeader* recVUBlocks[2] = {NULL, NULL};
static u8* recVUStack = NULL, *recVUStackPtr = NULL;
static vector<_x86regs> s_vecRegArray(128);

static VURegs* VU = NULL;
static list<VuBaseBlock*> s_listBlocks;
static u32 s_vu = 0;
static u32 s_UnconditionalDelay = 0; // 1 if there are two sequential branches and the last is unconditional
static u32 g_nLastBlockExecuted = 0;

static VuFunctionHeader* SuperVURecompileProgram(u32 startpc, int vuindex);
static VuBaseBlock* SuperVUBuildBlocks(VuBaseBlock* parent, u32 startpc, const VUPIPELINES& pipes);
static void SuperVUInitLiveness(VuBaseBlock* pblock);
static void SuperVULivenessAnalysis();
static void SuperVUEliminateDeadCode();
static void SuperVUAssignRegs();

//void SuperVUFreeXMMreg(int xmmreg, int xmmtype, int reg);
#define SuperVUFreeXMMreg 0&&
void SuperVUFreeXMMregs(u32* livevars);

static u32* SuperVUStaticAlloc(u32 size);
static void SuperVURecompile();

// allocate VU resources
void SuperVUAlloc(int vuindex)
{
	// The old -1 crap has been depreciated on this function.  Please
	// specify either 0 or 1, thanks.
	jASSUME(vuindex >= 0);

	// upper 4 bits must be zero!
	if (s_recVUMem == NULL)
	{
		// upper 4 bits must be zero!
		// Changed "first try base" to 0xf1e0000, since 0x0c000000 liked to fail a lot. (cottonvibes)
		s_recVUMem = SysMmapEx(0xf1e0000, VU_EXESIZE, 0x10000000, "SuperVUAlloc");
		
		if (s_recVUMem == NULL)
		{
			throw Exception::OutOfMemory(
			    fmt_string("SuperVU Error > failed to allocate recompiler memory (addr: 0x%x)", (u32)s_recVUMem)
			);
		}

		ProfilerRegisterSource("VURec", s_recVUMem, VU_EXESIZE);

		if (recVUStack == NULL) recVUStack = new u8[SUPERVU_STACKSIZE * 4];
	}

	if (vuindex >= 0)
	{
		jASSUME(s_recVUMem != NULL);

		if (recVUHeaders[vuindex] == NULL)
			recVUHeaders[vuindex] = new VuFunctionHeader* [s_MemSize[vuindex] / 8];
		if (recVUBlocks[vuindex] == NULL)
			recVUBlocks[vuindex] = new VuBlockHeader[s_MemSize[vuindex] / 8];
		if (s_plistCachedHeaders[vuindex] == NULL)
			s_plistCachedHeaders[vuindex] = new list<VuFunctionHeader*>[s_MemSize[vuindex] / 8];
	}
}

void DestroyCachedHeaders(int vuindex, int j)
{
	list<VuFunctionHeader*>::iterator it = s_plistCachedHeaders[vuindex][j].begin();
				
	while (it != s_plistCachedHeaders[vuindex][j].end())
	{
		delete *it;
		it++;
	}
	
	s_plistCachedHeaders[vuindex][j].clear();			
}

void DestroyVUHeaders(int vuindex)
{
	list<VuFunctionHeader*>::iterator it = s_listVUHeaders[vuindex].begin();
				
		while (it != s_listVUHeaders[vuindex].end())
		{
			delete *it;
			it++;
		}
		
	s_listVUHeaders[vuindex].clear();			
}

// destroy VU resources
void SuperVUDestroy(int vuindex)
{
	list<VuFunctionHeader*>::iterator it;

	if (vuindex < 0)
	{
		SuperVUDestroy(0);
		SuperVUDestroy(1);
		ProfilerTerminateSource("VURec");
		SafeSysMunmap(s_recVUMem, VU_EXESIZE);
		safe_delete_array(recVUStack);
	}
	else
	{
		safe_delete_array(recVUHeaders[vuindex]);
		safe_delete_array(recVUBlocks[vuindex]);

		if (s_plistCachedHeaders[vuindex] != NULL)
		{
			for (u32 j = 0; j < s_MemSize[vuindex] / 8; ++j)
			{
				DestroyCachedHeaders(vuindex, j);
			}
			safe_delete_array(s_plistCachedHeaders[vuindex]);
		}
		DestroyVUHeaders(vuindex);
	}
}

// reset VU
void SuperVUReset(int vuindex)
{
#ifdef PCSX2_DEBUG
	s_vucount = 0;
#endif

	if (s_recVUMem == NULL)
		return;

	//jASSUME( s_recVUMem != NULL );

	if (vuindex < 0)
	{
		DbgCon::Status("SuperVU reset > Resetting recompiler memory and structures.");

		// Does this cause problems on VU recompiler resets?  It could, if the VU works like
		// the EE used to, and actually tries to re-enter the recBlock after issuing a clear. (air)

		//memset_8<0xcd, VU_EXESIZE>(s_recVUMem);
		memzero_ptr<SUPERVU_STACKSIZE>(recVUStack);

		s_recVUPtr = s_recVUMem;
	}
	else
	{
		DbgCon::Status("SuperVU reset [VU%d] > Resetting the recs and junk", params vuindex);
		list<VuFunctionHeader*>::iterator it;
		if (recVUHeaders[vuindex]) memset(recVUHeaders[vuindex], 0, sizeof(VuFunctionHeader*) * (s_MemSize[vuindex] / 8));
		if (recVUBlocks[vuindex]) memset(recVUBlocks[vuindex], 0, sizeof(VuBlockHeader) * (s_MemSize[vuindex] / 8));

		if (s_plistCachedHeaders[vuindex] != NULL)
		{
			for (u32 j = 0; j < s_MemSize[vuindex] / 8; ++j)
			{
				DestroyCachedHeaders(vuindex, j);
			}
		}
		DestroyVUHeaders(vuindex);
	}
}

// clear the block and any joining blocks
void __fastcall SuperVUClear(u32 startpc, u32 size, int vuindex)
{
	vector<VuFunctionHeader::RANGE>::iterator itrange;
	list<VuFunctionHeader*>::iterator it = s_listVUHeaders[vuindex].begin();
	u32 endpc = startpc + (size + (8 - (size & 7))); // Adding this code to ensure size is always a multiple of 8, it can be simplified to startpc+size if size is always a multiple of 8 (cottonvibes)
	while (it != s_listVUHeaders[vuindex].end())
	{

		// for every fn, check if it has code in the range
		for(itrange = (*it)->ranges.begin(); itrange != (*it)->ranges.end(); itrange++)
		{
			if (startpc < (u32)itrange->start + itrange->size && itrange->start < endpc)
				break;
		}

		if (itrange != (*it)->ranges.end())
		{
			recVUHeaders[vuindex][(*it)->startpc/8] = NULL;
#ifdef SUPERVU_CACHING
			list<VuFunctionHeader*>* plist = &s_plistCachedHeaders[vuindex][(*it)->startpc/8];
			plist->push_back(*it);
			if (plist->size() > 30)
			{
				// list is too big, delete
				//Console::Notice("Performance warning: deleting cached VU program!");
				delete plist->front();
				plist->pop_front();
			}
			it = s_listVUHeaders[vuindex].erase(it);
#else
			delete *it;
			it = s_listVUHeaders[vuindex].erase(it);
#endif
		}
		else ++it;
	}
}

static VuFunctionHeader* s_pFnHeader = NULL;
static VuBaseBlock* s_pCurBlock = NULL;
static VuInstruction* s_pCurInst = NULL;
static u32 s_StatusRead = 0, s_MACRead = 0, s_ClipRead = 0; // read addrs
static u32 s_PrevStatusWrite = 0, s_PrevMACWrite = 0, s_PrevClipWrite = 0, s_PrevIWrite = 0;
static u32 s_WriteToReadQ = 0;

static u32 s_VIBranchDelay = 0; //Value of register to use in a vi branch delayed situation


u32 s_TotalVUCycles; // total cycles since start of program execution


u32 SuperVUGetVIAddr(int reg, int read)
{
	assert(s_pCurInst != NULL);

	switch (reg)
	{
		case REG_STATUS_FLAG:
			{
				u32 addr = (read == 2) ? s_PrevStatusWrite : (read ? s_StatusRead : s_pCurInst->pStatusWrite);
				assert(!read || addr != 0);
				return addr;
			}
		case REG_MAC_FLAG:
			{
				u32 addr = (read == 2) ? s_PrevMACWrite : (read ? s_MACRead : s_pCurInst->pMACWrite);
				return addr;
			}
		case REG_CLIP_FLAG:
			{
				u32 addr = (read == 2) ? s_PrevClipWrite : (read ? s_ClipRead : s_pCurInst->pClipWrite);
				assert(!read || addr != 0);
				return addr;
			}
		case REG_Q:
			return (read || s_WriteToReadQ) ? (uptr)&VU->VI[REG_Q] : (uptr)&VU->q;
		case REG_P:
			return read ? (uptr)&VU->VI[REG_P] : (uptr)&VU->p;
		case REG_I:
			return s_PrevIWrite;
	}

#ifdef SUPERVU_VIBRANCHDELAY
	if ((read != 0) && (s_pCurInst->regs[0].pipe == VUPIPE_BRANCH) && (s_pCurInst->vicached >= 0) && (s_pCurInst->vicached == reg))
	{
		return (uptr)&s_VIBranchDelay; // test for branch delays
	}
#endif

	return (uptr)&VU->VI[reg];
}

void SuperVUDumpBlock(list<VuBaseBlock*>& blocks, int vuindex)
{
	FILE *f;
	char str[256];
	u32 *mem;
	u32 i;

	Path::CreateDirectory("dumps");
	string filename(Path::Combine("dumps", fmt_string("svu%cdump%.4X.txt", s_vu ? '0' : '1', s_pFnHeader->startpc)));
	//Console::WriteLn( "dump1 %x => %s", params s_pFnHeader->startpc, filename );

	f = fopen(filename.c_str(), "w");

	fprintf(f, "Format: upper_inst lower_inst\ntype f:vf_live_vars vf_used_vars i:vi_live_vars vi_used_vars inst_cycle pq_inst\n");
	fprintf(f, "Type: %.2x - qread, %.2x - pread, %.2x - clip_write, %.2x - status_write\n"
	        "%.2x - mac_write, %.2x -qflush\n",
	        INST_Q_READ, INST_P_READ, INST_CLIP_WRITE, INST_STATUS_WRITE, INST_MAC_WRITE, INST_Q_WRITE);
	fprintf(f, "XMM: Upper: read0 read1 write acc temp; Lower: read0 read1 write acc temp\n\n");

	list<VuBaseBlock*>::iterator itblock;
	list<VuInstruction>::iterator itinst;
	VuBaseBlock::LISTBLOCKS::iterator itchild;

	for(itblock = blocks.begin(); itblock != blocks.end(); itblock++)
	{
		fprintf(f, "block:%c %x-%x; children: ", ((*itblock)->type&BLOCKTYPE_HASEOP) ? '*' : ' ',
		        (*itblock)->startpc, (*itblock)->endpc - 8);
		
		for(itchild = (*itblock)->blocks.begin(); itchild != (*itblock)->blocks.end(); itchild++)
		{
			fprintf(f, "%x ", (*itchild)->startpc);
		}
		fprintf(f, "; vuxyz = %x, vuxy = %x\n", (*itblock)->vuxyz&(*itblock)->insts.front().usedvars[1],
		        (*itblock)->vuxy&(*itblock)->insts.front().usedvars[1]);

		itinst = (*itblock)->insts.begin();
		i = (*itblock)->startpc;
		while (itinst != (*itblock)->insts.end())
		{
			assert(i <= (*itblock)->endpc);
			if (itinst->type & INST_DUMMY)
			{
				if (itinst->nParentPc >= 0 && !(itinst->type&INST_DUMMY_))
				{
					// search for the parent
					fprintf(f, "writeback 0x%x (%x)\n", itinst->type, itinst->nParentPc);
				}
			}
			else
			{
				mem = (u32*) & VU->Micro[i];
				char* pstr = disVU1MicroUF(mem[1], i + 4);
				fprintf(f, "%.4x: %-40s",  i, pstr);
				if (mem[1] & 0x80000000) fprintf(f, " I=%f(%.8x)\n", *(float*)mem, mem[0]);
				else fprintf(f, "%s\n", disVU1MicroLF(mem[0], i));
				i += 8;
			}

			++itinst;
		}

		fprintf(f, "\n");

		_x86regs* pregs;
		if ((*itblock)->nStartx86 >= 0 || (*itblock)->nEndx86 >= 0)
		{
			fprintf(f, "X86: AX CX DX BX SP BP SI DI\n");
		}

		if ((*itblock)->nStartx86 >= 0)
		{
			pregs = &s_vecRegArray[(*itblock)->nStartx86];
			fprintf(f, "STR: ");
			for (i = 0; i < iREGCNT_GPR; ++i)
			{
				if (pregs[i].inuse) 
					fprintf(f, "%.2d ", pregs[i].reg);
				else 
					fprintf(f, "-1 ");
			}
			fprintf(f, "\n");
		}

		if ((*itblock)->nEndx86 >= 0)
		{
			fprintf(f, "END: ");
			pregs = &s_vecRegArray[(*itblock)->nEndx86];
			for (i = 0; i < iREGCNT_GPR; ++i)
			{
				if (pregs[i].inuse) 
					fprintf(f, "%.2d ", pregs[i].reg);
				else 
					fprintf(f, "-1 ");
			}
			fprintf(f, "\n");
		}

		itinst = (*itblock)->insts.begin();
		for (i = (*itblock)->startpc; i < (*itblock)->endpc; ++itinst)
		{

			if (itinst->type & INST_DUMMY)
			{
			}
			else
			{
				sprintf(str, "%.4x:%x f:%.8x_%.8x", i, itinst->type, itinst->livevars[1], itinst->usedvars[1]);
				fprintf(f, "%-46s i:%.8x_%.8x c:%d pq:%d\n", str,
				        itinst->livevars[0], itinst->usedvars[0], (int)itinst->info.cycle, (int)itinst->pqcycles);

				sprintf(str, "XMM r0:%d r1:%d w:%d a:%d t:%x;",
				        itinst->vfread0[1], itinst->vfread1[1], itinst->vfwrite[1], itinst->vfacc[1], itinst->vffree[1]);
				fprintf(f, "%-46s r0:%d r1:%d w:%d a:%d t:%x\n", str,
				        itinst->vfread0[0], itinst->vfread1[0], itinst->vfwrite[0], itinst->vfacc[0], itinst->vffree[0]);
				i += 8;
			}
		}

#ifdef __LINUX__
		// dump the asm
		if ((*itblock)->pcode != NULL)
		{
			char command[255];
			FILE* fasm = fopen("mydump1", "wb");
			//Console::WriteLn("writing: %x, %x", params (*itblock)->startpc, (uptr)(*itblock)->pendcode - (uptr)(*itblock)->pcode);
			fwrite((*itblock)->pcode, 1, (uptr)(*itblock)->pendcode - (uptr)(*itblock)->pcode, fasm);
			fclose(fasm);
			sprintf(command, "objdump -D --target=binary --architecture=i386 -M intel mydump1 > tempdump");
			system(command);
			fasm = fopen("tempdump", "r");
			// read all of it and write it to f
			fseek(fasm, 0, SEEK_END);
			vector<char> vbuffer(ftell(fasm));
			fseek(fasm, 0, SEEK_SET);
			fread(&vbuffer[0], vbuffer.size(), 1, fasm);

			fprintf(f, "\n\n");
			fwrite(&vbuffer[0], vbuffer.size(), 1, f);
			fclose(fasm);
		}
#endif

		fprintf(f, "\n---------------\n");
	}

	fclose(f);
}

// uncomment to count svu exec time
//#define SUPERVU_COUNT

// Private methods
void* SuperVUGetProgram(u32 startpc, int vuindex)
{
	assert(startpc < s_MemSize[vuindex]);
	assert((startpc % 8) == 0);
	assert(recVUHeaders[vuindex] != NULL);
	VuFunctionHeader** pheader = &recVUHeaders[vuindex][startpc/8];

	if (*pheader == NULL)
	{

#ifdef SUPERVU_CACHING
		void* pmem = (vuindex & 1) ? VU1.Micro : VU0.Micro;
		// check if program exists in cache
		list<VuFunctionHeader*>::iterator it;
		for(it = s_plistCachedHeaders[vuindex][startpc/8].begin(); it != s_plistCachedHeaders[vuindex][startpc/8].end(); it++)
		{
			if ((*it)->IsSame(pmem))
			{
				// found, transfer to regular lists
				void* pfn = (*it)->pprogfunc;
				recVUHeaders[vuindex][startpc/8] = *it;
				s_listVUHeaders[vuindex].push_back(*it);
				s_plistCachedHeaders[vuindex][startpc/8].erase(it);
				return pfn;
			}
		}
#endif

		*pheader = SuperVURecompileProgram(startpc, vuindex);

		if (*pheader == NULL)
		{
			assert(s_TotalVUCycles > 0);
			if (vuindex)
				VU1.VI[REG_TPC].UL = startpc;
			else 
				VU0.VI[REG_TPC].UL = startpc;
			
			return (void*)SuperVUEndProgram;
		}

		assert((*pheader)->pprogfunc != NULL);
	}
	//else assert( (*pheader)->IsSame((vuindex&1) ? VU1.Micro : VU0.Micro) );

	assert((*pheader)->startpc == startpc);

	return (*pheader)->pprogfunc;
}

bool VuFunctionHeader::IsSame(void* pmem)
{
#ifdef SUPERVU_CACHING
	vector<RANGE>::iterator it;
	for(it = ranges.begin(); it != ranges.end(); it++)
	{
		if (memcmp_mmx((u8*)pmem + it->start, it->pmem, it->size))
			return false;
	}
#endif
	return true;
}

list<VuInstruction>::iterator VuBaseBlock::GetInstIterAtPc(int instpc)
{
	assert(instpc >= 0);

	u32 curpc = startpc;
	list<VuInstruction>::iterator it;
	for (it = insts.begin(); it != insts.end(); ++it)
	{
		if (it->type & INST_DUMMY) continue;
		if (curpc == instpc) break;
		curpc += 8;
	}

	if (it != insts.end()) return it;

	assert(0);
	return insts.begin();
}

void VuBaseBlock::GetInstsAtPc(int instpc, list<VuInstruction*>& listinsts)
{
	assert(instpc >= 0);

	listinsts.clear();

	u32 curpc = startpc;
	list<VuInstruction>::iterator it;
	for (it = insts.begin(); it != insts.end(); ++it)
	{
		if (it->type & INST_DUMMY) continue;
		if (curpc == instpc) break;
		curpc += 8;
	}

	if (it != insts.end())
	{
		listinsts.push_back(&(*it));
		return;
	}

	// look for the pc in other blocks
	for (list<VuBaseBlock*>::iterator itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); ++itblock)
	{
		if (*itblock == this) continue;

		if (instpc >= (*itblock)->startpc && instpc < (*itblock)->endpc)
		{
			listinsts.push_back(&(*(*itblock)->GetInstIterAtPc(instpc)));
		}
	}

	assert(listinsts.size() > 0);
}

static VuFunctionHeader* SuperVURecompileProgram(u32 startpc, int vuindex)
{
	assert(vuindex < 2);
	assert(s_recVUPtr != NULL);
	//Console::WriteLn("svu%c rec: %x", params '0'+vuindex, startpc);

	// if recPtr reached the mem limit reset whole mem
	if (((uptr)s_recVUPtr - (uptr)s_recVUMem) >= VU_EXESIZE - 0x40000)
	{
		//Console::WriteLn("SuperVU reset mem");
		SuperVUReset(0);
		SuperVUReset(1);
		SuperVUReset(-1);
		if (s_TotalVUCycles > 0)
		{
			// already executing, so return NULL
			return NULL;
		}
	}

	list<VuBaseBlock*>::iterator itblock;

	s_vu = vuindex;
	VU = s_vu ? &VU1 : &VU0;
	s_pFnHeader = new VuFunctionHeader();
	s_listVUHeaders[vuindex].push_back(s_pFnHeader);
	s_pFnHeader->startpc = startpc;

	memset(recVUBlocks[s_vu], 0, sizeof(VuBlockHeader) * (s_MemSize[s_vu] / 8));

	// analyze the global graph
	s_listBlocks.clear();
	VUPIPELINES pipes;
	memzero_obj(pipes.fmac);
	memzero_obj(pipes.fdiv);
	memzero_obj(pipes.efu);
	memzero_obj(pipes.ialu);
	SuperVUBuildBlocks(NULL, startpc, pipes);

	// fill parents
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		for(itchild = (*itblock)->blocks.begin(); itchild != (*itblock)->blocks.end(); itchild++)
		{
			(*itchild)->parents.push_back(*itblock);
		}

		//(*itblock)->type &= ~(BLOCKTYPE_IGNORE|BLOCKTYPE_ANALYZED);
	}

	assert(s_listBlocks.front()->startpc == startpc);
	s_listBlocks.front()->type |= BLOCKTYPE_FUNCTION;

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		SuperVUInitLiveness(*itblock);
	}

	SuperVULivenessAnalysis();
	SuperVUEliminateDeadCode();
	SuperVUAssignRegs();

#ifdef PCSX2_DEBUG
	if ((s_vu && (vudump&1)) || (!s_vu && (vudump&16))) SuperVUDumpBlock(s_listBlocks, s_vu);
#endif

	// code generation
	x86SetPtr(s_recVUPtr);
	branch = 0;

	SuperVURecompile();

	s_recVUPtr = x86Ptr;

	// set the function's range
	VuFunctionHeader::RANGE r;
	s_pFnHeader->ranges.reserve(s_listBlocks.size());

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		r.start = (*itblock)->startpc;
		r.size = (*itblock)->endpc - (*itblock)->startpc;
#ifdef SUPERVU_CACHING
		//memxor_mmx(r.checksum, &VU->Micro[r.start], r.size);
		r.pmem = malloc(r.size);
		memcpy_fast(r.pmem, &VU->Micro[r.start], r.size);
#endif
		s_pFnHeader->ranges.push_back(r);
	}

#if defined(PCSX2_DEBUG) && defined(__LINUX__)
	// dump at the end to capture the actual code
	if ((s_vu && (vudump&1)) || (!s_vu && (vudump&16))) SuperVUDumpBlock(s_listBlocks, s_vu);
#endif

	// destroy
	for (list<VuBaseBlock*>::iterator itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); ++itblock)
	{
		delete *itblock;
	}
	s_listBlocks.clear();

	assert(s_recVUPtr < s_recVUMem + VU_EXESIZE);

	return s_pFnHeader;
}

static int _recbranchAddr(u32 vucode)
{
	s32 bpc = pc + (_Imm11_ << 3);
	/*
		if ( bpc < 0 ) {
			Console::WriteLn("zerorec branch warning: bpc < 0 ( %x ); Using unsigned imm11", params bpc);
			bpc = pc + (_UImm11_ << 3);
		}*/
	bpc &= (s_MemSize[s_vu] - 1);

	return bpc;
}

// return inst that flushes everything
static VuInstruction SuperVUFlushInst()
{
	VuInstruction inst;
	// don't need to read q/p
	inst.type = INST_DUMMY_;//|INST_Q_READ|INST_P_READ;
	return inst;
}

void SuperVUAddWritebacks(VuBaseBlock* pblock, const list<WRITEBACK>& listWritebacks)
{
#ifdef SUPERVU_WRITEBACKS
	// regardless of repetition, add the pipes (for selfloops)
	list<WRITEBACK>::const_iterator itwriteback = listWritebacks.begin();
	list<VuInstruction>::iterator itinst = pblock->insts.begin(), itinst2;

	while (itwriteback != listWritebacks.end())
	{
		if (itinst != pblock->insts.end() && (itinst->info.cycle < itwriteback->cycle || (itinst->type&INST_DUMMY)))
		{
			++itinst;
			continue;
		}

		itinst2 = pblock->insts.insert(itinst, VuInstruction());
		itwriteback->InitInst(&(*itinst2), vucycle);
		++itwriteback;
	}
#endif
}

#ifdef SUPERVU_VIBRANCHDELAY
static VuInstruction* getDelayInst(VuInstruction* pInst)
{
	// check for the N cycle branch delay
	// example of 2 cycles delay (monster house) :
	//     sqi vi05
	//     sqi vi05
	//     ibeq vi05, vi03
	//   The ibeq should read the vi05 before the first sqi

	//more info:

	// iaddiu vi01, 0, 1
	// ibeq vi01, 0     <- reads vi01 before the iaddiu

	// iaddiu vi01, 0, 1
	// iaddiu vi01, vi01, 1
	// iaddiu vi01, vi01, 1
	// ibeq vi01, 0     <- reads vi01 before the last two iaddiu's (so the value read is 1)

	// ilw vi02, addr
	// iaddiu vi01, 0, 1
	// ibeq vi01, vi02     <- reads current values of both vi01 and vi02 because the branch instruction stalls

	int delay = 1;
	VuInstruction* pDelayInst = NULL;
	VuInstruction* pTargetInst = pInst->pPrevInst;
	while (1)
	{
		if (pTargetInst != NULL
		        && pTargetInst->info.cycle + delay == pInst->info.cycle
		        && (pTargetInst->regs[0].pipe == VUPIPE_IALU || pTargetInst->regs[0].pipe == VUPIPE_FMAC)
		        && ((pTargetInst->regs[0].VIwrite & pInst->regs[0].VIread) & 0xffff)
		        && (delay == 1 || ((pTargetInst->regs[0].VIwrite & pInst->regs[0].VIread) & 0xffff) == ((pTargetInst->regs[0].VIwrite & pInst->pPrevInst->regs[0].VIread) & 0xffff))
		        && !(pTargetInst->regs[0].VIread&((1 << REG_STATUS_FLAG) | (1 << REG_MAC_FLAG) | (1 << REG_CLIP_FLAG))))
		{
			pDelayInst = pTargetInst;
			pTargetInst = pTargetInst->pPrevInst;
			delay++;
			if (delay == 5) //maximum delay is 4 (length of the pipeline)
			{
				DevCon::WriteLn("supervu: cycle branch delay maximum (4) is reached");
				break;
			}
		}
		else break;
	}
	if (delay > 1) DevCon::WriteLn("supervu: %d cycle branch delay detected: %x %x", params delay - 1, pc, s_pFnHeader->startpc);
	return pDelayInst;
}
#endif

static VuBaseBlock* SuperVUBuildBlocks(VuBaseBlock* parent, u32 startpc, const VUPIPELINES& pipes)
{
	// check if block already exists
	//Console::WriteLn("startpc %x", params startpc);
	startpc &= (s_vu ? 0x3fff : 0xfff);
	VuBlockHeader* pbh = &recVUBlocks[s_vu][startpc/8];

	if (pbh->pblock != NULL)
	{

		VuBaseBlock* pblock = pbh->pblock;
		list<VuInstruction>::iterator itinst;

		if (pblock->startpc == startpc)
		{
			SuperVUAddWritebacks(pblock, pipes.listWritebacks);
			return pblock;
		}

		// have to divide the blocks, pnewblock is first block
		assert(startpc > pblock->startpc);
		assert(startpc < pblock->endpc);

		u32 dummyinst = (startpc - pblock->startpc) >> 3;

		// count inst non-dummy insts
		itinst = pblock->insts.begin();
		int cycleoff = 0;

		while (dummyinst > 0)
		{
			if (itinst->type & INST_DUMMY)
				++itinst;
			else
			{
				cycleoff = itinst->info.cycle;
				++itinst;
				--dummyinst;
			}
		}

		// NOTE: still leaves insts with their writebacks in different blocks
		while (itinst->type & INST_DUMMY)
			++itinst;

		// the difference in cycles between dummy insts (naruto utlimate ninja)
		int cyclediff = 0;
		if (parent == pblock)
			cyclediff = itinst->info.cycle - cycleoff;
		cycleoff = itinst->info.cycle;

		// new block
		VuBaseBlock* pnewblock = new VuBaseBlock();
		s_listBlocks.push_back(pnewblock);

		pnewblock->startpc = startpc;
		pnewblock->endpc = pblock->endpc;
		pnewblock->cycles = pblock->cycles - cycleoff + cyclediff;

		pnewblock->blocks.splice(pnewblock->blocks.end(), pblock->blocks);
		pnewblock->insts.splice(pnewblock->insts.end(), pblock->insts, itinst, pblock->insts.end());
		pnewblock->type = pblock->type;

		// any writebacks in the next 3 cycles also belong to original block
//		for(itinst = pnewblock->insts.begin(); itinst != pnewblock->insts.end(); ) {
//			if( (itinst->type & INST_DUMMY) && itinst->nParentPc >= 0 && itinst->nParentPc < (int)startpc ) {
//
//				if( !(itinst->type & INST_Q_WRITE) )
//					pblock->insts.push_back(*itinst);
//				itinst = pnewblock->insts.erase(itinst);
//				continue;
//			}
//
//			++itinst;
//		}

		pbh = &recVUBlocks[s_vu][startpc/8];
		for (u32 inst = startpc; inst < pblock->endpc; inst += 8)
		{
			if (pbh->pblock == pblock)
				pbh->pblock = pnewblock;
			++pbh;
		}

		for(itinst = pnewblock->insts.begin(); itinst != pnewblock->insts.end(); itinst++)
		{
			itinst->info.cycle -= cycleoff;
		}

		SuperVUAddWritebacks(pnewblock, pipes.listWritebacks);

		// old block
		pblock->blocks.push_back(pnewblock);
		pblock->endpc = startpc;
		pblock->cycles = cycleoff;
		pblock->type &= BLOCKTYPE_MACFLAGS;
		//pblock->insts.push_back(SuperVUFlushInst()); //don't need

		return pnewblock;
	}

	VuBaseBlock* pblock = new VuBaseBlock();
	s_listBlocks.push_back(pblock);

	int i = 0;
	branch = 0;
	pc = startpc;
	pblock->startpc = startpc;

	// clear stalls (might be a prob)
	memcpy(VU->fmac, pipes.fmac, sizeof(pipes.fmac));
	memcpy(&VU->fdiv, &pipes.fdiv, sizeof(pipes.fdiv));
	memcpy(&VU->efu, &pipes.efu, sizeof(pipes.efu));
	memcpy(VU->ialu, pipes.ialu, sizeof(pipes.ialu));
//	memset(VU->fmac, 0, sizeof(VU->fmac));
//	memset(&VU->fdiv, 0, sizeof(VU->fdiv));
//	memset(&VU->efu, 0, sizeof(VU->efu));

	vucycle = 0;

	u8 macflags = 0;

	list< WRITEBACK > listWritebacks;
	list< WRITEBACK >::iterator itwriteback;
	list<VuInstruction>::iterator itinst;
	u32 hasSecondBranch = 0;
	u32 needFullStatusFlag = 0;

#ifdef SUPERVU_WRITEBACKS
	listWritebacks = pipes.listWritebacks;
#endif

	// first analysis pass for status flags
	while (1)
	{
		u32* ptr = (u32*) & VU->Micro[pc];
		pc += 8;
		int prevbranch = branch;

		if (ptr[1] & 0x40000000)
			branch = 1;

		if (!(ptr[1] & 0x80000000))    // not I
		{
			switch (ptr[0] >> 25)
			{
				case 0x24: // jr
				case 0x25: // jalr
				case 0x20: // B
				case 0x21: // BAL
				case 0x28: // IBEQ
				case 0x2f: // IBGEZ
				case 0x2d: // IBGTZ
				case 0x2e: // IBLEZ
				case 0x2c: // IBLTZ
				case 0x29: // IBNE
					branch = 1;
					break;

				case 0x14: // fseq
				case 0x17: // fsor
					//needFullStatusFlag = 2;
					break;

				case 0x16: // fsand
					if ((ptr[0]&0xc0))
					{
						// sometimes full sticky bits are needed (simple series 2000 - oane chapara)
						//Console::WriteLn("needSticky: %x-%x", params s_pFnHeader->startpc, startpc);
						needFullStatusFlag = 2;
					}
					break;
			}
		}

		if (prevbranch)
			break;

		if (pc >= s_MemSize[s_vu])
		{
			Console::Error("inf vu0 prog %x", params  startpc);
			break;
		}
	}

	// second full pass
	pc = startpc;
	branch = 0;
	VuInstruction* pprevinst = NULL, *pinst = NULL;

	while (1)
	{

		if (pc == s_MemSize[s_vu])
		{
			branch |= 8;
			break;
		}

		if (!branch && pbh->pblock != NULL)
		{
			pblock->blocks.push_back(pbh->pblock);
			break;
		}

		int prevbranch = branch;

		if (!prevbranch)
		{
			pbh->pblock = pblock;
		}
		else assert(prevbranch || pbh->pblock == NULL);

		pblock->insts.push_back(VuInstruction());

		pprevinst = pinst;
		pinst = &pblock->insts.back();
		pinst->pPrevInst = pprevinst;
		SuperVUAnalyzeOp(VU, &pinst->info, pinst->regs);

#ifdef SUPERVU_VIBRANCHDELAY
		if (pinst->regs[0].pipe == VUPIPE_BRANCH && pblock->insts.size() > 1)
		{

			VuInstruction* pdelayinst = getDelayInst(pinst);
			if (pdelayinst)
			{
				pdelayinst->type |= INST_CACHE_VI;

				// find the correct register
				u32 mask = pdelayinst->regs[0].VIwrite & pinst->regs[0].VIread;
				for (int i = 0; i < 16; ++i)
				{
					if (mask & (1 << i))
					{
						pdelayinst->vicached = i;
						break;
					}
				}

				pinst->vicached = pdelayinst->vicached;
			}
		}
#endif

		if (prevbranch)
		{
			if (pinst->regs[0].pipe == VUPIPE_BRANCH)
				hasSecondBranch = 1;
			pinst->type |= INST_BRANCH_DELAY;
		}

		// check write back
		for (itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end();)
		{
			if (pinst->info.cycle >= itwriteback->cycle)
			{
				itinst = pblock->insts.insert(--pblock->insts.end(), VuInstruction());
				itwriteback->InitInst(&(*itinst), pinst->info.cycle);
				itwriteback = listWritebacks.erase(itwriteback);
			}
			else ++itwriteback;
		}

		// add new writebacks
		WRITEBACK w;
		const u32 allflags = (1 << REG_CLIP_FLAG) | (1 << REG_MAC_FLAG) | (1 << REG_STATUS_FLAG);
		for (int j = 0; j < 2; ++j) w.viwrite[j] = pinst->regs[j].VIwrite & allflags;

		if (pinst->info.macflag & VUOP_WRITE) w.viwrite[1] |= (1 << REG_MAC_FLAG);
		if (pinst->info.statusflag & VUOP_WRITE) w.viwrite[1] |= (1 << REG_STATUS_FLAG);

		if ((pinst->info.macflag | pinst->info.statusflag) & VUOP_READ)
			macflags = 1;
		if (pinst->regs[0].VIread & ((1 << REG_MAC_FLAG) | (1 << REG_STATUS_FLAG)))
			macflags = 1;

//		if( pinst->regs[1].pipe == VUPIPE_FMAC && (pinst->regs[1].VFwrite==0&&!(pinst->regs[1].VIwrite&(1<<REG_ACC_FLAG))) )
//			pinst->regs[0].VIread |= (1<<REG_MAC_FLAG)|(1<<REG_STATUS_FLAG);
//		uregs->VIwrite |= lregs->VIwrite & (1<<REG_STATUS_FLAG);

		if (w.viwrite[0] | w.viwrite[1])
		{

			// only if coming from fmac pipeline
			if (((pinst->info.statusflag&VUOP_WRITE) && !(pinst->regs[0].VIwrite&(1 << REG_STATUS_FLAG))) && needFullStatusFlag)
			{
				// don't read if first inst
				if (needFullStatusFlag == 1)
					w.viread[1] |= (1 << REG_STATUS_FLAG);
				else --needFullStatusFlag;
			}

			for (int j = 0; j < 2; ++j)
			{
				w.viread[j] |= pinst->regs[j].VIread & allflags;

				if ((pinst->regs[j].VIread&(1 << REG_STATUS_FLAG)) && (pinst->regs[j].VIwrite&(1 << REG_STATUS_FLAG)))
				{
					// don't need the read anymore
					pinst->regs[j].VIread &= ~(1 << REG_STATUS_FLAG);
				}
				if ((pinst->regs[j].VIread&(1 << REG_MAC_FLAG)) && (pinst->regs[j].VIwrite&(1 << REG_MAC_FLAG)))
				{
					// don't need the read anymore
					pinst->regs[j].VIread &= ~(1 << REG_MAC_FLAG);
				}

				pinst->regs[j].VIwrite &= ~allflags;
			}

			if (pinst->info.macflag & VUOP_READ) w.viread[1] |= 1 << REG_MAC_FLAG;
			if (pinst->info.statusflag & VUOP_READ) w.viread[1] |= 1 << REG_STATUS_FLAG;

			w.nParentPc = pc - 8;
			w.cycle = pinst->info.cycle + 4;
			listWritebacks.push_back(w);
		}

		if (pinst->info.q&VUOP_READ) pinst->type |= INST_Q_READ;
		if (pinst->info.p&VUOP_READ) pinst->type |= INST_P_READ;

		if (pinst->info.q&VUOP_WRITE)
		{
			pinst->pqcycles = QWaitTimes[pinst->info.pqinst] + 1;

			memset(&w, 0, sizeof(w));
			w.nParentPc = pc - 8;
			w.cycle = pinst->info.cycle + pinst->pqcycles;
			w.viwrite[0] = 1 << REG_Q;
			listWritebacks.push_back(w);
		}
		if (pinst->info.p&VUOP_WRITE)
			pinst->pqcycles = PWaitTimes[pinst->info.pqinst] + 1;

		if (prevbranch)
		{
			break;
		}

		// make sure there is always a branch
		// sensible soccer overflows on vu0, so increase the limit...
		if ((s_vu == 1 && i >= 0x799) || (s_vu == 0 && i >= 0x201))
		{
			Console::Error("VuRec base block doesn't terminate!");
			assert(0);
			break;
		}

		i++;
		pbh++;
	}

	if (macflags)
		pblock->type |= BLOCKTYPE_MACFLAGS;

	pblock->endpc = pc;
	u32 lastpc = pc;

	pblock->cycles = vucycle;

#ifdef SUPERVU_WRITEBACKS
	if (!branch || (branch&8))
#endif
	{
		// flush writebacks
		if (listWritebacks.size() > 0)
		{
			listWritebacks.sort(WRITEBACK::SortWritebacks);
			for (itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end(); ++itwriteback)
			{
				if (itwriteback->viwrite[0] & (1 << REG_Q))
				{
					// ignore all Q writebacks
					continue;
				}

				pblock->insts.push_back(VuInstruction());
				itwriteback->InitInst(&pblock->insts.back(), vucycle);
			}

			listWritebacks.clear();
		}
	}

	if (!branch) return pblock;

	if (branch & 8)
	{
		// what if also a jump?
		pblock->type |= BLOCKTYPE_EOP | BLOCKTYPE_HASEOP;

		// add an instruction to flush p and q (if written)
		pblock->insts.push_back(SuperVUFlushInst());
		return pblock;
	}

	// it is a (cond) branch or a jump
	u32 vucode = *(u32*)(VU->Micro + lastpc - 16);
	int bpc = _recbranchAddr(vucode) - 8;

	VUPIPELINES newpipes;
	memcpy(newpipes.fmac, VU->fmac, sizeof(newpipes.fmac));
	memcpy(&newpipes.fdiv, &VU->fdiv, sizeof(newpipes.fdiv));
	memcpy(&newpipes.efu, &VU->efu, sizeof(newpipes.efu));
	memcpy(newpipes.ialu, VU->ialu, sizeof(newpipes.ialu));

	for (i = 0; i < 8; ++i) newpipes.fmac[i].sCycle -= vucycle;
	newpipes.fdiv.sCycle -= vucycle;
	newpipes.efu.sCycle -= vucycle;
	for (i = 0; i < 8; ++i) newpipes.ialu[i].sCycle -= vucycle;

	if (listWritebacks.size() > 0)
	{
		// flush all when jumping, send down the pipe when in branching
		bool bFlushWritebacks = (vucode >> 25) == 0x24 || (vucode >> 25) == 0x25;//||(vucode>>25)==0x20||(vucode>>25)==0x21;

		listWritebacks.sort(WRITEBACK::SortWritebacks);
		for (itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end(); ++itwriteback)
		{
			if (itwriteback->viwrite[0] & (1 << REG_Q))
			{
				// ignore all Q writebacks
				continue;
			}

			if (itwriteback->cycle < vucycle || bFlushWritebacks)
			{
				pblock->insts.push_back(VuInstruction());
				itwriteback->InitInst(&pblock->insts.back(), vucycle);
			}
			else
			{
				newpipes.listWritebacks.push_back(*itwriteback);
				newpipes.listWritebacks.back().cycle -= vucycle;
			}
		}
	}

	if (newpipes.listWritebacks.size() > 0)  // other blocks might read the mac flags
		pblock->type |= BLOCKTYPE_MACFLAGS;

	u32 firstbranch = vucode >> 25;
	switch (firstbranch)
	{
		case 0x24: // jr
			pblock->type |= BLOCKTYPE_EOP; // jump out of procedure, since not returning, set EOP
			pblock->insts.push_back(SuperVUFlushInst());
			firstbranch = 0xff; //Non-Conditional Jump
			break;

		case 0x25: // jalr
			{
				// linking, so will return to procedure
				pblock->insts.push_back(SuperVUFlushInst());

				VuBaseBlock* pjumpblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				assert(pblock != NULL);

				pblock->blocks.push_back(pjumpblock);
				firstbranch = 0xff; //Non-Conditional Jump
				break;
			}
		case 0x20: // B
			{
				VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				assert(pblock != NULL);

				pblock->blocks.push_back(pbranchblock);
				firstbranch = 0xff; //Non-Conditional Jump
				break;
			}
		case 0x21: // BAL
			{
				VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				assert(pblock != NULL);
				pblock->blocks.push_back(pbranchblock);
				firstbranch = 0xff; //Non-Conditional Jump
				break;
			}
		case 0x28: // IBEQ
		case 0x2f: // IBGEZ
		case 0x2d: // IBGTZ
		case 0x2e: // IBLEZ
		case 0x2c: // IBLTZ
		case 0x29: // IBNE
			{
				VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				assert(pblock != NULL);
				pblock->blocks.push_back(pbranchblock);

				// if has a second branch that is B or BAL, skip this
				u32 secondbranch = (*(u32*)(VU->Micro + lastpc - 8)) >> 25;
				if (!hasSecondBranch || (secondbranch != 0x21 && secondbranch != 0x20))
				{
					pbranchblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

					pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
					pblock->blocks.push_back(pbranchblock);
				}

				break;
			}
		default:
			assert(pblock->blocks.size() == 1);
			break;
	}

	pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;

#ifdef SUPERVU_VIBRANCHDELAY
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// NOTE! This could still be a hack for KH2/GoW, but until we know how it properly works, this will do for now.///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (hasSecondBranch && firstbranch != 0xff)    //check the previous jump was conditional and there is a second branch
	{
#else
	if (hasSecondBranch)
	{
#endif

		u32 vucode = *(u32*)(VU->Micro + lastpc - 8);
		pc = lastpc;
		int bpc = _recbranchAddr(vucode);

		switch (vucode >> 25)
		{
			case 0x24: // jr
				Console::Error("svurec bad jr jump!");
				assert(0);
				break;

			case 0x25: // jalr
				{
					Console::Error("svurec bad jalr jump!");
					assert(0);
					break;
				}
			case 0x20: // B
				{
					VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

					// update pblock since could have changed
					pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;

					pblock->blocks.push_back(pbranchblock);
					break;
				}
			case 0x21: // BAL
				{
					VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

					// replace instead of pushing a new block
					pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
					pblock->blocks.push_back(pbranchblock);
					break;
				}
			case 0x28: // IBEQ
			case 0x2f: // IBGEZ
			case 0x2d: // IBGTZ
			case 0x2e: // IBLEZ
			case 0x2c: // IBLTZ
			case 0x29: // IBNE
				{
					VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

					// update pblock since could have changed
					pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
					pblock->blocks.push_back(pbranchblock);

					// only add the block if the previous branch doesn't include the next instruction (ie, if a direct jump)
					if (firstbranch == 0x24 || firstbranch == 0x25 || firstbranch == 0x20 || firstbranch == 0x21)
					{
						pbranchblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

						pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
						pblock->blocks.push_back(pbranchblock);
					}

					break;
				}

				jNO_DEFAULT;
		}
	}

	return recVUBlocks[s_vu][startpc/8].pblock;
}

static void SuperVUInitLiveness(VuBaseBlock* pblock)
{
	list<VuInstruction>::iterator itinst, itnext;

	assert(pblock->insts.size() > 0);

	for (itinst = pblock->insts.begin(); itinst != pblock->insts.end(); ++itinst)
	{

		if (itinst->type & INST_DUMMY_)
		{
			itinst->addvars[0] = itinst->addvars[1] = 0xffffffff;
			itinst->livevars[0] = itinst->livevars[1] = 0xffffffff;
			itinst->keepvars[0] = itinst->keepvars[1] = 0xffffffff;
			itinst->usedvars[0] = itinst->usedvars[1] = 0;
		}
		else
		{
			itinst->addvars[0] = itinst->regs[0].VIread | itinst->regs[1].VIread;
			itinst->addvars[1] = (itinst->regs[0].VFread0  ? (1 << itinst->regs[0].VFread0) : 0) |
			                     (itinst->regs[0].VFread1  ? (1 << itinst->regs[0].VFread1) : 0) |
			                     (itinst->regs[1].VFread0  ? (1 << itinst->regs[1].VFread0) : 0) |
			                     (itinst->regs[1].VFread1  ? (1 << itinst->regs[1].VFread1) : 0);

			// vf0 is not handled by VFread
			if (!itinst->regs[0].VFread0 && (itinst->regs[0].VIread & (1 << REG_VF0_FLAG))) itinst->addvars[1] |= 1;
			if (!itinst->regs[1].VFread0 && (itinst->regs[1].VIread & (1 << REG_VF0_FLAG))) itinst->addvars[1] |= 1;
			if (!itinst->regs[0].VFread1 && (itinst->regs[0].VIread & (1 << REG_VF0_FLAG)) && itinst->regs[0].VFr1xyzw != 0xff) itinst->addvars[1] |= 1;
			if (!itinst->regs[1].VFread1 && (itinst->regs[1].VIread & (1 << REG_VF0_FLAG)) && itinst->regs[1].VFr1xyzw != 0xff) itinst->addvars[1] |= 1;


			u32 vfwrite = 0;
			if (itinst->regs[0].VFwrite != 0)
			{
				if (itinst->regs[0].VFwxyzw != 0xf) itinst->addvars[1] |= 1 << itinst->regs[0].VFwrite;
				else vfwrite |= 1 << itinst->regs[0].VFwrite;
			}
			if (itinst->regs[1].VFwrite != 0)
			{
				if (itinst->regs[1].VFwxyzw != 0xf) itinst->addvars[1] |= 1 << itinst->regs[1].VFwrite;
				else vfwrite |= 1 << itinst->regs[1].VFwrite;
			}
			if ((itinst->regs[1].VIwrite & (1 << REG_ACC_FLAG)) && itinst->regs[1].VFwxyzw != 0xf)
				itinst->addvars[1] |= 1 << REG_ACC_FLAG;

			u32 viwrite = (itinst->regs[0].VIwrite | itinst->regs[1].VIwrite);

			itinst->usedvars[0] = itinst->addvars[0] | viwrite;
			itinst->usedvars[1] = itinst->addvars[1] | vfwrite;

//			itinst->addvars[0] &= ~viwrite;
//			itinst->addvars[1] &= ~vfwrite;
			itinst->keepvars[0] = ~viwrite;
			itinst->keepvars[1] = ~vfwrite;
		}
	}

	itinst = --pblock->insts.end();
	while (itinst != pblock->insts.begin())
	{
		itnext = itinst;
		--itnext;

		itnext->usedvars[0] |= itinst->usedvars[0];
		itnext->usedvars[1] |= itinst->usedvars[1];

		itinst = itnext;
	}
}

u32 COMPUTE_LIVE(u32 R, u32 K, u32 L)
{
	u32 live = R | ((L) & (K));
	// special process mac and status flags
	// only propagate liveness if doesn't write to the flag
	if (!(L&(1 << REG_STATUS_FLAG)) && !(K&(1 << REG_STATUS_FLAG)))
		live &= ~(1 << REG_STATUS_FLAG);
	if (!(L&(1 << REG_MAC_FLAG)) && !(K&(1 << REG_MAC_FLAG)))
		live &= ~(1 << REG_MAC_FLAG);
	return live;//|(1<<REG_STATUS_FLAG)|(1<<REG_MAC_FLAG);
}

static void SuperVULivenessAnalysis()
{
	BOOL changed;
	list<VuBaseBlock*>::reverse_iterator itblock;
	list<VuInstruction>::iterator itinst, itnext;
	VuBaseBlock::LISTBLOCKS::iterator itchild;

	u32 livevars[2];

	do
	{
		changed = FALSE;
		for (itblock = s_listBlocks.rbegin(); itblock != s_listBlocks.rend(); ++itblock)
		{

			u32 newlive;
			VuBaseBlock* pb = *itblock;

			// the last inst relies on the neighbor's insts
			itinst = --pb->insts.end();

			if (pb->blocks.size() > 0)
			{
				livevars[0] = 0;
				livevars[1] = 0;
				for (itchild = pb->blocks.begin(); itchild != pb->blocks.end(); ++itchild)
				{
					VuInstruction& front = (*itchild)->insts.front();
					livevars[0] |= front.livevars[0];
					livevars[1] |= front.livevars[1];
				}

				newlive = COMPUTE_LIVE(itinst->addvars[0], itinst->keepvars[0], livevars[0]);

				// should propagate status flags whose parent insts are not in this block
//				if( itinst->nParentPc >= 0 && (itinst->type & (INST_STATUS_WRITE|INST_MAC_WRITE)) )
//					newlive |= livevars[0]&((1<<REG_STATUS_FLAG)|(1<<REG_MAC_FLAG));

				if (itinst->livevars[0] != newlive)
				{
					changed = TRUE;
					itinst->livevars[0] = newlive;
				}

				newlive = COMPUTE_LIVE(itinst->addvars[1], itinst->keepvars[1], livevars[1]);
				if (itinst->livevars[1] != newlive)
				{
					changed = TRUE;
					itinst->livevars[1] = newlive;
				}
			}

			while (itinst != pb->insts.begin())
			{

				itnext = itinst;
				--itnext;

				newlive = COMPUTE_LIVE(itnext->addvars[0], itnext->keepvars[0], itinst->livevars[0]);

				// should propagate status flags whose parent insts are not in this block
//				if( itnext->nParentPc >= 0 && (itnext->type & (INST_STATUS_WRITE|INST_MAC_WRITE)) && !(itinst->type & (INST_STATUS_WRITE|INST_MAC_WRITE)) )
//					newlive |= itinst->livevars[0]&((1<<REG_STATUS_FLAG)|(1<<REG_MAC_FLAG));

				if (itnext->livevars[0] != newlive)
				{
					changed = TRUE;
					itnext->livevars[0] = newlive;
					itnext->livevars[1] = COMPUTE_LIVE(itnext->addvars[1], itnext->keepvars[1], itinst->livevars[1]);
				}
				else
				{
					newlive = COMPUTE_LIVE(itnext->addvars[1], itnext->keepvars[1], itinst->livevars[1]);
					if (itnext->livevars[1] != newlive)
					{
						changed = TRUE;
						itnext->livevars[1] = newlive;
					}
				}

				itinst = itnext;
			}

//			if( (livevars[0] | itinst->livevars[0]) != itinst->livevars[0] ) {
//				changed = TRUE;
//				itinst->livevars[0] |= livevars[0];
//			}
//			if( (livevars[1] | itinst->livevars[1]) != itinst->livevars[1] ) {
//				changed = TRUE;
//				itinst->livevars[1] |= livevars[1];
//			}
//
//			while( itinst != pb->insts.begin() ) {
//
//				itnext = itinst; --itnext;
//				if( (itnext->livevars[0] | (itinst->livevars[0] & itnext->keepvars[0])) != itnext->livevars[0] ) {
//					changed = TRUE;
//					itnext->livevars[0] |= itinst->livevars[0] & itnext->keepvars[0];
//					itnext->livevars[1] |= itinst->livevars[1] & itnext->keepvars[1];
//				}
//				else if( (itnext->livevars[1] | (itinst->livevars[1] & itnext->keepvars[1])) != itnext->livevars[1] ) {
//					changed = TRUE;
//					itnext->livevars[1] |= itinst->livevars[1] & itnext->keepvars[1];
//				}
//
//				itinst = itnext;
//			}
		}

	}
	while (changed);
}

static void SuperVUEliminateDeadCode()
{
	list<VuBaseBlock*>::iterator itblock;
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	list<VuInstruction>::iterator itinst, itnext;
	list<VuInstruction*> listParents;
	list<VuInstruction*>::iterator itparent;

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{

#ifdef PCSX2_DEBUG
		u32 startpc = (*itblock)->startpc;
		u32 curpc = startpc;
#endif

		itnext = (*itblock)->insts.begin();
		itinst = itnext++;
		while (itnext != (*itblock)->insts.end())
		{
			if (itinst->type & (INST_CLIP_WRITE | INST_MAC_WRITE | INST_STATUS_WRITE))
			{
				u32 live0 = itnext->livevars[0];
				if (itinst->nParentPc >= 0 && itnext->nParentPc >= 0 && itinst->nParentPc != itnext->nParentPc)    // superman returns
				{
					// take the live vars from the next next inst
					list<VuInstruction>::iterator itnextnext = itnext;
					++itnextnext;
					if (itnextnext != (*itblock)->insts.end())
					{
						live0 = itnextnext->livevars[0];
					}
				}

				itinst->regs[0].VIwrite &= live0;
				itinst->regs[1].VIwrite &= live0;

				u32 viwrite = itinst->regs[0].VIwrite | itinst->regs[1].VIwrite;

				(*itblock)->GetInstsAtPc(itinst->nParentPc, listParents);
				int removetype = 0;

				for(itparent = listParents.begin(); itparent != listParents.end(); itparent++)
				{
					VuInstruction* parent = *itparent;

					if (viwrite & (1 << REG_CLIP_FLAG))
					{
						parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_CLIP_FLAG));
						parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_CLIP_FLAG));
					}
					else
						removetype |= INST_CLIP_WRITE;

					if (parent->info.macflag && (itinst->type & INST_MAC_WRITE))
					{
						if (!(viwrite&(1 << REG_MAC_FLAG)))
						{
							//parent->info.macflag = 0;
							//							parent->regs[0].VIwrite &= ~(1<<REG_MAC_FLAG);
							//							parent->regs[1].VIwrite &= ~(1<<REG_MAC_FLAG);
							// can be nonzero when a writeback belong to a different block and one branch uses
							// it and this one doesn't
#ifndef SUPERVU_WRITEBACKS
							assert(!(parent->regs[0].VIwrite & (1 << REG_MAC_FLAG)) && !(parent->regs[1].VIwrite & (1 << REG_MAC_FLAG)));
#endif
							// if VUPIPE_FMAC and destination is vf00, probably need to keep the mac flag
							if (parent->regs[1].pipe == VUPIPE_FMAC && (parent->regs[1].VFwrite == 0 && !(parent->regs[1].VIwrite&(1 << REG_ACC_FLAG))))
							{
								parent->regs[0].VIwrite |= ((1 << REG_MAC_FLAG));
								parent->regs[1].VIwrite |= ((1 << REG_MAC_FLAG));
							}
							else
								removetype |= INST_MAC_WRITE;
						}
						else
						{
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_MAC_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_MAC_FLAG));
						}
					}
					else removetype |= INST_MAC_WRITE;

					if (parent->info.statusflag && (itinst->type & INST_STATUS_WRITE))
					{
						if (!(viwrite&(1 << REG_STATUS_FLAG)))
						{
							//parent->info.statusflag = 0;
							//							parent->regs[0].VIwrite &= ~(1<<REG_STATUS_FLAG);
							//							parent->regs[1].VIwrite &= ~(1<<REG_STATUS_FLAG);

							// can be nonzero when a writeback belong to a different block and one branch uses
							// it and this one doesn't
#ifndef SUPERVU_WRITEBACKS
							assert(!(parent->regs[0].VIwrite & (1 << REG_STATUS_FLAG)) && !(parent->regs[1].VIwrite & (1 << REG_STATUS_FLAG)));
#endif
							if (parent->regs[1].pipe == VUPIPE_FMAC && (parent->regs[1].VFwrite == 0 && !(parent->regs[1].VIwrite&(1 << REG_ACC_FLAG))))
							{
								parent->regs[0].VIwrite |= ((1 << REG_STATUS_FLAG));
								parent->regs[1].VIwrite |= ((1 << REG_STATUS_FLAG));
							}
							else
								removetype |= INST_STATUS_WRITE;
						}
						else
						{
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_STATUS_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_STATUS_FLAG));
						}
					}
					else removetype |= INST_STATUS_WRITE;
				}

				itinst->type &= ~removetype;
				if (itinst->type == 0)
				{
					itnext = (*itblock)->insts.erase(itinst);
					itinst = itnext++;
					continue;
				}
			}
#ifdef PCSX2_DEBUG
			else
			{
				curpc += 8;
			}
#endif
			itinst = itnext;
			++itnext;
		}

		if (itinst->type & INST_DUMMY)
		{
			// last inst with the children
			u32 mask = 0;
			for (itchild = (*itblock)->blocks.begin(); itchild != (*itblock)->blocks.end(); ++itchild)
			{
				mask |= (*itchild)->insts.front().livevars[0];
			}
			itinst->regs[0].VIwrite &= mask;
			itinst->regs[1].VIwrite &= mask;
			u32 viwrite = itinst->regs[0].VIwrite | itinst->regs[1].VIwrite;

			if (itinst->nParentPc >= 0)
			{

				(*itblock)->GetInstsAtPc(itinst->nParentPc, listParents);
				int removetype = 0;

				for(itparent = listParents.begin(); itparent != listParents.end(); itparent++)
				{
					VuInstruction* parent = *itparent;

					if (viwrite & (1 << REG_CLIP_FLAG))
					{
						parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_CLIP_FLAG));
						parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_CLIP_FLAG));
					}
					else removetype |= INST_CLIP_WRITE;

					if (parent->info.macflag && (itinst->type & INST_MAC_WRITE))
					{
						if (!(viwrite&(1 << REG_MAC_FLAG)))
						{
							//parent->info.macflag = 0;
#ifndef SUPERVU_WRITEBACKS
							assert(!(parent->regs[0].VIwrite & (1 << REG_MAC_FLAG)) && !(parent->regs[1].VIwrite & (1 << REG_MAC_FLAG)));
#endif
							removetype |= INST_MAC_WRITE;
						}
						else
						{
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_MAC_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_MAC_FLAG));
						}
					}
					else removetype |= INST_MAC_WRITE;

					if (parent->info.statusflag && (itinst->type & INST_STATUS_WRITE))
					{
						if (!(viwrite&(1 << REG_STATUS_FLAG)))
						{
							//parent->info.statusflag = 0;
#ifndef SUPERVU_WRITEBACKS
							assert(!(parent->regs[0].VIwrite & (1 << REG_STATUS_FLAG)) && !(parent->regs[1].VIwrite & (1 << REG_STATUS_FLAG)));
#endif
							removetype |= INST_STATUS_WRITE;
						}
						else
						{
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite & (1 << REG_STATUS_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite & (1 << REG_STATUS_FLAG));
						}
					}
					else removetype |= INST_STATUS_WRITE;
				}

				itinst->type &= ~removetype;
				if (itinst->type == 0)
				{
					(*itblock)->insts.erase(itinst);
				}
			}
		}
	}
}

void VuBaseBlock::AssignVFRegs()
{
	int i;
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	list<VuBaseBlock*>::iterator itblock;
	list<VuInstruction>::iterator itinst, itnext, itinst2;

	// init the start regs
	if (type & BLOCKTYPE_ANALYZED) return;  // nothing changed
	memcpy(xmmregs, startregs, sizeof(xmmregs));

	if (type & BLOCKTYPE_ANALYZED)
	{
		// check if changed
		for (i = 0; i < iREGCNT_XMM; ++i)
		{
			if (xmmregs[i].inuse != startregs[i].inuse)
				break;
			if (xmmregs[i].inuse && (xmmregs[i].reg != startregs[i].reg || xmmregs[i].type != startregs[i].type))
				break;
		}

		if (i == iREGCNT_XMM) return;  // nothing changed
	}

	u8* oldX86 = x86Ptr;

	for(itinst = insts.begin(); itinst != insts.end(); itinst++)
	{

		if (itinst->type & INST_DUMMY) continue;

		// reserve, go from upper to lower
		int lastwrite = -1;

		for (i = 1; i >= 0; --i)
		{
			_VURegsNum* regs = itinst->regs + i;


			// redo the counters so that the proper regs are released
			for (int j = 0; j < iREGCNT_XMM; ++j)
			{
				if (xmmregs[j].inuse)
				{
					if (xmmregs[j].type == XMMTYPE_VFREG)
					{
						int count = 0;
						itinst2 = itinst;

						if (i)
						{
							if (itinst2->regs[0].VFread0 == xmmregs[j].reg || itinst2->regs[0].VFread1 == xmmregs[j].reg || itinst2->regs[0].VFwrite == xmmregs[j].reg)
							{
								itinst2 = insts.end();
								break;
							}
							else
							{
								++count;
								++itinst2;
							}
						}

						while (itinst2 != insts.end())
						{
							if (itinst2->regs[0].VFread0 == xmmregs[j].reg || itinst2->regs[0].VFread1 == xmmregs[j].reg || itinst2->regs[0].VFwrite == xmmregs[j].reg ||
							        itinst2->regs[1].VFread0 == xmmregs[j].reg || itinst2->regs[1].VFread1 == xmmregs[j].reg || itinst2->regs[1].VFwrite == xmmregs[j].reg)
								break;

							++count;
							++itinst2;
						}
						xmmregs[j].counter = 1000 - count;
					}
					else
					{
						assert(xmmregs[j].type == XMMTYPE_ACC);

						int count = 0;
						itinst2 = itinst;

						if (i) ++itinst2;  // acc isn't used in lower insts

						while (itinst2 != insts.end())
						{
							assert(!((itinst2->regs[0].VIread | itinst2->regs[0].VIwrite) & (1 << REG_ACC_FLAG)));

							if ((itinst2->regs[1].VIread | itinst2->regs[1].VIwrite) & (1 << REG_ACC_FLAG))
								break;

							++count;
							++itinst2;
						}

						xmmregs[j].counter = 1000 - count;
					}
				}
			}

			if (regs->VFread0) _addNeededVFtoXMMreg(regs->VFread0);
			if (regs->VFread1) _addNeededVFtoXMMreg(regs->VFread1);
			if (regs->VFwrite) _addNeededVFtoXMMreg(regs->VFwrite);
			if (regs->VIread & (1 << REG_ACC_FLAG)) _addNeededACCtoXMMreg();
			if (regs->VIread & (1 << REG_VF0_FLAG)) _addNeededVFtoXMMreg(0);

			// alloc
			itinst->vfread0[i] = itinst->vfread1[i] = itinst->vfwrite[i] = itinst->vfacc[i] = -1;
			itinst->vfflush[i] = -1;

			if (regs->VFread0) 
				itinst->vfread0[i] = _allocVFtoXMMreg(VU, -1, regs->VFread0, 0);
			else if (regs->VIread & (1 << REG_VF0_FLAG)) 
				itinst->vfread0[i] = _allocVFtoXMMreg(VU, -1, 0, 0);
			
			if (regs->VFread1) 
				itinst->vfread1[i] = _allocVFtoXMMreg(VU, -1, regs->VFread1, 0);
			else if ((regs->VIread & (1 << REG_VF0_FLAG)) && regs->VFr1xyzw != 0xff) 
				itinst->vfread1[i] = _allocVFtoXMMreg(VU, -1, 0, 0);
			
			if (regs->VIread & (1 << REG_ACC_FLAG)) itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);

			int reusereg = -1; // 0 - VFwrite, 1 - VFAcc

			if (regs->VFwrite)
			{
				assert(!(regs->VIwrite&(1 << REG_ACC_FLAG)));

				if (regs->VFwxyzw == 0xf)
				{
					itinst->vfwrite[i] = _checkXMMreg(XMMTYPE_VFREG, regs->VFwrite, 0);
					if (itinst->vfwrite[i] < 0) reusereg = 0;
				}
				else
				{
					itinst->vfwrite[i] = _allocVFtoXMMreg(VU, -1, regs->VFwrite, 0);
				}
			}
			else if (regs->VIwrite & (1 << REG_ACC_FLAG))
			{

				if (regs->VFwxyzw == 0xf)
				{
					itinst->vfacc[i] = _checkXMMreg(XMMTYPE_ACC, 0, 0);
					if (itinst->vfacc[i] < 0) reusereg = 1;
				}
				else
				{
					itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);
				}
			}

			if (reusereg >= 0)
			{
				// reuse
				itnext = itinst;
				itnext++;

				u8 type = reusereg ? XMMTYPE_ACC : XMMTYPE_VFREG;
				u8 reg = reusereg ? 0 : regs->VFwrite;

				if (itinst->vfacc[i] >= 0 && lastwrite != itinst->vfacc[i] &&
				        (itnext == insts.end() || ((regs->VIread&(1 << REG_ACC_FLAG)) && (!(itnext->usedvars[0]&(1 << REG_ACC_FLAG)) || !(itnext->livevars[0]&(1 << REG_ACC_FLAG))))))
				{

					assert(reusereg == 0);
					if (itnext == insts.end() || (itnext->livevars[0]&(1 << REG_ACC_FLAG))) _freeXMMreg(itinst->vfacc[i]);
					xmmregs[itinst->vfacc[i]].inuse = 1;
					xmmregs[itinst->vfacc[i]].reg = reg;
					xmmregs[itinst->vfacc[i]].type = type;
					xmmregs[itinst->vfacc[i]].mode = 0;
					itinst->vfwrite[i] = itinst->vfacc[i];
				}
				else if (itinst->vfread0[i] >= 0 && lastwrite != itinst->vfread0[i] &&
				         (itnext == insts.end() || (regs->VFread0 > 0 && (!(itnext->usedvars[1]&(1 << regs->VFread0)) || !(itnext->livevars[1]&(1 << regs->VFread0))))))
				{

					if (itnext == insts.end() || (itnext->livevars[1]&regs->VFread0)) _freeXMMreg(itinst->vfread0[i]);
					
					xmmregs[itinst->vfread0[i]].inuse = 1;
					xmmregs[itinst->vfread0[i]].reg = reg;
					xmmregs[itinst->vfread0[i]].type = type;
					xmmregs[itinst->vfread0[i]].mode = 0;
					
					if (reusereg)
						itinst->vfacc[i] = itinst->vfread0[i];
					else 
						itinst->vfwrite[i] = itinst->vfread0[i];
				}
				else if (itinst->vfread1[i] >= 0 && lastwrite != itinst->vfread1[i] &&
				         (itnext == insts.end() || (regs->VFread1 > 0 && (!(itnext->usedvars[1]&(1 << regs->VFread1)) || !(itnext->livevars[1]&(1 << regs->VFread1))))))
				{

					if (itnext == insts.end() || (itnext->livevars[1]&regs->VFread1)) _freeXMMreg(itinst->vfread1[i]);
					
					xmmregs[itinst->vfread1[i]].inuse = 1;
					xmmregs[itinst->vfread1[i]].reg = reg;
					xmmregs[itinst->vfread1[i]].type = type;
					xmmregs[itinst->vfread1[i]].mode = 0;
					if (reusereg) 
						itinst->vfacc[i] = itinst->vfread1[i];
					else 
						itinst->vfwrite[i] = itinst->vfread1[i];
				}
				else
				{
					if (reusereg) 
						itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);
					else 
						itinst->vfwrite[i] = _allocVFtoXMMreg(VU, -1, regs->VFwrite, 0);
				}
			}

			if (itinst->vfwrite[i] >= 0) lastwrite = itinst->vfwrite[i];
			else if (itinst->vfacc[i] >= 0) lastwrite = itinst->vfacc[i];

			// always alloc at least 1 temp reg
			int free0 = (i || regs->VFwrite || regs->VFread0 || regs->VFread1 || (regs->VIwrite & (1 << REG_ACC_FLAG)) || (regs->VIread & (1 << REG_VF0_FLAG)))
			            ? _allocTempXMMreg(XMMT_FPS, -1) : -1;
			int free1 = 0, free2 = 0;

			if (i == 0 && itinst->vfwrite[1] >= 0 && (itinst->vfread0[0] == itinst->vfwrite[1] || itinst->vfread1[0] == itinst->vfwrite[1]))
			{
				itinst->vfflush[i] = _allocTempXMMreg(XMMT_FPS, -1);
			}

			if (i == 1 && (regs->VIwrite & (1 << REG_CLIP_FLAG)))
			{
				// CLIP inst, need two extra regs
				if (free0 < 0) free0 = _allocTempXMMreg(XMMT_FPS, -1);
				
				free1 = _allocTempXMMreg(XMMT_FPS, -1);
				free2 = _allocTempXMMreg(XMMT_FPS, -1);
				_freeXMMreg(free1);
				_freeXMMreg(free2);
			}
			else if (regs->VIwrite & (1 << REG_P))
			{
				// EFU inst, need extra reg
				free1 = _allocTempXMMreg(XMMT_FPS, -1);
				if (free0 == -1) free0 = free1;
				_freeXMMreg(free1);
			}

			if (itinst->vfflush[i] >= 0) _freeXMMreg(itinst->vfflush[i]);
			if (free0 >= 0) _freeXMMreg(free0);

			itinst->vffree[i] = (free0 & 0xf) | (free1 << 8) | (free2 << 16);
			if (free0 == -1) itinst->vffree[i] |= VFFREE_INVALID0;

			_clearNeededXMMregs();
		}
	}

	assert(x86Ptr == oldX86);
	u32 analyzechildren = !(type & BLOCKTYPE_ANALYZED);
	type |= BLOCKTYPE_ANALYZED;

	//memset(endregs, 0, sizeof(endregs));

	if (analyzechildren)
	{
		for(itchild = blocks.begin(); itchild != blocks.end(); itchild++)
		{
			(*itchild)->AssignVFRegs();
		}
	}
}

struct MARKOVBLANKET
{
	list<VuBaseBlock*> parents;
	list<VuBaseBlock*> children;
};

static MARKOVBLANKET s_markov;

void VuBaseBlock::AssignVIRegs(int parent)
{
	const int maxregs = 6;

	if (parent)
	{
		if ((type&BLOCKTYPE_ANALYZEDPARENT))
			return;

		type |= BLOCKTYPE_ANALYZEDPARENT;
		s_markov.parents.push_back(this);
		for (LISTBLOCKS::iterator it = blocks.begin(); it != blocks.end(); ++it)
		{
			(*it)->AssignVIRegs(0);
		}
		return;
	}

	if ((type&BLOCKTYPE_ANALYZED))
		return;

	// child
	assert(allocX86Regs == -1);
	allocX86Regs = s_vecRegArray.size();
	s_vecRegArray.resize(allocX86Regs + iREGCNT_GPR);

	_x86regs* pregs = &s_vecRegArray[allocX86Regs];
	memset(pregs, 0, sizeof(_x86regs)*iREGCNT_GPR);

	assert(parents.size() > 0);

	list<VuBaseBlock*>::iterator itparent;
	u32 usedvars = insts.front().usedvars[0];
	u32 livevars = insts.front().livevars[0];

	if (parents.size() > 0)
	{
		u32 usedvars2 = 0xffffffff;
		
		for(itparent = parents.begin(); itparent != parents.end(); itparent++)
		{
			usedvars2 &= (*itparent)->insts.front().usedvars[0];
		}
		
		usedvars |= usedvars2;
	}

	usedvars &= livevars;

	// currently order doesn't matter
	int num = 0;

	if (usedvars)
	{
		for (int i = 1; i < 16; ++i)
		{
			if (usedvars & (1 << i))
			{
				pregs[num].inuse = 1;
				pregs[num].reg = i;

				livevars &= ~(1 << i);

				if (++num >= maxregs) break;
			}
		}
	}

	if (num < maxregs)
	{
		livevars &= ~usedvars;
		livevars &= insts.back().usedvars[0];

		if (livevars)
		{
			for (int i = 1; i < 16; ++i)
			{
				if (livevars & (1 << i))
				{
					pregs[num].inuse = 1;
					pregs[num].reg = i;

					if (++num >= maxregs) break;
				}
			}
		}
	}

	s_markov.children.push_back(this);
	type |= BLOCKTYPE_ANALYZED;
	
	for(itparent = parents.begin(); itparent != parents.end(); itparent++)
	{
		(*itparent)->AssignVIRegs(1);
	}
}

static void SuperVUAssignRegs()
{
	list<VuBaseBlock*>::iterator itblock, itblock2;

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		(*itblock)->type &= ~BLOCKTYPE_ANALYZED;
	}
	s_listBlocks.front()->AssignVFRegs();

	// VI assignments, find markov blanket for each node in the graph
	// then allocate regs based on the commonly used ones
#ifdef SUPERVU_X86CACHING
	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		(*itblock)->type &= ~(BLOCKTYPE_ANALYZED | BLOCKTYPE_ANALYZEDPARENT);
	}
	s_vecRegArray.resize(0);
	u8 usedregs[16];

	// note: first block always has to start with no alloc regs
	bool bfirst = true;

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{

		if (!((*itblock)->type & BLOCKTYPE_ANALYZED))
		{

			if ((*itblock)->parents.size() == 0)
			{
				(*itblock)->type |= BLOCKTYPE_ANALYZED;
				bfirst = false;
				continue;
			}

			s_markov.children.clear();
			s_markov.parents.clear();
			(*itblock)->AssignVIRegs(0);

			// assign the regs
			int regid = s_vecRegArray.size();
			s_vecRegArray.resize(regid + iREGCNT_GPR);

			_x86regs* mergedx86 = &s_vecRegArray[regid];
			memset(mergedx86, 0, sizeof(_x86regs)*iREGCNT_GPR);

			if (!bfirst)
			{
				*(u32*)usedregs = *((u32*)usedregs + 1) = *((u32*)usedregs + 2) = *((u32*)usedregs + 3) = 0;

				for(itblock2 = s_markov.children.begin(); itblock2 != s_markov.children.end(); itblock2++)
				{
					assert((*itblock2)->allocX86Regs >= 0);
					_x86regs* pregs = &s_vecRegArray[(*itblock2)->allocX86Regs];
					for (int i = 0; i < iREGCNT_GPR; ++i)
					{
						if (pregs[i].inuse && pregs[i].reg < 16)
						{
							//assert( pregs[i].reg < 16);
							usedregs[pregs[i].reg]++;
						}
					}
				}

				int num = 1;
				for (int i = 0; i < 16; ++i)
				{
					if (usedregs[i] == s_markov.children.size())
					{
						// use
						mergedx86[num].inuse = 1;
						mergedx86[num].reg = i;
						mergedx86[num].type = (s_vu ? X86TYPE_VU1 : 0) | X86TYPE_VI;
						mergedx86[num].mode = MODE_READ;
						if (++num >= iREGCNT_GPR)
							break;
						if (num == ESP)
							++num;
					}
				}

				for(itblock2 = s_markov.children.begin(); itblock2 != s_markov.children.end(); itblock2++)
				{
					assert((*itblock2)->nStartx86 == -1);
					(*itblock2)->nStartx86 = regid;
				}

				for(itblock2 = s_markov.parents.begin(); itblock2 != s_markov.parents.end(); itblock2++)
				{
					assert((*itblock2)->nEndx86 == -1);
					(*itblock2)->nEndx86 = regid;
				}
			}

			bfirst = false;
		}
	}
#endif
}

//////////////////
// Recompilation
//////////////////

// cycles in which the last Q,P regs were finished (written to VU->VI[])
// the write occurs before the instruction is executed at that cycle
// compare with s_TotalVUCycles
// if less than 0, already flushed
int s_writeQ, s_writeP;

// declare the saved registers
uptr s_vu1esp, s_callstack;//, s_vu1esp
uptr s_vu1ebp, s_vuebx, s_vuedi, s_vu1esi;

static int s_recWriteQ, s_recWriteP; // wait times during recompilation
static int s_needFlush; // first bit - Q, second bit - P, third bit - Q has been written, fourth bit - P has been written

static int s_JumpX86;
static int s_ScheduleXGKICK = 0, s_XGKICKReg = -1;

void recVUMI_XGKICK_(VURegs *VU);

void SuperVUCleanupProgram(u32 startpc, int vuindex)
{
#ifdef SUPERVU_COUNT
	QueryPerformanceCounter(&svufinal);
	svutime += (u32)(svufinal.QuadPart - svubase.QuadPart);
#endif

	assert(s_vu1esp == 0);

	VU = vuindex ? &VU1 : &VU0;
	VU->cycle += s_TotalVUCycles;

	//VU cycle stealing hack, 3000 cycle maximum so it doesn't get out of hand
	if (s_TotalVUCycles < 3000)
		cpuRegs.cycle += s_TotalVUCycles * Config.Hacks.VUCycleSteal;
	else
		cpuRegs.cycle += 3000 * Config.Hacks.VUCycleSteal;

	if ((int)s_writeQ > 0) VU->VI[REG_Q] = VU->q;
	if ((int)s_writeP > 0)
	{
		assert(VU == &VU1);
		VU1.VI[REG_P] = VU1.p; // only VU1
	}

	//memset(recVUStack, 0, SUPERVU_STACKSIZE * 4);

	// Could clear allocation info to prevent possibly bad data being used in other parts of pcsx2;
	// not doing this because it's slow and not needed (rama)
	// _initXMMregs();
	// _initMMXregs();
	// _initX86regs();
}

#if defined(_MSC_VER)

// entry point of all vu programs from emulator calls
__declspec(naked) void SuperVUExecuteProgram(u32 startpc, int vuindex)
{
	__asm
	{
		mov eax, dword ptr [esp]
		mov s_TotalVUCycles, 0 // necessary to be here!
		add esp, 4
		mov s_callstack, eax
		call SuperVUGetProgram

		// save cpu state
		mov s_vu1ebp, ebp
		mov s_vu1esi, esi // have to save even in Release
		mov s_vuedi, edi // have to save even in Release
		mov s_vuebx, ebx
	}
#ifdef PCSX2_DEBUG
	__asm
	{
		mov s_vu1esp, esp
	}
#endif

	__asm
	{
		//stmxcsr s_ssecsr
		ldmxcsr g_sseVUMXCSR

		// init vars
		mov s_writeQ, 0xffffffff
		mov s_writeP, 0xffffffff

		jmp eax
	}
}

// exit point of all vu programs
__declspec(naked) static void SuperVUEndProgram()
{
	__asm
	{
		// restore cpu state
		ldmxcsr g_sseMXCSR

		mov ebp, s_vu1ebp
		mov esi, s_vu1esi
		mov edi, s_vuedi
		mov ebx, s_vuebx
	}

#ifdef PCSX2_DEBUG
	__asm
	{
		sub s_vu1esp, esp
	}
#endif

	__asm
	{
		call SuperVUCleanupProgram
		jmp s_callstack // so returns correctly
	}
}

#endif

// Flushes P/Q regs
void SuperVUFlush(int p, int wait)
{
	u8* pjmp[3];
	if (!(s_needFlush&(1 << p))) return;

	int recwait = p ? s_recWriteP : s_recWriteQ;
	if (!wait && s_pCurInst->info.cycle < recwait) return;

	if (recwait == 0)
	{
		// write didn't happen this block
		MOV32MtoR(EAX, p ? (uptr)&s_writeP : (uptr)&s_writeQ);
		OR32RtoR(EAX, EAX);
		pjmp[0] = JS8(0);

		if (s_pCurInst->info.cycle) SUB32ItoR(EAX, s_pCurInst->info.cycle);

		// if writeQ <= total+offset
		if (!wait)    // only write back if time is up
		{
			CMP32MtoR(EAX, (uptr)&s_TotalVUCycles);
			pjmp[1] = JG8(0);
		}
		else
		{
			// add (writeQ-total-offset) to s_TotalVUCycles
			// necessary?
			CMP32MtoR(EAX, (uptr)&s_TotalVUCycles);
			pjmp[2] = JLE8(0);
			MOV32RtoM((uptr)&s_TotalVUCycles, EAX);
			x86SetJ8(pjmp[2]);
		}
	}
	else if (wait && s_pCurInst->info.cycle < recwait)
	{
		ADD32ItoM((uptr)&s_TotalVUCycles, recwait);
	}

	MOV32MtoR(EAX, SuperVUGetVIAddr(p ? REG_P : REG_Q, 0));
	MOV32ItoM(p ? (uptr)&s_writeP : (uptr)&s_writeQ, 0x80000000);
	MOV32RtoM(SuperVUGetVIAddr(p ? REG_P : REG_Q, 1), EAX);

	if (recwait == 0)
	{
		if (!wait) x86SetJ8(pjmp[1]);
		x86SetJ8(pjmp[0]);
	}

	if (wait || (!p && recwait == 0 && s_pCurInst->info.cycle >= 12) || (!p && recwait > 0 && s_pCurInst->info.cycle >= recwait))
		s_needFlush &= ~(1 << p);
}

// executed only once per program
static u32* SuperVUStaticAlloc(u32 size)
{
	assert(recVUStackPtr + size <= recVUStack + SUPERVU_STACKSIZE);
	// always zero
	if (size == 4) *(u32*)recVUStackPtr = 0;
	else memset(recVUStackPtr, 0, size);
	recVUStackPtr += size;
	return (u32*)(recVUStackPtr - size);
}

static void SuperVURecompile()
{
	// save cpu state
	recVUStackPtr = recVUStack;

	_initXMMregs();

	list<VuBaseBlock*>::iterator itblock;

	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		(*itblock)->type &= ~BLOCKTYPE_ANALYZED;
	}
	
	s_listBlocks.front()->Recompile();
	
	// make sure everything compiled
	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		assert(((*itblock)->type & BLOCKTYPE_ANALYZED) && (*itblock)->pcode != NULL);
	}

	// link all blocks
	for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
	{
		VuBaseBlock::LISTBLOCKS::iterator itchild;

		assert((*itblock)->blocks.size() <= ArraySize((*itblock)->pChildJumps));

		int i = 0;
		for(itchild = (*itblock)->blocks.begin(); itchild != (*itblock)->blocks.end(); itchild++)
		{

			if ((u32)(uptr)(*itblock)->pChildJumps[i] == 0xffffffff)
				continue;

			if ((*itblock)->pChildJumps[i] == NULL)
			{
				VuBaseBlock* pchild = *itchild;

				if (pchild->type & BLOCKTYPE_HASEOP)
				{
					assert(pchild->blocks.size() == 0);

					AND32ItoM((uptr)&VU0.VI[ REG_VPU_STAT ].UL, s_vu ? ~0x100 : ~0x001); // E flag
					AND32ItoM((uptr)&VU->vifRegs->stat, ~VIF1_STAT_VEW);

					MOV32ItoM((uptr)&VU->VI[REG_TPC], pchild->endpc);
					JMP32((uptr)SuperVUEndProgram - ((uptr)x86Ptr + 5));
				}
				// only other case is when there are two branches
				else 
				{
					assert((*itblock)->insts.back().regs[0].pipe == VUPIPE_BRANCH);
				}

				continue;
			}

			if ((u32)(uptr)(*itblock)->pChildJumps[i] & 0x80000000)
			{
				// relative
				assert((uptr)(*itblock)->pChildJumps[i] <= 0xffffffff);
				(*itblock)->pChildJumps[i] = (u32*)((uptr)(*itblock)->pChildJumps[i] & 0x7fffffff);
				*(*itblock)->pChildJumps[i] = (uptr)(*itchild)->pcode - ((uptr)(*itblock)->pChildJumps[i] + 4);
			}
			else 
			{
				*(*itblock)->pChildJumps[i] = (uptr)(*itchild)->pcode;
			}

			++i;
		}
	}

	s_pFnHeader->pprogfunc = s_listBlocks.front()->pcode;
}

// debug


u32 s_saveecx, s_saveedx, s_saveebx, s_saveesi, s_saveedi, s_saveebp;
u32 g_curdebugvu;

//float vuDouble(u32 f);

#if defined(_MSC_VER)
__declspec(naked) static void svudispfn()
{
	__asm
	{
		mov g_curdebugvu, eax
		mov s_saveecx, ecx
		mov s_saveedx, edx
		mov s_saveebx, ebx
		mov s_saveesi, esi
		mov s_saveedi, edi
		mov s_saveebp, ebp
	}
#else

void svudispfntemp()
{
#endif

#ifdef PCSX2_DEBUG
	static u32 i;

	if (((vudump&8) && g_curdebugvu) || ((vudump&0x80) && !g_curdebugvu))    //&& g_vu1lastrec != g_vu1last ) {
	{

		if (skipparent != g_vu1lastrec)
		{
			for (i = 0; i < ArraySize(badaddrs); ++i)
			{
				if (s_svulast == badaddrs[i][1] && g_vu1lastrec == badaddrs[i][0])
					break;
			}

			if (i == ArraySize(badaddrs))
			{
				//static int curesp;
				//__asm mov curesp, esp
				//Console::WriteLn("tVU: %x %x %x", s_svulast, s_vucount, s_vufnheader);
				if (g_curdebugvu) iDumpVU1Registers();
				else iDumpVU0Registers();
				s_vucount++;
			}
		}

		g_vu1lastrec = s_svulast;
	}
#endif

#if defined(_MSC_VER)
	__asm
	{
	    mov ecx, s_saveecx
	    mov edx, s_saveedx
	    mov ebx, s_saveebx
	    mov esi, s_saveesi
	    mov edi, s_saveedi
	    mov ebp, s_saveebp
	    ret
	}
#endif
}

// frees all regs taking into account the livevars
void SuperVUFreeXMMregs(u32* livevars)
{
	for (int i = 0; i < iREGCNT_XMM; ++i)
	{
		if (xmmregs[i].inuse)
		{
			// same reg
			if ((xmmregs[i].mode & MODE_WRITE))
			{

#ifdef SUPERVU_INTERCACHING
				if (xmmregs[i].type == XMMTYPE_VFREG)
				{
					if (!(livevars[1] & (1 << xmmregs[i].reg))) continue;
				}
				else if (xmmregs[i].type == XMMTYPE_ACC)
				{
					if (!(livevars[0] & (1 << REG_ACC_FLAG))) continue;
				}
#endif

				if (xmmregs[i].mode & MODE_VUXYZ)
				{
					// ALWAYS update
					u32 addr = xmmregs[i].type == XMMTYPE_VFREG ? (uptr) & VU->VF[xmmregs[i].reg] : (uptr) & VU->ACC;

					if (xmmregs[i].mode & MODE_VUZ)
					{
						SSE_MOVHPS_XMM_to_M64(addr, (x86SSERegType)i);
						SSE_SHUFPS_M128_to_XMM((x86SSERegType)i, addr, 0xc4);
					}
					else 
					{
						SSE_MOVHPS_M64_to_XMM((x86SSERegType)i, addr + 8);
					}

					xmmregs[i].mode &= ~MODE_VUXYZ;
				}

				_freeXMMreg(i);
			}
		}
	}

	//_freeXMMregs();
}

void SuperVUTestVU0Condition(u32 incstack)
{
	if (s_vu && !SUPERVU_CHECKCONDITION) return;  // vu0 only

	CMP32ItoM((uptr)&s_TotalVUCycles, 512);	// sometimes games spin on vu0, so be careful with this value
	// woody hangs if too high

	if (incstack)
	{
		u8* ptr = JB8(0);

		ADD32ItoR(ESP, incstack);
		//CALLFunc((u32)timeout);
		JMP32((uptr)SuperVUEndProgram - ((uptr)x86Ptr + 5));

		x86SetJ8(ptr);
	}
	else JAE32((uptr)SuperVUEndProgram - ((uptr)x86Ptr + 6));
}

void VuBaseBlock::Recompile()
{
	if (type & BLOCKTYPE_ANALYZED) return;

	x86Align(16);
	pcode = x86Ptr;

#ifdef PCSX2_DEBUG
	MOV32ItoM((uptr)&s_vufnheader, s_pFnHeader->startpc);
	MOV32ItoM((uptr)&VU->VI[REG_TPC], startpc);
	MOV32ItoM((uptr)&s_svulast, startpc);
	
	list<VuBaseBlock*>::iterator itparent;
	for (itparent = parents.begin(); itparent != parents.end(); ++itparent)
	{
		if ((*itparent)->blocks.size() == 1 && (*itparent)->blocks.front()->startpc == startpc &&
		        ((*itparent)->insts.size() < 2 || (----(*itparent)->insts.end())->regs[0].pipe != VUPIPE_BRANCH))
		{
			MOV32ItoM((uptr)&skipparent, (*itparent)->startpc);
			break;
		}
	}

	if (itparent == parents.end()) MOV32ItoM((uptr)&skipparent, -1);

	MOV32ItoR(EAX, s_vu);
	CALLFunc((uptr)svudispfn);
#endif

	s_pCurBlock = this;
	s_needFlush = 3;
	pc = startpc;
	branch = 0;
	s_recWriteQ = s_recWriteP = 0;
	s_XGKICKReg = -1;
	s_ScheduleXGKICK = 0;

	s_ClipRead = s_PrevClipWrite = (uptr) & VU->VI[REG_CLIP_FLAG];
	s_StatusRead = s_PrevStatusWrite = (uptr) & VU->VI[REG_STATUS_FLAG];
	s_MACRead = s_PrevMACWrite = (uptr) & VU->VI[REG_MAC_FLAG];
	s_PrevIWrite = (uptr) & VU->VI[REG_I];
	s_JumpX86 = 0;
	s_UnconditionalDelay = 0;

	memcpy(xmmregs, startregs, sizeof(xmmregs));
#ifdef SUPERVU_X86CACHING
	if (nStartx86 >= 0)
		memcpy(x86regs, &s_vecRegArray[nStartx86], sizeof(x86regs));
	else
		_initX86regs();
#else
	_initX86regs();
#endif

	list<VuInstruction>::iterator itinst;
	for(itinst = insts.begin(); itinst != insts.end(); itinst++)
	{
		s_pCurInst = &(*itinst);
		if (s_JumpX86 > 0)
		{
			if (!x86regs[s_JumpX86].inuse)
			{
				// load
				s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_READ);
			}
			x86regs[s_JumpX86].needed = 1;
		}

		if (s_ScheduleXGKICK && s_XGKICKReg > 0)
		{
			assert(x86regs[s_XGKICKReg].inuse);
			x86regs[s_XGKICKReg].needed = 1;
		}
		itinst->Recompile(itinst, vuxyz);

		if (s_ScheduleXGKICK > 0)
		{
			if (s_ScheduleXGKICK-- == 1)
			{
				recVUMI_XGKICK_(VU);
			}
		}
	}
	assert(pc == endpc);
	assert(s_ScheduleXGKICK == 0);

	// flush flags
	if (s_PrevClipWrite != (uptr)&VU->VI[REG_CLIP_FLAG])
	{
		MOV32MtoR(EAX, s_PrevClipWrite);
		MOV32RtoM((uptr)&VU->VI[REG_CLIP_FLAG], EAX);
	}
	if (s_PrevStatusWrite != (uptr)&VU->VI[REG_STATUS_FLAG])
	{
		MOV32MtoR(EAX, s_PrevStatusWrite);
		MOV32RtoM((uptr)&VU->VI[REG_STATUS_FLAG], EAX);
	}
	if (s_PrevMACWrite != (uptr)&VU->VI[REG_MAC_FLAG])
	{
		MOV32MtoR(EAX, s_PrevMACWrite);
		MOV32RtoM((uptr)&VU->VI[REG_MAC_FLAG], EAX);
	}
// 	if( s_StatusRead != (uptr)&VU->VI[REG_STATUS_FLAG] ) {
//		// only lower 8 bits valid!
//		MOVZX32M8toR(EAX, s_StatusRead);
//		MOV32RtoM((uptr)&VU->VI[REG_STATUS_FLAG], EAX);
//	}
//	if( s_MACRead != (uptr)&VU->VI[REG_MAC_FLAG] ) {
//		// only lower 8 bits valid!
//		MOVZX32M8toR(EAX, s_MACRead);
//		MOV32RtoM((uptr)&VU->VI[REG_MAC_FLAG], EAX);
//	}
	if (s_PrevIWrite != (uptr)&VU->VI[REG_I])
	{
		MOV32ItoM((uptr)&VU->VI[REG_I], *(u32*)s_PrevIWrite); // never changes
	}

	ADD32ItoM((uptr)&s_TotalVUCycles, cycles);

	// compute branches, jumps, eop
	if (type & BLOCKTYPE_HASEOP)
	{
		// end
		_freeXMMregs();
		_freeX86regs();
		AND32ItoM((uptr)&VU0.VI[ REG_VPU_STAT ].UL, s_vu ? ~0x100 : ~0x001); // E flag
		AND32ItoM((uptr)&VU->vifRegs->stat, ~VIF1_STAT_VEW);
		
		if (!branch) MOV32ItoM((uptr)&VU->VI[REG_TPC], endpc);
		
		JMP32((uptr)SuperVUEndProgram - ((uptr)x86Ptr + 5));
	}
	else
	{

		u32 livevars[2] = {0};

		list<VuInstruction>::iterator lastinst = GetInstIterAtPc(endpc - 8);
		lastinst++;

		if (lastinst != insts.end())
		{
			livevars[0] = lastinst->livevars[0];
			livevars[1] = lastinst->livevars[1];
		}
		else
		{
			// take from children
			if (blocks.size() > 0)
			{
				LISTBLOCKS::iterator itchild;
				for(itchild = blocks.begin(); itchild != blocks.end(); itchild++)
				{
					livevars[0] |= (*itchild)->insts.front().livevars[0];
					livevars[1] |= (*itchild)->insts.front().livevars[1];
				}
			}
			else
			{
				livevars[0] = ~0;
				livevars[1] = ~0;
			}
		}

		SuperVUFreeXMMregs(livevars);

		// get rid of any writes, otherwise _freeX86regs will write
		x86regs[s_JumpX86].mode &= ~MODE_WRITE;

		if (branch == 1)
		{
			if (!x86regs[s_JumpX86].inuse)
			{
				assert(x86regs[s_JumpX86].type == X86TYPE_VUJUMP);
				s_JumpX86 = 0xffffffff; // notify to jump from g_recWriteback
			}
		}

		// align VI regs
#ifdef SUPERVU_X86CACHING
		if (nEndx86 >= 0)
		{
			_x86regs* endx86 = &s_vecRegArray[nEndx86];
			for (int i = 0; i < iREGCNT_GPR; ++i)
			{
				if (endx86[i].inuse)
				{

					if (s_JumpX86 == i && x86regs[s_JumpX86].inuse)
					{
						x86regs[s_JumpX86].inuse = 0;
						x86regs[EAX].inuse = 1;
						MOV32RtoR(EAX, s_JumpX86);
						s_JumpX86 = EAX;
					}

					if (x86regs[i].inuse)
					{
						if (x86regs[i].type == endx86[i].type && x86regs[i].reg == endx86[i].reg)
						{
							_freeX86reg(i);
							// will continue to use it
							continue;
						}

#ifdef SUPERVU_INTERCACHING
						if (x86regs[i].type == (X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0)))
						{
							if (livevars[0] & (1 << x86regs[i].reg))
								_freeX86reg(i);
							else
								x86regs[i].inuse = 0;
						}
						else 
#endif
						{
							_freeX86reg(i);
						}
					}

					// realloc
					_allocX86reg(i, endx86[i].type, endx86[i].reg, MODE_READ);
					if (x86regs[i].mode & MODE_WRITE)
					{
						_freeX86reg(i);
						x86regs[i].inuse = 1;
					}
				}
				else _freeX86reg(i);
			}
		}
		else _freeX86regs();
#else
		_freeX86regs();
#endif

		// store the last block executed
		MOV32ItoM((uptr)&g_nLastBlockExecuted, s_pCurBlock->startpc);

		switch (branch)
		{
			case 1: // branch, esi has new prog

				SuperVUTestVU0Condition(0);

				if (s_JumpX86 == 0xffffffff)
					JMP32M((uptr)&g_recWriteback);
				else
					JMPR(s_JumpX86);

				break;
			case 4: // jalr
				pChildJumps[0] = (u32*)0xffffffff;
				// fall through

			case 0x10: // jump, esi has new vupc
				{
					_freeXMMregs();
					_freeX86regs();

					SuperVUTestVU0Condition(8);

					// already onto stack
					CALLFunc((uptr)SuperVUGetProgram);
					ADD32ItoR(ESP, 8);
					JMPR(EAX);
					break;
				}

			case 0x13: // jr with uncon branch, uncond branch takes precendence (dropship)
				{
//					s32 delta = (s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff) << 3;
//					ADD32ItoRmOffset(ESP, delta, 0);

					ADD32ItoR(ESP, 8); // restore
					pChildJumps[0] = (u32*)((uptr)JMP32(0) | 0x80000000);
					break;
				}
			case 0:
			case 3: // unconditional branch
				pChildJumps[s_UnconditionalDelay] = (u32*)((uptr)JMP32(0) | 0x80000000);
				break;

			default:
				DevCon::Error("Bad branch %x\n", params branch);
				assert(0);
				break;
		}
	}

	pendcode = x86Ptr;
	type |= BLOCKTYPE_ANALYZED;

	LISTBLOCKS::iterator itchild;
	for(itchild = blocks.begin(); itchild != blocks.end(); itchild++)
	{
		(*itchild)->Recompile();
	}
}

#define GET_VUXYZMODE(reg) 0//((vuxyz&(1<<(reg)))?MODE_VUXYZ:0)

int VuInstruction::SetCachedRegs(int upper, u32 vuxyz)
{
	if (vfread0[upper] >= 0)
	{
		SuperVUFreeXMMreg(vfread0[upper], XMMTYPE_VFREG, regs[upper].VFread0);
		_allocVFtoXMMreg(VU, vfread0[upper], regs[upper].VFread0, MODE_READ | GET_VUXYZMODE(regs[upper].VFread0));
	}
	if (vfread1[upper] >= 0)
	{
		SuperVUFreeXMMreg(vfread1[upper], XMMTYPE_VFREG, regs[upper].VFread1);
		_allocVFtoXMMreg(VU, vfread1[upper], regs[upper].VFread1, MODE_READ | GET_VUXYZMODE(regs[upper].VFread1));
	}
	if (vfacc[upper] >= 0 && (regs[upper].VIread&(1 << REG_ACC_FLAG)))
	{
		SuperVUFreeXMMreg(vfacc[upper], XMMTYPE_ACC, 0);
		_allocACCtoXMMreg(VU, vfacc[upper], MODE_READ);
	}
	if (vfwrite[upper] >= 0)
	{
		assert(regs[upper].VFwrite > 0);
		SuperVUFreeXMMreg(vfwrite[upper], XMMTYPE_VFREG, regs[upper].VFwrite);
		_allocVFtoXMMreg(VU, vfwrite[upper], regs[upper].VFwrite,
		                 MODE_WRITE | (regs[upper].VFwxyzw != 0xf ? MODE_READ : 0) | GET_VUXYZMODE(regs[upper].VFwrite));
	}
	if (vfacc[upper] >= 0 && (regs[upper].VIwrite&(1 << REG_ACC_FLAG)))
	{
		SuperVUFreeXMMreg(vfacc[upper], XMMTYPE_ACC, 0);
		_allocACCtoXMMreg(VU, vfacc[upper], MODE_WRITE | (regs[upper].VFwxyzw != 0xf ? MODE_READ : 0));
	}

	int info = PROCESS_VU_SUPER;
	if (vfread0[upper] >= 0) info |= PROCESS_EE_SET_S(vfread0[upper]);
	if (vfread1[upper] >= 0) info |= PROCESS_EE_SET_T(vfread1[upper]);
	if (vfacc[upper] >= 0) info |= PROCESS_VU_SET_ACC(vfacc[upper]);
	if (vfwrite[upper] >= 0)
	{
		if (regs[upper].VFwrite == _Ft_ && vfread1[upper] < 0)
		{
			info |= PROCESS_EE_SET_T(vfwrite[upper]);
		}
		else
		{
			assert(regs[upper].VFwrite == _Fd_);
			info |= PROCESS_EE_SET_D(vfwrite[upper]);
		}
	}

	if (!(vffree[upper]&VFFREE_INVALID0))
	{
		SuperVUFreeXMMreg(vffree[upper]&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, vffree[upper]&0xf);
	}
	info |= PROCESS_VU_SET_TEMP(vffree[upper] & 0xf);

	if (vfflush[upper] >= 0)
	{
		SuperVUFreeXMMreg(vfflush[upper], XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, vfflush[upper]);
	}

	if (upper && (regs[upper].VIwrite & (1 << REG_CLIP_FLAG)))
	{
		// CLIP inst, need two extra temp registers, put it EEREC_D and EEREC_ACC
		assert(vfwrite[upper] == -1);
		SuperVUFreeXMMreg((vffree[upper] >> 8)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper] >> 8)&0xf);
		info |= PROCESS_EE_SET_D((vffree[upper] >> 8) & 0xf);

		SuperVUFreeXMMreg((vffree[upper] >> 16)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper] >> 16)&0xf);
		info |= PROCESS_EE_SET_ACC((vffree[upper] >> 16) & 0xf);

		_freeXMMreg((vffree[upper] >> 8)&0xf); // don't need anymore
		_freeXMMreg((vffree[upper] >> 16)&0xf); // don't need anymore
	}
	else if (regs[upper].VIwrite & (1 << REG_P))
	{
		SuperVUFreeXMMreg((vffree[upper] >> 8)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper] >> 8)&0xf);
		info |= PROCESS_EE_SET_D((vffree[upper] >> 8) & 0xf);
		_freeXMMreg((vffree[upper] >> 8)&0xf); // don't need anymore
	}

	if (vfflush[upper] >= 0) _freeXMMreg(vfflush[upper]);
	if (!(vffree[upper]&VFFREE_INVALID0))
		_freeXMMreg(vffree[upper]&0xf); // don't need anymore

	if ((regs[0].VIwrite | regs[1].VIwrite) & ((1 << REG_STATUS_FLAG) | (1 << REG_MAC_FLAG)))
		info |= PROCESS_VU_UPDATEFLAGS;

	return info;
}

void VuInstruction::Recompile(list<VuInstruction>::iterator& itinst, u32 vuxyz)
{
	//static PCSX2_ALIGNED16(VECTOR _VF);
	//static PCSX2_ALIGNED16(VECTOR _VFc);
	u32 *ptr;
	u8* pjmp;
	int vfregstore = 0;

	assert(s_pCurInst == this);
	s_WriteToReadQ = 0;

	ptr = (u32*) & VU->Micro[ pc ];

	if (type & INST_Q_READ)
		SuperVUFlush(0, (ptr[0] == 0x800003bf) || !!(regs[0].VIwrite & (1 << REG_Q)));
	if (type & INST_P_READ)
		SuperVUFlush(1, (ptr[0] == 0x800007bf) || !!(regs[0].VIwrite & (1 << REG_P)));

	if (type & INST_DUMMY)
	{

		// find nParentPc
		VuInstruction* pparentinst = NULL;

		// if true, will check if parent block was executed before getting the results of the flags (superman returns)
		int nParentCheckForExecution = -1;

//		int badaddrs[] = {
//			0x60,0x68,0x70,0x60,0x68,0x70,0x88,0x90,0x98,0x98,0xa8,0xb8,0x88,0x90,
//			0x4a8,0x4a8,0x398,0x3a0,0x3a8,0xa0
//		};

#ifdef SUPERVU_PROPAGATEFLAGS
		if (nParentPc != -1 && (nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc))
		{

//			if( !s_vu ) {
//				for(int j = 0; j < ARRAYSIZE(badaddrs); ++j) {
//					if( badaddrs[j] == nParentPc )
//						goto NoParent;
//					}
//				}

			list<VuBaseBlock*>::iterator itblock;
			for(itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); itblock++)
			{
				if (nParentPc >= (*itblock)->startpc && nParentPc < (*itblock)->endpc)
				{
					pparentinst = &(*(*itblock)->GetInstIterAtPc(nParentPc));
					//if( !s_vu ) SysPrintf("%x ", nParentPc);
					if (find(s_pCurBlock->parents.begin(), s_pCurBlock->parents.end(), *itblock) != s_pCurBlock->parents.end())
						nParentCheckForExecution = (*itblock)->startpc;
					break;
				}
			}

			assert(pparentinst != NULL);
		}
#endif

		if (type & INST_CLIP_WRITE)
		{
			if (nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc)
			{

				if (!CHECK_VUCLIPFLAGHACK && pparentinst != NULL)
				{

					if (nParentCheckForExecution >= 0)
					{
						if (pparentinst->pClipWrite == 0)
							pparentinst->pClipWrite = (uptr)SuperVUStaticAlloc(4);

						if (s_ClipRead == 0)
							s_ClipRead = (uptr) & VU->VI[REG_CLIP_FLAG];

						CMP32ItoM((uptr)&g_nLastBlockExecuted, nParentCheckForExecution);
						u8* jptr = JNE8(0);
						CMP32ItoM((uptr)&s_ClipRead, (uptr)&VU->VI[REG_CLIP_FLAG]);
						u8* jptr2 = JE8(0);
						MOV32MtoR(EAX, pparentinst->pClipWrite);
						MOV32RtoM(s_ClipRead, EAX);
						x86SetJ8(jptr);
						x86SetJ8(jptr2);
					}
				}
				else s_ClipRead = (uptr) & VU->VI[REG_CLIP_FLAG];
			}
			else
			{
				s_ClipRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pClipWrite;
				if (s_ClipRead == 0) Console::WriteLn("super ClipRead allocation error!");
			}
		}

		// before modifying, check if they will ever be read
		if (s_pCurBlock->type & BLOCKTYPE_MACFLAGS)
		{

			u8 outofblock = 0;
			if (type & INST_STATUS_WRITE)
			{

				if (nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc)
				{

					// reading from out of this block, so already flushed to mem
					if (pparentinst != NULL)    //&& pparentinst->pStatusWrite != NULL ) {
					{

						// might not have processed it yet, so reserve a mem loc
						if (pparentinst->pStatusWrite == 0)
						{
							pparentinst->pStatusWrite = (uptr)SuperVUStaticAlloc(4);
							//MOV32ItoM(pparentinst->pStatusWrite, 0);
						}

//						if( s_pCurBlock->prevFlagsOutOfBlock && s_StatusRead != NULL ) {
//							// or instead since don't now which parent we came from
//							MOV32MtoR(EAX, pparentinst->pStatusWrite);
//							OR32RtoM(s_StatusRead, EAX);
//							MOV32ItoM(pparentinst->pStatusWrite, 0);
//						}

						if (nParentCheckForExecution >= 0)
						{

							// don't now which parent we came from, so have to check
//							uptr tempstatus = (uptr)SuperVUStaticAlloc(4);
//							if( s_StatusRead != NULL )
//								MOV32MtoR(EAX, s_StatusRead);
//							else
//								MOV32MtoR(EAX, (uptr)&VU->VI[REG_STATUS_FLAG]);
//							s_StatusRead = tempstatus;
							
							if (s_StatusRead == 0)
								s_StatusRead = (uptr) & VU->VI[REG_STATUS_FLAG];

							CMP32ItoM((uptr)&g_nLastBlockExecuted, nParentCheckForExecution);
							u8* jptr = JNE8(0);
							MOV32MtoR(EAX, pparentinst->pStatusWrite);
							MOV32ItoM(pparentinst->pStatusWrite, 0);
							MOV32RtoM(s_StatusRead, EAX);
							x86SetJ8(jptr);
						}
						else
						{
							uptr tempstatus = (uptr)SuperVUStaticAlloc(4);
							MOV32MtoR(EAX, pparentinst->pStatusWrite);
							MOV32RtoM(tempstatus, EAX);
							MOV32ItoM(pparentinst->pStatusWrite, 0);
							s_StatusRead = tempstatus;
						}

						outofblock = 2;
					}
					else
						s_StatusRead = (uptr) & VU->VI[REG_STATUS_FLAG];
				}
				else
				{
					s_StatusRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pStatusWrite;
					if (s_StatusRead == 0) Console::WriteLn("super StatusRead allocation error!");
//					if( pc >= (u32)s_pCurBlock->endpc-8 ) {
//						// towards the end, so variable might be leaded to another block (silent hill 4)
//						uptr tempstatus = (uptr)SuperVUStaticAlloc(4);
//						MOV32MtoR(EAX, s_StatusRead);
//						MOV32RtoM(tempstatus, EAX);
//						MOV32ItoM(s_StatusRead, 0);
//						s_StatusRead = tempstatus;
//						}
				}
			}
			if (type & INST_MAC_WRITE)
			{

				if (nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc)
				{
					// reading from out of this block, so already flushed to mem

					if (pparentinst != NULL)   //&& pparentinst->pMACWrite != NULL ) {
					{
						// necessary for (katamari)
						// towards the end, so variable might be leaked to another block (silent hill 4)

						// might not have processed it yet, so reserve a mem loc
						if (pparentinst->pMACWrite == 0)
						{
							pparentinst->pMACWrite = (uptr)SuperVUStaticAlloc(4);
							//MOV32ItoM(pparentinst->pMACWrite, 0);
						}

//						if( s_pCurBlock->prevFlagsOutOfBlock && s_MACRead != NULL ) {
//							// or instead since don't now which parent we came from
//							MOV32MtoR(EAX, pparentinst->pMACWrite);
//							OR32RtoM(s_MACRead, EAX);
//							MOV32ItoM(pparentinst->pMACWrite, 0);
//						}
						if (nParentCheckForExecution >= 0)
						{

							// don't now which parent we came from, so have to check
//							uptr tempmac = (uptr)SuperVUStaticAlloc(4);
//							if( s_MACRead != NULL )
//								MOV32MtoR(EAX, s_MACRead);
//							else
//								MOV32MtoR(EAX, (uptr)&VU->VI[REG_MAC_FLAG]);
//							s_MACRead = tempmac;

							if (s_MACRead == 0) s_MACRead = (uptr) & VU->VI[REG_MAC_FLAG];

							CMP32ItoM((uptr)&g_nLastBlockExecuted, nParentCheckForExecution);
							u8* jptr = JNE8(0);
							MOV32MtoR(EAX, pparentinst->pMACWrite);
							MOV32ItoM(pparentinst->pMACWrite, 0);
							MOV32RtoM(s_MACRead, EAX);
							x86SetJ8(jptr);
						}
						else
						{
							uptr tempMAC = (uptr)SuperVUStaticAlloc(4);
							MOV32MtoR(EAX, pparentinst->pMACWrite);
							MOV32RtoM(tempMAC, EAX);
							MOV32ItoM(pparentinst->pMACWrite, 0);
							s_MACRead = tempMAC;
						}

						outofblock = 2;
					}
					else
						s_MACRead = (uptr) & VU->VI[REG_MAC_FLAG];

//					if( pc >= (u32)s_pCurBlock->endpc-8 ) {
//						// towards the end, so variable might be leaked to another block (silent hill 4)
//						uptr tempMAC = (uptr)SuperVUStaticAlloc(4);
//						MOV32MtoR(EAX, s_MACRead);
//						MOV32RtoM(tempMAC, EAX);
//						MOV32ItoM(s_MACRead, 0);
//						s_MACRead = tempMAC;
//					}
				}
				else
				{
					s_MACRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pMACWrite;
				}
			}

			s_pCurBlock->prevFlagsOutOfBlock = outofblock;
		}
		else if (pparentinst != NULL)
		{
			// make sure to reset the mac and status flags! (katamari)
			if (pparentinst->pStatusWrite)
				MOV32ItoM(pparentinst->pStatusWrite, 0);
			if (pparentinst->pMACWrite)
				MOV32ItoM(pparentinst->pMACWrite, 0);
		}

		assert(s_ClipRead != 0);
		assert(s_MACRead != 0);
		assert(s_StatusRead != 0);

		return;
	}

	s_pCurBlock->prevFlagsOutOfBlock = 0;

	if( IsDebugBuild )
		MOV32ItoR(EAX, pc);

	assert(!(type & (INST_CLIP_WRITE | INST_STATUS_WRITE | INST_MAC_WRITE)));
	pc += 8;

	list<VuInstruction>::const_iterator itinst2;

	if ((regs[0].VIwrite | regs[1].VIwrite) & ((1 << REG_MAC_FLAG) | (1 << REG_STATUS_FLAG)))
	{
		if (s_pCurBlock->type & BLOCKTYPE_MACFLAGS)
		{
			if (pMACWrite == 0)
			{
				pMACWrite = (uptr)SuperVUStaticAlloc(4);
				//MOV32ItoM(pMACWrite, 0);
			}
			if (pStatusWrite == 0)
			{
				pStatusWrite = (uptr)SuperVUStaticAlloc(4);
				//MOV32ItoM(pStatusWrite, 0);
			}
		}
		else
		{
			assert(s_StatusRead == (uptr)&VU->VI[REG_STATUS_FLAG]);
			assert(s_MACRead == (uptr)&VU->VI[REG_MAC_FLAG]);
			pMACWrite = s_MACRead;
			pStatusWrite = s_StatusRead;
		}
	}

	if ((pClipWrite == 0) && ((regs[0].VIwrite | regs[1].VIwrite) & (1 << REG_CLIP_FLAG)))
	{
		pClipWrite = (uptr)SuperVUStaticAlloc(4);
		//MOV32ItoM(pClipWrite, 0);
	}

#ifdef SUPERVU_X86CACHING
	// redo the counters so that the proper regs are released
	for (int j = 0; j < iREGCNT_GPR; ++j)
	{
		if (x86regs[j].inuse && X86_ISVI(x86regs[j].type))
		{
			int count = 0;
			itinst2 = itinst;

			while (itinst2 != s_pCurBlock->insts.end())
			{
				if ((itinst2->regs[0].VIread | itinst2->regs[0].VIwrite | itinst2->regs[1].VIread | itinst2->regs[1].VIwrite) && (1 << x86regs[j].reg))
					break;

				++count;
				++itinst2;
			}

			x86regs[j].counter = 1000 - count;
		}
	}
#endif

	if (s_vu == 0 && (ptr[1] & 0x20000000))   // M flag
	{
		OR8ItoM((uptr)&VU->flags, VUFLAG_MFLAGSET);
	}
	if (ptr[1] & 0x10000000)   // D flag
	{
		TEST32ItoM((uptr)&VU0.VI[REG_FBRST].UL, s_vu ? 0x400 : 0x004);
		u8* ptr = JZ8(0);
		OR32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, s_vu ? 0x200 : 0x002);
		_callFunctionArg1((uptr)hwIntcIrq, MEM_CONSTTAG, s_vu ? INTC_VU1 : INTC_VU0);
		x86SetJ8(ptr);
	}
	if (ptr[1] & 0x08000000)   // T flag
	{
		TEST32ItoM((uptr)&VU0.VI[REG_FBRST].UL, s_vu ? 0x800 : 0x008);
		u8* ptr = JZ8(0);
		OR32ItoM((uptr)&VU0.VI[REG_VPU_STAT].UL, s_vu ? 0x400 : 0x004);
		_callFunctionArg1((uptr)hwIntcIrq, MEM_CONSTTAG, s_vu ? INTC_VU1 : INTC_VU0);
		x86SetJ8(ptr);
	}

	// check upper flags
	if (ptr[1] & 0x80000000)   // I flag
	{

		assert(!(regs[0].VIwrite & ((1 << REG_Q) | (1 << REG_P))));

		VU->code = ptr[1];
		s_vuInfo = SetCachedRegs(1, vuxyz);
		if (s_JumpX86 > 0) x86regs[s_JumpX86].needed = 1;
		if (s_ScheduleXGKICK && s_XGKICKReg > 0) x86regs[s_XGKICKReg].needed = 1;

		recVU_UPPER_OPCODE[ VU->code & 0x3f ](VU, s_vuInfo);

		s_PrevIWrite = (uptr)ptr;
		_clearNeededXMMregs();
		_clearNeededX86regs();
	}
	else
	{
		if (regs[0].VIwrite & (1 << REG_Q))
		{

			// search for all the insts between this inst and writeback
			itinst2 = itinst;
			++itinst2;
			u32 cacheq = (itinst2 == s_pCurBlock->insts.end());
			u32* codeptr2 = ptr + 2;

			while (itinst2 != s_pCurBlock->insts.end())
			{
				if (!(itinst2->type & INST_DUMMY) && ((itinst2->regs[0].VIwrite&(1 << REG_Q)) || codeptr2[0] == 0x800003bf))  // waitq, or fdiv inst
				{
					break;
				}
				if ((itinst2->type & INST_Q_WRITE) && itinst2->nParentPc == pc - 8)
				{
					break;
				}
				if (itinst2->type & INST_Q_READ)
				{
					cacheq = 1;
					break;
				}
				if (itinst2->type & INST_DUMMY)
				{
					++itinst2;
					continue;
				}
				codeptr2 += 2;
				++itinst2;
			}

			if (itinst2 == s_pCurBlock->insts.end())
				cacheq = 1;

			int x86temp = -1;
			if (cacheq)
				x86temp = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);

			// new is written so flush old
			// if type & INST_Q_READ, already flushed
			if (!(type & INST_Q_READ) && s_recWriteQ == 0) MOV32MtoR(EAX, (uptr)&s_writeQ);

			if (cacheq)
				MOV32MtoR(x86temp, (uptr)&s_TotalVUCycles);

			if (!(type & INST_Q_READ))
			{
				if (s_recWriteQ == 0)
				{
					OR32RtoR(EAX, EAX);
					pjmp = JS8(0);
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_Q, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_Q, 1), EAX);
					x86SetJ8(pjmp);
				}
				else if (s_needFlush & 1)
				{
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_Q, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_Q, 1), EAX);
					s_needFlush &= ~1;
				}
			}

			// write new Q
			if (cacheq)
			{
				assert(s_pCurInst->pqcycles > 1);
				ADD32ItoR(x86temp, s_pCurInst->info.cycle + s_pCurInst->pqcycles);
				MOV32RtoM((uptr)&s_writeQ, x86temp);
				s_needFlush |= 1;
			}
			else
			{
				// won't be writing back
				s_WriteToReadQ = 1;
				s_needFlush &= ~1;
				MOV32ItoM((uptr)&s_writeQ, 0x80000001);
			}

			s_recWriteQ = s_pCurInst->info.cycle + s_pCurInst->pqcycles;

			if (x86temp >= 0)
				_freeX86reg(x86temp);
		}

		if (regs[0].VIwrite & (1 << REG_P))
		{
			int x86temp = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);

			// new is written so flush old
			if (!(type & INST_P_READ) && s_recWriteP == 0)
				MOV32MtoR(EAX, (uptr)&s_writeP);
			MOV32MtoR(x86temp, (uptr)&s_TotalVUCycles);

			if (!(type & INST_P_READ))
			{
				if (s_recWriteP == 0)
				{
					OR32RtoR(EAX, EAX);
					pjmp = JS8(0);
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_P, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_P, 1), EAX);
					x86SetJ8(pjmp);
				}
				else if (s_needFlush & 2)
				{
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_P, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_P, 1), EAX);
					s_needFlush &= ~2;
				}
			}

			// write new P
			assert(s_pCurInst->pqcycles > 1);
			ADD32ItoR(x86temp, s_pCurInst->info.cycle + s_pCurInst->pqcycles);
			MOV32RtoM((uptr)&s_writeP, x86temp);
			s_needFlush |= 2;

			s_recWriteP = s_pCurInst->info.cycle + s_pCurInst->pqcycles;

			_freeX86reg(x86temp);
		}
		
		// waitq
		if (ptr[0] == 0x800003bf) SuperVUFlush(0, 1);
		// waitp
		if (ptr[0] == 0x800007bf) SuperVUFlush(1, 1);

#ifdef PCSX2_DEVBUILD
		if (regs[1].VIread & regs[0].VIwrite & ~((1 << REG_Q) | (1 << REG_P) | (1 << REG_VF0_FLAG) | (1 << REG_ACC_FLAG)))
		{
			Console::Notice("*PCSX2*: Warning, VI write to the same reg %x in both lower/upper cycle %x", params regs[1].VIread & regs[0].VIwrite, s_pCurBlock->startpc);
		}
#endif

		u32 modewrite = 0;
		if (vfwrite[1] >= 0 && xmmregs[vfwrite[1]].inuse && xmmregs[vfwrite[1]].type == XMMTYPE_VFREG && xmmregs[vfwrite[1]].reg == regs[1].VFwrite)
			modewrite = xmmregs[vfwrite[1]].mode & MODE_WRITE;

		VU->code = ptr[1];
		s_vuInfo = SetCachedRegs(1, vuxyz);

		if (vfwrite[1] >= 0)
		{
			assert(regs[1].VFwrite > 0);

			if (vfwrite[0] == vfwrite[1])
			{
				//Console::WriteLn("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle %x", params s_pCurBlock->startpc);
			}

			if (vfread0[0] == vfwrite[1] || vfread1[0] == vfwrite[1])
			{
				assert(regs[0].VFread0 == regs[1].VFwrite || regs[0].VFread1 == regs[1].VFwrite);
				assert(vfflush[0] >= 0);
				if (modewrite)
				{
					SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[regs[1].VFwrite], (x86SSERegType)vfwrite[1]);
				}
				vfregstore = 1;
			}
		}

		if (s_JumpX86 > 0) x86regs[s_JumpX86].needed = 1;
		if (s_ScheduleXGKICK && s_XGKICKReg > 0) x86regs[s_XGKICKReg].needed = 1;

		recVU_UPPER_OPCODE[ VU->code & 0x3f ](VU, s_vuInfo);
		_clearNeededXMMregs();
		_clearNeededX86regs();

		// necessary because status can be set by both upper and lower
		if (regs[1].VIwrite & (1 << REG_STATUS_FLAG))
		{
			assert(pStatusWrite != 0);
			s_PrevStatusWrite = pStatusWrite;
		}

		VU->code = ptr[0];
		s_vuInfo = SetCachedRegs(0, vuxyz);

		if (vfregstore)
		{
			// load
			SSE_MOVAPS_M128_to_XMM(vfflush[0], (uptr)&VU->VF[regs[1].VFwrite]);

			assert(xmmregs[vfwrite[1]].mode & MODE_WRITE);

			// replace with vfflush
			if (_Fs_ == regs[1].VFwrite)
			{
				s_vuInfo &= ~PROCESS_EE_SET_S(0xf);
				s_vuInfo |= PROCESS_EE_SET_S(vfflush[0]);
			}
			if (_Ft_ == regs[1].VFwrite)
			{
				s_vuInfo &= ~PROCESS_EE_SET_T(0xf);
				s_vuInfo |= PROCESS_EE_SET_T(vfflush[0]);
			}

			xmmregs[vfflush[0]].mode |= MODE_NOFLUSH | MODE_WRITE; // so that lower inst doesn't flush
		}

		// notify vuinsts that upper inst is a fmac
		if (regs[1].pipe == VUPIPE_FMAC)
			s_vuInfo |= PROCESS_VU_SET_FMAC();

		if (s_JumpX86 > 0) x86regs[s_JumpX86].needed = 1;
		if (s_ScheduleXGKICK && s_XGKICKReg > 0) x86regs[s_XGKICKReg].needed = 1;

#ifdef SUPERVU_VIBRANCHDELAY
		if (type & INST_CACHE_VI)
		{
			assert(vicached >= 0);
			int cachedreg = _allocX86reg(-1, X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), vicached, MODE_READ);
			MOV32RtoM((uptr)&s_VIBranchDelay, cachedreg);
		}
#endif

		// check if inst before branch and the write is the same as the read in the branch (wipeout)
//		int oldreg=0;
//		if( pc == s_pCurBlock->endpc-16 ) {
//			itinst2 = itinst; ++itinst2;
//			if( itinst2->regs[0].pipe == VUPIPE_BRANCH && (itinst->regs[0].VIwrite&itinst2->regs[0].VIread) ) {
//
//				CALLFunc((u32)branchfn);
//				assert( itinst->regs[0].VIwrite & 0xffff );
//				Console::WriteLn("vi write before branch");
//				for(s_CacheVIReg = 0; s_CacheVIReg < 16; ++s_CacheVIReg) {
//					if( itinst->regs[0].VIwrite & (1<<s_CacheVIReg) )
//						break;
//				}
//
//				oldreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), s_CacheVIReg, MODE_READ);
//				s_CacheVIX86 = _allocX86reg(-1, X86TYPE_VITEMP, s_CacheVIReg, MODE_WRITE);
//				MOV32RtoR(s_CacheVIX86, oldreg);
//			}
//		}
//		else if( pc == s_pCurBlock->endpc-8 && s_CacheVIReg >= 0 ) {
//			assert( s_CacheVIX86 > 0 && x86regs[s_CacheVIX86].inuse && x86regs[s_CacheVIX86].reg == s_CacheVIReg && x86regs[s_CacheVIX86].type == X86TYPE_VITEMP );
//
//			oldreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), s_CacheVIReg, MODE_READ);
//			x86regs[s_CacheVIX86].needed = 1;
//			assert( x86regs[oldreg].mode & MODE_WRITE );
//
//			x86regs[s_CacheVIX86].type = X86TYPE_VI|(s_vu?X86TYPE_VU1:0);
//			x86regs[oldreg].type = X86TYPE_VITEMP;
//		}

		recVU_LOWER_OPCODE[ VU->code >> 25 ](VU, s_vuInfo);

//		if( pc == s_pCurBlock->endpc-8 && s_CacheVIReg >= 0 ) {
//			// revert
//			x86regs[s_CacheVIX86].inuse = 0;
//			x86regs[oldreg].type = X86TYPE_VI|(s_vu?X86TYPE_VU1:0);
//		}

		_clearNeededXMMregs();
		_clearNeededX86regs();
	}

	// clip is always written so ok
	if ((regs[0].VIwrite | regs[1].VIwrite) & (1 << REG_CLIP_FLAG))
	{
		assert(pClipWrite != 0);
		s_PrevClipWrite = pClipWrite;
	}

	if ((regs[0].VIwrite | regs[1].VIwrite) & (1 << REG_STATUS_FLAG))
	{
		assert(pStatusWrite != 0);
		s_PrevStatusWrite = pStatusWrite;
	}

	if ((regs[0].VIwrite | regs[1].VIwrite) & (1 << REG_MAC_FLAG))
	{
		assert(pMACWrite != 0);
		s_PrevMACWrite = pMACWrite;
	}
}

///////////////////////////////////
// Super VU Recompilation Tables //
///////////////////////////////////

void recVUMI_BranchHandle()
{
	int bpc = _recbranchAddr(VU->code);
	int curjump = 0;

	if (s_pCurInst->type & INST_BRANCH_DELAY)
	{
		assert((branch&0x17) != 0x10 && (branch&0x17) != 4); // no jump handlig for now

		if ((branch & 0x7) == 3)
		{
			// previous was a direct jump
			curjump = 1;
		}
		else if (branch & 1) curjump = 2;
	}

	assert(s_JumpX86 > 0);

	if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 || SUPERVU_CHECKCONDITION)
		MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);
	MOV32ItoR(s_JumpX86, 1);		// use 1 to disable optimization to XOR
	s_pCurBlock->pChildJumps[curjump] = (u32*)x86Ptr - 1;

	if (!(s_pCurInst->type & INST_BRANCH_DELAY))
	{
		j8Ptr[1] = JMP8(0);
		x86SetJ8(j8Ptr[ 0 ]);

		if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 || SUPERVU_CHECKCONDITION)
			MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), pc + 8);
		MOV32ItoR(s_JumpX86, 1);	// use 1 to disable optimization to XOR
		s_pCurBlock->pChildJumps[curjump+1] = (u32*)x86Ptr - 1;

		x86SetJ8(j8Ptr[ 1 ]);
	}
	else
		x86SetJ8(j8Ptr[ 0 ]);

	branch |= 1;
}

// supervu specific insts
void recVUMI_IBQ_prep()
{
	int isreg, itreg;

	if (_Is_ == 0)
	{
#ifdef SUPERVU_VIBRANCHDELAY
		if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _It_)
		{
			itreg = -1;
		}
		else
#endif
		{
			itreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _It_, MODE_READ);
		}

		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if (itreg >= 0)
		{
			CMP16ItoR(itreg, 0);
		}
		else CMP16ItoM(SuperVUGetVIAddr(_It_, 1), 0);
	}
	else if (_It_ == 0)
	{
#ifdef SUPERVU_VIBRANCHDELAY
		if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
		{
			isreg = -1;
		}
		else
#endif
		{
			isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
		}

		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if (isreg >= 0)
		{
			CMP16ItoR(isreg, 0);
		}
		else CMP16ItoM(SuperVUGetVIAddr(_Is_, 1), 0);

	}
	else
	{
		_addNeededX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _It_);

#ifdef SUPERVU_VIBRANCHDELAY
		if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
		{
			isreg = -1;
		}
		else
#endif
		{
			isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
		}

#ifdef SUPERVU_VIBRANCHDELAY
		if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _It_)
		{
			itreg = -1;

			if (isreg <= 0)
			{
				// allocate fsreg
				if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
				{
					isreg = _allocX86reg(-1, X86TYPE_TEMP, 0, MODE_READ | MODE_WRITE);
					MOV32MtoR(isreg, SuperVUGetVIAddr(_Is_, 1));
				}
				else
					isreg = _allocX86reg(-1, X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
			}
		}
		else
#endif
		{
			itreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _It_, MODE_READ);
		}

		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if (isreg >= 0)
		{
			if (itreg >= 0)
			{
				CMP16RtoR(isreg, itreg);
			}
			else CMP16MtoR(isreg, SuperVUGetVIAddr(_It_, 1));
		}
		else if (itreg >= 0)
		{
			CMP16MtoR(itreg, SuperVUGetVIAddr(_Is_, 1));
		}
		else
		{
			isreg = _allocX86reg(-1, X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
			CMP16MtoR(isreg, SuperVUGetVIAddr(_It_, 1));
		}
	}
}

void recVUMI_IBEQ(VURegs* vuu, s32 info)
{
	recVUMI_IBQ_prep();
	j8Ptr[ 0 ] = JNE8(0);
	recVUMI_BranchHandle();
}

void recVUMI_IBGEZ(VURegs* vuu, s32 info)
{
	int isreg;
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

#ifdef SUPERVU_VIBRANCHDELAY
	if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
	{
		isreg = -1;
	}
	else
#endif
	{
		isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	}

	if (isreg >= 0)
	{
		TEST16RtoR(isreg, isreg);
		j8Ptr[ 0 ] = JS8(0);
	}
	else
	{
		CMP16ItoM(SuperVUGetVIAddr(_Is_, 1), 0x0);
		j8Ptr[ 0 ] = JL8(0);
	}

	recVUMI_BranchHandle();
}

void recVUMI_IBGTZ(VURegs* vuu, s32 info)
{
	int isreg;
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

#ifdef SUPERVU_VIBRANCHDELAY
	if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
	{
		isreg = -1;
	}
	else
#endif
	{
		isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	}

	if (isreg >= 0)
	{
		CMP16ItoR(isreg, 0);
		j8Ptr[ 0 ] = JLE8(0);
	}
	else
	{
		CMP16ItoM(SuperVUGetVIAddr(_Is_, 1), 0x0);
		j8Ptr[ 0 ] = JLE8(0);
	}
	recVUMI_BranchHandle();
}

void recVUMI_IBLEZ(VURegs* vuu, s32 info)
{
	int isreg;
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

#ifdef SUPERVU_VIBRANCHDELAY
	if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
	{
		isreg = -1;
	}
	else
#endif
	{
		isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	}

	if (isreg >= 0)
	{
		CMP16ItoR(isreg, 0);
		j8Ptr[ 0 ] = JG8(0);
	}
	else
	{
		CMP16ItoM(SuperVUGetVIAddr(_Is_, 1), 0x0);
		j8Ptr[ 0 ] = JG8(0);
	}
	recVUMI_BranchHandle();
}

void recVUMI_IBLTZ(VURegs* vuu, s32 info)
{
	int isreg;
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

#ifdef SUPERVU_VIBRANCHDELAY
	if (s_pCurInst->vicached >= 0 && s_pCurInst->vicached == _Is_)
	{
		isreg = -1;
	}
	else
#endif
	{
		isreg = _checkX86reg(X86TYPE_VI | (VU == &VU1 ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	}

	if (isreg >= 0)
	{
		TEST16RtoR(isreg, isreg);
		j8Ptr[ 0 ] = JNS8(0);
	}
	else
	{
		CMP16ItoM(SuperVUGetVIAddr(_Is_, 1), 0x0);
		j8Ptr[ 0 ] = JGE8(0);
	}
	recVUMI_BranchHandle();
}

void recVUMI_IBNE(VURegs* vuu, s32 info)
{
	recVUMI_IBQ_prep();
	j8Ptr[ 0 ] = JE8(0);
	recVUMI_BranchHandle();
}

void recVUMI_B(VURegs* vuu, s32 info)
{
	// supervu will take care of the rest
	int bpc = _recbranchAddr(VU->code);
	if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 || SUPERVU_CHECKCONDITION)
		MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);

	// loops to self, so check condition
	if (bpc == s_pCurBlock->startpc && (s_vu == 0 || SUPERVU_CHECKCONDITION))
	{
		SuperVUTestVU0Condition(0);
	}

	if (s_pCurBlock->blocks.size() > 1)
	{
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);
		MOV32ItoR(s_JumpX86, 1);
		s_pCurBlock->pChildJumps[(s_pCurInst->type & INST_BRANCH_DELAY)?1:0] = (u32*)x86Ptr - 1;
		s_UnconditionalDelay = 1;
	}

	branch |= 3;
}

void recVUMI_BAL(VURegs* vuu, s32 info)
{
	int bpc = _recbranchAddr(VU->code);
	if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 || SUPERVU_CHECKCONDITION)
		MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);

	// loops to self, so check condition
	if (bpc == s_pCurBlock->startpc && (s_vu == 0 || SUPERVU_CHECKCONDITION))
	{
		SuperVUTestVU0Condition(0);
	}

	if (_It_)
	{
		_deleteX86reg(X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _It_, 2);
		MOV16ItoM(SuperVUGetVIAddr(_It_, 0), (pc + 8) >> 3);
	}

	if (s_pCurBlock->blocks.size() > 1)
	{
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);
		MOV32ItoR(s_JumpX86, 1);
		s_pCurBlock->pChildJumps[(s_pCurInst->type & INST_BRANCH_DELAY)?1:0] = (u32*)x86Ptr - 1;
		s_UnconditionalDelay = 1;
	}

	branch |= 3;
}

void recVUMI_JR(VURegs* vuu, s32 info)
{
	int isreg = _allocX86reg(-1, X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	LEA32RStoR(EAX, isreg, 3);

	//Mask the address to something valid
	if (vuu == &VU0)
		AND32ItoR(EAX, 0xfff);
	else
		AND32ItoR(EAX, 0x3fff);

	if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0) MOV32RtoM(SuperVUGetVIAddr(REG_TPC, 0), EAX);

	if (!(s_pCurBlock->type & BLOCKTYPE_HASEOP))
	{
		PUSH32I(s_vu);
		PUSH32R(EAX);
	}
	branch |= 0x10; // 0x08 is reserved
}

void recVUMI_JALR(VURegs* vuu, s32 info)
{
	_addNeededX86reg(X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _It_);

	int isreg = _allocX86reg(-1, X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	LEA32RStoR(EAX, isreg, 3);

	//Mask the address to something valid
	if (vuu == &VU0)
		AND32ItoR(EAX, 0xfff);
	else
		AND32ItoR(EAX, 0x3fff);

	if (_It_)
	{
		_deleteX86reg(X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _It_, 2);
		MOV16ItoM(SuperVUGetVIAddr(_It_, 0), (pc + 8) >> 3);
	}

	if ((s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0) MOV32RtoM(SuperVUGetVIAddr(REG_TPC, 0), EAX);

	if (!(s_pCurBlock->type & BLOCKTYPE_HASEOP))
	{
		PUSH32I(s_vu);
		PUSH32R(EAX);
	}

	branch |= 4;
}

#ifdef PCSX2_DEVBUILD
void vu1xgkick(u32* pMem, u32 addr)
{
	assert(addr < 0x4000);
#ifdef SUPERVU_COUNT
	StopSVUCounter();
#endif

	GSGIFTRANSFER1(pMem, addr);

#ifdef SUPERVU_COUNT
	StartSVUCounter();
#endif
}
#endif

void recVUMI_XGKICK_(VURegs *VU)
{
	assert(s_XGKICKReg > 0 && x86regs[s_XGKICKReg].inuse && x86regs[s_XGKICKReg].type == X86TYPE_VITEMP);

	x86regs[s_XGKICKReg].inuse = 0; // so free doesn't flush
	_freeX86regs();
	_freeXMMregs();

	OR32ItoM((uptr)&psHu32(GIF_STAT), (GIF_STAT_APATH1 | GIF_STAT_OPH)); // Set PATH1 GIF Status Flags
	PUSH32R(s_XGKICKReg);
	PUSH32I((uptr)VU->Mem);

	if (mtgsThread) {
		CALLFunc((uptr)VU1XGKICK_MTGSTransfer);
		ADD32ItoR(ESP, 8);
	}
	else {
#ifdef PCSX2_DEVBUILD
		CALLFunc((uptr)vu1xgkick);
		ADD32ItoR(ESP, 8);
#else
		CALLFunc((uptr)GSgifTransfer1);
#endif
	}
	AND32ItoM((uptr)&psHu32(GIF_STAT), ~(GIF_STAT_APATH1 | GIF_STAT_OPH)); // Clear PATH1 GIF Status Flags
	s_ScheduleXGKICK = 0;
}

void recVUMI_XGKICK(VURegs *VU, int info)
{
	if (s_ScheduleXGKICK) {
		// second xgkick, so launch the first
		recVUMI_XGKICK_(VU);
	}

	int isreg = _allocX86reg(X86ARG2, X86TYPE_VI | (s_vu ? X86TYPE_VU1 : 0), _Is_, MODE_READ);
	_freeX86reg(isreg); // flush
	x86regs[isreg].inuse = 1;
	x86regs[isreg].type = X86TYPE_VITEMP;
	x86regs[isreg].needed = 1;
	x86regs[isreg].mode = MODE_WRITE | MODE_READ;
	SHL32ItoR(isreg, 4);
	AND32ItoR(isreg, 0x3fff);
	s_XGKICKReg = isreg;
	
	if (!SUPERVU_XGKICKDELAY || pc == s_pCurBlock->endpc) {
		recVUMI_XGKICK_(VU);
	}
	else {
		s_ScheduleXGKICK = (CHECK_XGKICKHACK) ? (min((u32)4, (s_pCurBlock->endpc-pc)/8)) : 2; 
	}
}

void recVU_UPPER_FD_00(VURegs* VU, s32 info);
void recVU_UPPER_FD_01(VURegs* VU, s32 info);
void recVU_UPPER_FD_10(VURegs* VU, s32 info);
void recVU_UPPER_FD_11(VURegs* VU, s32 info);
void recVULowerOP(VURegs* VU, s32 info);
void recVULowerOP_T3_00(VURegs* VU, s32 info);
void recVULowerOP_T3_01(VURegs* VU, s32 info);
void recVULowerOP_T3_10(VURegs* VU, s32 info);
void recVULowerOP_T3_11(VURegs* VU, s32 info);
void recVUunknown(VURegs* VU, s32 info);

void (*recVU_LOWER_OPCODE[128])(VURegs* VU, s32 info) =
{
	recVUMI_LQ    , recVUMI_SQ    , recVUunknown , recVUunknown,
	recVUMI_ILW   , recVUMI_ISW   , recVUunknown , recVUunknown,
	recVUMI_IADDIU, recVUMI_ISUBIU, recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_FCEQ  , recVUMI_FCSET , recVUMI_FCAND, recVUMI_FCOR, /* 0x10 */
	recVUMI_FSEQ  , recVUMI_FSSET , recVUMI_FSAND, recVUMI_FSOR,
	recVUMI_FMEQ  , recVUunknown  , recVUMI_FMAND, recVUMI_FMOR,
	recVUMI_FCGET , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_B     , recVUMI_BAL   , recVUunknown , recVUunknown, /* 0x20 */
	recVUMI_JR    , recVUMI_JALR  , recVUunknown , recVUunknown,
	recVUMI_IBEQ  , recVUMI_IBNE  , recVUunknown , recVUunknown,
	recVUMI_IBLTZ , recVUMI_IBGTZ , recVUMI_IBLEZ, recVUMI_IBGEZ,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x30 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVULowerOP  , recVUunknown  , recVUunknown , recVUunknown, /* 0x40*/
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x50 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x60 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x70 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
};

void (*recVULowerOP_T3_00_OPCODE[32])(VURegs* VU, s32 info) =
{
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_MOVE  , recVUMI_LQI   , recVUMI_DIV  , recVUMI_MTIR,
	recVUMI_RNEXT , recVUunknown  , recVUunknown , recVUunknown, /* 0x10 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUMI_MFP   , recVUMI_XTOP , recVUMI_XGKICK,
	recVUMI_ESADD , recVUMI_EATANxy, recVUMI_ESQRT, recVUMI_ESIN,
};

void (*recVULowerOP_T3_01_OPCODE[32])(VURegs* VU, s32 info) =
{
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_MR32  , recVUMI_SQI   , recVUMI_SQRT , recVUMI_MFIR,
	recVUMI_RGET  , recVUunknown  , recVUunknown , recVUunknown, /* 0x10 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUMI_XITOP, recVUunknown,
	recVUMI_ERSADD, recVUMI_EATANxz, recVUMI_ERSQRT, recVUMI_EATAN,
};

void (*recVULowerOP_T3_10_OPCODE[32])(VURegs* VU, s32 info) =
{
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUMI_LQD   , recVUMI_RSQRT, recVUMI_ILWR,
	recVUMI_RINIT , recVUunknown  , recVUunknown , recVUunknown, /* 0x10 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_ELENG , recVUMI_ESUM  , recVUMI_ERCPR, recVUMI_EEXP,
};

void (*recVULowerOP_T3_11_OPCODE[32])(VURegs* VU, s32 info) =
{
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUMI_SQD   , recVUMI_WAITQ, recVUMI_ISWR,
	recVUMI_RXOR  , recVUunknown  , recVUunknown , recVUunknown, /* 0x10 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_ERLENG, recVUunknown  , recVUMI_WAITP, recVUunknown,
};

void (*recVULowerOP_OPCODE[64])(VURegs* VU, s32 info) =
{
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x10 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown, /* 0x20 */
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVUMI_IADD  , recVUMI_ISUB  , recVUMI_IADDI, recVUunknown, /* 0x30 */
	recVUMI_IAND  , recVUMI_IOR   , recVUunknown , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown , recVUunknown,
	recVULowerOP_T3_00, recVULowerOP_T3_01, recVULowerOP_T3_10, recVULowerOP_T3_11,
};

void (*recVU_UPPER_OPCODE[64])(VURegs* VU, s32 info) =
{
	recVUMI_ADDx  , recVUMI_ADDy  , recVUMI_ADDz  , recVUMI_ADDw,
	recVUMI_SUBx  , recVUMI_SUBy  , recVUMI_SUBz  , recVUMI_SUBw,
	recVUMI_MADDx , recVUMI_MADDy , recVUMI_MADDz , recVUMI_MADDw,
	recVUMI_MSUBx , recVUMI_MSUBy , recVUMI_MSUBz , recVUMI_MSUBw,
	recVUMI_MAXx  , recVUMI_MAXy  , recVUMI_MAXz  , recVUMI_MAXw,  /* 0x10 */
	recVUMI_MINIx , recVUMI_MINIy , recVUMI_MINIz , recVUMI_MINIw,
	recVUMI_MULx  , recVUMI_MULy  , recVUMI_MULz  , recVUMI_MULw,
	recVUMI_MULq  , recVUMI_MAXi  , recVUMI_MULi  , recVUMI_MINIi,
	recVUMI_ADDq  , recVUMI_MADDq , recVUMI_ADDi  , recVUMI_MADDi, /* 0x20 */
	recVUMI_SUBq  , recVUMI_MSUBq , recVUMI_SUBi  , recVUMI_MSUBi,
	recVUMI_ADD   , recVUMI_MADD  , recVUMI_MUL   , recVUMI_MAX,
	recVUMI_SUB   , recVUMI_MSUB  , recVUMI_OPMSUB, recVUMI_MINI,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown,  /* 0x30 */
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown,
	recVU_UPPER_FD_00, recVU_UPPER_FD_01, recVU_UPPER_FD_10, recVU_UPPER_FD_11,
};

void (*recVU_UPPER_FD_00_TABLE[32])(VURegs* VU, s32 info) =
{
	recVUMI_ADDAx, recVUMI_SUBAx , recVUMI_MADDAx, recVUMI_MSUBAx,
	recVUMI_ITOF0, recVUMI_FTOI0, recVUMI_MULAx , recVUMI_MULAq ,
	recVUMI_ADDAq, recVUMI_SUBAq, recVUMI_ADDA  , recVUMI_SUBA  ,
	recVUunknown , recVUunknown , recVUunknown  , recVUunknown  ,
	recVUunknown , recVUunknown , recVUunknown  , recVUunknown  ,
	recVUunknown , recVUunknown , recVUunknown  , recVUunknown  ,
	recVUunknown , recVUunknown , recVUunknown  , recVUunknown  ,
	recVUunknown , recVUunknown , recVUunknown  , recVUunknown  ,
};

void (*recVU_UPPER_FD_01_TABLE[32])(VURegs* VU, s32 info) =
{
	recVUMI_ADDAy , recVUMI_SUBAy  , recVUMI_MADDAy, recVUMI_MSUBAy,
	recVUMI_ITOF4 , recVUMI_FTOI4 , recVUMI_MULAy , recVUMI_ABS   ,
	recVUMI_MADDAq, recVUMI_MSUBAq, recVUMI_MADDA , recVUMI_MSUBA ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
};

void (*recVU_UPPER_FD_10_TABLE[32])(VURegs* VU, s32 info) =
{
	recVUMI_ADDAz , recVUMI_SUBAz  , recVUMI_MADDAz, recVUMI_MSUBAz,
	recVUMI_ITOF12, recVUMI_FTOI12, recVUMI_MULAz , recVUMI_MULAi ,
	recVUMI_ADDAi, recVUMI_SUBAi , recVUMI_MULA  , recVUMI_OPMULA,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
};

void (*recVU_UPPER_FD_11_TABLE[32])(VURegs* VU, s32 info) =
{
	recVUMI_ADDAw , recVUMI_SUBAw  , recVUMI_MADDAw, recVUMI_MSUBAw,
	recVUMI_ITOF15, recVUMI_FTOI15, recVUMI_MULAw , recVUMI_CLIP  ,
	recVUMI_MADDAi, recVUMI_MSUBAi, recVUunknown  , recVUMI_NOP   ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
	recVUunknown  , recVUunknown  , recVUunknown  , recVUunknown  ,
};

void recVU_UPPER_FD_00(VURegs* VU, s32 info)
{
	recVU_UPPER_FD_00_TABLE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVU_UPPER_FD_01(VURegs* VU, s32 info)
{
	recVU_UPPER_FD_01_TABLE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVU_UPPER_FD_10(VURegs* VU, s32 info)
{
	recVU_UPPER_FD_10_TABLE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVU_UPPER_FD_11(VURegs* VU, s32 info)
{
	recVU_UPPER_FD_11_TABLE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVULowerOP(VURegs* VU, s32 info)
{
	recVULowerOP_OPCODE[ VU->code & 0x3f ](VU, info);
}

void recVULowerOP_T3_00(VURegs* VU, s32 info)
{
	recVULowerOP_T3_00_OPCODE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVULowerOP_T3_01(VURegs* VU, s32 info)
{
	recVULowerOP_T3_01_OPCODE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVULowerOP_T3_10(VURegs* VU, s32 info)
{
	recVULowerOP_T3_10_OPCODE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVULowerOP_T3_11(VURegs* VU, s32 info)
{
	recVULowerOP_T3_11_OPCODE[(VU->code >> 6) & 0x1f ](VU, info);
}

void recVUunknown(VURegs* VU, s32 info)
{
	Console::Notice("Unknown SVU micromode opcode called");
}
