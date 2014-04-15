#include "stdafx.h"
#include "Emu/Cell/RawSPUThread.h"

RawSPUThread::RawSPUThread(u32 index, CPUThreadType type)
	: SPUThread(type)
	, MemoryBlock()
	, m_index(index)
{
	Memory.MemoryBlocks.push_back(SetRange(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, RAW_SPU_OFFSET));
	Reset();
}

RawSPUThread::~RawSPUThread()
{
	for(int i=0; i<Memory.MemoryBlocks.size(); ++i)
	{
		if(Memory.MemoryBlocks[i]->GetStartAddr() == GetStartAddr())
		{
			Memory.MemoryBlocks.erase(Memory.MemoryBlocks.begin() + i);
			break;
		}
	}

	//Close();
}

bool RawSPUThread::Read8(const u64 addr, u8* value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Read8(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Read8(0x%x)", m_index, offset);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Read16(const u64 addr, u16* value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Read16(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Read16(0x%x)", m_index, offset);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Read32(const u64 addr, u32* value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Read32(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	switch(offset)
	{
	case MFC_LSA_offs:          ConLog.Warning("RawSPUThread[%d]: Read32(MFC_LSA)", m_index);           *value = MFC2.LSA.GetValue(); break;
	case MFC_EAH_offs:          ConLog.Warning("RawSPUThread[%d]: Read32(MFC_EAH)", m_index);           *value = MFC2.EAH.GetValue(); break;
	case MFC_EAL_offs:          ConLog.Warning("RawSPUThread[%d]: Read32(MFC_EAL)", m_index);           *value = MFC2.EAL.GetValue(); break;
	case MFC_Size_Tag_offs:     ConLog.Warning("RawSPUThread[%d]: Read32(MFC_Size_Tag)", m_index);      *value = MFC2.Size_Tag.GetValue(); break;
	case MFC_CMDStatus_offs:    ConLog.Warning("RawSPUThread[%d]: Read32(MFC_CMDStatus)", m_index);     *value = MFC2.CMDStatus.GetValue(); break;
	case MFC_QStatus_offs:
		ConLog.Warning("RawSPUThread[%d]: Read32(MFC_QStatus)", m_index);
		*value = MFC2.QStatus.GetValue(); 
	break;
	case Prxy_QueryType_offs:   ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_QueryType)", m_index);    *value = Prxy.QueryType.GetValue(); break;
	case Prxy_QueryMask_offs:   ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_QueryMask)", m_index);    *value = Prxy.QueryMask.GetValue(); break;
	case Prxy_TagStatus_offs:   ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_TagStatus)", m_index);    *value = Prxy.TagStatus.GetValue(); break;
	case SPU_Out_MBox_offs:
		ConLog.Warning("RawSPUThread[%d]: Read32(SPU_Out_MBox)", m_index);
		SPU.Out_MBox.PopUncond(*value); //if Out_MBox is empty yet, the result will be undefined 
	break;
	case SPU_In_MBox_offs:      ConLog.Warning("RawSPUThread[%d]: Read32(SPU_In_MBox)", m_index);       while(!SPU.In_MBox.Pop(*value)  && !Emu.IsStopped()) Sleep(1); break;
	case SPU_MBox_Status_offs: //ConLog.Warning("RawSPUThread[%d]: Read32(SPU_MBox_Status)", m_index);
		//SPU.MBox_Status.SetValue(SPU.Out_MBox.GetCount() ? SPU.MBox_Status.GetValue() | 1 : SPU.MBox_Status.GetValue() & ~1);
		SPU.MBox_Status.SetValue((SPU.Out_MBox.GetCount() & 0xff) | (SPU.In_MBox.GetFreeCount() << 8));
		*value = SPU.MBox_Status.GetValue();
		break;
	case SPU_RunCntl_offs:      ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RunCntl)", m_index);       *value = SPU.RunCntl.GetValue(); break;
	case SPU_Status_offs:       ConLog.Warning("RawSPUThread[%d]: Read32(SPU_Status)", m_index);        *value = SPU.Status.GetValue(); break;
	case SPU_NPC_offs:          ConLog.Warning("RawSPUThread[%d]: Read32(SPU_NPC)", m_index);           *value = SPU.NPC.GetValue(); break;
	case SPU_RdSigNotify1_offs: ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RdSigNotify1)", m_index);  *value = SPU.SNR[0].GetValue(); break;
	case SPU_RdSigNotify2_offs: ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RdSigNotify2)", m_index);  *value = SPU.SNR[1].GetValue(); break;

	default:
		ConLog.Error("RawSPUThread[%d]: Read32(0x%x)", m_index, offset);
		Emu.Pause();
	break;
	}

	return true;
}

