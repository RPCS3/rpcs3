#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/ErrorCodes.h"
#include "Emu/SysCalls/lv2/sys_spu.h"
#include "Emu/SysCalls/lv2/sys_event_flag.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUDecoder.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPUInterpreter2.h"
#include "Emu/Cell/SPURecompiler.h"

#include <cfenv>

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

	__forceinline spu_inter_func_t operator [] (u32 opcode) const
	{
		return funcs[opcode >> 21];
	}
}
g_spu_inter_func_list;

SPUThread& GetCurrentSPUThread()
{
	CPUThread* thread = GetCurrentCPUThread();

	if(!thread || (thread->GetType() != CPU_THREAD_SPU && thread->GetType() != CPU_THREAD_RAW_SPU))
	{
		throw std::string("GetCurrentSPUThread: bad thread");
	}

	return *(SPUThread*)thread;
}

SPUThread::SPUThread(CPUThreadType type) : CPUThread(type)
{
	assert(type == CPU_THREAD_SPU || type == CPU_THREAD_RAW_SPU);

	Reset();
}

SPUThread::~SPUThread()
{
}

void SPUThread::Task()
{
	std::fesetround(FE_TOWARDZERO);

	if (m_custom_task)
	{
		return m_custom_task(*this);
	}
	
	if (m_dec)
	{
		return CPUThread::Task();
	}
	
	while (true)
	{
		// read opcode
		const spu_opcode_t opcode = { vm::read32(PC + offset) };

		// get interpreter function
		const auto func = g_spu_inter_func_list[opcode.opcode];

		if (m_events)
		{
			// process events
			if (Emu.IsStopped())
			{
				return;
			}

			if (m_events & CPU_EVENT_STOP && (IsStopped() || IsPaused()))
			{
				m_events &= ~CPU_EVENT_STOP;
				return;
			}
		}

		// call interpreter function
		func(*this, opcode);

		// next instruction
		//PC += 4;
		NextPc(4);
	}
}

void SPUThread::DoReset()
{
	InitRegs();
}

void SPUThread::InitRegs()
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

	ch_event_mask = 0;
	ch_event_stat = {};

	ch_dec_start_timestamp = get_time(); // ???
	ch_dec_value = 0;

	run_ctrl = {};
	status = {};
	npc = {};

	int0.clear();
	int2.clear();

	GPR[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

void SPUThread::InitStack()
{
	m_stack_size = 0x2000; // this value is wrong
	m_stack_addr = offset + 0x40000 - m_stack_size; // stack is the part of SPU Local Storage
}

void SPUThread::CloseStack()
{
	// nothing to do here
}

void SPUThread::DoRun()
{
	m_dec = nullptr;

	switch (auto mode = Ini.SPUDecoderMode.GetValue())
	{
	case 0: // original interpreter
	{
		m_dec = new SPUDecoder(*new SPUInterpreter(*this));
		break;
	}
		
	case 1: // alternative interpreter
	{
		g_spu_inter_func_list.initialize(); // initialize helper table
		break;
	}

	case 2:
	{
		m_dec = new SPURecompilerCore(*this);
		break;
	}

	default:
	{
		LOG_ERROR(SPU, "Invalid SPU decoder mode: %d", mode);
		Emu.Pause();
	}
	}
}

void SPUThread::DoResume()
{
}

void SPUThread::DoPause()
{
}

void SPUThread::DoStop()
{
	delete m_dec;
	m_dec = nullptr;
}

void SPUThread::DoClose()
{
}

void SPUThread::FastCall(u32 ls_addr)
{
	// can't be called from another thread (because it doesn't make sense)
	write32(0x0, 2);

	auto old_PC = PC;
	auto old_LR = GPR[0]._u32[3];
	auto old_stack = GPR[1]._u32[3]; // only saved and restored (may be wrong)
	auto old_task = decltype(m_custom_task)();

	m_status = Running;
	PC = ls_addr;
	GPR[0]._u32[3] = 0x0;
	m_custom_task.swap(old_task);

	SPUThread::Task();

	PC = old_PC;
	GPR[0]._u32[3] = old_LR;
	GPR[1]._u32[3] = old_stack;
	m_custom_task.swap(old_task);
}

void SPUThread::FastStop()
{
	m_status = Stopped;
	m_events |= CPU_EVENT_STOP;
}

void SPUThread::FastRun()
{
	m_status = Running;
	Exec();
}

void SPUThread::do_dma_transfer(u32 cmd, spu_mfc_arg_t args)
{
	if (cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK))
	{
		_mm_mfence();
	}

	u32 eal = vm::cast(args.ea, "ea");

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
				spu.write_snr(SYS_SPU_THREAD_SNR2 == offset, read32(args.lsa));
				return;
			}
			else
			{
				LOG_ERROR(SPU, "do_dma_transfer(cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x): invalid MMIO offset", cmd, args.lsa, args.ea, args.tag, args.size);
				throw __FUNCTION__;
			}
		}
		else
		{
			LOG_ERROR(SPU, "do_dma_transfer(cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x): invalid thread type", cmd, args.lsa, args.ea, args.tag, args.size);
			throw __FUNCTION__;
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

	LOG_ERROR(SPU, "do_dma_transfer(cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x): invalid cmd (%s)", cmd, args.lsa, args.ea, args.tag, args.size, get_mfc_cmd_name(cmd));
	throw __FUNCTION__;
}

