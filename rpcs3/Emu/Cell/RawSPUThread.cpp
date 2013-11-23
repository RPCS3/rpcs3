#include "stdafx.h"
#include "Emu/Cell/RawSPUThread.h"

RawSPUThread::RawSPUThread(u32 index, CPUThreadType type)
	: SPUThread(type)
	, m_index(index)
{
	Memory.MemoryBlocks.Add(MemoryBlock::SetRange(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, RAW_SPU_OFFSET));
	Reset();
}

RawSPUThread::~RawSPUThread()
{
	for(int i=0; i<Memory.MemoryBlocks.GetCount(); ++i)
	{
		if(&Memory.MemoryBlocks[i] == this)
		{
			Memory.MemoryBlocks.RemoveFAt(i);
			return;
		}
	}
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
	case MFC_LSA_offs:				ConLog.Warning("RawSPUThread[%d]: Read32(MFC_LSA)", m_index);			*value = MFC.LSA.GetValue(); break;
	case MFC_EAH_offs:				ConLog.Warning("RawSPUThread[%d]: Read32(MFC_EAH)", m_index);			*value = MFC.EAH.GetValue(); break;
	case MFC_EAL_offs:				ConLog.Warning("RawSPUThread[%d]: Read32(MFC_EAL)", m_index);			*value = MFC.EAL.GetValue(); break;
	case MFC_Size_Tag_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(MFC_Size_Tag)", m_index);		*value = MFC.Size_Tag.GetValue(); break;
	case MFC_CMDStatus_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(MFC_CMDStatus)", m_index);		*value = MFC.CMDStatus.GetValue(); break;
	case MFC_QStatus_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(MFC_QStatus)", m_index);		*value = MFC.QStatus.GetValue(); break;
	case Prxy_QueryType_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_QueryType)", m_index);	*value = Prxy.QueryType.GetValue(); break;
	case Prxy_QueryMask_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_QueryMask)", m_index);	*value = Prxy.QueryMask.GetValue(); break;
	case Prxy_TagStatus_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(Prxy_TagStatus)", m_index);	*value = Prxy.TagStatus.GetValue(); break;
	case SPU_Out_MBox_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(SPU_Out_MBox)", m_index);		while(!SPU.Out_MBox.Pop(*value) && !Emu.IsStopped()) Sleep(1); break;
	case SPU_In_MBox_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(SPU_In_MBox)", m_index);		*value = SPU.In_MBox.GetValue(); break;
	case SPU_MBox_Status_offs:		//ConLog.Warning("RawSPUThread[%d]: Read32(SPU_MBox_Status)", m_index);
		SPU.MBox_Status.SetValue(SPU.Out_MBox.GetCount() ? SPU.MBox_Status.GetValue() | 1 : SPU.MBox_Status.GetValue() & ~1);
		SPU.MBox_Status.SetValue((SPU.MBox_Status.GetValue() & ~0xff00) | (SPU.In_MBox.GetCount() << 8));
		*value = SPU.MBox_Status.GetValue();
		break;
	case SPU_RunCntl_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RunCntl)", m_index);		*value = SPU.RunCntl.GetValue(); break;
	case SPU_Status_offs:			ConLog.Warning("RawSPUThread[%d]: Read32(SPU_Status)", m_index);		*value = SPU.Status.GetValue(); break;
	case SPU_NPC_offs:				ConLog.Warning("RawSPUThread[%d]: Read32(SPU_NPC)", m_index);			*value = SPU.NPC.GetValue(); break;
	case SPU_RdSigNotify1_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RdSigNotify1)", m_index);	*value = SPU.RdSigNotify1.GetValue(); break;
	case SPU_RdSigNotify2_offs:		ConLog.Warning("RawSPUThread[%d]: Read32(SPU_RdSigNotify2)", m_index);	*value = SPU.RdSigNotify2.GetValue(); break;

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
	case MFC_LSA_offs:				ConLog.Warning("RawSPUThread[%d]: Write32(MFC_LSA, 0x%x)", m_index, value);				MFC.LSA.SetValue(value); break;
	case MFC_EAH_offs:				ConLog.Warning("RawSPUThread[%d]: Write32(MFC_EAH, 0x%x)", m_index, value);				MFC.EAH.SetValue(value); break;
	case MFC_EAL_offs:				ConLog.Warning("RawSPUThread[%d]: Write32(MFC_EAL, 0x%x)", m_index, value);				MFC.EAL.SetValue(value); break;
	case MFC_Size_Tag_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(MFC_Size_Tag, 0x%x)", m_index, value);		MFC.Size_Tag.SetValue(value); break;
	case MFC_CMDStatus_offs:
	{
		ConLog.Warning("RawSPUThread[%d]: Write32(MFC_CMDStatus, 0x%x)", m_index, value);
		MFC.CMDStatus.SetValue(value);
		u16 op = value & MFC_MASK_CMD;

		switch(op)
		{
		case MFC_PUT_CMD:
		case MFC_GET_CMD:
		{
			u32 lsa = MFC.LSA.GetValue();
			u64 ea = (u64)MFC.EAL.GetValue() | ((u64)MFC.EAH.GetValue() << 32);
			u32 size_tag = MFC.Size_Tag.GetValue();
			u16 tag = (u16)size_tag;
			u16 size = size_tag >> 16;

			ConLog.Warning("RawSPUThread[%d]: DMA %s:", m_index, op == MFC_PUT_CMD ? "PUT" : "GET");
			ConLog.Warning("*** lsa  = 0x%x", lsa);
			ConLog.Warning("*** ea   = 0x%llx", ea);
			ConLog.Warning("*** tag  = 0x%x", tag);
			ConLog.Warning("*** size = 0x%x", size);
			ConLog.SkipLn();

			MFC.CMDStatus.SetValue(dmac.Cmd(value, tag, lsa, ea, size));
		}
		break;

		default:
			ConLog.Error("RawSPUThread[%d]: Unknown MFC cmd. (opcode=0x%x, cmd=0x%x)", m_index, op, value);
		break;
		}
	}
	break;
	case MFC_QStatus_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(MFC_QStatus, 0x%x)", m_index, value);			MFC.QStatus.SetValue(value); break;
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
		MFC.QStatus.SetValue(Prxy.QueryMask.GetValue());
	}
	break;
	case Prxy_QueryMask_offs:		ConLog.Warning("RawSPUThread[%d]: Write32(Prxy_QueryMask, 0x%x)", m_index, value);		Prxy.QueryMask.SetValue(value); break;
	case Prxy_TagStatus_offs:		ConLog.Warning("RawSPUThread[%d]: Write32(Prxy_TagStatus, 0x%x)", m_index, value);		Prxy.TagStatus.SetValue(value); break;
	case SPU_Out_MBox_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(SPU_Out_MBox, 0x%x)", m_index, value);		while(!SPU.Out_MBox.Push(value) && !Emu.IsStopped()) Sleep(1); break;
	case SPU_In_MBox_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(SPU_In_MBox, 0x%x)", m_index, value);			SPU.In_MBox.SetValue(value); break;
	case SPU_MBox_Status_offs:		ConLog.Warning("RawSPUThread[%d]: Write32(SPU_MBox_Status, 0x%x)", m_index, value);		SPU.MBox_Status.SetValue(value); break;
	case SPU_RunCntl_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RunCntl, 0x%x)", m_index, value);			SPU.RunCntl.SetValue(value); break;
	case SPU_Status_offs:			ConLog.Warning("RawSPUThread[%d]: Write32(SPU_Status, 0x%x)", m_index, value);			SPU.Status.SetValue(value); break;
	case SPU_NPC_offs:				ConLog.Warning("RawSPUThread[%d]: Write32(SPU_NPC, 0x%x)", m_index, value);				SPU.NPC.SetValue(value); break;
	case SPU_RdSigNotify1_offs:		ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RdSigNotify1, 0x%x)", m_index, value);	SPU.RdSigNotify1.SetValue(value); break;
	case SPU_RdSigNotify2_offs:		ConLog.Warning("RawSPUThread[%d]: Write32(SPU_RdSigNotify2, 0x%x)", m_index, value);	SPU.RdSigNotify2.SetValue(value); break;

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
	ConLog.Write("%s enter", PPCThread::GetFName());

	const Array<u64>& bp = Emu.GetBreakPoints();

	try
	{
		for(uint i=0; i<bp.GetCount(); ++i)
		{
			if(bp[i] == m_offset + PC)
			{
				Emu.Pause();
				break;
			}
		}

		bool is_last_paused = false;
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

			dmac.DoCmd();

			if(SPU.RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
			{
				if(!is_last_paused)
				{
					if(is_last_paused = SPU.RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
					{
						SPU.NPC.SetValue(PC);
						SPU.Status.SetValue(SPU_STATUS_WAITING_FOR_CHANNEL);
					}
				}

				Sleep(1);
				continue;
			}

			if(is_last_paused)
			{
				is_last_paused = false;
				PC = SPU.NPC.GetValue();
				SPU.Status.SetValue(SPU_STATUS_RUNNING);
			}

			Step();
			NextPc(m_dec->DecodeMemory(PC + m_offset));

			for(uint i=0; i<bp.GetCount(); ++i)
			{
				if(bp[i] == m_offset + PC)
				{
					Emu.Pause();
					break;
				}
			}
		}
	}
	catch(const wxString& e)
	{
		ConLog.Error("Exception: %s", e);
	}
	catch(const char* e)
	{
		ConLog.Error("Exception: %s", e);
	}

	ConLog.Write("%s leave", PPCThread::GetFName());
}
