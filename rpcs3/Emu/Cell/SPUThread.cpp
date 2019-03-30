#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/sysinfo.h"
#include "Emu/Memory/vm.h"
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

#ifdef _MSC_VER
bool operator ==(const u128& lhs, const u128& rhs)
{
	return lhs.lo == rhs.lo && lhs.hi == rhs.hi;
}
#endif

static FORCE_INLINE void mov_rdata(u128* const dst, const u128* const src)
{
	{
		const u128 data0 = src[0];
		const u128 data1 = src[1];
		const u128 data2 = src[2];
		dst[0] = data0;
		dst[1] = data1;
		dst[2] = data2;
	}

	{
		const u128 data0 = src[3];
		const u128 data1 = src[4];
		const u128 data2 = src[5];
		dst[3] = data0;
		dst[4] = data1;
		dst[5] = data2;
	}

	{
		const u128 data0 = src[6];
		const u128 data1 = src[7];
		dst[6] = data0;
		dst[7] = data1;
	}
};

extern u64 get_timebased_time();
extern u64 get_system_time();

extern const spu_decoder<spu_interpreter_precise> g_spu_interpreter_precise;

extern const spu_decoder<spu_interpreter_fast> g_spu_interpreter_fast;

extern thread_local u64 g_tls_fault_spu;

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

template <>
void fmt_class_string<spu_block_size_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_block_size_type type)
	{
		switch (type)
		{
		case spu_block_size_type::safe: return "Safe";
		case spu_block_size_type::mega: return "Mega";
		case spu_block_size_type::giga: return "Giga";
		}

		return unknown;
	});
}

namespace spu
{
	namespace scheduler
	{
		std::array<std::atomic<u8>, 65536> atomic_instruction_table = {};
		constexpr u32 native_jiffy_duration_us = 1500; //About 1ms resolution with a half offset

		void acquire_pc_address(u32 pc, u32 timeout_ms = 3)
		{
			const u8 max_concurrent_instructions = (u8)g_cfg.core.preferred_spu_threads;
			const u32 pc_offset = pc >> 2;

			if (atomic_instruction_table[pc_offset].load(std::memory_order_consume) >= max_concurrent_instructions)
			{
				if (timeout_ms > 0)
				{
					const u64 timeout = timeout_ms * 1000u; //convert to microseconds
					const u64 start = get_system_time();
					auto remaining = timeout;

					while (atomic_instruction_table[pc_offset].load(std::memory_order_consume) >= max_concurrent_instructions)
					{
						if (remaining >= native_jiffy_duration_us)
							std::this_thread::sleep_for(1ms);
						else
							std::this_thread::yield();

						const auto now = get_system_time();
						const auto elapsed = now - start;

						if (elapsed > timeout) break;
						remaining = timeout - elapsed;
					}
				}
				else
				{
					//Slight pause if function is overburdened
					const auto count = atomic_instruction_table[pc_offset].load(std::memory_order_consume) * 100ull;
					busy_wait(count);
				}
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

			concurrent_execution_watchdog(spu_thread& spu)
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

const auto spu_putllc_tx = build_function_asm<u32(*)(u32 raddr, u64 rtime, const void* _old, const void* _new)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();
	Label retry = c.newLabel();

	// Prepare registers
	c.mov(x86::rax, imm_ptr(&vm::g_reservations));
	c.mov(x86::r10, x86::qword_ptr(x86::rax));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::rax));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.shr(args[0], 4);
	c.lea(x86::r10, x86::qword_ptr(x86::r10, args[0]));
	c.mov(args[0].r32(), 3);

	// Prepare data (Windows has only 6 volatile vector registers)
	c.vmovups(x86::ymm0, x86::yword_ptr(args[2], 0));
	c.vmovups(x86::ymm1, x86::yword_ptr(args[2], 32));
	c.vmovups(x86::ymm2, x86::yword_ptr(args[2], 64));
	c.vmovups(x86::ymm3, x86::yword_ptr(args[2], 96));
#ifdef _WIN32
	c.vmovups(x86::ymm4, x86::yword_ptr(args[3], 0));
	c.vmovups(x86::ymm5, x86::yword_ptr(args[3], 96));
#else
	c.vmovups(x86::ymm6, x86::yword_ptr(args[3], 0));
	c.vmovups(x86::ymm7, x86::yword_ptr(args[3], 32));
	c.vmovups(x86::ymm8, x86::yword_ptr(args[3], 64));
	c.vmovups(x86::ymm9, x86::yword_ptr(args[3], 96));
#endif

	// Begin transaction
	Label begin = build_transaction_enter(c, fall);
	c.cmp(x86::qword_ptr(x86::r10), args[1]);
	c.jne(fail);
	c.vxorps(x86::ymm0, x86::ymm0, x86::yword_ptr(x86::r11, 0));
	c.vxorps(x86::ymm1, x86::ymm1, x86::yword_ptr(x86::r11, 32));
	c.vxorps(x86::ymm2, x86::ymm2, x86::yword_ptr(x86::r11, 64));
	c.vxorps(x86::ymm3, x86::ymm3, x86::yword_ptr(x86::r11, 96));
	c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
	c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
	c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
	c.vptest(x86::ymm0, x86::ymm0);
	c.jnz(fail);
#ifdef _WIN32
	c.vmovups(x86::ymm2, x86::yword_ptr(args[3], 32));
	c.vmovups(x86::ymm3, x86::yword_ptr(args[3], 64));
	c.vmovaps(x86::yword_ptr(x86::r11, 0), x86::ymm4);
	c.vmovaps(x86::yword_ptr(x86::r11, 32), x86::ymm2);
	c.vmovaps(x86::yword_ptr(x86::r11, 64), x86::ymm3);
	c.vmovaps(x86::yword_ptr(x86::r11, 96), x86::ymm5);
#else
	c.vmovaps(x86::yword_ptr(x86::r11, 0), x86::ymm6);
	c.vmovaps(x86::yword_ptr(x86::r11, 32), x86::ymm7);
	c.vmovaps(x86::yword_ptr(x86::r11, 64), x86::ymm8);
	c.vmovaps(x86::yword_ptr(x86::r11, 96), x86::ymm9);
#endif
	c.add(x86::qword_ptr(x86::r10), 2);
	c.xend();
	c.vzeroupper();
	c.mov(x86::eax, 1);
	c.ret();