void SPUThread::do_dma_list_cmd(u32 cmd, spu_mfc_arg_t args)
{
	if (!(cmd & MFC_LIST_MASK))
	{
		LOG_ERROR(SPU, "do_dma_list_cmd(cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x): invalid cmd (%s)", cmd, args.lsa, args.ea, args.tag, args.size, get_mfc_cmd_name(cmd));
		throw __FUNCTION__;
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

		if (rec->sb.data() & se16(0x8000))
		{
			ch_stall_stat.push_bit_or(1 << args.tag);

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
		do_dma_transfer(cmd, ch_mfc_args);
		return;
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
		do_dma_list_cmd(cmd, ch_mfc_args);
		return;
	}

	case MFC_GETLLAR_CMD: // acquire reservation
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		vm::reservation_acquire(vm::get_ptr(offset + ch_mfc_args.lsa), vm::cast(ch_mfc_args.ea), 128, [this]()
		{
			ch_event_stat |= SPU_EVENT_LR;
			Notify();
		});

		ch_atomic_stat.push_uncond(MFC_GETLLAR_SUCCESS);
		return;
	}

	case MFC_PUTLLC_CMD: // store conditionally
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		if (vm::reservation_update(vm::cast(ch_mfc_args.ea), vm::get_ptr(offset + ch_mfc_args.lsa), 128))
		{
			ch_atomic_stat.push_uncond(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			ch_atomic_stat.push_uncond(MFC_PUTLLC_FAILURE);
		}

		return;
	}

	case MFC_PUTLLUC_CMD: // store unconditionally
	case MFC_PUTQLLUC_CMD:
	{
		if (ch_mfc_args.size != 128)
		{
			break;
		}

		vm::reservation_op(vm::cast(ch_mfc_args.ea), 128, [this]()
		{
			memcpy(vm::priv_ptr(vm::cast(ch_mfc_args.ea)), vm::get_ptr(offset + ch_mfc_args.lsa), 128);
		});

		if (cmd == MFC_PUTLLUC_CMD)
		{
			ch_atomic_stat.push_uncond(MFC_PUTLLUC_SUCCESS);
		}
		else
		{
			// tag may be used here
		}

		return;
	}
	}

	LOG_ERROR(SPU, "Unknown DMA %s: cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", get_mfc_cmd_name(cmd), cmd, ch_mfc_args.lsa, ch_mfc_args.ea, ch_mfc_args.tag, ch_mfc_args.size);
	throw __FUNCTION__;
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
	case SPU_RdEventStat:     return ch_event_stat.read_relaxed() & ch_event_mask ? 1 : 0; break;
	}

	LOG_ERROR(SPU, "get_ch_count(ch=%d [%s]): unknown/illegal channel", ch, ch < 128 ? spu_ch_name[ch] : "???");
	throw __FUNCTION__;
}

