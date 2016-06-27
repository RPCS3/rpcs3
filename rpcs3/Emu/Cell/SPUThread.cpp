#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_interrupt.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPURecompiler.h"

#include "Emu/Memory/wait_engine.h"

#include <cmath>
#include <cfenv>
#include <thread>

extern u64 get_timebased_time();

extern std::mutex& get_current_thread_mutex();

enum class spu_decoder_type
{
	precise,
	fast,
	asmjit,
	llvm,
};

cfg::map_entry<spu_decoder_type> g_cfg_spu_decoder(cfg::root.core, "SPU Decoder", 2,
{
	{ "Interpreter (precise)", spu_decoder_type::precise },
	{ "Interpreter (fast)", spu_decoder_type::fast },
	{ "Recompiler (ASMJIT)", spu_decoder_type::asmjit },
	{ "Recompiler (LLVM)", spu_decoder_type::llvm },
});

const spu_decoder<spu_interpreter_precise> s_spu_interpreter_precise;
const spu_decoder<spu_interpreter_fast> s_spu_interpreter_fast;

void spu_int_ctrl_t::set(u64 ints)
{
	// leave only enabled interrupts
	ints &= mask;

	// notify if at least 1 bit was set
	if (ints && ~stat.fetch_or(ints) & ints && tag)
	{
		LV2_LOCK;

		if (tag && tag->handler)
		{
			tag->handler->signal++;
			(*tag->handler->thread)->notify();
		}
	}
}

const spu_imm_table_t g_spu_imm;

spu_imm_table_t::scale_table_t::scale_table_t()
{
	for (s32 i = -155; i < 174; i++)
	{
		m_data[i + 155] = _mm_set1_ps(static_cast<float>(std::exp2(i)));
	}
}

spu_imm_table_t::spu_imm_table_t()
{
	for (u32 i = 0; i < sizeof(fsm) / sizeof(fsm[0]); i++)
	{
		for (u32 j = 0; j < 4; j++)
		{
			fsm[i]._u32[j] = (i & (1 << j)) ? 0xffffffff : 0;
		}
	}

	for (u32 i = 0; i < sizeof(fsmh) / sizeof(fsmh[0]); i++)
	{
		for (u32 j = 0; j < 8; j++)
		{
			fsmh[i]._u16[j] = (i & (1 << j)) ? 0xffff : 0;
		}
	}

	for (u32 i = 0; i < sizeof(fsmb) / sizeof(fsmb[0]); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			fsmb[i]._u8[j] = (i & (1 << j)) ? 0xff : 0;
		}
	}

	for (u32 i = 0; i < sizeof(sldq_pshufb) / sizeof(sldq_pshufb[0]); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
		}
	}

	for (u32 i = 0; i < sizeof(srdq_pshufb) / sizeof(srdq_pshufb[0]); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			srdq_pshufb[i]._u8[j] = (j + i > 15) ? 0xff : static_cast<u8>(j + i);
		}
	}

	for (u32 i = 0; i < sizeof(rldq_pshufb) / sizeof(rldq_pshufb[0]); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
		}
	}
}

std::string SPUThread::get_name() const
{
	return fmt::format("%sSPU[0x%x] Thread (%s)", offset > RAW_SPU_BASE_ADDR ? "Raw" : "", id, m_name);
}

std::string SPUThread::dump() const
{
	std::string ret = "Registers:\n=========\n";

	for (uint i = 0; i<128; ++i) ret += fmt::format("GPR[%d] = 0x%s\n", i, gpr[i].to_hex().c_str());

	return ret;
}