	// Touch memory after transaction failure
	c.bind(fall);
	c.sar(x86::eax, 24);
	c.js(fail);
	c.sub(args[0].r32(), 1);
	c.jz(retry);
	c.lock().add(x86::qword_ptr(x86::r11), 0);
	c.lock().add(x86::qword_ptr(x86::r10), 0);
#ifdef _WIN32
	c.vmovups(x86::ymm4, x86::yword_ptr(args[3], 0));
	c.vmovups(x86::ymm5, x86::yword_ptr(args[3], 96));
#endif
	c.jmp(begin);

	c.bind(fail);
	build_transaction_abort(c, 0xff);
	c.xor_(x86::eax, x86::eax);
	c.ret();
	c.bind(retry);
	c.mov(x86::eax, 2);
	c.ret();
});

const auto spu_getll_tx = build_function_asm<u64(*)(u32 raddr, void* rdata)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();

	// Prepare registers
	c.mov(x86::rax, imm_ptr(&vm::g_reservations));
	c.mov(x86::r10, x86::qword_ptr(x86::rax));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::rax));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.shr(args[0], 4);
	c.lea(x86::r10, x86::qword_ptr(x86::r10, args[0]));
	c.mov(args[0].r32(), 2);

	// Begin transaction
	Label begin = build_transaction_enter(c, fall);
	c.mov(x86::rax, x86::qword_ptr(x86::r10));
	c.vmovaps(x86::ymm0, x86::yword_ptr(x86::r11, 0));
	c.vmovaps(x86::ymm1, x86::yword_ptr(x86::r11, 32));
	c.vmovaps(x86::ymm2, x86::yword_ptr(x86::r11, 64));
	c.vmovaps(x86::ymm3, x86::yword_ptr(x86::r11, 96));
	c.xend();
	c.vmovups(x86::yword_ptr(args[1], 0), x86::ymm0);
	c.vmovups(x86::yword_ptr(args[1], 32), x86::ymm1);
	c.vmovups(x86::yword_ptr(args[1], 64), x86::ymm2);
	c.vmovups(x86::yword_ptr(args[1], 96), x86::ymm3);
	c.vzeroupper();
	c.ret();

	// Touch memory after transaction failure
	c.bind(fall);
	c.pause();
	c.mov(x86::rax, x86::qword_ptr(x86::r11));
	c.mov(x86::rax, x86::qword_ptr(x86::r10));
	c.sub(args[0], 1);
	c.jnz(begin);
	c.mov(x86::eax, 1);
	c.ret();
});

const auto spu_putlluc_tx = build_function_asm<bool(*)(u32 raddr, const void* rdata)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();

	// Prepare registers
	c.mov(x86::rax, imm_ptr(&vm::g_reservations));
	c.mov(x86::r10, x86::qword_ptr(x86::rax));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::r11, x86::qword_ptr(x86::rax));
	c.lea(x86::r11, x86::qword_ptr(x86::r11, args[0]));
	c.shr(args[0], 4);
	c.lea(x86::r10, x86::qword_ptr(x86::r10, args[0]));
	c.mov(args[0].r32(), 2);

	// Prepare data
	c.vmovups(x86::ymm0, x86::yword_ptr(args[1], 0));
	c.vmovups(x86::ymm1, x86::yword_ptr(args[1], 32));
	c.vmovups(x86::ymm2, x86::yword_ptr(args[1], 64));
	c.vmovups(x86::ymm3, x86::yword_ptr(args[1], 96));

	// Begin transaction
	Label begin = build_transaction_enter(c, fall);
	c.vmovaps(x86::yword_ptr(x86::r11, 0), x86::ymm0);
	c.vmovaps(x86::yword_ptr(x86::r11, 32), x86::ymm1);
	c.vmovaps(x86::yword_ptr(x86::r11, 64), x86::ymm2);
	c.vmovaps(x86::yword_ptr(x86::r11, 96), x86::ymm3);
	c.add(x86::qword_ptr(x86::r10), 2);
	c.xend();
	c.vzeroupper();
	c.mov(x86::eax, 1);
	c.ret();

	// Touch memory after transaction failure
	c.bind(fall);
	c.pause();
	c.lock().add(x86::qword_ptr(x86::r11), 0);
	c.lock().add(x86::qword_ptr(x86::r10), 0);
	c.sub(args[0], 1);
	c.jnz(begin);
	c.xor_(x86::eax, x86::eax);
	c.ret();
});

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
		m_data[i + 155].vf = _mm_set1_ps(static_cast<float>(std::exp2(i)));
	}
}

spu_imm_table_t::spu_imm_table_t()
{
	for (u32 i = 0; i < std::size(sldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
		}
	}

	for (u32 i = 0; i < std::size(srdq_pshufb); i++)
	{
		const u32 im = (0u - i) & 0x1f;

		for (u32 j = 0; j < 16; j++)
		{
			srdq_pshufb[i]._u8[j] = (j + im > 15) ? 0xff : static_cast<u8>(j + im);
		}
	}

	for (u32 i = 0; i < std::size(rldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
		}
	}
}

