#include "stdafx.h"
#include "Utilities/lockless.h"
#include "Utilities/sysinfo.h"
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
#include "Emu/Cell/RawSPUThread.h"

#include <cmath>
#include <cfenv>
#include <atomic>
#include <thread>

const bool s_use_rtm = utils::has_rtm();

#ifdef _MSC_VER
bool operator ==(const u128& lhs, const u128& rhs)
{
	return lhs.lo == rhs.lo && lhs.hi == rhs.hi;
}
#endif

extern u64 get_timebased_time();

extern thread_local u64 g_tls_fault_spu;

const spu_decoder<spu_interpreter_precise> s_spu_interpreter_precise;
const spu_decoder<spu_interpreter_fast> s_spu_interpreter_fast;

std::atomic<u64> g_num_spu_threads = { 0ull };

template <>
void fmt_class_string<spu_decoder_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_decoder_type type)
	{
		switch (type)
		{
		case spu_decoder_type::precise: return "Interpreter (precise)";
		case spu_decoder_type::fast: return "Interpreter (fast)";
		case spu_decoder_type::asmjit: return "Recompiler (ASMJIT)";
		case spu_decoder_type::llvm: return "Recompiler (LLVM)";
		}

		return unknown;
	});
}

namespace spu
{
	namespace scheduler
	{
		std::array<std::atomic<u8>, 65536> atomic_instruction_table = {};
		constexpr u32 native_jiffy_duration_us = 2000000;

		void acquire_pc_address(u32 pc, u32 timeout_ms = 3)
		{
			const u8 max_concurrent_instructions = (u8)g_cfg.core.preferred_spu_threads;
			const u32 pc_offset = pc >> 2;

			if (timeout_ms > 0)
			{
				while (timeout_ms--)
				{
					if (atomic_instruction_table[pc_offset].load(std::memory_order_consume) >= max_concurrent_instructions)
						std::this_thread::sleep_for(1ms);
				}
			}
			else
			{
				std::this_thread::yield();
			}

			atomic_instruction_table[pc_offset]++;
		}

		void release_pc_address(u32 pc)
		{
			const u32 pc_offset = pc >> 2;

			atomic_instruction_table[pc_offset]--;
		}

		struct concurrent_execution_watchdog
		{
			u32 pc = 0;
			bool active = false;

			concurrent_execution_watchdog(SPUThread& spu)
				:pc(spu.pc)
			{
				if (g_cfg.core.preferred_spu_threads > 0)
				{
					acquire_pc_address(pc, (u32)g_cfg.core.spu_delay_penalty);
					active = true;
				}
			}

			~concurrent_execution_watchdog()
			{
				if (active)
					release_pc_address(pc);
			}
		};
	}
}