bool RawSPUThread::Read64(const u64 addr, u64* value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Read64(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Read64(0x%x)", m_index, offset);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Read128(const u64 addr, u128* value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Read128(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Read128(0x%x)", m_index, offset);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Write8(const u64 addr, const u8 value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Write8(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Write8(0x%x, 0x%x)", m_index, offset, value);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Write16(const u64 addr, const u16 value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Write16(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Write16(0x%x, 0x%x)", m_index, offset, value);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Write32(const u64 addr, const u32 value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Write32(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;

	switch(offset)
	{
	case MFC_LSA_offs:	MFC2.LSA.SetValue(value); break;
	case MFC_EAH_offs:	MFC2.EAH.SetValue(value); break;
	case MFC_EAL_offs:	MFC2.EAL.SetValue(value); break;
	case MFC_Size_Tag_offs:	MFC2.Size_Tag.SetValue(value); break;
	case MFC_CMDStatus_offs:
		MFC2.CMDStatus.SetValue(value);
		EnqMfcCmd(MFC2);
	break;
	case MFC_QStatus_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(MFC_QStatus, 0x%x)", m_index, value);			MFC2.QStatus.SetValue(value); break;
	case Prxy_QueryType_offs:
	{
		ConLog.Warning("RawSPUThread[%d]: Write32(Prxy_QueryType, 0x%x)", m_index, value);
		Prxy.QueryType.SetValue(value);

		switch(value)
		{
		case 2:
			ConLog.Warning("RawSPUThread[%d]: Prxy Query Immediate.", m_index);
		break;

		default:
			ConLog.Error("RawSPUThread[%d]: Unknown Prxy Query Type. (prxy_query=0x%x)", m_index, value);
		break;
		}

		Prxy.QueryType.SetValue(0);
		MFC2.QStatus.SetValue(Prxy.QueryMask.GetValue());
	}
	break;
	case Prxy_QueryMask_offs:   ConLog.Warning("RawSPUThread[%d]: Write32(Prxy_QueryMask, 0x%x)", m_index, value);      Prxy.QueryMask.SetValue(value); break;
	case Prxy_TagStatus_offs:   ConLog.Warning("RawSPUThread[%d]: Write32(Prxy_TagStatus, 0x%x)", m_index, value);      Prxy.TagStatus.SetValue(value); break;
	case SPU_Out_MBox_offs:     ConLog.Warning("RawSPUThread[%d]: Write32(SPU_Out_MBox, 0x%x)", m_index, value);        while(!SPU.Out_MBox.Push(value) && !Emu.IsStopped()) Sleep(1); break;
	case SPU_In_MBox_offs:
		ConLog.Warning("RawSPUThread[%d]: Write32(SPU_In_MBox, 0x%x)", m_index, value);
		SPU.In_MBox.PushUncond(value); //if In_MBox is already full, the last message will be overwritten  
	break;
	case SPU_MBox_Status_offs:  ConLog.Warning("RawSPUThread[%d]: Write32(SPU_MBox_Status, 0x%x)", m_index, value);     SPU.MBox_Status.SetValue(value); break;
	case SPU_RunCntl_offs:      ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RunCntl, 0x%x)", m_index, value);         SPU.RunCntl.SetValue(value); break;
	case SPU_Status_offs:       ConLog.Warning("RawSPUThread[%d]: Write32(SPU_Status, 0x%x)", m_index, value);          SPU.Status.SetValue(value); break;
	case SPU_NPC_offs:          ConLog.Warning("RawSPUThread[%d]: Write32(SPU_NPC, 0x%x)", m_index, value);             SPU.NPC.SetValue(value); break;
	case SPU_RdSigNotify1_offs: ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RdSigNotify1, 0x%x)", m_index, value);    SPU.SNR[0].SetValue(value); break;
	case SPU_RdSigNotify2_offs: ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RdSigNotify2, 0x%x)", m_index, value);    SPU.SNR[1].SetValue(value); break;

	default:
		ConLog.Error("RawSPUThread[%d]: Write32(0x%x, 0x%x)", m_index, offset, value);
		Emu.Pause();
	break;
	}

	return true;
}

