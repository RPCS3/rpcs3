#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"

#include "Emu/Cell/RawSPUThread.h"

RawSPUThread::RawSPUThread(CPUThreadType type)
	: SPUThread(type)
	, MemoryBlock()
{
	m_index = Memory.InitRawSPU(this);
	Reset();
}

RawSPUThread::~RawSPUThread()
{
	Memory.CloseRawSPU(this, m_index);
}

bool RawSPUThread::Read32(const u64 addr, u32* value)
{
	const u64 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;

	switch (offset)
	{
	case MFC_CMDStatus_offs:
	{
		*value = MFC2.CMDStatus.GetValue();
		break;
	}

	case MFC_QStatus_offs:
	{
		// TagStatus is not used: mask is written directly
		*value = MFC2.QueryMask.GetValue();
		break;
	}

	case SPU_Out_MBox_offs:
	{
		// if Out_MBox is empty, the result is undefined
		SPU.Out_MBox.PopUncond(*value);
		break;
	}

	case SPU_MBox_Status_offs:
	{
		*value = (SPU.Out_MBox.GetCount() & 0xff) | (SPU.In_MBox.GetFreeCount() << 8);
		break;
	}
		
	case SPU_Status_offs:
	{
		*value = SPU.Status.GetValue();
		break;
	}

	default:
	{
		// TODO: read value from LS if necessary (not important)
		LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Read32(0x%llx)", m_index, offset);
		return false;
	}
	}

	return true;
}

bool RawSPUThread::Write32(const u64 addr, const u32 value)
{
	const u64 offset = addr - GetStartAddr() - RAW_SPU_PROB_OFFSET;

	switch (offset)
	{
	case MFC_LSA_offs:
	{
		MFC2.LSA.SetValue(value);
		break;
	}

	case MFC_EAH_offs:
	{
		MFC2.EAH.SetValue(value);
		break;
	}

	case MFC_EAL_offs:
	{
		MFC2.EAL.SetValue(value);
		break;
	}

	case MFC_Size_Tag_offs:
	{
		MFC2.Size_Tag.SetValue(value);
		break;
	}

	case MFC_CMDStatus_offs:
	{
		MFC2.CMDStatus.SetValue(value);
		EnqMfcCmd(MFC2);
		break;
	}
		
	case Prxy_QueryType_offs:
	{
		switch(value)
		{
		case 2: break;

		default:
		{
			LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Unknown Prxy Query Type. (prxy_query=0x%x)", m_index, value);
			return false;
		}
		}

		MFC2.QueryType.SetValue(value); // not used
		break;
	}

	case Prxy_QueryMask_offs:
	{
		MFC2.QueryMask.SetValue(value); // TagStatus is not used
		break;
	}

	case SPU_In_MBox_offs:
	{
		// if In_MBox is already full, the last message is overwritten  
		SPU.In_MBox.PushUncond(value); 
		break;
	}

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
			return false;
		}
		break;
	}

	case SPU_NPC_offs:
	{
		SPU.NPC.SetValue(value);
		break;
	}

	case SPU_RdSigNotify1_offs:
	{
		WriteSNR(0, value);
		break;
	}

	case SPU_RdSigNotify2_offs:
	{
		WriteSNR(1, value);
		break;
	}

	default:
	{
		// TODO: write value to LS if necessary (not important)
		LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Write32(0x%llx, 0x%x)", m_index, offset, value);
		return false;
	}
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

	SPUThread::Task();

	SPU.NPC.SetValue(PC);
}