void spu_int_ctrl_t::set(u64 ints)
{
	// leave only enabled interrupts
	ints &= mask;

	// notify if at least 1 bit was set
	if (ints && ~stat.fetch_or(ints) & ints && tag)
	{
		reader_lock rlock(id_manager::g_mutex);

		if (tag)
		{
			if (auto handler = tag->handler.lock())
			{
				handler->exec();
			}
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

void SPUThread::on_spawn()
{
	if (g_cfg.core.bind_spu_cores)
	{
		//Get next secondary core number
		auto core_count = std::thread::hardware_concurrency();
		if (core_count > 0 && core_count <= 16)
		{
			auto half_count = core_count / 2;
			auto assigned_secondary_core = ((g_num_spu_threads % half_count) * 2) + 1;

			thread_ctrl::set_ideal_processor_core((s32)assigned_secondary_core);
		}
	}

	if (g_cfg.core.lower_spu_priority)
	{
		thread_ctrl::set_native_priority(-1);
	}

	g_num_spu_threads++;
}

void SPUThread::on_init(const std::shared_ptr<void>& _this)
{
	if (!offset)
	{
		const_cast<u32&>(offset) = verify("SPU LS" HERE, vm::alloc(0x40000, vm::main));

		cpu_thread::on_init(_this);
	}
}

std::string SPUThread::get_name() const
{
	return fmt::format("%sSPU[0x%x] Thread (%s)", offset >= RAW_SPU_BASE_ADDR ? "Raw" : "", id, m_name);
}

std::string SPUThread::dump() const
{
	std::string&& ret = cpu_thread::dump();
	ret += fmt::format("\n" "Tag mask: 0x%08x\n" "MFC entries: %u\n", +ch_tag_mask, mfc_queue.size());
	ret += "Registers:\n=========\n";

	for (uint i = 0; i<128; ++i) ret += fmt::format("GPR[%d] = %s\n", i, gpr[i]);

	return ret;
}

void SPUThread::cpu_init()
{
	gpr = {};
	fpscr.Reset();

	ch_mfc_cmd = {};

	srr0 = 0;
	ch_tag_upd = 0;
	ch_tag_mask = 0;
	ch_tag_stat.data.store({});
	ch_stall_mask = 0;
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
	raddr = 0;

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
	
	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
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
		g_cfg.core.spu_decoder == spu_decoder_type::precise ? &s_spu_interpreter_precise.get_table() :
		g_cfg.core.spu_decoder == spu_decoder_type::fast ? &s_spu_interpreter_fast.get_table() :
		(fmt::throw_exception<std::logic_error>("Invalid SPU decoder"), nullptr));

	// LS base address
	const auto base = vm::ps3::_ptr<const u32>(offset);

	while (true)
	{
		if (!test(state))
		{
			// Read opcode
			const u32 op = base[pc / 4];

			// Call interpreter function
			table[spu_decode(op)](*this, { op });

			// Next instruction
			pc += 4;
			continue;
		}

		if (check_state()) return;
	}
}

SPUThread::~SPUThread()
{
	// Deallocate Local Storage
	vm::dealloc_verbose_nothrow(offset);
}

SPUThread::SPUThread(const std::string& name)
	: cpu_thread(idm::last_id())
	, m_name(name)
	, index(0)
	, offset(0)
	, group(nullptr)
{
}

SPUThread::SPUThread(const std::string& name, u32 index, lv2_spu_group* group)
	: cpu_thread(idm::last_id())
	, m_name(name)
	, index(index)
	, offset(0)
	, group(group)
{
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

void SPUThread::do_dma_transfer(const spu_mfc_cmd& args, bool from_mfc)
{
	const bool is_get = (args.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK)) == MFC_GET_CMD;

	u32 eal = args.eal;
	u32 lsa = args.lsa & 0x3ffff;

	if (eal >= SYS_SPU_THREAD_BASE_LOW && offset < RAW_SPU_BASE_ADDR) // SPU Thread Group MMIO (LS and SNR)
	{
		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		if (group && index < group->num && group->threads[index])
		{
			auto& spu = static_cast<SPUThread&>(*group->threads[index]);

			if (offset + args.size - 1 < 0x40000) // LS access
			{
				eal = spu.offset + offset; // redirect access
			}
			else if (!is_get && args.size == 4 && (offset == SYS_SPU_THREAD_SNR1 || offset == SYS_SPU_THREAD_SNR2))
			{
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, _ref<u32>(lsa));
				return;
			}
			else
			{
				fmt::throw_exception("Invalid MMIO offset (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}
		}
		else
		{
			fmt::throw_exception("Invalid thread type (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
		}
	}

	if (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK)) _mm_mfence();

	void* dst = vm::base(eal);
	void* src = vm::base(offset + lsa);

	if (is_get)
	{
		std::swap(dst, src);
	}

	switch (u32 size = args.size)
	{
	case 1:
	{
		*static_cast<u8*>(dst) = *static_cast<const u8*>(src);
		break;
	}
	case 2:
	{
		*static_cast<u16*>(dst) = *static_cast<const u16*>(src);
		break;
	}
	case 4:
	{
		//if (is_get && !from_mfc)
		{
			*static_cast<u32*>(dst) = *static_cast<const u32*>(src);
			break;
		}

		//_mm_stream_si32(static_cast<s32*>(dst), *static_cast<const s32*>(src));
		break;
	}
	case 8:
	{
		//if (is_get && !from_mfc)
		{
			*static_cast<u64*>(dst) = *static_cast<const u64*>(src);
			break;
		}

		//_mm_stream_si64(static_cast<s64*>(dst), *static_cast<const s64*>(src));
		break;
	}
	default:
	{
		auto vdst = static_cast<__m128i*>(dst);
		auto vsrc = static_cast<const __m128i*>(src);
		auto vcnt = size / sizeof(__m128i);

		//if (is_get && !from_mfc)
		{
			while (vcnt >= 8)
			{
				const __m128i data[]
				{
					_mm_load_si128(vsrc + 0),
					_mm_load_si128(vsrc + 1),
					_mm_load_si128(vsrc + 2),
					_mm_load_si128(vsrc + 3),
					_mm_load_si128(vsrc + 4),
					_mm_load_si128(vsrc + 5),
					_mm_load_si128(vsrc + 6),
					_mm_load_si128(vsrc + 7),
				};

				_mm_store_si128(vdst + 0, data[0]);
				_mm_store_si128(vdst + 1, data[1]);
				_mm_store_si128(vdst + 2, data[2]);
				_mm_store_si128(vdst + 3, data[3]);
				_mm_store_si128(vdst + 4, data[4]);
				_mm_store_si128(vdst + 5, data[5]);
				_mm_store_si128(vdst + 6, data[6]);
				_mm_store_si128(vdst + 7, data[7]);

				vcnt -= 8;
				vsrc += 8;
				vdst += 8;
			}

			while (vcnt--)
			{
				_mm_store_si128(vdst++, _mm_load_si128(vsrc++));
			}

			break;
		}

		// Disabled
		while (vcnt >= 8)
		{
			const __m128i data[]
			{
				_mm_load_si128(vsrc + 0),
				_mm_load_si128(vsrc + 1),
				_mm_load_si128(vsrc + 2),
				_mm_load_si128(vsrc + 3),
				_mm_load_si128(vsrc + 4),
				_mm_load_si128(vsrc + 5),
				_mm_load_si128(vsrc + 6),
				_mm_load_si128(vsrc + 7),
			};

			_mm_stream_si128(vdst + 0, data[0]);
			_mm_stream_si128(vdst + 1, data[1]);
			_mm_stream_si128(vdst + 2, data[2]);
			_mm_stream_si128(vdst + 3, data[3]);
			_mm_stream_si128(vdst + 4, data[4]);
			_mm_stream_si128(vdst + 5, data[5]);
			_mm_stream_si128(vdst + 6, data[6]);
			_mm_stream_si128(vdst + 7, data[7]);

			vcnt -= 8;
			vsrc += 8;
			vdst += 8;
		}

		while (vcnt--)
		{
			_mm_stream_si128(vdst++, _mm_load_si128(vsrc++));
		}
	}
	}

	if (is_get && from_mfc)
	{
		//_mm_sfence();
	}
}

void SPUThread::process_mfc_cmd()
{
	spu::scheduler::concurrent_execution_watchdog watchdog(*this);
	LOG_TRACE(SPU, "DMAC: cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);

	const auto mfc = fxm::check_unlocked<mfc_thread>();
	const u32 max_imm_dma_size = g_cfg.core.max_spu_immediate_write_size;

	// Check queue size
	auto check_queue_size = [&]()
	{
		while (mfc_queue.size() >= 16)
		{
			if (test(state, cpu_flag::stop + cpu_flag::dbg_global_stop))
			{
				return;
			}

			// TODO: investigate lost notifications
			std::this_thread::yield();
			_mm_lfence();
		}
	};

	switch (ch_mfc_cmd.cmd)
	{
	case MFC_GETLLAR_CMD:
	{
		auto& data = vm::ps3::_ref<decltype(rdata)>(ch_mfc_cmd.eal);

		const u32 _addr = ch_mfc_cmd.eal;
		const u64 _time = vm::reservation_acquire(raddr, 128);

		if (raddr && raddr != ch_mfc_cmd.eal)
		{
			ch_event_stat |= SPU_EVENT_LR;
		}

		const bool is_polling = false;// raddr == _addr && rtime == _time; // TODO

		_mm_lfence();
		raddr = _addr;
		rtime = _time;

		if (is_polling)
		{
			vm::waiter waiter;
			waiter.owner = this;
			waiter.addr  = raddr;
			waiter.size  = 128;
			waiter.stamp = rtime;
			waiter.data  = rdata.data();
			waiter.init();

			while (vm::reservation_acquire(raddr, 128) == waiter.stamp && rdata == data)
			{
				if (test(state, cpu_flag::stop))
				{
					break;
				}

				thread_ctrl::wait_for(100);
			}
		}
		else if (s_use_rtm && utils::transaction_enter())
		{
			if (!vm::reader_lock{vm::try_to_lock})
			{
				_xabort(0);
			}

			rtime = vm::reservation_acquire(raddr, 128);
			rdata = data;
			_xend();

			_ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ffff) = rdata;
			return ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
		}
		else
		{
			rdata = data;
			_mm_lfence();
		}

		// Hack: ensure no other atomic updates have happened during reading the data
		if (is_polling || UNLIKELY(vm::reservation_acquire(raddr, 128) != rtime))
		{
			// TODO: vm::check_addr
			vm::reader_lock lock;
			rtime = vm::reservation_acquire(raddr, 128);
			rdata = data;
		}

		// Copy to LS
		_ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ffff) = rdata;

		return ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
	}

	case MFC_PUTLLC_CMD:
	{
		// Store conditionally
		auto& data = vm::ps3::_ref<decltype(rdata)>(ch_mfc_cmd.eal);
		const auto to_write = _ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ffff);

		bool result = false;

		if (raddr == ch_mfc_cmd.eal && rtime == vm::reservation_acquire(raddr, 128) && rdata == data)
		{
			// TODO: vm::check_addr
			if (s_use_rtm && utils::transaction_enter())
			{
				if (!vm::reader_lock{vm::try_to_lock})
				{
					_xabort(0);
				}

				if (rtime == vm::reservation_acquire(raddr, 128) && rdata == data)
				{
					data = to_write;
					result = true;

					vm::reservation_update(raddr, 128);
					vm::notify(raddr, 128);
				}

				_xend();
			}
			else
			{
				vm::writer_lock lock;

				if (rtime == vm::reservation_acquire(raddr, 128) && rdata == data)
				{
					data = to_write;
					result = true;

					vm::reservation_update(raddr, 128);
					vm::notify(raddr, 128);
				}
			}
		}

		if (result)
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_FAILURE);
		}

		if (raddr && !result)
		{
			ch_event_stat |= SPU_EVENT_LR;
		}

		raddr = 0;
		return;
	}
	case MFC_PUTLLUC_CMD:
	{
		if (raddr && ch_mfc_cmd.eal == raddr)
		{
			ch_event_stat |= SPU_EVENT_LR;
			raddr = 0;
		}

		auto& data = vm::ps3::_ref<decltype(rdata)>(ch_mfc_cmd.eal);
		const auto to_write = _ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ffff);

		vm::reservation_acquire(ch_mfc_cmd.eal, 128);

		// Store unconditionally
		// TODO: vm::check_addr

		if (s_use_rtm && utils::transaction_enter())
		{
			if (!vm::reader_lock{vm::try_to_lock})
			{
				_xabort(0);
			}

			data = to_write;
			vm::reservation_update(ch_mfc_cmd.eal, 128);
			vm::notify(ch_mfc_cmd.eal, 128);
			_xend();

			ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
			return;
		}

		vm::writer_lock lock(0);
		data = to_write;
		vm::reservation_update(ch_mfc_cmd.eal, 128);
		vm::notify(ch_mfc_cmd.eal, 128);

		ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		return;
	}
	case MFC_PUTQLLUC_CMD:
	{
		ch_mfc_cmd.size = 128;
		break;
	}
	case MFC_SNDSIG_CMD:
	case MFC_SNDSIGB_CMD:
	case MFC_SNDSIGF_CMD:
	{
		ch_mfc_cmd.size = 4;
		// Fallthrough
	}
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
		// Try to process small transfers immediately
		if (ch_mfc_cmd.size <= max_imm_dma_size && mfc_queue.size() == 0)
		{
			vm::reader_lock lock(vm::try_to_lock);

			if (!lock)
			{
				break;
			}

			if (!vm::check_addr(ch_mfc_cmd.eal, ch_mfc_cmd.size, vm::page_readable | (ch_mfc_cmd.cmd & MFC_PUT_CMD ? vm::page_writable : 0)))
			{
				// TODO
				break;
			}

			do_dma_transfer(ch_mfc_cmd, false);
			return;
		}
		
		break;
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
		if (ch_mfc_cmd.size <= max_imm_dma_size && mfc_queue.size() == 0 && (ch_stall_mask & (1u << ch_mfc_cmd.tag)) == 0)
		{
			vm::reader_lock lock(vm::try_to_lock);

			if (!lock)
			{
				break;
			}

			struct list_element
			{
				be_t<u16> sb;
				be_t<u16> ts;
				be_t<u32> ea;
			};
			
			u32 total_size = 0;

			while (ch_mfc_cmd.size && total_size <= max_imm_dma_size)
			{
				ch_mfc_cmd.lsa &= 0x3fff0;

				const list_element item = _ref<list_element>(ch_mfc_cmd.eal & 0x3fff8);

				if (item.sb & 0x8000)
				{
					break;
				}

				const u32 size = item.ts;
				const u32 addr = item.ea;

				if (size)
				{
					if (total_size + size > max_imm_dma_size)
					{
						break;
					}

					if (!vm::check_addr(addr, size, vm::page_readable | (ch_mfc_cmd.cmd & MFC_PUT_CMD ? vm::page_writable : 0)))
					{
						// TODO
						break;
					}

					spu_mfc_cmd transfer;
					transfer.eal = addr;
					transfer.eah = 0;
					transfer.lsa = ch_mfc_cmd.lsa | (addr & 0xf);
					transfer.tag = ch_mfc_cmd.tag;
					transfer.cmd = MFC(ch_mfc_cmd.cmd & ~MFC_LIST_MASK);
					transfer.size = size;

					do_dma_transfer(transfer);
					const u32 add_size = std::max<u32>(size, 16);
					ch_mfc_cmd.lsa += add_size;
					total_size += add_size;
				}

				ch_mfc_cmd.eal += 8;
				ch_mfc_cmd.size -= 8;
			}

			if (ch_mfc_cmd.size == 0)
			{
				return;
			}
		}

		break;
	}
	case MFC_BARRIER_CMD:
	case MFC_EIEIO_CMD:
	case MFC_SYNC_CMD:
	{
		ch_mfc_cmd.size = 0;

		if (mfc_queue.size() == 0)
		{
			_mm_mfence();
			return;
		}

		break;
	}
	default:
	{
		fmt::throw_exception("Unknown command (cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);
	}
	}

	// Enqueue
	check_queue_size();
	verify(HERE), mfc_queue.try_push(ch_mfc_cmd);

	//if (test(mfc->state, cpu_flag::is_waiting))
	{
		mfc->notify();
	}
}

