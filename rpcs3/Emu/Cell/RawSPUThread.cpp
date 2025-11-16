#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Loader/ELF.h"
#include "util/asm.hpp"

#include "SPUThread.h"

inline void try_start(spu_thread& spu)
{
	bool notify = false;

	if (~spu.status_npc.load().status & SPU_STATUS_RUNNING)
	{
		reader_lock lock(spu.run_ctrl_mtx);

		if (spu.status_npc.fetch_op([](spu_thread::status_npc_sync_var& value)
		{
			if (value.status & SPU_STATUS_RUNNING)
			{
				return false;
			}

			value.status = SPU_STATUS_RUNNING | (value.status & SPU_STATUS_IS_ISOLATED);
			return true;
		}).second)
		{
			spu.state -= cpu_flag::stop;
			notify = true;
		}
	}

	if (notify)
	{
		spu.state.notify_one();
	}
};

bool spu_thread::read_reg(const u32 addr, u32& value)
{
	const u32 offset = addr - (RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index) - RAW_SPU_PROB_OFFSET;

	spu_log.trace("RawSPU[%u]: Read32(0x%x, offset=0x%x)", index, addr, offset);

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
		case MFC_SDCRT_CMD:
		case MFC_SDCRTST_CMD:
		{
			value = MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL;
			return true;
		}
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
		case MFC_SDCRZ_CMD:
		{
			if (cmd.size)
			{
				// Perform transfer immediately
				do_dma_transfer(nullptr, cmd, ls);
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
			atomic_fence_seq_cst();
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

	case Prxy_QueryMask_offs:
	{
		value = mfc_prxy_mask;
		return true;
	}

	case Prxy_QueryType_offs:
	{
		value = 0;
		return true;
	}

	case SPU_Out_MBox_offs:
	{
		value = ch_out_mbox.pop();
		return true;
	}

	case SPU_MBox_Status_offs:
	{
		// Load channel counts atomically
		auto counts = std::make_tuple(ch_out_mbox.get_count(), ch_in_mbox.get_count(), ch_out_intr_mbox.get_count());

		while (true)
		{
			atomic_fence_acquire();

			const auto counts_check = std::make_tuple(ch_out_mbox.get_count(), ch_in_mbox.get_count(), ch_out_intr_mbox.get_count());

			if (counts_check == counts)
			{
				break;
			}

			// Update and reload
			counts = counts_check;
		}

		const u32 out_mbox = std::get<0>(counts);
		const u32 in_mbox = 4 - std::get<1>(counts);
		const u32 intr_mbox = std::get<2>(counts);

		value = (out_mbox & 0xff) | ((in_mbox << 8) & 0xff00) | ((intr_mbox << 16) & 0xff0000);
		return true;
	}

	case SPU_Status_offs:
	{
		value = status_npc.load().status;
		return true;
	}

	case Prxy_TagStatus_offs:
	{
		value = mfc_prxy_mask;
		return true;
	}

	case SPU_NPC_offs:
	{
		const auto current = status_npc.load();
		value = !(current.status & SPU_STATUS_RUNNING) ? current.npc : 0;
		return true;
	}

	case SPU_RunCntl_offs:
	{
		value = run_ctrl;
		return true;
	}
	}

	spu_log.error("RawSPU[%u]: Read32(0x%x): unknown/illegal offset (0x%x)", index, addr, offset);
	return false;
}

bool spu_thread::write_reg(const u32 addr, const u32 value)
{
	const u32 offset = addr - (RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index) - RAW_SPU_PROB_OFFSET;

	spu_log.trace("RawSPU[%u]: Write32(0x%x, offset=0x%x, value=0x%x)", index, addr, offset, value);

	switch (offset)
	{
	case MFC_LSA_offs:
	{
		if (value >= SPU_LS_SIZE)
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
		if (!ch_in_mbox.push(value).op_done)
		{
			if (auto cpu = cpu_thread::get_current())
			{
				cpu->state += cpu_flag::again;
			}
		}

		return true;
	}

	case SPU_RunCntl_offs:
	{
		run_ctrl = value;

		if (value == SPU_RUNCNTL_RUN_REQUEST)
		{
			try_start(*this);
		}
		else if (value == SPU_RUNCNTL_STOP_REQUEST)
		{
			if (get_current_cpu_thread() == this)
			{
				// TODO
				state += cpu_flag::stop + cpu_flag::ret;
				return true;
			}

			std::scoped_lock lock(run_ctrl_mtx);

			if (status_npc.load().status & SPU_STATUS_RUNNING)
			{
				state += cpu_flag::stop + cpu_flag::ret;

				for (status_npc_sync_var old; (old = status_npc).status & SPU_STATUS_RUNNING;)
				{
					utils::bless<atomic_t<u32>>(&status_npc)[0].wait(old.status);
				}
			}
		}
		else
		{
			break;
		}

		return true;
	}

	case SPU_NPC_offs:
	{
		status_npc.fetch_op([value = value & 0x3fffd](status_npc_sync_var& state)
		{
			if (!(state.status & SPU_STATUS_RUNNING))
			{
				state.npc = value;
				return true;
			}

			return false;
		});

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

	spu_log.error("RawSPU[%u]: Write32(0x%x, value=0x%x): unknown/illegal offset (0x%x)", index, addr, value, offset);
	return false;
}

bool spu_thread::test_is_problem_state_register_offset(u32 offset, bool for_read, bool for_write) noexcept
{
	if (for_read)
	{
		switch (offset)
		{
		case MFC_CMDStatus_offs:
		case MFC_QStatus_offs:
		case SPU_Out_MBox_offs:
		case SPU_MBox_Status_offs:
		case Prxy_QueryType_offs:
		case Prxy_QueryMask_offs:
		case SPU_Status_offs:
		case Prxy_TagStatus_offs:
		case SPU_NPC_offs:
		case SPU_RunCntl_offs:
			return true;
		default: break;
		}
	}

	if (for_write)
	{
		switch (offset)
		{
		case MFC_LSA_offs:
		case MFC_EAH_offs:
		case MFC_EAL_offs:
		case MFC_Size_Tag_offs:
		case MFC_Class_CMD_offs:
		case Prxy_QueryType_offs:
		case Prxy_QueryMask_offs:
		case SPU_In_MBox_offs:
		case SPU_RunCntl_offs:
		case SPU_NPC_offs:
		case SPU_RdSigNotify1_offs:
		case SPU_RdSigNotify2_offs:
		case (SPU_RdSigNotify2_offs & 0xffff): // Fow now accept both (this is used for an optimization so it can be imperfect)
			return true;
		default: break;
		}
	}

	return false;
}

void spu_load_exec(const spu_exec_object& elf)
{
	spu_thread::g_raw_spu_ctr++;

	auto spu = idm::make_ptr<named_thread<spu_thread>>(nullptr, 0, "test_spu", 0);
	ensure(vm::get(vm::spu)->falloc(spu->vm_offset(), SPU_LS_SIZE, &spu->shm, vm::page_size_64k));
	spu->map_ls(*spu->shm, spu->ls);

	for (const auto& prog : elf.progs)
	{
		if (prog.p_type == 0x1u /* LOAD */ && prog.p_memsz)
		{
			std::memcpy(spu->_ptr<void>(prog.p_vaddr), prog.bin.data(), prog.p_filesz);
		}
	}

	spu_thread::g_raw_spu_id[0] = spu->id;

	spu->status_npc = {SPU_STATUS_RUNNING, elf.header.e_entry};
	atomic_storage<u32>::release(spu->pc, elf.header.e_entry);

	const auto funcs = spu->discover_functions(0, { spu->ls , SPU_LS_SIZE }, true, umax);

	if (spu_log.notice && !funcs.empty())
	{
		std::string to_log;

		for (usz i = 0; i < funcs.size(); i++)
		{
			if (i == 0 && funcs.size() < 4)
			{
				// Skip newline in this case
				to_log += ' ';
			}
			else if (i % 4 == 0)
			{
				fmt::append(to_log, "\n[%02u] ", i / 8);
			}
			else
			{
				to_log += ", ";
			}

			fmt::append(to_log, "0x%05x", funcs[i]);
		}

		spu_log.notice("Found SPU function(s) at:%s", to_log);
	}

	if (!funcs.empty())
	{
		spu_log.success("Found %u SPU function(s)", funcs.size());
	}
}

void spu_load_rel_exec(const spu_rel_object& elf)
{
	spu_thread::g_raw_spu_ctr++;

	auto spu = idm::make_ptr<named_thread<spu_thread>>(nullptr, 0, "test_spu", 0);
	ensure(vm::get(vm::spu)->falloc(spu->vm_offset(), SPU_LS_SIZE, &spu->shm, vm::page_size_64k));
	spu->map_ls(*spu->shm, spu->ls);

	u32 total_memsize = 0;

	// Compute executable data size
	for (const auto& shdr : elf.shdrs)
	{
		if (shdr.sh_type == sec_type::sht_progbits && shdr.sh_flags().all_of(sh_flag::shf_alloc))
		{
			total_memsize = utils::align<u32>(total_memsize + shdr.sh_size, 4);
		}
	}

	// Place executable data in SPU local memory
	u32 offs = 0;

	for (const auto& shdr : elf.shdrs)
	{
		if (shdr.sh_type == sec_type::sht_progbits && shdr.sh_flags().all_of(sh_flag::shf_alloc))
		{
			std::memcpy(spu->_ptr<void>(offs), shdr.get_bin().data(), shdr.sh_size);
			offs = utils::align<u32>(offs + shdr.sh_size, 4);
		}
	}

	spu_log.success("Loaded 0x%x of SPU relocatable executable data", total_memsize);

	spu_thread::g_raw_spu_id[0] = spu->id;

	spu->status_npc = {SPU_STATUS_RUNNING, elf.header.e_entry};
	atomic_storage<u32>::release(spu->pc, elf.header.e_entry);
}
