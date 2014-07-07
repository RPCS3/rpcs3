#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/lv2/sys_lwmutex.h"
#include "Emu/SysCalls/lv2/sys_event.h" 

#include "Emu/Cell/RawSPUThread.h"

RawSPUThread::RawSPUThread(u32 index, CPUThreadType type)
	: SPUThread(type)
	, MemoryBlock()
	, m_index(index)
{
	Memory.InitRawSPU(SetRange(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, RAW_SPU_PROB_OFFSET), m_index);
	Reset();
}

RawSPUThread::~RawSPUThread()
{
	Memory.CloseRawSPU(this, m_index);
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
	case MFC_LSA_offs:          LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_LSA)", m_index);           *value = MFC2.LSA.GetValue(); break;
	case MFC_EAH_offs:          LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_EAH)", m_index);           *value = MFC2.EAH.GetValue(); break;
	case MFC_EAL_offs:          LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_EAL)", m_index);           *value = MFC2.EAL.GetValue(); break;
	case MFC_Size_Tag_offs:     LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_Size_Tag)", m_index);      *value = MFC2.Size_Tag.GetValue(); break;
	case MFC_CMDStatus_offs:    LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_CMDStatus)", m_index);     *value = MFC2.CMDStatus.GetValue(); break;
	case MFC_QStatus_offs:
		LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(MFC_QStatus)", m_index);
		*value = MFC2.QStatus.GetValue(); 
	break;
	case Prxy_QueryType_offs:   LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(Prxy_QueryType)", m_index);    *value = Prxy.QueryType.GetValue(); break;
	case Prxy_QueryMask_offs:   LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(Prxy_QueryMask)", m_index);    *value = Prxy.QueryMask.GetValue(); break;
	case Prxy_TagStatus_offs:   LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(Prxy_TagStatus)", m_index);    *value = Prxy.TagStatus.GetValue(); break;
	case SPU_Out_MBox_offs:
		//LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_Out_MBox)", m_index);
		SPU.Out_MBox.PopUncond(*value); //if Out_MBox is empty yet, the result will be undefined 
	break;
	case SPU_In_MBox_offs:      LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_In_MBox)", m_index);       while(!SPU.In_MBox.Pop(*value)  && !Emu.IsStopped()) Sleep(1); break;
	case SPU_MBox_Status_offs: //LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_MBox_Status)", m_index);
		//SPU.MBox_Status.SetValue(SPU.Out_MBox.GetCount() ? SPU.MBox_Status.GetValue() | 1 : SPU.MBox_Status.GetValue() & ~1);
		SPU.MBox_Status.SetValue((SPU.Out_MBox.GetCount() & 0xff) | (SPU.In_MBox.GetFreeCount() << 8));
		*value = SPU.MBox_Status.GetValue();
		break;
	case SPU_RunCntl_offs:      LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_RunCntl)", m_index);       *value = (u32)IsRunning(); break;
	case SPU_Status_offs:
		//LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_Status)", m_index);
		*value = SPU.Status.GetValue();
		break;
	case SPU_NPC_offs:          LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_NPC)", m_index);           *value = SPU.NPC.GetValue(); break;
	case SPU_RdSigNotify1_offs: LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_RdSigNotify1)", m_index);  *value = SPU.SNR[0].GetValue(); break;
	case SPU_RdSigNotify2_offs: LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Read32(SPU_RdSigNotify2)", m_index);  *value = SPU.SNR[1].GetValue(); break;

	default:
		LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Read32(0x%x)", m_index, offset);
		Emu.Pause();
	break;
	}

	return true;
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
	case MFC_QStatus_offs:			LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(MFC_QStatus, 0x%x)", m_index, value);			MFC2.QStatus.SetValue(value); break;
	case Prxy_QueryType_offs:
	{
		LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(Prxy_QueryType, 0x%x)", m_index, value);
		Prxy.QueryType.SetValue(value);

		switch(value)
		{
		case 2:
			LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Prxy Query Immediate.", m_index);
		break;

		default:
			LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Unknown Prxy Query Type. (prxy_query=0x%x)", m_index, value);
		break;
		}

		Prxy.QueryType.SetValue(0);
		MFC2.QStatus.SetValue(Prxy.QueryMask.GetValue());
	}
	break;
	case Prxy_QueryMask_offs:   LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(Prxy_QueryMask, 0x%x)", m_index, value);      Prxy.QueryMask.SetValue(value); break;
	case Prxy_TagStatus_offs:   LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(Prxy_TagStatus, 0x%x)", m_index, value);      Prxy.TagStatus.SetValue(value); break;
	case SPU_Out_MBox_offs:     LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_Out_MBox, 0x%x)", m_index, value);        while(!SPU.Out_MBox.Push(value) && !Emu.IsStopped()) Sleep(1); break;
	case SPU_In_MBox_offs:
		//LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_In_MBox, 0x%x)", m_index, value);
		SPU.In_MBox.PushUncond(value); //if In_MBox is already full, the last message will be overwritten  
	break;
	case SPU_MBox_Status_offs:  LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_MBox_Status, 0x%x)", m_index, value);     SPU.MBox_Status.SetValue(value); break;
	case SPU_RunCntl_offs:
	{
		if (value == SPU_RUNCNTL_RUNNABLE)
		{
			SPU.Status.SetValue(SPU_STATUS_RUNNING);
			Exec();
		}
		else if (value == SPU_RUNCNTL_STOP)
		{
			SPU.Status.SetValue(SPU_STATUS_STOPPED);
			Stop();
		}
		else
		{
			LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Write32(SPU_RunCtrl, 0x%x): unknown value", m_index, value);
			Emu.Pause();
		}
		break;
	}
	case SPU_Status_offs:       LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_Status, 0x%x)", m_index, value);          SPU.Status.SetValue(value); break;
	case SPU_NPC_offs:          LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_NPC, 0x%x)", m_index, value);             SPU.NPC.SetValue(value); break;
	case SPU_RdSigNotify1_offs: LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_RdSigNotify1, 0x%x)", m_index, value);    SPU.SNR[0].SetValue(value); break;
	case SPU_RdSigNotify2_offs: LOG_WARNING(Log::SPU, "RawSPUThread[%d]: Write32(SPU_RdSigNotify2, 0x%x)", m_index, value);    SPU.SNR[1].SetValue(value); break;

	default:
		LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Write32(0x%x, 0x%x)", m_index, offset, value);
		Emu.Pause();
	break;
	}

	return true;
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
	PC = SPU.NPC.GetValue();

	CPUThread::Task();

	SPU.NPC.SetValue(PC);
}
