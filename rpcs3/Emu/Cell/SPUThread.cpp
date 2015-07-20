#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/ErrorCodes.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_event_flag.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_interrupt.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPUInterpreter2.h"
#include "Emu/Cell/SPURecompiler.h"

#include <cfenv>

extern u64 get_timebased_time();

void spu_int_ctrl_t::set(u64 ints)
{
	// leave only enabled interrupts
	ints &= mask.load();

	// notify if at least 1 bit was set
	if (ints && ~stat._or(ints) & ints && tag)
	{
		LV2_LOCK;

		if (tag && tag->handler)
		{
			tag->handler->signal++;

			tag->handler->thread->cv.notify_one();
		}
	}
}

void spu_int_ctrl_t::clear(u64 ints)
{
	stat &= ~ints;
}

const g_spu_imm_table_t g_spu_imm;

class spu_inter_func_list_t
{
	std::array<spu_inter_func_t, 2048> funcs = {};

	std::mutex m_mutex;

public:
	void initialize()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (funcs[0]) return; // check if already initialized

		auto inter = new SPUInterpreter2;
		SPUDecoder dec(*inter);

		for (u32 i = 0; i < funcs.size(); i++)
		{
			inter->func = spu_interpreter::DEFAULT;
			
			dec.Decode(i << 21);

			funcs[i] = inter->func;
		}
	}

	force_inline spu_inter_func_t operator [](u32 opcode) const
	{
		return funcs[opcode >> 21];
	}
}
g_spu_inter_func_list;

SPUThread::SPUThread(CPUThreadType type, const std::string& name, std::function<std::string()> thread_name, u32 index, u32 offset)
	: CPUThread(type, name, std::move(thread_name))
	, index(index)
	, offset(offset)
{
}

SPUThread::SPUThread(const std::string& name, u32 index)
	: CPUThread(CPU_THREAD_SPU, name, WRAP_EXPR(fmt::format("SPU[0x%x] Thread (%s)[0x%08x]", m_id, m_name.c_str(), PC)))
	, index(index)
	, offset(vm::alloc(0x40000, vm::main))
{
	if (!offset)
	{
		throw EXCEPTION("Failed to allocate SPU local storage");
	}
}

SPUThread::~SPUThread()
{
	if (m_type == CPU_THREAD_SPU)
	{
		join();

		if (!vm::dealloc(offset, vm::main))
		{
			throw EXCEPTION("Failed to deallocate SPU local storage");
		}
	}
	else if (joinable())
	{
		throw EXCEPTION("Thread not joined");
	}
}

bool SPUThread::is_paused() const
{
	if (CPUThread::is_paused())
	{
		return true;
	}

	if (const auto group = tg.lock())
	{
		if (group->state >= SPU_THREAD_GROUP_STATUS_WAITING && group->state <= SPU_THREAD_GROUP_STATUS_SUSPENDED)
		{
			return true;
		}
	}

	return false;
}

void SPUThread::dump_info() const
{
	CPUThread::dump_info();
}

void SPUThread::task()
{
	std::fesetround(FE_TOWARDZERO);

	if (custom_task)
	{
		if (check_status()) return;

		return custom_task(*this);
	}
	
	if (m_dec)
	{
		while (true)
		{
			if (m_state.load() && check_status()) break;

			// decode instruction using specified decoder
			m_dec->DecodeMemory(PC + offset);

			// next instruction
			PC += 4;
		}
	}
	else
	{
		while (true)
		{
			if (m_state.load() && check_status()) break;

			// read opcode
			const spu_opcode_t opcode = { vm::read32(PC + offset) };

			// get interpreter function
			const auto func = g_spu_inter_func_list[opcode.opcode];

			// call interpreter function
			func(*this, opcode);

			// next instruction
			PC += 4;
		}
	}
}