void SPUThread::cpu_init()
{
	gpr = {};
	fpscr.Reset();

	ch_mfc_args = {};
	mfc_queue.clear();

	ch_tag_mask = 0;
	ch_tag_stat.data.store({});
	ch_stall_stat.data.store({});
	ch_atomic_stat.data.store({});

	ch_in_mbox.clear();

	ch_out_mbox.data.store({});
	ch_out_intr_mbox.data.store({});

	snr_config = 0;

	ch_snr1.data.store({});
	ch_snr2.data.store({});

	ch_event_mask = 0;
	ch_event_stat = 0;
	last_raddr = 0;

	ch_dec_start_timestamp = get_timebased_time(); // ???
	ch_dec_value = 0;

	run_ctrl = 0;
	status = 0;
	npc = 0;

	int_ctrl[0].clear();
	int_ctrl[1].clear();
	int_ctrl[2].clear();

	gpr[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

extern thread_local std::string(*g_tls_log_prefix)();

void SPUThread::cpu_task()
{
	std::fesetround(FE_TOWARDZERO);

	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}

	if (g_cfg_spu_decoder.get() == spu_decoder_type::asmjit)
	{
		if (!spu_db) spu_db = fxm::get_always<SPUDatabase>();
		return spu_recompiler_base::enter(*this);
	}

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<SPUThread*>(get_current_cpu_thread());

		return fmt::format("%s [0x%05x]", cpu->get_name(), cpu->pc);
	};

	// Select opcode table
	const auto& table = *(
		g_cfg_spu_decoder.get() == spu_decoder_type::precise ? &s_spu_interpreter_precise.get_table() :
		g_cfg_spu_decoder.get() == spu_decoder_type::fast ? &s_spu_interpreter_fast.get_table() :
		throw std::logic_error("Invalid SPU decoder"));

	// LS base address
	const auto base = vm::_ptr<const u32>(offset);

	while (true)
	{
		if (!state.load())
		{
			// Read opcode
			const u32 op = base[pc / 4];

			// Call interpreter function
			table[spu_decode(op)](*this, { op });

			// Next instruction
			pc += 4;
			continue;
		}

		if (check_status()) return;
	}
}

SPUThread::~SPUThread()
{
	// Deallocate Local Storage
	vm::dealloc_verbose_nothrow(offset);
}

SPUThread::SPUThread(const std::string& name)
	: cpu_thread(cpu_type::spu)
	, m_name(name)
	, index(0)
	, offset(0)
{
}

SPUThread::SPUThread(const std::string& name, u32 index)
	: cpu_thread(cpu_type::spu)
	, m_name(name)
	, index(index)
	, offset(vm::alloc(0x40000, vm::main))
{
	ENSURES(offset);
}

void SPUThread::push_snr(u32 number, u32 value)
{
	// Get channel
	const auto channel = number & 1 ? &ch_snr2 : &ch_snr1;

	// Check corresponding SNR register settings
	if ((snr_config >> number) & 1)
	{
		channel->push_or(*this, value);
	}
	else
	{
		channel->push(*this, value);
	}
}

void SPUThread::do_dma_transfer(u32 cmd, spu_mfc_arg_t args)
{
	if (cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK))
	{
		_mm_mfence();
	}

	u32 eal = vm::cast(args.ea, HERE);

	if (eal >= SYS_SPU_THREAD_BASE_LOW && offset >= RAW_SPU_BASE_ADDR) // SPU Thread Group MMIO (LS and SNR)
	{
		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		const auto group = tg.lock();

		if (group && index < group->num && group->threads[index])
		{
			auto& spu = static_cast<SPUThread&>(*group->threads[index]);

			if (offset + args.size - 1 < 0x40000) // LS access
			{
				eal = spu.offset + offset; // redirect access
			}
			else if ((cmd & MFC_PUT_CMD) && args.size == 4 && (offset == SYS_SPU_THREAD_SNR1 || offset == SYS_SPU_THREAD_SNR2))
			{
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, _ref<u32>(args.lsa));
				return;
			}
			else
			{
				throw EXCEPTION("Invalid MMIO offset (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)", cmd, args.lsa, args.ea, args.tag, args.size);
			}
		}
		else
		{
			throw EXCEPTION("Invalid thread type (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)", cmd, args.lsa, args.ea, args.tag, args.size);
		}
	}

	switch (cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK))
	{
	case MFC_PUT_CMD:
	case MFC_PUTR_CMD:
	{
		std::memcpy(vm::base(eal), vm::base(offset + args.lsa), args.size);
		return;
	}

	case MFC_GET_CMD:
	{
		std::memcpy(vm::base(offset + args.lsa), vm::base(eal), args.size);
		return;
	}
	}

	throw EXCEPTION("Invalid command %s (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)", get_mfc_cmd_name(cmd), cmd, args.lsa, args.ea, args.tag, args.size);
}

