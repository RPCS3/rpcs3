#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Callback.h"

#include "Emu/Cell/RawSPUThread.h"

thread_local spu_mfc_arg_t raw_spu_mfc[8] = {};

RawSPUThread::RawSPUThread(const std::string& name, u32 index)
	: SPUThread(CPU_THREAD_RAW_SPU, name, COPY_EXPR(fmt::format("RawSPU%d[0x%x] Thread (%s)[0x%08x]", index, m_id, m_name.c_str(), PC)), index, RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index)
{
	if (!vm::falloc(offset, 0x40000))
	{
		throw EXCEPTION("Failed to allocate RawSPU local storage");
	}
}

RawSPUThread::~RawSPUThread()
{
	join();

	// Deallocate Local Storage
	vm::dealloc_verbose_nothrow(offset);
}

bool RawSPUThread::read_reg(const u32 addr, u32& value)
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
		bool notify;

		std::tie(value, notify) = ch_out_mbox.pop();

		if (notify)
		{
			// notify if necessary
			std::lock_guard<std::mutex> lock(mutex);

			cv.notify_one();
		}

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

bool RawSPUThread::write_reg(const u32 addr, const u32 value)
{
	auto try_start = [this]()
	{
		if (status.atomic_op([](u32& status) -> bool
		{
			if (status & SPU_STATUS_RUNNING)
			{
				return false;
			}
			else
			{
				status = SPU_STATUS_RUNNING;
				return true;
			}
		}))
		{
			exec();
		}
	};

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
			try_start();
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
			int_ctrl[2].set(SPU_INT2_STAT_DMA_TAG_GROUP_COMPLETION_INT); // TODO
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
		if (ch_in_mbox.push(value))
		{
			// notify if necessary
			std::lock_guard<std::mutex> lock(mutex);

			cv.notify_one();
		}

		return true;
	}

	case SPU_RunCntl_offs:
	{
		if (value == SPU_RUNCNTL_RUN_REQUEST)
		{
			try_start();
		}
		else if (value == SPU_RUNCNTL_STOP_REQUEST)
		{
			status &= ~SPU_STATUS_RUNNING;
			stop();
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
		push_snr(0, value);
		return true;
	}

	case SPU_RdSigNotify2_offs:
	{
		push_snr(1, value);
		return true;
	}
	}

	LOG_ERROR(SPU, "RawSPUThread[%d]: Write32(0x%x, value=0x%x): unknown/illegal offset (0x%x)", index, addr, value, offset);
	return false;
}

void RawSPUThread::task()
{
	// get next PC and SPU Interrupt status
	PC = npc.exchange(0);

	set_interrupt_status((PC & 1) != 0);

	PC &= 0x3FFFC;

	SPUThread::task();

	// save next PC and current SPU Interrupt status
	npc.store(PC | ((ch_event_stat.load() & SPU_EVENT_INTR_ENABLED) != 0));
}