void SPUThread::init_regs()
{
	memset(GPR, 0, sizeof(GPR));
	FPSCR.Reset();

	ch_mfc_args = {};
	mfc_queue.clear();

	ch_tag_mask = 0;
	ch_tag_stat = {};
	ch_stall_stat = {};
	ch_atomic_stat = {};

	ch_in_mbox.clear();

	ch_out_mbox = {};
	ch_out_intr_mbox = {};

	snr_config = 0;

	ch_snr1 = {};
	ch_snr2 = {};

	ch_event_mask = {};
	ch_event_stat = {};
	last_raddr = 0;

	ch_dec_start_timestamp = get_timebased_time(); // ???
	ch_dec_value = 0;

	run_ctrl = {};
	status = {};
	npc = {};

	int_ctrl = {};

	GPR[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

void SPUThread::init_stack()
{
	// nothing to do
}

void SPUThread::close_stack()
{
	// nothing to do here
}

void SPUThread::do_run()
{
	m_dec = nullptr;

	switch (auto mode = Ini.SPUDecoderMode.GetValue())
	{
	case 0: // original interpreter
	{
		m_dec.reset(new SPUDecoder(*new SPUInterpreter(*this)));
		break;
	}
		
	case 1: // alternative interpreter
	{
		g_spu_inter_func_list.initialize(); // initialize helper table
		break;
	}

	case 2:
	{
		m_dec.reset(new SPURecompilerCore(*this));
		break;
	}

	default:
	{
		LOG_ERROR(SPU, "Invalid SPU decoder mode: %d", mode);
		Emu.Pause();
	}
	}
}

void SPUThread::fast_call(u32 ls_addr)
{
	if (!is_current())
	{
		throw EXCEPTION("Called from the wrong thread");
	}

	write32(0x0, 2);

	auto old_PC = PC;
	auto old_LR = GPR[0]._u32[3];
	auto old_stack = GPR[1]._u32[3]; // only saved and restored (may be wrong)
	auto old_task = std::move(custom_task);

	PC = ls_addr;
	GPR[0]._u32[3] = 0x0;
	custom_task = nullptr;

	try
	{
		task();
	}
	catch (CPUThreadReturn)
	{
	}

	m_state &= ~CPU_STATE_RETURN;

	PC = old_PC;
	GPR[0]._u32[3] = old_LR;
	GPR[1]._u32[3] = old_stack;
	custom_task = std::move(old_task);
}

void SPUThread::do_dma_transfer(u32 cmd, spu_mfc_arg_t args)
{
	if (cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK))
	{
		_mm_mfence();
	}

	u32 eal = VM_CAST(args.ea);

	if (eal >= SYS_SPU_THREAD_BASE_LOW && m_type == CPU_THREAD_SPU) // SPU Thread Group MMIO (LS and SNR)
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
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, read32(args.lsa));
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
		memcpy(vm::get_ptr(eal), vm::get_ptr(offset + args.lsa), args.size);
		return;
	}

	case MFC_GET_CMD:
	{
		memcpy(vm::get_ptr(offset + args.lsa), vm::get_ptr(eal), args.size);
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
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "DMA %s: cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", get_mfc_cmd_name(cmd), cmd, ch_mfc_args.lsa, ch_mfc_args.ea, ch_mfc_args.tag, ch_mfc_args.size);
	}

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

		const u32 raddr = VM_CAST(ch_mfc_args.ea);

		vm::reservation_acquire(vm::get_ptr(offset + ch_mfc_args.lsa), raddr, 128);

		if (last_raddr)
		{
			ch_event_stat |= SPU_EVENT_LR;
		}

		last_raddr = raddr;

		return ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
	}

	case MFC_PUTLLC_CMD: // store conditionally
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		if (vm::reservation_update(VM_CAST(ch_mfc_args.ea), vm::get_ptr(offset + ch_mfc_args.lsa), 128))
		{
			if (last_raddr == 0)
			{
				throw EXCEPTION("Unexpected: PUTLLC command succeeded, but GETLLAR command not detected");
			}

			last_raddr = 0;

			return ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			if (last_raddr != 0)
			{
				ch_event_stat |= SPU_EVENT_LR;

				last_raddr = 0;
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

		vm::reservation_op(VM_CAST(ch_mfc_args.ea), 128, [this]()
		{
			std::memcpy(vm::priv_ptr(VM_CAST(ch_mfc_args.ea)), vm::get_ptr(offset + ch_mfc_args.lsa), 128);
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
	}

	throw EXCEPTION("Unknown command %s (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)", get_mfc_cmd_name(cmd), cmd, ch_mfc_args.lsa, ch_mfc_args.ea, ch_mfc_args.tag, ch_mfc_args.size);
}

u32 SPUThread::get_events(bool waiting)
{
	// check reservation status and set SPU_EVENT_LR if lost
	if (last_raddr != 0 && !vm::reservation_test(get_thread_ctrl()))
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
			if (u32 res = stat & ch_event_mask.load())
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
	return ch_event_stat.load() & ch_event_mask.load();
}

void SPUThread::set_events(u32 mask)
{
	if (u32 unimpl = mask & ~SPU_EVENT_IMPLEMENTED)
	{
		throw EXCEPTION("Unimplemented events (0x%x)", unimpl);
	}

	// set new events, get old event mask
	const u32 old_stat = ch_event_stat._or(mask);

	// notify if some events were set
	if (~old_stat & mask && old_stat & SPU_EVENT_WAITING)
	{
		std::lock_guard<std::mutex> lock(mutex);

		if (ch_event_stat.load() & SPU_EVENT_WAITING)
		{
			cv.notify_one();
		}
	}
}

void SPUThread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// detect enabling interrupts with events masked
		if (u32 mask = ch_event_mask.load())
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
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "get_ch_count(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
	}

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

u32 SPUThread::get_ch_value(u32 ch)
{
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
	}

	auto read_channel = [this](spu_channel_t& channel) -> u32
	{
		std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

		while (true)
		{
			bool result;
			u32 value;

			std::tie(result, value) = channel.try_pop();

			if (result)
			{
				return value;
			}

			CHECK_EMU_STATUS;

			if (is_stopped()) throw CPUThreadStop{};

			if (!lock)
			{
				lock.lock();
				continue;
			}

			cv.wait(lock);
		}
	};

	switch (ch)
	{
	//case SPU_RdSRR0:
	//	value = SRR0;
	//	break;
	case SPU_RdInMbox:
	{
		std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

		while (true)
		{
			bool result;
			u32 value;
			u32 count;
			
			std::tie(result, value, count) = ch_in_mbox.try_pop();

			if (result)
			{
				if (count + 1 == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				return value;
			}

			CHECK_EMU_STATUS;

			if (is_stopped()) throw CPUThreadStop{};

			if (!lock)
			{
				lock.lock();
				continue;
			}

			cv.wait(lock);
		}
	}

	case MFC_RdTagStat:
	{
		return read_channel(ch_tag_stat);
	}

	case MFC_RdTagMask:
	{
		return ch_tag_mask;
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
		return ch_dec_value - (u32)(get_timebased_time() - ch_dec_start_timestamp);
	}

	case SPU_RdEventMask:
	{
		return ch_event_mask.load();
	}

	case SPU_RdEventStat:
	{
		std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

		// start waiting or return immediately
		if (u32 res = get_events(true))
		{
			return res;
		}

		if (ch_event_mask.load() & SPU_EVENT_LR)
		{
			// register waiter if polling reservation status is required
			vm::wait_op(*this, last_raddr, 128, WRAP_EXPR(get_events(true) || is_stopped()));
		}
		else
		{
			lock.lock();

			// simple waiting loop otherwise
			while (!get_events(true) && !is_stopped())
			{
				CHECK_EMU_STATUS;

				cv.wait(lock);
			}
		}

		ch_event_stat &= ~SPU_EVENT_WAITING;

		if (is_stopped()) throw CPUThreadStop{};
		
		return get_events();
	}

	case SPU_RdMachStat:
	{
		// HACK: "Not isolated" status
		// Return SPU Interrupt status in LSB
		return (ch_event_stat.load() & SPU_EVENT_INTR_ENABLED) != 0;
	}
	}

	throw EXCEPTION("Unknown/illegal channel (ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
}

void SPUThread::set_ch_value(u32 ch, u32 value)
{
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "set_ch_value(ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
	}

	switch (ch)
	{
	//case SPU_WrSRR0:
	//	SRR0 = value & 0x3FFFC;  //LSLR & ~3
	//	break;
	case SPU_WrOutIntrMbox:
	{
		if (m_type == CPU_THREAD_RAW_SPU)
		{
			std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

			while (!ch_out_intr_mbox.try_push(value))
			{
				CHECK_EMU_STATUS;

				if (is_stopped()) throw CPUThreadStop{};

				if (!lock)
				{
					lock.lock();
					continue;
				}

				cv.wait(lock);
			}

			int_ctrl[2].set(SPU_INT2_STAT_MAILBOX_INT);
			return;
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

				if (Ini.HLELogging.GetValue())
				{
					LOG_NOTICE(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);
				}

				const auto queue = this->spup[spup].lock();

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return ch_in_mbox.set_values(1, CELL_ENOTCONN); // TODO: check error passing
				}

				if (queue->events.size() >= queue->size)
				{
					return ch_in_mbox.set_values(1, CELL_EBUSY);
				}

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, m_id, ((u64)spup << 32) | (value & 0x00ffffff), data);

				return ch_in_mbox.set_values(1, CELL_OK);
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

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);
				}

				const auto queue = this->spup[spup].lock();

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return;
				}

				// TODO: check passing spup value
				if (queue->events.size() >= queue->size)
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x) failed (queue is full)", spup, (value & 0x00ffffff), data);
					return;
				}

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, m_id, ((u64)spup << 32) | (value & 0x00ffffff), data);
				return;
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

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);
				}

				const auto eflag = Emu.GetIdManager().get<lv2_event_flag_t>(data);

				if (!eflag)
				{
					return ch_in_mbox.set_values(1, CELL_ESRCH);
				}

				const u64 bitptn = 1ull << flag;

				if (~eflag->pattern.fetch_or(bitptn) & bitptn)
				{
					// notify if the bit was set
					eflag->notify_all(lv2_lock);
				}
				
				return ch_in_mbox.set_values(1, CELL_OK);
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

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);
				}

				const auto eflag = Emu.GetIdManager().get<lv2_event_flag_t>(data);

				if (!eflag)
				{
					return;
				}

				const u64 bitptn = 1ull << flag;

				if (~eflag->pattern.fetch_or(bitptn) & bitptn)
				{
					// notify if the bit was set
					eflag->notify_all(lv2_lock);
				}
				
				return;
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
		std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

		while (!ch_out_mbox.try_push(value))
		{
			CHECK_EMU_STATUS;

			if (is_stopped()) throw CPUThreadStop{};

			if (!lock)
			{
				lock.lock();
				continue;
			}

			cv.wait(lock);
		}

		return;
	}

	case MFC_WrTagMask:
	{
		ch_tag_mask = value;
		return;
	}

	case MFC_WrTagUpdate:
	{
		ch_tag_stat.set_value(ch_tag_mask); // hack
		return;
	}

	case MFC_LSA:
	{
		if (value >= 0x40000)
		{
			break;
		}

		ch_mfc_args.lsa = value;
		return;
	}

	case MFC_EAH:
	{
		ch_mfc_args.eah = value;
		return;
	}

	case MFC_EAL:
	{
		ch_mfc_args.eal = value;
		return;
	}

	case MFC_Size:
	{
		if (value > 16 * 1024)
		{
			break;
		}

		ch_mfc_args.size = (u16)value;
		return;
	}

	case MFC_TagID:
	{
		if (value >= 32)
		{
			break;
		}

		ch_mfc_args.tag = (u16)value;
		return;
	}

	case MFC_Cmd:
	{
		process_mfc_cmd(value);
		ch_mfc_args = {}; // clear non-persistent data
		return;
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

		return;
	}

	case SPU_WrDec:
	{
		ch_dec_start_timestamp = get_timebased_time();
		ch_dec_value = value;
		return;
	}

	case SPU_WrEventMask:
	{
		// detect masking events with enabled interrupt status
		if (value && ch_event_stat.load() & SPU_EVENT_INTR_ENABLED)
		{
			throw EXCEPTION("SPU Interrupts not implemented (mask=0x%x)", value);
		}

		// detect masking unimplemented events
		if (value & ~SPU_EVENT_IMPLEMENTED)
		{
			break;
		}

		ch_event_mask.store(value);
		return;
	}

	case SPU_WrEventAck:
	{
		if (value & ~SPU_EVENT_IMPLEMENTED)
		{
			break;
		}

		ch_event_stat &= ~value;
		return;
	}
	}

	throw EXCEPTION("Unknown/illegal channel (ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
}

