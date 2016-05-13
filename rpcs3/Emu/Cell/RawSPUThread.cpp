#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Loader/ELF.h"

#include "Emu/Cell/RawSPUThread.h"

// Originally, SPU MFC registers are accessed externally in a concurrent manner (don't mix with channels, SPU MFC channels are isolated)
thread_local spu_mfc_arg_t raw_spu_mfc[8] = {};

void RawSPUThread::cpu_task()
{
	// get next PC and SPU Interrupt status
	pc = npc.exchange(0);

	set_interrupt_status((pc & 1) != 0);

	pc &= 0x3fffc;

	SPUThread::cpu_task();

	// save next PC and current SPU Interrupt status
	npc = pc | ((ch_event_stat & SPU_EVENT_INTR_ENABLED) != 0);
}

void RawSPUThread::on_init()
{
	if (!offset)
	{
		// Install correct SPU index and LS address
		const_cast<u32&>(index) = id;
		const_cast<u32&>(offset) = vm::falloc(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, 0x40000);
		VERIFY(offset);

		SPUThread::on_init();
	}
}

RawSPUThread::RawSPUThread(const std::string& name)
	: SPUThread(name)
{
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
		value = ch_out_mbox.pop(*this);
		return true;
	}

	case SPU_MBox_Status_offs:
	{
		value = (ch_out_mbox.get_count() & 0xff) | ((4 - ch_in_mbox.get_count()) << 8 & 0xff00) | (ch_out_intr_mbox.get_count() << 16 & 0xff0000);
		return true;
	}
		
	case SPU_Status_offs:
	{
		value = status;
		return true;
	}
	}

	LOG_ERROR(SPU, "RawSPUThread[%d]: Read32(0x%x): unknown/illegal offset (0x%x)", index, addr, offset);
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
			state -= cpu_state::stop;
			(*this)->lock_notify();
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
		return true;
	}

	case SPU_In_MBox_offs:
	{
		ch_in_mbox.push(*this, value);
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
			state += cpu_state::stop;
		}
		else
		{
			break;
		}

		run_ctrl = value;
		return true;
	}

	case SPU_NPC_offs:
	{
		if ((value & 2) || value >= 0x40000)
		{
			break;
		}

		npc = value;
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

template<>
void spu_exec_loader::load() const
{
	auto spu = idm::make_ptr<RawSPUThread>("TEST_SPU");

	for (const auto& prog : progs)
	{
		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			std::memcpy(vm::base(spu->offset + prog.p_vaddr), prog.bin.data(), prog.p_filesz);
		}
	}

	spu->cpu_init();
	spu->npc = header.e_entry;
}