u32 SPUThread::get_events(bool waiting)
{
	// Check reservation status and set SPU_EVENT_LR if lost
	if (raddr && (vm::reservation_acquire(raddr, sizeof(rdata)) != rtime || rdata != vm::ps3::_ref<decltype(rdata)>(raddr)))
	{
		ch_event_stat |= SPU_EVENT_LR;
		raddr = 0;
	}

	// SPU Decrementer Event
	if (!ch_dec_value || (ch_dec_value - (get_timebased_time() - ch_dec_start_timestamp)) >> 31)
	{
		if ((ch_event_stat & SPU_EVENT_TM) == 0)
		{
			ch_event_stat |= SPU_EVENT_TM;
		}
	}

	// Simple polling or polling with atomically set/removed SPU_EVENT_WAITING flag
	return !waiting ? ch_event_stat & ch_event_mask : ch_event_stat.atomic_op([&](u32& stat) -> u32
	{
		if (u32 res = stat & ch_event_mask)
		{
			stat &= ~SPU_EVENT_WAITING;
			return res;
		}

		stat |= SPU_EVENT_WAITING;
		return 0;
	});
}

void SPUThread::set_events(u32 mask)
{
	if (u32 unimpl = mask & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("Unimplemented events (0x%x)" HERE, unimpl);
	}

	// Set new events, get old event mask
	const u32 old_stat = ch_event_stat.fetch_or(mask);

	// Notify if some events were set
	if (~old_stat & mask && old_stat & SPU_EVENT_WAITING && ch_event_stat & SPU_EVENT_WAITING)
	{
		notify();
	}
}