void SPUThread::stop_and_signal(u32 code)
{
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "stop_and_signal(code=0x%x)", code);
	}

	if (m_type == CPU_THREAD_RAW_SPU)
	{
		status.atomic_op([code](u32& status)
		{
			status = (status & 0xffff) | (code << 16);
			status |= SPU_STATUS_STOPPED_BY_STOP;
			status &= ~SPU_STATUS_RUNNING;
		});

		int_ctrl[2].set(SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT);

		return stop();
	}

	switch (code)
	{
	case 0x001:
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return;
	}

	case 0x002:
	{
		m_state |= CPU_STATE_RETURN;
		return;
	}

	case 0x003:
	{
		auto iter = m_addr_to_hle_function_map.find(PC);
		assert(iter != m_addr_to_hle_function_map.end());

		auto return_to_caller = iter->second(*this);
		if (return_to_caller)
		{
			PC = (GPR[0]._u32[3] & 0x3fffc) - 4;
		}
		return;
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
			return ch_in_mbox.set_values(1, CELL_EBUSY);
		}

		const u32 spuq = ch_out_mbox.get_value();

		ch_out_mbox.set_value(spuq, 0);

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_receive_event(spuq=0x%x)", spuq);
		}

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL);
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
			return ch_in_mbox.set_values(1, CELL_EINVAL); // TODO: check error value
		}

		// check thread group status
		while (group->state >= SPU_THREAD_GROUP_STATUS_WAITING && group->state <= SPU_THREAD_GROUP_STATUS_SUSPENDED)
		{
			CHECK_EMU_STATUS;

			if (is_stopped()) throw CPUThreadStop{};

			group->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
		}

		// change group status
		if (group->state == SPU_THREAD_GROUP_STATUS_RUNNING)
		{
			group->state = SPU_THREAD_GROUP_STATUS_WAITING;

			for (auto& thread : group->threads)
			{
				if (thread) thread->sleep(); // trigger status check
			}
		}
		else
		{
			throw EXCEPTION("Unexpected SPU Thread Group state (%d)", group->state);
		}

		if (queue->events.size())
		{
			auto& event = queue->events.front();
			ch_in_mbox.set_values(4, CELL_OK, static_cast<u32>(std::get<1>(event)), static_cast<u32>(std::get<2>(event)), static_cast<u32>(std::get<3>(event)));

			queue->events.pop_front();
		}
		else
		{
			// add waiter; protocol is ignored in current implementation
			sleep_queue_entry_t waiter(*this, queue->sq);

			// wait on the event queue
			while (!unsignal())
			{
				CHECK_EMU_STATUS;

				if (is_stopped()) throw CPUThreadStop{};

				cv.wait(lv2_lock);
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
			if (thread) thread->awake(); // untrigger status check
		}

		group->cv.notify_all();

		return;
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

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_group_exit(status=0x%x)", value);
		}

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		for (auto thread : group->threads)
		{
			if (thread && thread.get() != this)
			{
				thread->stop();
			}
		}

		group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
		group->exit_status = value;
		group->join_state |= SPU_TGJSF_GROUP_EXIT;
		group->cv.notify_one();

		return stop();
	}

	case 0x102:
	{
		/* ===== sys_spu_thread_exit ===== */

		LV2_LOCK;

		if (!ch_out_mbox.get_count())
		{
			throw EXCEPTION("sys_spu_thread_exit(): Out_MBox is empty");
		}

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_exit(status=0x%x)", ch_out_mbox.get_value());
		}

		const auto group = tg.lock();

		if (!group)
		{
			throw EXCEPTION("Invalid SPU Thread Group");
		}

		status |= SPU_STATUS_STOPPED_BY_STOP;
		group->cv.notify_one();

		return stop();
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
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "halt()");
	}

	if (m_type == CPU_THREAD_RAW_SPU)
	{
		status.atomic_op([](u32& status)
		{
			status |= SPU_STATUS_STOPPED_BY_HALT;
			status &= ~SPU_STATUS_RUNNING;
		});

		int_ctrl[2].set(SPU_INT2_STAT_SPU_HALT_OR_STEP_INT);

		return stop();
	}

	status |= SPU_STATUS_STOPPED_BY_HALT;
	throw EXCEPTION("Halt");
}

spu_thread::spu_thread(u32 entry, const std::string& name, u32 stack_size, u32 prio)
{
	auto spu = Emu.GetIdManager().make_ptr<SPUThread>(name, 0x13370666);

	spu->PC = entry;

	thread = std::move(spu);
}