bool RawSPUThread::Write64(const u64 addr, const u64 value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Write64(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Write64(0x%x, 0x%llx)", m_index, offset, value);
	Emu.Pause();
	return false;
}

bool RawSPUThread::Write128(const u64 addr, const u128 value)
{
	if(addr < GetStartAddr() + RAW_SPU_PROB_OFFSET)
	{
		return MemoryBlock::Write128(addr, value);
	}

	u32 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;
	ConLog.Error("RawSPUThread[%d]: Write128(0x%x, 0x%llx_%llx)", m_index, offset, value._u64[1], value._u64[0]);
	Emu.Pause();
	return false;
}

void RawSPUThread::InitRegs()
{
	dmac.ls_offset = m_offset = GetStartAddr() + RAW_SPU_LS_OFFSET;
	SPUThread::InitRegs();
}

u32 RawSPUThread::GetIndex() const
{
	return m_index;
}

void RawSPUThread::Task()
{
	if (Ini.HLELogging.GetValue()) ConLog.Write("%s enter", PPCThread::GetFName().c_str());

	const std::vector<u64>& bp = Emu.GetBreakPoints();

	try
	{
		for(uint i=0; i<bp.size(); ++i)
		{
			if(bp[i] == m_offset + PC)
			{
				Emu.Pause();
				break;
			}
		}

		bool is_last_paused = true;

		while(true)
		{
			int status = ThreadStatus();

			if(status == CPUThread_Stopped || status == CPUThread_Break)
			{
				break;
			}

			if(status == CPUThread_Sleeping)
			{
				Sleep(1);
				continue;
			}

			//dmac.DoCmd();

			if(SPU.RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
			{
				if(!is_last_paused)
				{
					is_last_paused = true;
					SPU.NPC.SetValue(PC);
					SPU.Status.SetValue(SPU_STATUS_WAITING_FOR_CHANNEL);
				}

				Sleep(1);
				continue;
			}

			if(is_last_paused)
			{
				is_last_paused = false;
				PC = SPU.NPC.GetValue();
				SPU.Status.SetValue(SPU_STATUS_RUNNING);
				ConLog.Warning("Starting RawSPU...");
			}

			Step();
			NextPc(m_dec->DecodeMemory(PC + m_offset));

			if(status == CPUThread_Step)
			{
				m_is_step = false;
				continue;
			}

			for(uint i=0; i<bp.size(); ++i)
			{
				if(bp[i] == PC)
				{
					Emu.Pause();
					continue;
				}
			}
		}
	}
	catch(const std::string& e)
	{
		ConLog.Error("Exception: %s", e.c_str());
	}
	catch(const char* e)
	{
		ConLog.Error("Exception: %s", e);
	}

	if (Ini.HLELogging.GetValue()) ConLog.Write("%s leave", PPCThread::GetFName().c_str());
}