void SPUThread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// detect enabling interrupts with events masked
		if (u32 mask = ch_event_mask & ~SPU_EVENT_INTR_IMPLEMENTED)
		{
			fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x)" HERE, mask);
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
	case SPU_WrOutMbox:       return ch_out_mbox.get_count() ^ 1;
	case SPU_WrOutIntrMbox:   return ch_out_intr_mbox.get_count() ^ 1;
	case SPU_RdInMbox:        return ch_in_mbox.get_count();
	case MFC_RdTagStat:       return ch_tag_stat.get_count();
	case MFC_RdListStallStat: return ch_stall_stat.get_count();
	case MFC_WrTagUpdate:     return ch_tag_upd == 0;
	case SPU_RdSigNotify1:    return ch_snr1.get_count();
	case SPU_RdSigNotify2:    return ch_snr2.get_count();
	case MFC_RdAtomicStat:    return ch_atomic_stat.get_count();
	case SPU_RdEventStat:     return get_events() != 0;
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool SPUThread::get_ch_value(u32 ch, u32& out)
{
	LOG_TRACE(SPU, "get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

	auto read_channel = [&](spu_channel_t& channel)
	{
		if (channel.try_pop(out))
			return true;

		for (int i = 0; i < 10 && channel.get_count() == 0; i++)
		{
			busy_wait();
		}

		u32 ctr = 0;
		while (!channel.try_pop(out))
		{
			if (test(state, cpu_flag::stop))
			{
				return false;
			}

			if (ctr > 10000)
			{
				ctr = 0;
				std::this_thread::yield();
			}
			else
			{
				ctr++;
				thread_ctrl::wait();
			}
		}

		return true;
	};

	switch (ch)
	{
	case SPU_RdSRR0:
	{
		out = srr0;
		break;
	}
	case SPU_RdInMbox:
	{
		while (true)
		{
			for (int i = 0; i < 10 && ch_in_mbox.get_count() == 0; i++)
			{
				busy_wait();
			}

			if (const uint old_count = ch_in_mbox.try_pop(out))
			{
				if (old_count == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				return true;
			}

			if (test(state & cpu_flag::stop))
			{
				return false;
			}

			thread_ctrl::wait();
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

		//Polling: We might as well hint to the scheduler to slot in another thread since this one is counting down
		if (g_cfg.core.spu_loop_detection && out > spu::scheduler::native_jiffy_duration_us)
			std::this_thread::yield();

		return true;
	}

	case SPU_RdEventMask:
	{
		out = ch_event_mask;
		return true;
	}

	case SPU_RdEventStat:
	{
		u32 res = get_events();

		if (res)
		{
			out = res;
			return true;
		}

		vm::waiter waiter;

		if (ch_event_mask & SPU_EVENT_LR)
		{
			waiter.owner = this;
			waiter.addr = raddr;
			waiter.size = 128;
			waiter.stamp = rtime;
			waiter.data = rdata.data();
			waiter.init();
		}

		while (!(res = get_events(true)))
		{
			if (test(state & cpu_flag::stop))
			{
				return false;
			}

			thread_ctrl::wait_for(100);
		}
		
		out = res;
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

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool SPUThread::set_ch_value(u32 ch, u32 value)
{
	LOG_TRACE(SPU, "set_ch_value(ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);

	switch (ch)
	{
	case SPU_WrSRR0:
	{
		srr0 = value;
		break;
	}
		
	case SPU_WrOutIntrMbox:
	{
		if (offset >= RAW_SPU_BASE_ADDR)
		{
			while (!ch_out_intr_mbox.try_push(value))
			{
				if (test(state & cpu_flag::stop))
				{
					return false;
				}

				thread_ctrl::wait();
			}

			int_ctrl[2].set(SPU_INT2_STAT_MAILBOX_INT);
			return true;
		}
		
		const u32 code = value >> 24;
		{
			if (code < 64)
			{
				/* ===== sys_spu_thread_send_event (used by spu_printf) ===== */

				u32 spup = code & 63;
				u32 data;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty" HERE, value, spup);
				}

				if (u32 count = ch_in_mbox.get_count())
				{
					fmt::throw_exception("sys_spu_thread_send_event(value=0x%x, spup=%d): In_MBox is not empty (count=%d)" HERE, value, spup, count);
				}

				LOG_TRACE(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = (semaphore_lock{group->mutex}, this->spup[spup].lock());

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					ch_in_mbox.set_values(1, CELL_ENOTCONN);
					return true;
				}

				ch_in_mbox.set_values(1, CELL_OK);

				if (!queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, id, ((u64)spup << 32) | (value & 0x00ffffff), data))
				{
					ch_in_mbox.set_values(1, CELL_EBUSY);
				}

				return true;
			}
			else if (code < 128)
			{
				/* ===== sys_spu_thread_throw_event ===== */

				u32 spup = code & 63;
				u32 data;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty" HERE, value, spup);
				}

				LOG_TRACE(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = (semaphore_lock{group->mutex}, this->spup[spup].lock());

				if (!queue)
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x): event queue not connected", spup, (value & 0x00ffffff), data);
					return true;
				}

				// TODO: check passing spup value
				if (!queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, id, ((u64)spup << 32) | (value & 0x00ffffff), data))
				{
					LOG_WARNING(SPU, "sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x) failed (queue is full)", spup, (value & 0x00ffffff), data);
				}

				return true;
			}
			else if (code == 128)
			{
				/* ===== sys_event_flag_set_bit ===== */

				u32 flag = value & 0xffffff;
				u32 data;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty" HERE, value, flag);
				}

				if (u32 count = ch_in_mbox.get_count())
				{
					fmt::throw_exception("sys_event_flag_set_bit(value=0x%x (flag=%d)): In_MBox is not empty (%d)" HERE, value, flag, count);
				}

				LOG_TRACE(SPU, "sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);

				ch_in_mbox.set_values(1, CELL_OK);

				// Use the syscall to set flag
				if (s32 res = sys_event_flag_set(data, 1ull << flag))
				{
					ch_in_mbox.set_values(1, res);
				}

				return true;
			}
			else if (code == 192)
			{
				/* ===== sys_event_flag_set_bit_impatient ===== */

				u32 flag = value & 0xffffff;
				u32 data;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty" HERE, value, flag);
				}

				LOG_TRACE(SPU, "sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);

				// Use the syscall to set flag
				sys_event_flag_set(data, 1ull << flag);
				return true;
			}
			else
			{
				if (ch_out_mbox.get_count())
				{
					fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x); Out_MBox = 0x%x" HERE, value, ch_out_mbox.get_value());
				}
				else
				{
					fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x)" HERE, value);
				}
			}
		}
	}

	case SPU_WrOutMbox:
	{
		while (!ch_out_mbox.try_push(value))
		{
			if (test(state & cpu_flag::stop))
			{
				return false;
			}

			thread_ctrl::wait();
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
		if (value > 2)
		{
			break;
		}

		ch_tag_stat.set_value(0, false);
		ch_tag_upd = value;

		if (ch_tag_mask == 0)
		{
			// TODO
			ch_tag_stat.set_value(0);
		}
		else if (mfc_queue.size() == 0 && (!value || ch_tag_upd.exchange(0)))
		{
			ch_tag_stat.set_value(ch_tag_mask);
		}
		else if (!value)
		{
			u32 completed = ch_tag_mask;

			for (u32 i = 0; completed && i < 16; i++)
			{
				const auto& _cmd = mfc_queue.get_push(i);

				if (_cmd.size)
				{
					completed &= ~(1u << _cmd.tag);
				}
			}

			ch_tag_stat.set_value(completed);
		}
		else
		{
			auto mfc = fxm::check_unlocked<mfc_thread>();
			
			//if (test(mfc->state, cpu_flag::is_waiting))
			{
				mfc->notify();
			}
		}

		return true;
	}

	case MFC_LSA:
	{
		ch_mfc_cmd.lsa = value;
		return true;
	}

	case MFC_EAH:
	{
		ch_mfc_cmd.eah = value;
		return true;
	}

	case MFC_EAL:
	{
		ch_mfc_cmd.eal = value;
		return true;
	}

	case MFC_Size:
	{
		ch_mfc_cmd.size = value & 0xffff;
		return true;
	}

	case MFC_TagID:
	{
		ch_mfc_cmd.tag = value & 0xff;
		return true;
	}

	case MFC_Cmd:
	{
		ch_mfc_cmd.cmd = MFC(value & 0xff);
		process_mfc_cmd();
		ch_mfc_cmd = {}; // clear non-persistent data
		return true;
	}

	case MFC_WrListStallAck:
	{
		// Reset stall status for specified tag
		if (atomic_storage<u32>::btr(ch_stall_mask.raw(), value))
		{
			auto mfc = fxm::check_unlocked<mfc_thread>();
			
			//if (test(mfc->state, cpu_flag::is_waiting))
			{
				mfc->notify();
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
		if (value & ~SPU_EVENT_INTR_IMPLEMENTED && ch_event_stat & SPU_EVENT_INTR_ENABLED)
		{
			fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x)" HERE, value);
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

	case 69:
	{
		return true;
	}
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s], value=0x%x)" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???", value);
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
		state += cpu_flag::stop;
		return true; // ???
	}

	switch (code)
	{
	case 0x000:
	{
		LOG_WARNING(SPU, "STOP 0x0");

		// HACK: find an ILA instruction
		for (u32 addr = pc; addr < 0x40000; addr += 4)
		{
			const u32 instr = _ref<u32>(addr);

			if (instr >> 25 == 0x21)
			{
				pc = addr;
				return false;
			}

			if (instr > 0x1fffff)
			{
				break;
			}
		}

		// HACK: wait for executable code
		while (!_ref<u32>(pc))
		{
			if (test(state & cpu_flag::stop))
			{
				return false;
			}

			thread_ctrl::wait_for(1000);
		}

		return false;
	}

	case 0x001:
	{
		thread_ctrl::wait_for(1000); // hack
		return true;
	}

	case 0x002:
	{
		state += cpu_flag::ret;
		return true;
	}

	case 0x110:
	{
		/* ===== sys_spu_thread_receive_event ===== */

		u32 spuq;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_receive_event(): Out_MBox is empty" HERE);
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			LOG_ERROR(SPU, "sys_spu_thread_receive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		LOG_TRACE(SPU, "sys_spu_thread_receive_event(spuq=0x%x)", spuq);

		if (group->type & SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT) // this check may be inaccurate
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		std::shared_ptr<lv2_event_queue> queue;

		while (true)
		{
			queue.reset();

			// Check group status, wait if necessary
			while (group->run_state >= SPU_THREAD_GROUP_STATUS_WAITING && group->run_state <= SPU_THREAD_GROUP_STATUS_SUSPENDED)
			{
				if (test(state & cpu_flag::stop))
				{
					return false;
				}

				thread_ctrl::wait();
			}

			reader_lock rlock(id_manager::g_mutex);

			semaphore_lock lock(group->mutex);

			if (group->run_state >= SPU_THREAD_GROUP_STATUS_WAITING && group->run_state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// Try again
				continue;
			}

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

			semaphore_lock qlock(queue->mutex);

			if (queue->events.empty())
			{
				queue->sq.emplace_back(this);
				group->run_state = SPU_THREAD_GROUP_STATUS_WAITING;

				for (auto& thread : group->threads)
				{
					if (thread)
					{
						thread->state += cpu_flag::suspend;
					}
				}

				// Wait
				break;
			}
			else
			{
				// Return the event immediately
				const auto event = queue->events.front();
				const auto data1 = static_cast<u32>(std::get<1>(event));
				const auto data2 = static_cast<u32>(std::get<2>(event));
				const auto data3 = static_cast<u32>(std::get<3>(event));
				ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
				queue->events.pop_front();
				return true;
			}
		}

		while (true)
		{
			if (test(state & cpu_flag::stop))
			{
				return false;
			}

			if (!state.test_and_reset(cpu_flag::signal))
			{
				thread_ctrl::wait();
			}
			else
			{
				break;
			}
		}

		semaphore_lock lock(group->mutex);
		
		if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING)
		{
			group->run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
		}
		else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			group->run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		}

		for (auto& thread : group->threads)
		{
			if (thread)
			{
				thread->state -= cpu_flag::suspend;

				if (thread.get() != this)
				{
					thread->notify();
				}
			}
		}

		return true;
	}

	case 0x101:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		u32 value;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_group_exit(): Out_MBox is empty" HERE);
		}

		LOG_TRACE(SPU, "sys_spu_thread_group_exit(status=0x%x)", value);

		semaphore_lock lock(group->mutex);

		for (auto& thread : group->threads)
		{
			if (thread && thread.get() != this)
			{
				thread->state += cpu_flag::stop;
				thread->notify();
			}
		}

		group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;
		group->exit_status = value;
		group->join_state |= SPU_TGJSF_GROUP_EXIT;
		group->cv.notify_one();

		state += cpu_flag::stop;
		return true;
	}

	case 0x102:
	{
		/* ===== sys_spu_thread_exit ===== */

		if (!ch_out_mbox.get_count())
		{
			fmt::throw_exception("sys_spu_thread_exit(): Out_MBox is empty" HERE);
		}

		LOG_TRACE(SPU, "sys_spu_thread_exit(status=0x%x)", ch_out_mbox.get_value());

		semaphore_lock lock(group->mutex);

		status |= SPU_STATUS_STOPPED_BY_STOP;
		group->cv.notify_one();

		state += cpu_flag::stop;
		return true;
	}
	}

	if (!ch_out_mbox.get_count())
	{
		fmt::throw_exception("Unknown STOP code: 0x%x (Out_MBox is empty)" HERE, code);
	}
	else
	{
		fmt::throw_exception("Unknown STOP code: 0x%x (Out_MBox=0x%x)" HERE, code, ch_out_mbox.get_value());
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

		throw cpu_flag::stop;
	}

	status |= SPU_STATUS_STOPPED_BY_HALT;
	fmt::throw_exception("Halt" HERE);
}

void SPUThread::fast_call(u32 ls_addr)
{
	// LS:0x0: this is originally the entry point of the interrupt handler, but interrupts are not implemented
	_ref<u32>(0) = 0x00000002; // STOP 2

	auto old_pc = pc;
	auto old_lr = gpr[0]._u32[3];
	auto old_stack = gpr[1]._u32[3]; // only saved and restored (may be wrong)

	pc = ls_addr;
	gpr[0]._u32[3] = 0x0;

	try
	{
		cpu_task();
	}
	catch (cpu_flag _s)
	{
		state += _s;
		if (_s != cpu_flag::ret) throw;
	}

	state -= cpu_flag::ret;

	pc = old_pc;
	gpr[0]._u32[3] = old_lr;
	gpr[1]._u32[3] = old_stack;
}
