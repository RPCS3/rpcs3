#include "stdafx.h"
#include "Emu/Cell/RawSPUThread.h"

RawSPUThread::RawSPUThread(PPCThreadType type) : SPUThread(type)
{
	Reset();
}

RawSPUThread::~RawSPUThread()
{
}

void RawSPUThread::InitRegs()
{
	GPR[3]._u64[1] = m_args[0];
	GPR[4]._u64[1] = m_args[1];
	GPR[5]._u64[1] = m_args[2];
	GPR[6]._u64[1] = m_args[3];

	u32 num = Emu.GetCPU().GetThreadNumById(GetType(), GetId());

	m_offset = RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * num + RAW_SPU_LS_OFFSET;
	MFC_LSA.SetAddr(GetRawSPURegAddrByNum(num, MFC_LSA_offs));
	MFC_EAH.SetAddr(GetRawSPURegAddrByNum(num, MFC_EAH_offs));
	MFC_EAL.SetAddr(GetRawSPURegAddrByNum(num, MFC_EAL_offs));
	MFC_Size_Tag.SetAddr(GetRawSPURegAddrByNum(num, MFC_Size_Tag_offs));
	MFC_CMDStatus.SetAddr(GetRawSPURegAddrByNum(num, MFC_CMDStatus_offs));
	MFC_QStatus.SetAddr(GetRawSPURegAddrByNum(num, MFC_QStatus_offs));
	Prxy_QueryType.SetAddr(GetRawSPURegAddrByNum(num, Prxy_QueryType_offs));
	Prxy_QueryMask.SetAddr(GetRawSPURegAddrByNum(num, Prxy_QueryMask_offs));
	Prxy_TagStatus.SetAddr(GetRawSPURegAddrByNum(num, Prxy_TagStatus_offs));
	SPU_Out_MBox.SetAddr(GetRawSPURegAddrByNum(num, SPU_Out_MBox_offs));
	SPU_In_MBox.SetAddr(GetRawSPURegAddrByNum(num, SPU_In_MBox_offs));
	SPU_MBox_Status.SetAddr(GetRawSPURegAddrByNum(num, SPU_MBox_Status_offs));
	SPU_RunCntl.SetAddr(GetRawSPURegAddrByNum(num, SPU_RunCntl_offs));
	SPU_Status.SetAddr(GetRawSPURegAddrByNum(num, SPU_Status_offs));
	SPU_NPC.SetAddr(GetRawSPURegAddrByNum(num, SPU_NPC_offs));
	SPU_RdSigNotify1.SetAddr(GetRawSPURegAddrByNum(num, SPU_RdSigNotify1_offs));
	SPU_RdSigNotify2.SetAddr(GetRawSPURegAddrByNum(num, SPU_RdSigNotify2_offs));

	SPU_RunCntl.SetValue(SPU_RUNCNTL_STOP);
	SPU_Status.SetValue(SPU_STATUS_RUNNING);
	Prxy_QueryType.SetValue(0);
	MFC_CMDStatus.SetValue(0);
	PC = SPU_NPC.GetValue();
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

			if(status == PPCThread_Stopped || status == PPCThread_Break)
			{
				break;
			}

			if(status == PPCThread_Sleeping)
			{
				Sleep(1);
				continue;
			}

			if(MFC_CMDStatus.GetValue() == 0x40)
			{
				MFC_CMDStatus.SetValue(0);
				u32 lsa = MFC_LSA.GetValue();
				u64 ea = (u64)MFC_EAL.GetValue() | ((u64)MFC_EAH.GetValue() << 32);
				u32 size_tag = MFC_Size_Tag.GetValue();
				u16 tag = (u16)size_tag;
				u16 size = size_tag >> 16;
				ConLog.Warning("RawSPU DMA GET:");
				ConLog.Warning("*** lsa  = 0x%x", lsa);
				ConLog.Warning("*** ea   = 0x%llx", ea);
				ConLog.Warning("*** tag  = 0x%x", tag);
				ConLog.Warning("*** size = 0x%x", size);
				ConLog.SkipLn();
				memcpy(Memory + m_offset + lsa, Memory + ea, size);
			}

			if(Prxy_QueryType.GetValue() == 2)
			{
				Prxy_QueryType.SetValue(0);
				u32 mask = Prxy_QueryMask.GetValue();
				//
				MFC_QStatus.SetValue(mask);
			}

			if(SPU_RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
			{
				if(!is_last_paused)
				{
					if(is_last_paused = SPU_RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
					{
						SPU_NPC.SetValue(PC);
						SPU_Status.SetValue(SPU_STATUS_WAITING_FOR_CHANNEL);
					}
				}

				Sleep(1);
				continue;
			}

			if(is_last_paused)
			{
				is_last_paused = false;
				PC = SPU_NPC.GetValue();
				SPU_Status.SetValue(SPU_STATUS_RUNNING);
			}

			DoCode(Memory.Read32(m_offset + PC));
			NextPc();

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