#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Loader/ELF.h"

#include "Emu/Cell/RawSPUThread.h"

// Originally, SPU MFC registers are accessed externally in a concurrent manner (don't mix with channels, SPU MFC channels are isolated)
thread_local spu_mfc_cmd g_tls_mfc[8] = {};

void RawSPUThread::cpu_task()
{
	// get next PC and SPU Interrupt status
	pc = npc.exchange(0);

	interrupts_enabled = (pc & 1) != 0;

	pc &= 0x3fffc;

	SPUThread::cpu_task();

	// save next PC and current SPU Interrupt status
    npc = pc | (interrupts_enabled);
}

void RawSPUThread::on_init(const std::shared_ptr<void>& _this)
{
	if (!offset)
	{
		// Install correct SPU index and LS address
		const_cast<u32&>(index) = id;
		const_cast<u32&>(offset) = verify(HERE, vm::falloc(RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index, 0x40000));

		cpu_thread::on_init(_this);
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
		value = g_tls_mfc[index].cmd;
		return true;
	}

	case MFC_QStatus_offs:
	{
		const auto size = mfc_proxy.size();
		value = (size ? 0 : MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG) | (8 - size);
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

	case Prxy_TagStatus_offs:
	{
		value = mfc_proxy.size() ? 0 : +mfc_prxy_mask;
		return true;
	}

	case SPU_NPC_offs:
	{
		//npc = pc | ((ch_event_stat & SPU_EVENT_INTR_ENABLED) != 0);
		value = npc;
		return true;
	}
	
	case SPU_RdSigNotify1_offs:
	{
		get_ch_value(SPU_RdSigNotify1, value);
		return true;
	}

	case SPU_RdSigNotify2_offs:
	{
		get_ch_value(SPU_RdSigNotify2, value);
		return true;
	}


	case SPU_RunCntl_offs:
	{
		value = run_ctrl;
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
		if (!status.test_and_set(SPU_STATUS_RUNNING))
		{
			run();
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

		g_tls_mfc[index].lsa = value;
		return true;
	}

	case MFC_EAH_offs:
	{
		g_tls_mfc[index].eah = value;
		return true;
	}

	case MFC_EAL_offs:
	{
		g_tls_mfc[index].eal = value;
		return true;
	}

	case MFC_Size_Tag_offs:
	{
		g_tls_mfc[index].tag = value & 0x1f;
		g_tls_mfc[index].size = (value >> 16) & 0x7fff;
		return true;
	}

	case MFC_Class_CMD_offs:
	{
		g_tls_mfc[index].cmd = MFC(value & 0xff);
		do_dma_transfer(g_tls_mfc[index]);
		g_tls_mfc[index] = {};
		g_tls_mfc[index].cmd = MFC(MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL);

		if (value & MFC_START_MASK)
		{
			try_start();
		}

		return true;
	}
		
	case Prxy_QueryType_offs:
	{
		// TODO
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
		mfc_prxy_mask = value;
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
			state += cpu_flag::stop;
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

void spu_load_exec(const spu_exec_object& elf)
{
	auto spu = idm::make_ptr<RawSPUThread>("TEST_SPU");

	for (const auto& prog : elf.progs)
	{
		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			std::memcpy(vm::base(spu->offset + prog.p_vaddr), prog.bin.data(), prog.p_filesz);
		}
	}

	spu->cpu_init();
	spu->npc = elf.header.e_entry;

	fxm::get_always<mfc_thread>()->add_spu(std::move(spu));
}
