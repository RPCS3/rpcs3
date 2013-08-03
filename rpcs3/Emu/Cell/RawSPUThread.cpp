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
	mfc.dmac.ls_offset = m_offset;
	mfc.dmac.proxy_pos = 0;
	mfc.dmac.queue_pos = 0;
	mfc.MFC_LSA.SetAddr(GetRawSPURegAddrByNum(num, MFC_LSA_offs));
	mfc.MFC_EAH.SetAddr(GetRawSPURegAddrByNum(num, MFC_EAH_offs));
	mfc.MFC_EAL.SetAddr(GetRawSPURegAddrByNum(num, MFC_EAL_offs));
	mfc.MFC_Size_Tag.SetAddr(GetRawSPURegAddrByNum(num, MFC_Size_Tag_offs));
	mfc.MFC_CMDStatus.SetAddr(GetRawSPURegAddrByNum(num, MFC_CMDStatus_offs));
	mfc.MFC_QStatus.SetAddr(GetRawSPURegAddrByNum(num, MFC_QStatus_offs));
	mfc.Prxy_QueryType.SetAddr(GetRawSPURegAddrByNum(num, Prxy_QueryType_offs));
	mfc.Prxy_QueryMask.SetAddr(GetRawSPURegAddrByNum(num, Prxy_QueryMask_offs));
	mfc.Prxy_TagStatus.SetAddr(GetRawSPURegAddrByNum(num, Prxy_TagStatus_offs));
	mfc.SPU_Out_MBox.SetAddr(GetRawSPURegAddrByNum(num, SPU_Out_MBox_offs));
	mfc.SPU_In_MBox.SetAddr(GetRawSPURegAddrByNum(num, SPU_In_MBox_offs));
	mfc.SPU_MBox_Status.SetAddr(GetRawSPURegAddrByNum(num, SPU_MBox_Status_offs));
	mfc.SPU_RunCntl.SetAddr(GetRawSPURegAddrByNum(num, SPU_RunCntl_offs));
	mfc.SPU_Status.SetAddr(GetRawSPURegAddrByNum(num, SPU_Status_offs));
	mfc.SPU_NPC.SetAddr(GetRawSPURegAddrByNum(num, SPU_NPC_offs));
	mfc.SPU_RdSigNotify1.SetAddr(GetRawSPURegAddrByNum(num, SPU_RdSigNotify1_offs));
	mfc.SPU_RdSigNotify2.SetAddr(GetRawSPURegAddrByNum(num, SPU_RdSigNotify2_offs));

	mfc.SPU_RunCntl.SetValue(SPU_RUNCNTL_STOP);
	mfc.SPU_Status.SetValue(SPU_STATUS_RUNNING);
	mfc.Prxy_QueryType.SetValue(0);
	mfc.MFC_CMDStatus.SetValue(0);
	PC = mfc.SPU_NPC.GetValue();
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

			mfc.Handle();

			if(mfc.SPU_RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
			{
				if(!is_last_paused)
				{
					if(is_last_paused = mfc.SPU_RunCntl.GetValue() != SPU_RUNCNTL_RUNNABLE)
					{
						mfc.SPU_NPC.SetValue(PC);
						mfc.SPU_Status.SetValue(SPU_STATUS_WAITING_FOR_CHANNEL);
					}
				}

				Sleep(1);
				continue;
			}

			if(is_last_paused)
			{
				is_last_paused = false;
				PC = mfc.SPU_NPC.GetValue();
				mfc.SPU_Status.SetValue(SPU_STATUS_RUNNING);
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