std::string spu_thread::get_name() const
{
	return fmt::format("%sSPU[0x%x] Thread (%s)", offset >= RAW_SPU_BASE_ADDR ? "Raw" : "", id, spu_name.get());
}

std::string spu_thread::dump() const
{
	std::string ret = cpu_thread::dump();

	fmt::append(ret, "\nBlock Weight: %u (Retreats: %u)", block_counter, block_failure);
	fmt::append(ret, "\n[%s]", ch_mfc_cmd);
	fmt::append(ret, "\nTag Mask: 0x%08x", ch_tag_mask);
	fmt::append(ret, "\nMFC Stall: 0x%08x", ch_stall_mask);
	fmt::append(ret, "\nMFC Queue Size: %u", mfc_size);

	for (u32 i = 0; i < 16; i++)
	{
		if (i < mfc_size)
		{
			fmt::append(ret, "\n%s", mfc_queue[i]);
		}
		else
		{
			fmt::append(ret, "\n[-]");
		}
	}

	ret += "\nRegisters:\n=========";

	for (u32 i = 0; i < 128; i++)
	{
		fmt::append(ret, "\nGPR[%d] = %s", i, gpr[i]);
	}

	return ret;
}

void spu_thread::cpu_init()
{
	gpr = {};
	fpscr.Reset();

	ch_mfc_cmd = {};

	srr0 = 0;
	mfc_size = 0;
	mfc_barrier = 0;
	mfc_fence = 0;
	ch_tag_upd = 0;
	ch_tag_mask = 0;
	mfc_prxy_mask = 0;
	ch_tag_stat.data.release({});
	ch_stall_mask = 0;
	ch_stall_stat.data.release({});
	ch_atomic_stat.data.release({});

	ch_in_mbox.clear();

	ch_out_mbox.data.release({});
	ch_out_intr_mbox.data.release({});

	snr_config = 0;

	ch_snr1.data.release({});
	ch_snr2.data.release({});

	ch_event_mask = 0;
	ch_event_stat = 0;
	interrupts_enabled = false;
	raddr = 0;

	ch_dec_start_timestamp = get_timebased_time(); // ???
	ch_dec_value = 0;

	run_ctrl.release(0);
	status.release(0);
	npc.release(0);

	int_ctrl[0].clear();
	int_ctrl[1].clear();
	int_ctrl[2].clear();

	gpr[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

void spu_thread::cpu_stop()
{
	if (!group && offset >= RAW_SPU_BASE_ADDR)
	{
		// Save next PC and current SPU Interrupt Status
		npc = pc | (interrupts_enabled);
	}
	else if (group && is_stopped())
	{
		if (verify(HERE, group->running--) == 1)
		{
			{
				std::lock_guard lock(group->mutex);
				group->stop_count++;
				group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;

				if (!group->join_state)
				{
					group->join_state = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
				}

				if (const auto ppu = std::exchange(group->waiter, nullptr))
				{
					// Send exit status directly to the joining thread
					ppu->gpr[4] = group->join_state;
					ppu->gpr[5] = group->exit_status;
					group->join_state.release(0);
				}
			}

			// Notify on last thread stopped
			group->cond.notify_all();
		}
	}
}

extern thread_local std::string(*g_tls_log_prefix)();

void spu_thread::cpu_task()
{
	// Get next PC and SPU Interrupt status
	pc = npc.exchange(0);

	set_interrupt_status((pc & 1) != 0);

	pc &= 0x3fffc;

	std::fesetround(FE_TOWARDZERO);

	if (g_cfg.core.set_daz_and_ftz && g_cfg.core.spu_decoder != spu_decoder_type::precise)
	{
		// Set DAZ and FTZ
		_mm_setcsr(_mm_getcsr() | 0x8840);
	}

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<spu_thread*>(get_current_cpu_thread());
		return fmt::format("%s [0x%05x]", thread_ctrl::get_name(), cpu->pc);
	};

	if (jit)
	{
		// Register SPU runtime user
		spu_runtime::passive_lock _passive_lock(jit->get_runtime());

		while (true)
		{
			if (UNLIKELY(state))
			{
				if (check_state())
				{
					if (state & cpu_flag::jit_return)
					{
						// Handle jit_return as a special case
						jit->get_runtime().handle_return(this);
						continue;
					}

					break;
				}
			}

			spu_runtime::g_dispatcher[pc / 4](*this, vm::_ptr<u8>(offset), nullptr);
		}

		// Print some stats
		LOG_NOTICE(SPU, "Stats: Block Weight: %u (Retreats: %u);", block_counter, block_failure);
		cpu_stop();
		return;
	}

	if (spu_runtime::g_interpreter)
	{
		while (true)
		{
			if (UNLIKELY(state))
			{
				if (check_state())
					break;
			}

			spu_runtime::g_interpreter(*this, vm::_ptr<u8>(offset), nullptr);
		}

		cpu_stop();
		return;
	}

	// Select opcode table
	const auto& table = *(
		g_cfg.core.spu_decoder == spu_decoder_type::precise ? &g_spu_interpreter_precise.get_table() :
		g_cfg.core.spu_decoder == spu_decoder_type::fast ? &g_spu_interpreter_fast.get_table() :
		(fmt::throw_exception<std::logic_error>("Invalid SPU decoder"), nullptr));

	// LS pointer
	const auto base = vm::_ptr<const u8>(offset);

	while (true)
	{
		if (UNLIKELY(state))
		{
			if (check_state())
				break;
		}

		const u32 op = *reinterpret_cast<const be_t<u32>*>(base + pc);
		if (table[spu_decode(op)](*this, {op}))
			pc += 4;
	}

	cpu_stop();
}

void spu_thread::cpu_mem()
{
	//vm::passive_lock(*this);
}

void spu_thread::cpu_unmem()
{
	//state.test_and_set(cpu_flag::memory);
}

spu_thread::~spu_thread()
{
	// Deallocate Local Storage
	vm::dealloc_verbose_nothrow(offset);

	// Deallocate RawSPU ID
	if (!group && offset >= RAW_SPU_BASE_ADDR)
	{
		g_raw_spu_id[index] = 0;
		g_raw_spu_ctr--;
	}
}

spu_thread::spu_thread(vm::addr_t ls, lv2_spu_group* group, u32 index, std::string_view name)
	: cpu_thread(idm::last_id())
	, spu_name(name)
	, index(index)
	, offset(ls)
	, group(group)
{
	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
	{
		jit = spu_recompiler_base::make_asmjit_recompiler();
	}

	if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		jit = spu_recompiler_base::make_llvm_recompiler();
	}

	if (g_cfg.core.spu_decoder != spu_decoder_type::fast && g_cfg.core.spu_decoder != spu_decoder_type::precise)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Initialize stack mirror
			std::memset(stack_mirror.data(), 0xff, sizeof(stack_mirror));
		}
	}

	if (!group && offset >= RAW_SPU_BASE_ADDR)
	{
		cpu_init();
	}
}