void SPUThread::do_dma_list_cmd(u32 cmd, spu_mfc_arg_t args)
{
	if (!(cmd & MFC_LIST_MASK))
	{
		throw EXCEPTION("Invalid command %s (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)", get_mfc_cmd_name(cmd), cmd, args.lsa, args.ea, args.tag, args.size);
	}

	const u32 list_addr = args.ea & 0x3ffff;
	const u32 list_size = args.size / 8;
	args.lsa &= 0x3fff0;

	struct list_element
	{
		be_t<u16> sb; // Stall-and-Notify bit (0x8000)
		be_t<u16> ts; // List Transfer Size
		be_t<u32> ea; // External Address Low
	};

	for (u32 i = 0; i < list_size; i++)
	{
		auto rec = vm::ptr<list_element>::make(offset + list_addr + i * 8);

		const u32 size = rec->ts;
		const u32 addr = rec->ea;

		if (size)
		{
			spu_mfc_arg_t transfer;
			transfer.ea = addr;
			transfer.lsa = args.lsa | (addr & 0xf);
			transfer.tag = args.tag;
			transfer.size = size;

			do_dma_transfer(cmd & ~MFC_LIST_MASK, transfer);

			args.lsa += std::max<u32>(size, 16);
		}

		if (rec->sb & 0x8000)
		{
			ch_stall_stat.set_value((1 << args.tag) | ch_stall_stat.get_value());

			spu_mfc_arg_t stalled;
			stalled.ea = (args.ea & ~0xffffffff) | (list_addr + (i + 1) * 8);
			stalled.lsa = args.lsa;
			stalled.tag = args.tag;
			stalled.size = (list_size - i - 1) * 8;

			mfc_queue.emplace_back(cmd, stalled);
			return;
		}
	}
}

void SPUThread::process_mfc_cmd(u32 cmd)
{
	LOG_TRACE(SPU, "DMA %s: cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", get_mfc_cmd_name(cmd), cmd, ch_mfc_args.lsa, ch_mfc_args.ea, ch_mfc_args.tag, ch_mfc_args.size);

	switch (cmd)
	{
	case MFC_PUT_CMD:
	case MFC_PUTB_CMD:
	case MFC_PUTF_CMD:
	case MFC_PUTR_CMD:
	case MFC_PUTRB_CMD:
	case MFC_PUTRF_CMD:
	case MFC_GET_CMD:
	case MFC_GETB_CMD:
	case MFC_GETF_CMD:
	{
		return do_dma_transfer(cmd, ch_mfc_args);
	}

	case MFC_PUTL_CMD:
	case MFC_PUTLB_CMD:
	case MFC_PUTLF_CMD:
	case MFC_PUTRL_CMD:
	case MFC_PUTRLB_CMD:
	case MFC_PUTRLF_CMD:
	case MFC_GETL_CMD:
	case MFC_GETLB_CMD:
	case MFC_GETLF_CMD:
	{
		return do_dma_list_cmd(cmd, ch_mfc_args);
	}

	case MFC_GETLLAR_CMD: // acquire reservation
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		const u32 raddr = vm::cast(ch_mfc_args.ea, HERE);

		vm::reservation_acquire(vm::base(offset + ch_mfc_args.lsa), raddr, 128);

		if (std::exchange(last_raddr, raddr))
		{
			ch_event_stat |= SPU_EVENT_LR;
		}

		return ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
	}

	case MFC_PUTLLC_CMD: // store conditionally
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		if (vm::reservation_update(vm::cast(ch_mfc_args.ea, HERE), vm::base(offset + ch_mfc_args.lsa), 128))
		{
			if (std::exchange(last_raddr, 0) == 0)
			{
				throw std::runtime_error("PUTLLC succeeded without GETLLAR" HERE);
			}

			return ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			if (std::exchange(last_raddr, 0))
			{
				ch_event_stat |= SPU_EVENT_LR;
			}

			return ch_atomic_stat.set_value(MFC_PUTLLC_FAILURE);
		}
	}

	case MFC_PUTLLUC_CMD: // store unconditionally
	case MFC_PUTQLLUC_CMD:
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		vm::reservation_op(vm::cast(ch_mfc_args.ea, HERE), 128, [this]()
		{
			std::memcpy(vm::base_priv(vm::cast(ch_mfc_args.ea, HERE)), vm::base(offset + ch_mfc_args.lsa), 128);
		});

		if (last_raddr != 0 && vm::g_tls_did_break_reservation)
		{
			ch_event_stat |= SPU_EVENT_LR;

			last_raddr = 0;
		}

		if (cmd == MFC_PUTLLUC_CMD)
		{
			ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		}

		return;
	}

	case MFC_BARRIER_CMD:
	case MFC_EIEIO_CMD:
	case MFC_SYNC_CMD:
		_mm_mfence();
		return;
	}

	throw EXCEPTION("Unknown command %s (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)",
		get_mfc_cmd_name(cmd), cmd, ch_mfc_args.lsa, ch_mfc_args.ea, ch_mfc_args.tag, ch_mfc_args.size);
}

