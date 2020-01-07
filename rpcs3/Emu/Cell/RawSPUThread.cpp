#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Loader/ELF.h"

#include "Emu/Cell/RawSPUThread.h"

#include <atomic>

inline void try_start(spu_thread& spu)
{
	if (spu.status.fetch_op([](u32& status)
	{
		if (status & SPU_STATUS_RUNNING)
		{
			return false;
		}

		status = SPU_STATUS_RUNNING;
		return true;
	}).second)
	{
		spu.state -= cpu_flag::stop;
		thread_ctrl::notify(static_cast<named_thread<spu_thread>&>(spu));
	}
};

bool spu_thread::read_reg(const u32 addr, u32& value)
{
	const u32 offset = addr - RAW_SPU_BASE_ADDR - index * RAW_SPU_OFFSET - RAW_SPU_PROB_OFFSET;

	switch (offset)
	{
	case MFC_CMDStatus_offs:
	{
		spu_mfc_cmd cmd;

		// All arguments must be written for all command types, even for sync barriers
		if (std::scoped_lock lock(mfc_prxy_mtx); mfc_prxy_write_state.all == 0x1f)
		{
			cmd = mfc_prxy_cmd;
			mfc_prxy_write_state.all = 0;
		}
		else
		{
			value = MFC_PPU_DMA_CMD_SEQUENCE_ERROR;
			return true;
		}

		switch (cmd.cmd)
		{
		case MFC_SNDSIG_CMD:
		case MFC_SNDSIGB_CMD:
		case MFC_SNDSIGF_CMD:
		{
			if (cmd.size != 4)
			{
				// Invalid for MFC but may be different for MFC proxy (TODO)
				break;
			}

			[[fallthrough]];
		}
		case MFC_PUT_CMD:
		case MFC_PUTB_CMD:
		case MFC_PUTF_CMD:
		case MFC_PUTS_CMD:
		case MFC_PUTBS_CMD:
		case MFC_PUTFS_CMD:
		case MFC_PUTR_CMD:
		case MFC_PUTRF_CMD:
		case MFC_PUTRB_CMD:
		case MFC_GET_CMD:
		case MFC_GETB_CMD:
		case MFC_GETF_CMD:
		case MFC_GETS_CMD:
		case MFC_GETBS_CMD:
		case MFC_GETFS_CMD:
		{
			if (cmd.size)
			{
				// Perform transfer immediately
				do_dma_transfer(cmd);
			}

			if (cmd.cmd & MFC_START_MASK)
			{
				try_start(*this);
			}

			value = MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL;
			return true;
		}
		case MFC_BARRIER_CMD:
		case MFC_EIEIO_CMD:
		case MFC_SYNC_CMD:
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);
			value = MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL;
			return true;
		}
		default: break;
		}

		break;
	}

	case MFC_QStatus_offs:
	{
		value = MFC_PROXY_COMMAND_QUEUE_EMPTY_FLAG | 8;
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
		value = mfc_prxy_mask;
		return true;
	}

	case SPU_NPC_offs:
	{
		//npc = pc | ((ch_event_stat & SPU_EVENT_INTR_ENABLED) != 0);
		value = npc;
		return true;
	}

	case SPU_RunCntl_offs:
	{
		value = run_ctrl;
		return true;
	}
	}

	spu_log.error("RawSPUThread[%d]: Read32(0x%x): unknown/illegal offset (0x%x)", index, addr, offset);
	return false;
}

bool spu_thread::write_reg(const u32 addr, const u32 value)
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

		std::lock_guard lock(mfc_prxy_mtx);
		mfc_prxy_cmd.lsa = value;
		mfc_prxy_write_state.lsa = true;
		return true;
	}

	case MFC_EAH_offs:
	{
		std::lock_guard lock(mfc_prxy_mtx);
		mfc_prxy_cmd.eah = value;
		mfc_prxy_write_state.eah = true;
		return true;
	}

	case MFC_EAL_offs:
	{
		std::lock_guard lock(mfc_prxy_mtx);
		mfc_prxy_cmd.eal = value;
		mfc_prxy_write_state.eal = true;
		return true;
	}

	case MFC_Size_Tag_offs:
	{
		std::lock_guard lock(mfc_prxy_mtx);
		mfc_prxy_cmd.tag = value & 0x1f;
		mfc_prxy_cmd.size = (value >> 16) & 0x7fff;
		mfc_prxy_write_state.tag_size = true;
		return true;
	}

	case MFC_Class_CMD_offs:
	{
		std::lock_guard lock(mfc_prxy_mtx);
		mfc_prxy_cmd.cmd = MFC(value & 0xff);
		mfc_prxy_write_state.cmd = true;
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
			try_start(*this);
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

	spu_log.error("RawSPUThread[%d]: Write32(0x%x, value=0x%x): unknown/illegal offset (0x%x)", index, addr, value, offset);
	return false;
}

void spu_load_exec(const spu_exec_object& elf)
{
	auto ls0 = vm::cast(vm::falloc(RAW_SPU_BASE_ADDR, 0x80000, vm::spu));
	auto spu = idm::make_ptr<named_thread<spu_thread>>("TEST_SPU", ls0, nullptr, 0, "", 0);

	spu_thread::g_raw_spu_ctr++;
	spu_thread::g_raw_spu_id[0] = spu->id;

	for (const auto& prog : elf.progs)
	{
		if (prog.p_type == 0x1 /* LOAD */ && prog.p_memsz)
		{
			std::memcpy(vm::base(spu->offset + prog.p_vaddr), prog.bin.data(), prog.p_filesz);
		}
	}

	spu->npc = elf.header.e_entry;
}