void spu_thread::push_snr(u32 number, u32 value)
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

void spu_thread::do_dma_transfer(const spu_mfc_cmd& args)
{
	const bool is_get = (args.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK | MFC_START_MASK)) == MFC_GET_CMD;

	u32 eal = args.eal;
	u32 lsa = args.lsa & 0x3ffff;

	// SPU Thread Group MMIO (LS and SNR) and RawSPU MMIO
	if (eal >= RAW_SPU_BASE_ADDR)
	{
		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		if (eal < SYS_SPU_THREAD_BASE_LOW)
		{
			// RawSPU MMIO
			auto thread = idm::get<named_thread<spu_thread>>(find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

			if (!thread)
			{
				fmt::throw_exception("RawSPU not found (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}

			u32 value;
			if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + args.size - 1 < 0x40000) // LS access
			{
			}
			else if (args.size == 4 && is_get && thread->read_reg(eal, value))
			{
				_ref<u32>(lsa) = value;
				return;
			}
			else if (args.size == 4 && !is_get && thread->write_reg(eal, _ref<u32>(lsa)))
			{
				return;
			}
			else
			{
				fmt::throw_exception("Invalid RawSPU MMIO offset (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}
		}
		else if (this->offset >= RAW_SPU_BASE_ADDR)
		{
			fmt::throw_exception("SPU MMIO used for RawSPU (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
		}
		else if (group && group->threads[index])
		{
			auto& spu = static_cast<spu_thread&>(*group->threads[index]);

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

	u8* dst = (u8*)vm::base(eal);
	u8* src = (u8*)vm::base(offset + lsa);

	if (UNLIKELY(!is_get && !g_use_rtm))
	{
		switch (u32 size = args.size)
		{
		case 1:
		{
			auto& res = vm::reservation_lock(eal, 1);
			*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
			res.release(res.load() + 1);
			break;
		}
		case 2:
		{
			auto& res = vm::reservation_lock(eal, 2);
			*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
			res.release(res.load() + 1);
			break;
		}
		case 4:
		{
			auto& res = vm::reservation_lock(eal, 4);
			*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
			res.release(res.load() + 1);
			break;
		}
		case 8:
		{
			auto& res = vm::reservation_lock(eal, 8);
			*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
			res.release(res.load() + 1);
			break;
		}
		default:
		{
			if (((eal & 127) + size) <= 128)
			{
				// Lock one cache line
				auto& res = vm::reservation_lock(eal, 128);

				while (size)
				{
					*reinterpret_cast<u128*>(dst) = *reinterpret_cast<const u128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}

				res.release(res.load() + 1);
				break;
			}

			auto lock = vm::passive_lock(eal & -128u, ::align(eal + size, 128));

			while (size >= 128)
			{
				mov_rdata(reinterpret_cast<u128*>(dst), reinterpret_cast<const u128*>(src));

				dst += 128;
				src += 128;
				size -= 128;
			}

			while (size)
			{
				*reinterpret_cast<u128*>(dst) = *reinterpret_cast<const u128*>(src);

				dst += 16;
				src += 16;
				size -= 16;
			}

			lock->release(0);
			break;
		}
		}

		return;
	}

	if (is_get)
	{
		std::swap(dst, src);
	}

	switch (u32 size = args.size)
	{
	case 1:
	{
		*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
		break;
	}
	case 2:
	{
		*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
		break;
	}
	case 4:
	{
		*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
		break;
	}
	case 8:
	{
		*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
		break;
	}
	default:
	{
		while (size >= 128)
		{
			mov_rdata(reinterpret_cast<u128*>(dst), reinterpret_cast<const u128*>(src));

			dst += 128;
			src += 128;
			size -= 128;
		}

		while (size)
		{
			*reinterpret_cast<u128*>(dst) = *reinterpret_cast<const u128*>(src);

			dst += 16;
			src += 16;
			size -= 16;
		}

		break;
	}
	}
}

bool spu_thread::do_dma_check(const spu_mfc_cmd& args)
{
	const u32 mask = utils::rol32(1, args.tag);

	if (UNLIKELY(mfc_barrier & mask || (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && mfc_fence & mask)))
	{
		// Check for special value combination (normally impossible)
		if (false)
		{
			// Update barrier/fence masks if necessary
			mfc_barrier = 0;
			mfc_fence = 0;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if ((mfc_queue[i].cmd & ~0xc) == MFC_BARRIER_CMD)
				{
					mfc_barrier |= -1;
					continue;
				}

				if (true)
				{
					const u32 _mask = utils::rol32(1u, mfc_queue[i].tag);

					// A command with barrier hard blocks that tag until it's been dealt with
					if (mfc_queue[i].cmd & MFC_BARRIER_MASK)
					{
						mfc_barrier |= _mask;
					}

					// A new command that has a fence can't be executed until the stalled list has been dealt with
					mfc_fence |= _mask;
				}
			}

			if (mfc_barrier & mask || (args.cmd & MFC_FENCE_MASK && mfc_fence & mask))
			{
				return false;
			}

			return true;
		}

		return false;
	}

	return true;
}

bool spu_thread::do_list_transfer(spu_mfc_cmd& args)
{
	struct list_element
	{
		be_t<u16> sb; // Stall-and-Notify bit (0x8000)
		be_t<u16> ts; // List Transfer Size
		be_t<u32> ea; // External Address Low
	} item{};

	while (args.size)
	{
		if (UNLIKELY(item.sb & 0x8000))
		{
			ch_stall_mask |= utils::rol32(1, args.tag);

			if (!ch_stall_stat.get_count())
			{
				ch_event_stat |= SPU_EVENT_SN;
			}

			ch_stall_stat.set_value(utils::rol32(1, args.tag) | ch_stall_stat.get_value());

			args.tag |= 0x80; // Set stalled status
			return false;
		}

		args.lsa &= 0x3fff0;
		item = _ref<list_element>(args.eal & 0x3fff8);

		const u32 size = item.ts & 0x7fff;
		const u32 addr = item.ea;

		LOG_TRACE(SPU, "LIST: addr=0x%x, size=0x%x, lsa=0x%05x, sb=0x%x", addr, size, args.lsa | (addr & 0xf), item.sb);

		if (size)
		{
			spu_mfc_cmd transfer;
			transfer.eal  = addr;
			transfer.eah  = 0;
			transfer.lsa  = args.lsa | (addr & 0xf);
			transfer.tag  = args.tag;
			transfer.cmd  = MFC(args.cmd & ~MFC_LIST_MASK);
			transfer.size = size;

			do_dma_transfer(transfer);
			const u32 add_size = std::max<u32>(size, 16);
			args.lsa += add_size;
		}

		args.eal += 8;
		args.size -= 8;
	}

	return true;
}

void spu_thread::do_putlluc(const spu_mfc_cmd& args)
{
	const u32 addr = args.eal & -128u;

	if (raddr && addr == raddr)
	{
		// Last check for event before we clear the reservation
		if ((vm::reservation_acquire(addr, 128) & ~1ull) != rtime || rdata != vm::_ref<decltype(rdata)>(addr))
		{
			ch_event_stat |= SPU_EVENT_LR;
		}

		raddr = 0;
	}

	const auto& to_write = _ref<decltype(rdata)>(args.lsa & 0x3ff80);

	// Store unconditionally
	if (LIKELY(g_use_rtm))
	{
		u64 count = 1;

		while (!spu_putlluc_tx(addr, to_write.data()))
		{
			std::this_thread::yield();
			count += 2;
		}

		if (count > 5)
		{
			LOG_ERROR(SPU, "%s took too long: %u", args.cmd, count);
		}
	}
	else
	{
		auto& data = vm::_ref<decltype(rdata)>(addr);
		auto& res = vm::reservation_lock(addr, 128);

		*reinterpret_cast<atomic_t<u32>*>(&data) += 0;

		if (g_cfg.core.spu_accurate_putlluc)
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr);
			mov_rdata(data.data(), to_write.data());
			res.release(res.load() + 1);
		}
		else
		{
			mov_rdata(data.data(), to_write.data());
			res.release(res.load() + 1);
		}
	}

	vm::reservation_notifier(addr, 128).notify_all();
}

void spu_thread::do_mfc(bool wait)
{
	u32 removed = 0;
	u32 barrier = 0;
	u32 fence = 0;

	// Process enqueued commands
	std::remove_if(mfc_queue + 0, mfc_queue + mfc_size, [&](spu_mfc_cmd& args)
	{
		if ((args.cmd & ~0xc) == MFC_BARRIER_CMD)
		{
			if (&args - mfc_queue <= removed)
			{
				// Remove barrier-class command if it's the first in the queue
				_mm_mfence();
				removed++;
				return true;
			}

			// Block all tags
			barrier |= -1;
			return false;
		}

		// Select tag bit in the tag mask or the stall mask
		const u32 mask = utils::rol32(1, args.tag);

		if (barrier & mask)
		{
			fence |= mask;
			return false;
		}

		if (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && fence & mask)
		{
			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			return false;
		}

		if (args.cmd & MFC_LIST_MASK)
		{
			if (!(args.tag & 0x80))
			{
				if (do_list_transfer(args))
				{
					removed++;
					return true;
				}
			}

			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			fence |= mask;
			return false;
		}

		if (args.cmd == MFC_PUTQLLUC_CMD)
		{
			if (fence & mask)
			{
				return false;
			}

			do_putlluc(args);
		}
		else if (args.size)
		{
			do_dma_transfer(args);
		}

		removed++;
		return true;
	});

	mfc_size -= removed;
	mfc_barrier = barrier;
	mfc_fence = fence;

	if (removed && ch_tag_upd)
	{
		const u32 completed = get_mfc_completed();

		if (completed && ch_tag_upd == 1)
		{
			ch_tag_stat.set_value(completed);
			ch_tag_upd = 0;
		}
		else if (completed == ch_tag_mask && ch_tag_upd == 2)
		{
			ch_tag_stat.set_value(completed);
			ch_tag_upd = 0;
		}
	}
}

u32 spu_thread::get_mfc_completed()
{
	return ch_tag_mask & ~mfc_fence;
}

bool spu_thread::process_mfc_cmd()
{
	// Stall infinitely if MFC queue is full
	while (UNLIKELY(mfc_size >= 16))
	{
		if (is_stopped())
		{
			return false;
		}

		thread_ctrl::wait();
	}

	spu::scheduler::concurrent_execution_watchdog watchdog(*this);
	LOG_TRACE(SPU, "DMAC: cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);

	switch (ch_mfc_cmd.cmd)
	{
	case MFC_GETLLAR_CMD:
	{
		const u32 addr = ch_mfc_cmd.eal & -128u;
		auto& data = vm::_ref<decltype(rdata)>(addr);
		auto& dst = _ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ff80);
		u64 ntime;

		const bool is_polling = false; // TODO

		if (is_polling)
		{
			rtime = vm::reservation_acquire(addr, 128);

			while (rdata == data && vm::reservation_acquire(addr, 128) == rtime)
			{
				if (is_stopped())
				{
					break;
				}

				thread_ctrl::wait_for(100);
			}
		}

		if (LIKELY(g_use_rtm))
		{
			u64 count = 1;

			if (g_cfg.core.spu_accurate_getllar)
			{
			 	while ((ntime = spu_getll_tx(addr, dst.data())) & 1)
				{
					std::this_thread::yield();
					count += 2;
				}
			}
			else
			{
				for (;; count++, busy_wait(300))
				{
					ntime = vm::reservation_acquire(addr, 128);
					dst = data;

					if (LIKELY(vm::reservation_acquire(addr, 128) == ntime))
					{
						break;
					}
				}
			}

			if (count > 15)
			{
				LOG_ERROR(SPU, "%s took too long: %u", ch_mfc_cmd.cmd, count);
			}
		}
		else
		{
			auto& res = vm::reservation_lock(addr, 128);
			const u64 old_time = res.load() & ~1ull;

			if (g_cfg.core.spu_accurate_getllar)
			{
				*reinterpret_cast<atomic_t<u32>*>(&data) += 0;

				// Full lock (heavyweight)
				// TODO: vm::check_addr
				vm::writer_lock lock(addr);

				ntime = old_time;
				mov_rdata(dst.data(), data.data());
				res.release(old_time);
			}
			else
			{
				ntime = old_time;
				mov_rdata(dst.data(), data.data());
				res.release(old_time);
			}
		}

		if (const u32 _addr = raddr)
		{
			// Last check for event before we replace the reservation with a new one
			if ((vm::reservation_acquire(_addr, 128) & ~1ull) != rtime || rdata != vm::_ref<decltype(rdata)>(_addr))
			{
				ch_event_stat |= SPU_EVENT_LR;

				if (_addr == addr)
				{
					// Lost current reservation
					raddr = 0;
					ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
					return true;
				}
			}
		}

		raddr = addr;
		rtime = ntime;
		mov_rdata(rdata.data(), dst.data());

		ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
		return true;
	}

	case MFC_PUTLLC_CMD:
	{
		// Store conditionally
		const u32 addr = ch_mfc_cmd.eal & -128u;
		u32 result = 0;

		if (raddr == addr && rtime == (vm::reservation_acquire(raddr, 128) & ~1ull))
		{
			const auto& to_write = _ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ff80);

			if (LIKELY(g_use_rtm))
			{
				while (true)
				{
					result = spu_putllc_tx(addr, rtime, rdata.data(), to_write.data());

					if (result < 2)
					{
						break;
					}

					// Retry
					std::this_thread::yield();
				}
			}
			else if (auto& data = vm::_ref<decltype(rdata)>(addr); rdata == data)
			{
				auto& res = vm::reservation_lock(raddr, 128);
				const u64 old_time = res.load() & ~1ull;

				if (rtime == old_time)
				{
					*reinterpret_cast<atomic_t<u32>*>(&data) += 0;

					// Full lock (heavyweight)
					// TODO: vm::check_addr
					vm::writer_lock lock(addr);

					if (rdata == data)
					{
						mov_rdata(data.data(), to_write.data());
						res.release(old_time + 2);
						result = 1;
					}
					else
					{
						res.release(old_time);
					}
				}
				else
				{
					res.release(old_time);
				}
			}
		}

		if (result)
		{
			vm::reservation_notifier(addr, 128).notify_all();
			ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			if (raddr)
			{
				// Last check for event before we clear the reservation
				if (raddr == addr || rtime != (vm::reservation_acquire(raddr, 128) & ~1ull) || rdata != vm::_ref<decltype(rdata)>(raddr))
				{
					ch_event_stat |= SPU_EVENT_LR;
				}
			}

			ch_atomic_stat.set_value(MFC_PUTLLC_FAILURE);
		}

		raddr = 0;
		return true;
	}
	case MFC_PUTLLUC_CMD:
	{
		do_putlluc(ch_mfc_cmd);
		ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		return true;
	}
	case MFC_PUTQLLUC_CMD:
	{
		const u32 mask = utils::rol32(1, ch_mfc_cmd.tag);

		if (UNLIKELY((mfc_barrier | mfc_fence) & mask))
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= mask;
		}
		else
		{
			do_putlluc(ch_mfc_cmd);
		}

		return true;
	}
	case MFC_SNDSIG_CMD:
	case MFC_SNDSIGB_CMD:
	case MFC_SNDSIGF_CMD:
	{
		if (ch_mfc_cmd.size != 4)
		{
			break;
		}

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
		if (LIKELY(ch_mfc_cmd.size <= 0x4000))
		{
			if (LIKELY(do_dma_check(ch_mfc_cmd)))
			{
				if (ch_mfc_cmd.size)
				{
					do_dma_transfer(ch_mfc_cmd);
				}

				return true;
			}

			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= utils::rol32(1, ch_mfc_cmd.tag);

			if (ch_mfc_cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, ch_mfc_cmd.tag);
			}

			return true;
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
		if (LIKELY(ch_mfc_cmd.size <= 0x4000))
		{
			auto& cmd = mfc_queue[mfc_size];
			cmd = ch_mfc_cmd;

			if (LIKELY(do_dma_check(cmd)))
			{
				if (LIKELY(do_list_transfer(cmd)))
				{
					return true;
				}
			}

			mfc_size++;
			mfc_fence |= utils::rol32(1, cmd.tag);

			if (cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, cmd.tag);
			}

			return true;
		}

		break;
	}
	case MFC_BARRIER_CMD:
	case MFC_EIEIO_CMD:
	case MFC_SYNC_CMD:
	{
		if (mfc_size == 0)
		{
			_mm_mfence();
		}
		else
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_barrier |= -1;
		}

		return true;
	}
	default:
	{
		break;
	}
	}

	fmt::throw_exception("Unknown command (cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE,
		ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);
}