u32 SPUThread::get_ch_value(u32 ch)
{
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
	}

	switch (ch)
	{
	//case SPU_RdSRR0:
	//	value = SRR0;
	//	break;
	case SPU_RdInMbox:
	{
		u32 result, count;
		while (!ch_in_mbox.pop(result, count) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}
		
		if (count + 1 == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
		{
			int2.set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
		}
		
		return result;
	}

	case MFC_RdTagStat:
	{
		u32 result;
		while (!ch_tag_stat.pop(result) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		return result;
	}

	case MFC_RdTagMask:
	{
		return ch_tag_mask;
	}

	case SPU_RdSigNotify1:
	{
		u32 result;
		while (!ch_snr1.pop(result) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		return result;
	}

	case SPU_RdSigNotify2:
	{
		u32 result;
		while (!ch_snr2.pop(result) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		return result;
	}

	case MFC_RdAtomicStat:
	{
		u32 result;
		while (!ch_atomic_stat.pop(result) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		return result;
	}

	case MFC_RdListStallStat:
	{
		u32 result;
		while (!ch_stall_stat.pop(result) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}

		return result;
	}

	case SPU_RdDec:
	{
		return ch_dec_value - (u32)(get_time() - ch_dec_start_timestamp);
	}

	case SPU_RdEventMask:
	{
		return ch_event_mask;
	}

	case SPU_RdEventStat:
	{
		u32 result;
		while (!(result = ch_event_stat.read_relaxed() & ch_event_mask) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		}
		
		return result;
	}

	case SPU_RdMachStat:
	{
		return 1; // hack (not isolated, interrupts enabled)
	}
	}

	LOG_ERROR(SPU, "get_ch_value(ch=%d [%s]): unknown/illegal channel", ch, ch < 128 ? spu_ch_name[ch] : "???");
	throw __FUNCTION__;
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
			while (!ch_out_intr_mbox.push(value) && !Emu.IsStopped())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			}

			int2.set(SPU_INT2_STAT_MAILBOX_INT);
			return;
		}
		else
		{
			const u8 code = value >> 24;
			if (code < 64)
			{
				/* ===== sys_spu_thread_send_event (used by spu_printf) ===== */

				u8 spup = code & 63;

				u32 data;
				if (!ch_out_mbox.pop(data))
				{
					LOG_ERROR(SPU, "sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
					throw __FUNCTION__;
				}

				if (Ini.HLELogging.GetValue())
				{
					LOG_NOTICE(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);
				}

				LV2_LOCK;

				const auto queue = this->spup[spup].lock();

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return ch_in_mbox.push_uncond(CELL_ENOTCONN); // TODO: check error passing
				}

				if (queue->events.size() >= queue->size)
				{
					return ch_in_mbox.push_uncond(CELL_EBUSY);
				}

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, GetId(), ((u64)spup << 32) | (value & 0x00ffffff), data);

				return ch_in_mbox.push_uncond(CELL_OK);
			}
			else if (code < 128)
			{
				/* ===== sys_spu_thread_throw_event ===== */

				const u8 spup = code & 63;

				u32 data;
				if (!ch_out_mbox.pop(data))
				{
					LOG_ERROR(SPU, "sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
					throw __FUNCTION__;
				}

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);
				}

				LV2_LOCK;

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

				queue->push(lv2_lock, SYS_SPU_THREAD_EVENT_USER_KEY, GetId(), ((u64)spup << 32) | (value & 0x00ffffff), data);
				return;
			}
			else if (code == 128)
			{
				/* ===== sys_event_flag_set_bit ===== */
				u32 flag = value & 0xffffff;

				u32 data;
				if (!ch_out_mbox.pop(data))
				{
					LOG_ERROR(SPU, "sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
					throw __FUNCTION__;
				}

				if (flag > 63)
				{
					LOG_ERROR(SPU, "sys_event_flag_set_bit(id=%d, value=0x%x): flag > 63", data, value, flag);
					throw __FUNCTION__;
				}

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);
				}

				LV2_LOCK;

				const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(data);

				if (!ef)
				{
					return ch_in_mbox.push_uncond(CELL_ESRCH);
				}

				while (ef->cancelled)
				{
					ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
				}

				ef->flags |= 1ull << flag;

				if (ef->waiters)
				{
					ef->cv.notify_all();
				}
				
				return ch_in_mbox.push_uncond(CELL_OK);
			}
			else if (code == 192)
			{
				/* ===== sys_event_flag_set_bit_impatient ===== */
				u32 flag = value & 0xffffff;

				u32 data;
				if (!ch_out_mbox.pop(data))
				{
					LOG_ERROR(SPU, "sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
					throw __FUNCTION__;
				}

				if (flag > 63)
				{
					LOG_ERROR(SPU, "sys_event_flag_set_bit_impatient(id=%d, value=0x%x): flag > 63", data, value, flag);
					throw __FUNCTION__;
				}

				if (Ini.HLELogging.GetValue())
				{
					LOG_WARNING(SPU, "sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);
				}

				LV2_LOCK;

				const auto ef = Emu.GetIdManager().GetIDData<event_flag_t>(data);

				if (!ef)
				{
					return;
				}

				while (ef->cancelled)
				{
					ef->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
				}

				ef->flags |= 1ull << flag;

				if (ef->waiters)
				{
					ef->cv.notify_all();
				}
				
				return;
			}
			else
			{
				if (ch_out_mbox.get_count())
				{
					LOG_ERROR(SPU, "SPU_WrOutIntrMbox: unknown data (value=0x%x); Out_MBox = 0x%x", value, ch_out_mbox.get_value());
				}
				else
				{
					LOG_ERROR(SPU, "SPU_WrOutIntrMbox: unknown data (value=0x%x)", value);
				}

				throw __FUNCTION__;
			}
		}
	}

	case SPU_WrOutMbox:
	{
		while (!ch_out_mbox.push(value) && !Emu.IsStopped())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
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
		ch_tag_stat.push_uncond(ch_tag_mask); // hack
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
		ch_dec_start_timestamp = get_time();
		ch_dec_value = value;
		return;
	}

	case SPU_WrEventMask:
	{
		if (value & ~(SPU_EVENT_IMPLEMENTED))
		{
			break;
		}

		ch_event_mask = value;
		return;
	}

	case SPU_WrEventAck:
	{
		ch_event_stat &= ~value;
		return;
	}
	}

	LOG_ERROR(SPU, "set_ch_value(ch=%d [%s], value=0x%x): unknown/illegal channel", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
	throw __FUNCTION__;
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

		FastStop();

		int2.set(SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT);
		return;
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
		FastStop();
		return;
	}

	case 0x003:
	{
		auto iter = m_addr_to_hle_function_map.find(PC);
		assert(iter != m_addr_to_hle_function_map.end());

		auto return_to_caller = iter->second(*this);
		if (return_to_caller)
		{
			SetBranch(GPR[0]._u32[3] & 0x3fffc);
		}
		return;
	}

	case 0x110:
	{
		/* ===== sys_spu_thread_receive_event ===== */

		u32 spuq = 0;
		if (!ch_out_mbox.pop(spuq))
		{
			LOG_ERROR(SPU, "sys_spu_thread_receive_event(): cannot read Out_MBox");
			throw __FUNCTION__;
		}

		if (ch_in_mbox.get_count())
		{
			LOG_ERROR(SPU, "sys_spu_thread_receive_event(spuq=0x%x): In_MBox is not empty", spuq);
			return ch_in_mbox.push_uncond(CELL_EBUSY);
		}

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_receive_event(spuq=0x%x)", spuq);
		}

		LV2_LOCK;

		std::shared_ptr<event_queue_t> queue;

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
			return ch_in_mbox.push_uncond(CELL_EINVAL); // TODO: check error value
		}

		// protocol is ignored in current implementation
		queue->waiters++;

		while (queue->events.empty())
		{
			if (queue->cancelled)
			{
				return ch_in_mbox.push_uncond(CELL_ECANCELED);
			}

			if (Emu.IsStopped())
			{
				LOG_WARNING(SPU, "sys_spu_thread_receive_event(spuq=0x%x) aborted", spuq);
				return;
			}

			queue->cv.wait_for(lv2_lock, std::chrono::milliseconds(1));
		}

		auto& event = queue->events.front();
		ch_in_mbox.push_uncond(CELL_OK);
		ch_in_mbox.push_uncond((u32)event.data1);
		ch_in_mbox.push_uncond((u32)event.data2);
		ch_in_mbox.push_uncond((u32)event.data3);
		
		queue->events.pop_front();
		queue->waiters--;

		if (queue->events.size())
		{
			queue->cv.notify_one();
		}

		return;
	}

	case 0x101:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		u32 value;
		if (!ch_out_mbox.pop(value))
		{
			LOG_ERROR(SPU, "sys_spu_thread_group_exit(): cannot read Out_MBox");
			throw __FUNCTION__;
		}

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_group_exit(status=0x%x)", value);
		}

		LV2_LOCK;

		const auto group = tg.lock();

		if (!group)
		{
			LOG_ERROR(SPU, "sys_spu_thread_group_exit(status=0x%x): invalid group", value);
			throw __FUNCTION__;
		}

		for (auto t : group->threads)
		{
			if (t)
			{
				auto& spu = static_cast<SPUThread&>(*t);

				spu.FastStop();
			}
		}

		group->state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
		group->exit_status = value;
		group->join_state |= SPU_TGJSF_GROUP_EXIT;
		group->join_cv.notify_one();
		return;
	}

	case 0x102:
	{
		/* ===== sys_spu_thread_exit ===== */

		if (!ch_out_mbox.get_count())
		{
			LOG_ERROR(SPU, "sys_spu_thread_exit(): Out_MBox is empty");
			throw __FUNCTION__;
		}

		if (Ini.HLELogging.GetValue())
		{
			LOG_NOTICE(SPU, "sys_spu_thread_exit(status=0x%x)", ch_out_mbox.get_value());
		}

		LV2_LOCK;

		status |= SPU_STATUS_STOPPED_BY_STOP;
		FastStop();
		return;
	}
	}

	if (!ch_out_mbox.get_count())
	{
		LOG_ERROR(SPU, "Unknown STOP code: 0x%x", code);
	}
	else
	{
		LOG_ERROR(SPU, "Unknown STOP code: 0x%x; Out_MBox=0x%x", code, ch_out_mbox.get_value());
	}
	
	throw __FUNCTION__;
}

void SPUThread::halt()
{
	if (Ini.HLELogging.GetValue())
	{
		LOG_NOTICE(SPU, "halt(code=0x%x)");
	}

	if (m_type == CPU_THREAD_RAW_SPU)
	{
		status.atomic_op([](u32& status)
		{
			status |= SPU_STATUS_STOPPED_BY_HALT;
			status &= ~SPU_STATUS_RUNNING;
		});

		FastStop();

		int2.set(SPU_INT2_STAT_SPU_HALT_OR_STEP_INT);
		return;
	}

	status |= SPU_STATUS_STOPPED_BY_HALT;
	throw "HALT";
}

spu_thread::spu_thread(u32 entry, const std::string& name, u32 stack_size, u32 prio)
{
	thread = Emu.GetCPU().AddThread(CPU_THREAD_SPU);

	thread->SetName(name);
	thread->SetEntry(entry);
	thread->SetStackSize(stack_size ? stack_size : Emu.GetInfo().GetProcParam().primary_stacksize);
	thread->SetPrio(prio ? prio : Emu.GetInfo().GetProcParam().primary_prio);

	argc = 0;
}
