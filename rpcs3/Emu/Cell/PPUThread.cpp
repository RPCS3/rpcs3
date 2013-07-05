#include "stdafx.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/PPUDisAsm.h"

extern gcmInfo gcm_info;

PPUThread& GetCurrentPPUThread()
{
	PPCThread* thread = GetCurrentPPCThread();

	if(!thread || thread->IsSPU()) throw wxString("GetCurrentPPUThread: bad thread");

	return *(PPUThread*)thread;
}

PPUThread::PPUThread() 
	: PPCThread(PPC_THREAD_PPU)
	, SysCalls(*this)
{
	Reset();
}

PPUThread::~PPUThread()
{
	//~PPCThread();
}

void PPUThread::DoReset()
{
	//reset regs
	memset(VPR,	 0, sizeof(VPR));
	memset(FPR,  0, sizeof(FPR));
	memset(GPR,  0, sizeof(GPR));
	memset(SPRG, 0, sizeof(SPRG));
	
	CR.CR		= 0;
	LR			= 0;
	CTR			= 0;
	USPRG0		= 0;
	TB			= 0;
	XER.XER		= 0;
	FPSCR.FPSCR	= 0;
	VSCR.VSCR	= 0;

	cycle = 0;

	reserve = false;
	reserve_addr = 0;
}

void PPUThread::AddArgv(const wxString& arg)
{
	stack_point -= arg.Len() + 1;
	stack_point = Memory.AlignAddr(stack_point, 0x10) - 0x10;
	argv_addr.AddCpy(stack_point);
	Memory.WriteString(stack_point, arg);
}

void PPUThread::InitRegs()
{
	const u32 pc = Memory.Read32(entry);
	const u32 rtoc = Memory.Read32(entry + 4);

	ConLog.Write("entry = 0x%x", entry);
	ConLog.Write("rtoc = 0x%x", rtoc);

	SetPc(pc);
	
	u64 argc = m_arg;
	u64 argv = 0;

	if(argv_addr.GetCount())
	{
		argc = argv_addr.GetCount();
		stack_point -= 0xc + 4 * argc;
		argv = stack_point;

		mem64_t argv_list(argv);
		for(int i=0; i<argc; ++i) argv_list += argv_addr[i];
	}

	const s32 thread_num = Emu.GetCPU().GetThreadNumById(!IsSPU(), GetId());

	if(thread_num < 0)
	{
		ConLog.Error("GetThreadNumById failed.");
		Emu.Pause();
		return;
	}

	/*
	const s32 tls_size = Emu.GetTLSFilesz() * thread_num;

	if(tls_size >= Emu.GetTLSMemsz())
	{
		ConLog.Error("Out of TLS memory.");
		Emu.Pause();
		return;
	}
	*/

	stack_point = Memory.AlignAddr(stack_point, 0x200) - 0x200;

	GPR[1] = stack_point;
	GPR[2] = rtoc;

	if(argc)
	{
		GPR[3] = argc;
		GPR[4] = argv;
		GPR[5] = argv ? argv + 0xc + 4 * argc : 0; //unk
	}
	else
	{
		GPR[3] = m_arg;
	}

	GPR[0] = pc;
	GPR[8] = entry;
	GPR[11] = 0x80;
	GPR[12] = Emu.GetMallocPageSize();
	GPR[13] = Memory.PRXMem.Alloc(0x10000) + 0x7060 - 8;
	GPR[28] = GPR[4];
	GPR[29] = GPR[3];
	GPR[31] = GPR[5];

	CTR = PC;
	CR.CR = 0x22000082;
	VSCR.NJ = 1;
}

u64 PPUThread::GetFreeStackSize() const
{
	return (GetStackAddr() + GetStackSize()) - GPR[1];
}

void PPUThread::DoRun()
{
	switch(Ini.CPUDecoderMode.GetValue())
	{
	case 0:
		m_dec = new PPU_Decoder(*new PPU_DisAsm(*this));
	break;

	case 1:
	case 2:
		m_dec = new PPU_Decoder(*new PPU_Interpreter(*this));
	break;
	}
}

void PPUThread::DoResume()
{
}

void PPUThread::DoPause()
{
}

void PPUThread::DoStop()
{
	if(m_dec)
	{
		delete m_dec;
		m_dec = nullptr;
	}
}

bool dump_enable = false;

void PPUThread::DoCode(const s32 code)
{
	if(dump_enable)
	{
		static wxFile f("dump.txt", wxFile::write);
		static PPU_DisAsm disasm(*this, DumpMode);
		static PPU_Decoder decoder(disasm);
		disasm.dump_pc = PC;
		decoder.Decode(code);
		f.Write(disasm.last_opcode);
	}

	if(++cycle > 220)
	{
		cycle = 0;
		TB++;
	}

	m_dec->Decode(code);
}

bool FPRdouble::IsINF(PPCdouble d)
{
	return wxFinite(d) ? 1 : 0;
}

bool FPRdouble::IsNaN(PPCdouble d)
{
	return wxIsNaN(d) ? 1 : 0;
}

bool FPRdouble::IsQNaN(PPCdouble d)
{
	return d.GetType() == FPR_QNAN;
}

bool FPRdouble::IsSNaN(PPCdouble d)
{
	return d.GetType() == FPR_SNAN;
}

int FPRdouble::Cmp(PPCdouble a, PPCdouble b)
{
	if(a < b) return CR_LT;
	if(a > b) return CR_GT;
	if(a == b) return CR_EQ;

	return CR_SO;
}