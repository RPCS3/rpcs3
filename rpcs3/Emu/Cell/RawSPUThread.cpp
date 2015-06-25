#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Callback.h"

#include "Emu/Cell/RawSPUThread.h"

thread_local spu_mfc_arg_t raw_spu_mfc[8] = {};

RawSPUThread::RawSPUThread(CPUThreadType type)
	: SPUThread(type)
{
}

RawSPUThread::~RawSPUThread()
{
}

void RawSPUThread::start()
{
	bool do_start;

	status.atomic_op([&do_start](u32& status)
	{
		if (status & SPU_STATUS_RUNNING)
		{
			do_start = false;
		}
		else
		{
			status = SPU_STATUS_RUNNING;
			do_start = true;
		}
	});

	if (do_start)
	{
		// starting thread directly in SIGSEGV handler may cause problems
		Emu.GetCallbackManager().Async([this](PPUThread& PPU)
		{
			FastRun();
		});
	}
}

bool RawSPUThread::ReadReg(const u32 addr, u32& value)
{
	const u32 offset = addr - RAW_SPU_BASE_ADDR - index * RAW_SPU_OFFSET - RAW_SPU_PROB_OFFSET;

	switch (offset)
	{
	case MFC_CMDStatus_offs:
	{
		value = MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL;
		return true;
	}

	case MFC_QStatus_offs:
	{
		value = MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG | MFC_PPU_MAX_QUEUE_SPACE;
		return true;
	}

	case SPU_Out_MBox_offs:
	{
		value = ch_out_mbox.pop_uncond();
		return true;
	}

	case SPU_MBox_Status_offs:
	{
		value = (ch_out_mbox.get_count() & 0xff) | ((4 - ch_in_mbox.get_count()) << 8 & 0xff00) | (ch_out_intr_mbox.get_count() << 16 & 0xff0000);
		return true;
	}
		
	case SPU_Status_offs:
	{
		value = status.load();
		return true;
	}
	}

	LOG_ERROR(Log::SPU, "RawSPUThread[%d]: Read32(0x%x): unknown/illegal offset (0x%x)", index, addr, offset);
	return false;
}

bool RawSPUThread::WriteReg(const u32 addr, const u32 value)
{
	const u32 offset = addr - RAW_SPU_BASE_ADDR - index * RAW_SPU_OFFSET - RAW_SPU_PROB_OFFSET;

	switch (offset)
	{
	case MFC_LSA_offs:
	{
		if (value >= 0x40000)
		{
			break;
		}

		raw_spu_mfc[index].lsa = value;
		return true;
	}

	case MFC_EAH_offs:
	{
		raw_spu_mfc[index].eah = value;
		return true;
	}

	case MFC_EAL_offs:
	{
		raw_spu_mfc[index].eal = value;
		return true;
	}

	case MFC_Size_Tag_offs:
	{
		if (value >> 16 > 16 * 1024 || (u16)value >= 32)
		{
			break;
		}

		raw_spu_mfc[index].size_tag = value;
		return true;
	}

	case MFC_Class_CMD_offs:
	{
		do_dma_transfer(value & ~MFC_START_MASK, raw_spu_mfc[index]);
		raw_spu_mfc[index] = {}; // clear non-persistent data

		if (value & MFC_START_MASK)
		{
			start();
		}

		return true;
	}
		
	case Prxy_QueryType_offs:
	{
		// 0 - no query requested; cancel previous request
		// 1 - set (interrupt) status upon completion of any enabled tag groups
		// 2 - set (interrupt) status upon completion of all enabled tag groups

		if (value > 2)
		{
			break;
		}

		if (value)
		{
			int2.set(SPU_INT2_STAT_DMA_TAG_GROUP_COMPLETION_INT); // TODO
		}

		return true;
	}

	case Prxy_QueryMask_offs:
	{
		//proxy_tag_mask = value;
		return true;
	}

	case SPU_In_MBox_offs:
	{
		ch_in_mbox.push_uncond(value); 
		return true;
	}

	case SPU_RunCntl_offs:
	{
		if (value == SPU_RUNCNTL_RUN_REQUEST)
		{
			start();
		}
		else if (value == SPU_RUNCNTL_STOP_REQUEST)
		{
			status &= ~SPU_STATUS_RUNNING;
			FastStop();
		}
		else
		{
			break;
		}

		run_ctrl.store(value);
		return true;
	}

	case SPU_NPC_offs:
	{
		if ((value & 2) || value >= 0x40000)
		{
			break;
		}

		npc.store(value);
		return true;
	}

	case SPU_RdSigNotify1_offs:
	{
		write_snr(0, value);
		return true;
	}

	case SPU_RdSigNotify2_offs:
	{
		write_snr(1, value);
		return true;
	}
	}

	LOG_ERROR(SPU, "RawSPUThread[%d]: Write32(0x%x, value=0x%x): unknown/illegal offset (0x%x)", index, addr, value, offset);
	return false;
}

void RawSPUThread::Task()
{
	PC = npc.exchange(0) & ~3;

	SPUThread::Task();

	npc.store(PC | 1);
}