u32 spu_thread::get_events(bool waiting)
{
	const u32 mask1 = ch_event_mask;

	if (mask1 & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)" HERE, mask1);
	}

	// Check reservation status and set SPU_EVENT_LR if lost
	if (raddr && ((vm::reservation_acquire(raddr, sizeof(rdata)) & ~1ull) != rtime || rdata != vm::_ref<decltype(rdata)>(raddr)))
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
	return !waiting ? ch_event_stat & mask1 : ch_event_stat.atomic_op([&](u32& stat) -> u32
	{
		if (u32 res = stat & mask1)
		{
			stat &= ~SPU_EVENT_WAITING;
			return res;
		}

		stat |= SPU_EVENT_WAITING;
		return 0;
	});
}

void spu_thread::set_events(u32 mask)
{
	if (mask & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)" HERE, mask);
	}

	// Set new events, get old event mask
	const u32 old_stat = ch_event_stat.fetch_or(mask);

	// Notify if some events were set
	if (~old_stat & mask && old_stat & SPU_EVENT_WAITING && ch_event_stat & SPU_EVENT_WAITING)
	{
		notify();
	}
}

void spu_thread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// Detect enabling interrupts with events masked
		if (ch_event_mask & ~SPU_EVENT_INTR_IMPLEMENTED)
		{
			fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x)" HERE, +ch_event_mask);
		}

		interrupts_enabled = true;
	}
	else
	{
		interrupts_enabled = false;
	}
}