u32 SPUThread::get_events(bool waiting)
{
	// check reservation status and set SPU_EVENT_LR if lost
	if (last_raddr != 0 && !vm::reservation_test(operator->()))
	{
		ch_event_stat |= SPU_EVENT_LR;

		last_raddr = 0;
	}

	// initialize waiting
	if (waiting)
	{
		// polling with atomically set/removed SPU_EVENT_WAITING flag
		return ch_event_stat.atomic_op([this](u32& stat) -> u32
		{
			if (u32 res = stat & ch_event_mask)
			{
				stat &= ~SPU_EVENT_WAITING;
				return res;
			}
			else
			{
				stat |= SPU_EVENT_WAITING;
				return 0;
			}
		});
	}

	// simple polling
	return ch_event_stat & ch_event_mask;
}

void SPUThread::set_events(u32 mask)
{
	if (u32 unimpl = mask & ~SPU_EVENT_IMPLEMENTED)
	{
		throw EXCEPTION("Unimplemented events (0x%x)", unimpl);
	}

	// Set new events, get old event mask
	const u32 old_stat = ch_event_stat.fetch_or(mask);

	// Notify if some events were set
	if (~old_stat & mask && old_stat & SPU_EVENT_WAITING && ch_event_stat & SPU_EVENT_WAITING)
	{
		(*this)->lock_notify();
	}
}

void SPUThread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// detect enabling interrupts with events masked
		if (u32 mask = ch_event_mask)
		{
			throw EXCEPTION("SPU Interrupts not implemented (mask=0x%x)", mask);
		}

		ch_event_stat |= SPU_EVENT_INTR_ENABLED;
	}
	else
	{
		ch_event_stat &= ~SPU_EVENT_INTR_ENABLED;
	}
}