u32 spu_thread::get_ch_count(u32 ch)
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
	case MFC_Cmd:             return 16 - mfc_size;
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

s64 spu_thread::get_ch_value(u32 ch)
{
	LOG_TRACE(SPU, "get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

	auto read_channel = [&](spu_channel& channel) -> s64
	{
		for (int i = 0; i < 10 && channel.get_count() == 0; i++)
		{
			busy_wait();
		}

		u32 out;

		while (!channel.try_pop(out))
		{
			if (is_stopped())
			{
				return -1;
			}

			thread_ctrl::wait();
		}

		return out;
	};

	switch (ch)
	{
	case SPU_RdSRR0:
	{
		return srr0;
	}
	case SPU_RdInMbox:
	{
		while (true)
		{
			for (int i = 0; i < 10 && ch_in_mbox.get_count() == 0; i++)
			{
				busy_wait();
			}

			u32 out;

			if (const uint old_count = ch_in_mbox.try_pop(out))
			{
				if (old_count == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				return out;
			}

			if (is_stopped())
			{
				return -1;
			}

			thread_ctrl::wait();
		}
	}

	case MFC_RdTagStat:
	{
		if (ch_tag_stat.get_count())
		{
			u32 out = ch_tag_stat.get_value();
			ch_tag_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
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
		if (ch_atomic_stat.get_count())
		{
			u32 out = ch_atomic_stat.get_value();
			ch_atomic_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_atomic_stat);
	}

	case MFC_RdListStallStat:
	{
		if (ch_stall_stat.get_count())
		{
			u32 out = ch_stall_stat.get_value();
			ch_stall_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_stall_stat);
	}

	case SPU_RdDec:
	{
		u32 out = ch_dec_value - (u32)(get_timebased_time() - ch_dec_start_timestamp);

		//Polling: We might as well hint to the scheduler to slot in another thread since this one is counting down
		if (g_cfg.core.spu_loop_detection && out > spu::scheduler::native_jiffy_duration_us)
			std::this_thread::yield();

		return out;
	}

	case SPU_RdEventMask:
	{
		return ch_event_mask;
	}

	case SPU_RdEventStat:
	{
		u32 res = get_events();

		if (res)
		{
			return res;
		}

		const u32 mask1 = ch_event_mask;

		if (mask1 & SPU_EVENT_LR && raddr)
		{
			if (mask1 != SPU_EVENT_LR && mask1 != SPU_EVENT_LR + SPU_EVENT_TM)
			{
				// Combining LR with other flags needs another solution
				fmt::throw_exception("Not supported: event mask 0x%x" HERE, mask1);
			}

			const auto pseudo_lock = vm::reservation_notifier(raddr, 128).lock_one();

			while (res = get_events(), !res)
			{
				if (is_stopped())
				{
					return -1;
				}

				pseudo_lock.wait(100);
			}

			return res;
		}

		while (res = get_events(true), !res)
		{
			if (is_stopped())
			{
				return -1;
			}

			thread_ctrl::wait_for(100);
		}

		return res;
	}

	case SPU_RdMachStat:
	{
		// HACK: "Not isolated" status
		// Return SPU Interrupt status in LSB
		return interrupts_enabled == true;
	}
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool spu_thread::set_ch_value(u32 ch, u32 value)
{
	LOG_TRACE(SPU, "set_ch_value(ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);

	switch (ch)
	{
	case SPU_WrSRR0:
	{
		srr0 = value;
		return true;
	}

	case SPU_WrOutIntrMbox:
	{
		if (offset >= RAW_SPU_BASE_ADDR)
		{
			while (!ch_out_intr_mbox.try_push(value))
			{
				if (is_stopped())
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

				const auto queue = (std::lock_guard{group->mutex}, this->spup[spup].lock());

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

				const auto queue = (std::lock_guard{group->mutex}, this->spup[spup].lock());

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
			if (is_stopped())
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

		if (ch_tag_upd)
		{
			const u32 completed = get_mfc_completed();

			if (completed && ch_tag_upd == 1)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = 0;
			}
			else if (completed == value && ch_tag_upd == 2)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = 0;
			}
		}

		return true;
	}

	case MFC_WrTagUpdate:
	{
		if (value > 2)
		{
			break;
		}

		const u32 completed = get_mfc_completed();

		if (!value)
		{
			ch_tag_upd = 0;
			ch_tag_stat.set_value(completed);
		}
		else if (completed && value == 1)
		{
			ch_tag_upd = 0;
			ch_tag_stat.set_value(completed);
		}
		else if (completed == ch_tag_mask && value == 2)
		{
			ch_tag_upd = 0;
			ch_tag_stat.set_value(completed);
		}
		else
		{
			ch_tag_upd = value;
			ch_tag_stat.set_value(0, false);
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
		ch_mfc_cmd.size = value & 0x7fff;
		return true;
	}

	case MFC_TagID:
	{
		ch_mfc_cmd.tag = value & 0x1f;
		return true;
	}

	case MFC_Cmd:
	{
		ch_mfc_cmd.cmd = MFC(value & 0xff);
		return process_mfc_cmd();
	}

	case MFC_WrListStallAck:
	{
		// Reset stall status for specified tag
		const u32 tag_mask = utils::rol32(1, value);

		if (ch_stall_mask & tag_mask)
		{
			ch_stall_mask &= ~tag_mask;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if (mfc_queue[i].tag == (value | 0x80))
				{
					// Unset stall bit
					mfc_queue[i].tag &= 0x7f;
				}
			}

			do_mfc(true);
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
		ch_event_mask = value;
		return true;
	}

	case SPU_WrEventAck:
	{
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

bool spu_thread::stop_and_signal(u32 code)
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
		return true;
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
			if (is_stopped())
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
				if (is_stopped())
				{
					return false;
				}

				thread_ctrl::wait();
			}

			reader_lock rlock(id_manager::g_mutex);

			std::lock_guard lock(group->mutex);

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

			std::lock_guard qlock(queue->mutex);

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
			if (is_stopped())
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

		std::lock_guard lock(group->mutex);

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
					thread_ctrl::notify(*thread);
				}
			}
		}

		return true;
	}

	case 0x111:
	{
		/* ===== sys_spu_thread_tryreceive_event ===== */

		u32 spuq;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_tryreceive_event(): Out_MBox is empty" HERE);
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			LOG_ERROR(SPU, "sys_spu_thread_tryreceive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		LOG_TRACE(SPU, "sys_spu_thread_tryreceive_event(spuq=0x%x)", spuq);

		std::lock_guard lock(group->mutex);

		std::shared_ptr<lv2_event_queue> queue;

		for (auto& v : this->spuq)
		{
			if (spuq == v.first)
			{
				if (queue = v.second.lock())
				{
					break;
				}
			}
		}

		if (!queue)
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		std::lock_guard qlock(queue->mutex);

		if (queue->events.empty())
		{
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		const auto event = queue->events.front();
		const auto data1 = static_cast<u32>(std::get<1>(event));
		const auto data2 = static_cast<u32>(std::get<2>(event));
		const auto data3 = static_cast<u32>(std::get<3>(event));
		ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
		queue->events.pop_front();
		return true;
	}

	case 0x100:
	{
		if (ch_out_mbox.get_count())
		{
			fmt::throw_exception("STOP code 0x100: Out_MBox is not empty" HERE);
		}

		_mm_mfence();
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

		std::lock_guard lock(group->mutex);

		for (auto& thread : group->threads)
		{
			if (thread && thread.get() != this)
			{
				thread->state += cpu_flag::stop;
				thread_ctrl::notify(*thread);
			}
		}

		group->exit_status = value;
		group->join_state = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;

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
		status |= SPU_STATUS_STOPPED_BY_STOP;
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

void spu_thread::halt()
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

void spu_thread::fast_call(u32 ls_addr)
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

DECLARE(spu_thread::g_raw_spu_ctr){};
DECLARE(spu_thread::g_raw_spu_id){};