u32 SPUThread::get_ch_count(u32 ch)
{
	LOG_TRACE(SPU, "get_ch_count(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

	switch (ch)
	{
	//case MFC_Cmd:             return 16;
	//case SPU_WrSRR0:          return 1; break;
	//case SPU_RdSRR0:          return 1; break;
	case SPU_WrOutMbox:       return ch_out_mbox.get_count() ^ 1; break;
	case SPU_WrOutIntrMbox:   return ch_out_intr_mbox.get_count() ^ 1; break;
	case SPU_RdInMbox:        return ch_in_mbox.get_count(); break;
	case MFC_RdTagStat:       return ch_tag_stat.get_count(); break;
	case MFC_RdListStallStat: return ch_stall_stat.get_count(); break;
	case MFC_WrTagUpdate:     return ch_tag_stat.get_count(); break; // hack
	case SPU_RdSigNotify1:    return ch_snr1.get_count(); break;
	case SPU_RdSigNotify2:    return ch_snr2.get_count(); break;
	case MFC_RdAtomicStat:    return ch_atomic_stat.get_count(); break;
	case SPU_RdEventStat:     return get_events() ? 1 : 0; break;
	}

	throw EXCEPTION("Unknown/illegal channel (ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool SPUThread::get_ch_value(u32 ch, u32& out)
{
	LOG_TRACE(SPU, "get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

	auto read_channel = [&](spu_channel_t& channel)
	{
		if (!channel.try_pop(out))
		{
			cpu_thread_lock{*this}, thread_ctrl::wait(WRAP_EXPR(state & cpu_state::stop || channel.try_pop(out)));

			return !state.test(cpu_state::stop);
		}

		return true;
	};

	switch (ch)
	{
	//case SPU_RdSRR0:
	//	value = SRR0;
	//	break;
	case SPU_RdInMbox:
	{
		std::unique_lock<std::mutex> lock(get_current_thread_mutex(), std::defer_lock);

		while (true)
		{
			if (const uint old_count = ch_in_mbox.try_pop(out))
			{
				if (old_count == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				return true;
			}

			CHECK_EMU_STATUS;

			if (state & cpu_state::stop)
			{
				return false;
			}

			if (!lock)
			{
				lock.lock();
				continue;
			}

			get_current_thread_cv().wait(lock);
		}
	}

	case MFC_RdTagStat:
	{
		return read_channel(ch_tag_stat);
	}

	case MFC_RdTagMask:
	{
		out = ch_tag_mask;
		return true;
	}

	case SPU_RdSigNotify1:
	{
		return read_channel(ch_snr1);
	}

	case SPU_RdSigNotify2:
	{
		return read_channel(ch_snr2);
	}

	case MFC_RdAtomicStat:
	{
		return read_channel(ch_atomic_stat);
	}

	case MFC_RdListStallStat:
	{
		return read_channel(ch_stall_stat);
	}

	case SPU_RdDec:
	{
		out = ch_dec_value - (u32)(get_timebased_time() - ch_dec_start_timestamp);
		return true;
	}

	case SPU_RdEventMask:
	{
		out = ch_event_mask;
		return true;
	}

	case SPU_RdEventStat:
	{
		std::unique_lock<std::mutex> lock(get_current_thread_mutex(), std::defer_lock);

		// start waiting or return immediately
		if (u32 res = get_events(true))
		{
			out = res;
			return true;
		}

		if (ch_event_mask & SPU_EVENT_LR)
		{
			// register waiter if polling reservation status is required
			vm::wait_op(last_raddr, 128, WRAP_EXPR(get_events(true) || state & cpu_state::stop));
		}
		else
		{
			lock.lock();

			// simple waiting loop otherwise
			while (!get_events(true) && !(state & cpu_state::stop))
			{
				CHECK_EMU_STATUS;

				get_current_thread_cv().wait(lock);
			}
		}

		ch_event_stat &= ~SPU_EVENT_WAITING;

		if (state & cpu_state::stop)
		{
			return false;
		}
		
		out = get_events();
		return true;
	}

	case SPU_RdMachStat:
	{
		// HACK: "Not isolated" status
		// Return SPU Interrupt status in LSB
		out = (ch_event_stat & SPU_EVENT_INTR_ENABLED) != 0;
		return true;
	}
	}

	throw EXCEPTION("Unknown/illegal channel (ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool SPUThread::set_ch_value(u32 ch, u32 value)
{
	LOG_TRACE(SPU, "set_ch_value(ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);

	switch (ch)
	{
	//case SPU_WrSRR0:
	//	SRR0 = value & 0x3FFFC;  //LSLR & ~3
	//	break;
	case SPU_WrOutIntrMbox:
	{
		if (offset >= RAW_SPU_BASE_ADDR)
		{
			std::unique_lock<std::mutex> lock(get_current_thread_mutex(), std::defer_lock);

			while (!ch_out_intr_mbox.try_push(value))
			{
				CHECK_EMU_STATUS;

				if (state & cpu_state::stop)
				{
					return false;
				}

				if (!lock)
				{
					lock.lock();
					continue;
				}

				get_current_thread_cv().wait(lock);
			}

			int_ctrl[2].set(SPU_INT2_STAT_MAILBOX_INT);
			return true;
		}
		else
		{
			const u8 code = value >> 24;
			if (code < 64)
			{
				/* ===== sys_spu_thread_send_event (used by spu_printf) ===== */

				LV2_LOCK;

				const u8 spup = code & 63;

				if (!ch_out_mbox.get_count())
				{
					throw EXCEPTION("sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				if (u32 count = ch_in_mbox.get_count())
				{
					throw EXCEPTION("sys_spu_thread_send_event(value=0x%x, spup=%d): In_MBox is not empty (count=%d)", value, spup, count);
				}

				const u32 data = ch_out_mbox.get_value();

				ch_out_mbox.set_value(data, 0);

				LOG_TRACE(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = this->spup[spup].lock();

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return ch_in_mbox.set_values(1, CELL_ENOTCONN), true; // TODO: check error passing
				}

				if (queue->events() >= queue->size)
				{
					return ch_in_mbox.set_values(1, CELL_EBUSY), true;
				}

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, id, ((u64)spup << 32) | (value & 0x00ffffff), data);

				return ch_in_mbox.set_values(1, CELL_OK), true;
			}
			else if (code < 128)
			{
				/* ===== sys_spu_thread_throw_event ===== */

				LV2_LOCK;

				const u8 spup = code & 63;

				if (!ch_out_mbox.get_count())
				{
					throw EXCEPTION("sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				const u32 data = ch_out_mbox.get_value();

				ch_out_mbox.set_value(data, 0);

				LOG_TRACE(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = this->spup[spup].lock();

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return true;
				}

				// TODO: check passing spup value
				if (queue->events() >= queue->size)
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x) failed (queue is full)", spup, (value & 0x00ffffff), data);
					return true;
				}

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, id, ((u64)spup << 32) | (value & 0x00ffffff), data);
				return true;
			}
			else if (code == 128)
			{
				/* ===== sys_event_flag_set_bit ===== */

				LV2_LOCK;

				const u32 flag = value & 0xffffff;

				if (!ch_out_mbox.get_count())
				{
					throw EXCEPTION("sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				if (u32 count = ch_in_mbox.get_count())
				{
					throw EXCEPTION("sys_event_flag_set_bit(value=0x%x (flag=%d)): In_MBox is not empty (%d)", value, flag, count);
				}

				const u32 data = ch_out_mbox.get_value();

				ch_out_mbox.set_value(data, 0);

				if (flag > 63)
				{
					throw EXCEPTION("sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d)): Invalid flag", data, value, flag);
				}

				LOG_TRACE(SPU, "sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);

				const auto eflag = idm::get<lv2_event_flag_t>(data);

				if (!eflag)
				{
					return ch_in_mbox.set_values(1, CELL_ESRCH), true;
				}

				const u64 bitptn = 1ull << flag;

				if (~eflag->pattern.fetch_or(bitptn) & bitptn)
				{
					// notify if the bit was set
					eflag->notify_all(lv2_lock);
				}
				
				return ch_in_mbox.set_values(1, CELL_OK), true;
			}
			else if (code == 192)
			{
				/* ===== sys_event_flag_set_bit_impatient ===== */

				LV2_LOCK;

				const u32 flag = value & 0xffffff;

				if (!ch_out_mbox.get_count())
				{
					throw EXCEPTION("sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				const u32 data = ch_out_mbox.get_value();

				ch_out_mbox.set_value(data, 0);

				if (flag > 63)
				{
					throw EXCEPTION("sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d)): Invalid flag", data, value, flag);
				}

				LOG_TRACE(SPU, "sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);

				const auto eflag = idm::get<lv2_event_flag_t>(data);

				if (!eflag)
				{
					return true;
				}

				const u64 bitptn = 1ull << flag;

				if (~eflag->pattern.fetch_or(bitptn) & bitptn)
				{
					// notify if the bit was set
					eflag->notify_all(lv2_lock);
				}
				
				return true;
			}
			else
			{
				if (ch_out_mbox.get_count())
				{
					throw EXCEPTION("SPU_WrOutIntrMbox: unknown data (value=0x%x); Out_MBox = 0x%x", value, ch_out_mbox.get_value());
				}
				else
				{
					throw EXCEPTION("SPU_WrOutIntrMbox: unknown data (value=0x%x)", value);
				}
			}
		}
	}

	case SPU_WrOutMbox:
	{
		std::unique_lock<std::mutex> lock(get_current_thread_mutex(), std::defer_lock);

		while (!ch_out_mbox.try_push(value))
		{
			CHECK_EMU_STATUS;

			if (state & cpu_state::stop)
			{
				return false;
			}

			if (!lock)
			{
				lock.lock();
				continue;
			}

			get_current_thread_cv().wait(lock);
		}

		return true;
	}

	case MFC_WrTagMask:
	{
		ch_tag_mask = value;
		return true;
	}

	case MFC_WrTagUpdate:
	{
		ch_tag_stat.set_value(ch_tag_mask); // hack
		return true;
	}

	case MFC_LSA:
	{
		if (value >= 0x40000)
		{
			break;
		}

		ch_mfc_args.lsa = value;
		return true;
	}

	case MFC_EAH:
	{
		ch_mfc_args.eah = value;
		return true;
	}

	case MFC_EAL:
	{
		ch_mfc_args.eal = value;
		return true;
	}

	case MFC_Size:
	{
		if (value > 16 * 1024)
		{
			break;
		}

		ch_mfc_args.size = (u16)value;
		return true;
	}

	case MFC_TagID:
	{
		if (value >= 32)
		{
			break;
		}

		ch_mfc_args.tag = (u16)value;
		return true;
	}

	case MFC_Cmd:
	{
		process_mfc_cmd(value);
		ch_mfc_args = {}; // clear non-persistent data
		return true;
	}

	case MFC_WrListStallAck:
	{
		if (value >= 32)
		{
			break;
		}

		size_t processed = 0;

		for (size_t i = 0; i < mfc_queue.size(); i++)
		{
			if (mfc_queue[i].second.tag == value)
			{
				do_dma_list_cmd(mfc_queue[i].first, mfc_queue[i].second);
				mfc_queue[i].second.tag = 0xdead;
				processed++;
			}
		}

		while (processed)
		{
			for (size_t i = 0; i < mfc_queue.size(); i++)
			{
				if (mfc_queue[i].second.tag == 0xdead)
				{
					mfc_queue.erase(mfc_queue.begin() + i);
					processed--;
					break;
				}
			}
		}

		return true;
	}

	case SPU_WrDec:
	{
		ch_dec_start_timestamp = get_timebased_time();
		ch_dec_value = value;
		return true;
	}

	case SPU_WrEventMask:
	{
		// detect masking events with enabled interrupt status
		if (value && ch_event_stat & SPU_EVENT_INTR_ENABLED)
		{
			throw EXCEPTION("SPU Interrupts not implemented (mask=0x%x)", value);
		}

		// detect masking unimplemented events
		if (value & ~SPU_EVENT_IMPLEMENTED)
		{
			break;
		}

		ch_event_mask = value;
		return true;
	}

	case SPU_WrEventAck:
	{
		if (value & ~SPU_EVENT_IMPLEMENTED)
		{
			break;
		}

		ch_event_stat &= ~value;
		return true;
	}
	}

	throw EXCEPTION("Unknown/illegal channel (ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
}

bool SPUThread::stop_and_signal(u32 code)
{
	LOG_TRACE(SPU, "stop_and_signal(code=0x%x)", code);

	if (offset >= RAW_SPU_BASE_ADDR)
	{
		status.atomic_op([code](u32& status)
		{
			status = (status & 0xffff) | (code << 16);
			status |= SPU_STATUS_STOPPED_BY_STOP;
			status &= ~SPU_STATUS_RUNNING;
		});

		int_ctrl[2].set(SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT);
		state += cpu_state::stop;
		return true; // ???
	}

	switch (code)
	{
	case 0x001:
	{
		std::this_thread::sleep_for(1ms); // hack
		return true;
	}

	case 0x002:
	{
		state += cpu_state::ret;
		return true;
	}

	case 0x003:
	{
		const auto found = m_addr_to_hle_function_map.find(pc);

		if (found == m_addr_to_hle_function_map.end())
		{
			throw EXCEPTION("HLE function not registered (PC=0x%05x)", pc);
		}

		if (const auto return_to_caller = found->second(*this))
		{
			pc = (gpr[0]._u32[3] & 0x3fffc) - 4;
		}

		return true;
	}

	case 0x110:
	{
		/* ===== sys_spu_thread_receive_event ===== */

		LV2_LOCK;

		if (!ch_out_mbox.get_count())
		{
			throw EXCEPTION("sys_spu_thread_receive_event(): Out_MBox is empty");
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			LOG_ERROR(SPU, "sys_spu_thread_receive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		const u32 spuq = ch_out_mbox.get_value();

		ch_out_mbox.set_value(spuq, 0);

		LOG_TRACE(SPU, "sys_spu_thread_receive_event(spuq=0x%x)", spuq);

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		std::shared_ptr<lv2_event_queue_t> queue;

		for (auto& v : this->spuq)
		{
			if (spuq == v.first)
			{
				queue = v.second.lock();

				if (queue)
				{
					break;
				}
			}
		}

		if (!queue)
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true; // TODO: check error value
		}

		// check thread group status
		while (group->state >= SPU_THREAD_GROUP_STATUS_WAITING && group->state <= SPU_THREAD_GROUP_STATUS_SUSPENDED)
		{
			CHECK_EMU_STATUS;

			if (state & cpu_state::stop)
			{
				return false;
			}

			group->cv.wait_for(lv2_lock, 1ms);
		}

		// change group status
		if (group->state == SPU_THREAD_GROUP_STATUS_RUNNING)
		{
			group->state = SPU_THREAD_GROUP_STATUS_WAITING;

			for (auto& thread : group->threads)
			{
				if (thread)
				{
					thread->state += cpu_state::suspend;
				}
			}
		}
		else
		{
			throw EXCEPTION("Unexpected SPU Thread Group state (%d)", group->state);
		}

		if (queue->events())
		{
			const auto event = queue->pop(lv2_lock);
			ch_in_mbox.set_values(4, CELL_OK, static_cast<u32>(std::get<1>(event)), static_cast<u32>(std::get<2>(event)), static_cast<u32>(std::get<3>(event)));
		}
		else
		{
			// add waiter; protocol is ignored in current implementation
			sleep_entry<cpu_thread> waiter(queue->thread_queue(lv2_lock), *this);

			// wait on the event queue
			while (!state.test_and_reset(cpu_state::signal))
			{
				CHECK_EMU_STATUS;

				if (state & cpu_state::stop)
				{
					return false;
				}

				get_current_thread_cv().wait(lv2_lock);
			}

			// event data must be set by push()
		}
		
		// restore thread group status
		if (group->state == SPU_THREAD_GROUP_STATUS_WAITING)
		{
			group->state = SPU_THREAD_GROUP_STATUS_RUNNING;
		}
		else if (group->state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			group->state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		}
		else
		{
			throw EXCEPTION("Unexpected SPU Thread Group state (%d)", group->state);
		}

		for (auto& thread : group->threads)
		{
			if (thread && thread.get() != this)
			{
				thread->state -= cpu_state::suspend;
				(*thread)->lock_notify();
			}
		}

		state -= cpu_state::suspend;
		group->cv.notify_all();

		return true;
	}

	case 0x101:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		LV2_LOCK;

		if (!ch_out_mbox.get_count())
		{
			throw EXCEPTION("sys_spu_thread_group_exit(): Out_MBox is empty");
		}

		const u32 value = ch_out_mbox.get_value();

		ch_out_mbox.set_value(value, 0);

		LOG_TRACE(SPU, "sys_spu_thread_group_exit(status=0x%x)", value);

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		for (auto& thread : group->threads)
		{
			if (thread && thread.get() != this)
			{
				thread->state += cpu_state::stop;
				(*thread)->lock_notify();
			}
		}

		group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
		group->exit_status = value;
		group->join_state |= SPU_TGJSF_GROUP_EXIT;
		group->cv.notify_one();

		state += cpu_state::stop;
		return true;
	}

	case 0x102:
	{
		/* ===== sys_spu_thread_exit ===== */

		LV2_LOCK;

		if (!ch_out_mbox.get_count())
		{
			throw EXCEPTION("sys_spu_thread_exit(): Out_MBox is empty");
		}

		LOG_TRACE(SPU, "sys_spu_thread_exit(status=0x%x)", ch_out_mbox.get_value());

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		status |= SPU_STATUS_STOPPED_BY_STOP;
		group->cv.notify_one();

		state += cpu_state::stop;
		return true;
	}
	}

	if (!ch_out_mbox.get_count())
	{
		throw EXCEPTION("Unknown STOP code: 0x%x (Out_MBox is empty)", code);
	}
	else
	{
		throw EXCEPTION("Unknown STOP code: 0x%x (Out_MBox=0x%x)", code, ch_out_mbox.get_value());
	}
}

void SPUThread::halt()
{
	LOG_TRACE(SPU, "halt()");

	if (offset >= RAW_SPU_BASE_ADDR)
	{
		status.atomic_op([](u32& status)
		{
			status |= SPU_STATUS_STOPPED_BY_HALT;
			status &= ~SPU_STATUS_RUNNING;
		});

		int_ctrl[2].set(SPU_INT2_STAT_SPU_HALT_OR_STEP_INT);

		throw cpu_state::stop;
	}

	status |= SPU_STATUS_STOPPED_BY_HALT;
	throw EXCEPTION("Halt");
}

void SPUThread::fast_call(u32 ls_addr)
{
	// LS:0x0: this is originally the entry point of the interrupt handler, but interrupts are not implemented
	_ref<u32>(0) = 0x00000002; // STOP 2

	auto old_pc = pc;
	auto old_lr = gpr[0]._u32[3];
	auto old_stack = gpr[1]._u32[3]; // only saved and restored (may be wrong)
	auto old_task = std::move(custom_task);

	pc = ls_addr;
	gpr[0]._u32[3] = 0x0;
	custom_task = nullptr;

	try
	{
		cpu_task();
	}
	catch (cpu_state _s)
	{
		state += _s;
		if (_s != cpu_state::ret) throw;
	}

	state -= cpu_state::ret;

	pc = old_pc;
	gpr[0]._u32[3] = old_lr;
	gpr[1]._u32[3] = old_stack;
	custom_task = std::move(old_task);
}

void SPUThread::RegisterHleFunction(u32 addr, std::function<bool(SPUThread&)> function)
{
	m_addr_to_hle_function_map[addr] = function;
	_ref<u32>(addr) = 0x00000003; // STOP 3
}

void SPUThread::UnregisterHleFunction(u32 addr)
{
	m_addr_to_hle_function_map.erase(addr);
}

void SPUThread::UnregisterHleFunctions(u32 start_addr, u32 end_addr)
{
	for (auto iter = m_addr_to_hle_function_map.begin(); iter != m_addr_to_hle_function_map.end();)
	{
		if (iter->first >= start_addr && iter->first <= end_addr)
		{
			m_addr_to_hle_function_map.erase(iter++);
		}
		else
		{
			iter++;
		}
	}
}
