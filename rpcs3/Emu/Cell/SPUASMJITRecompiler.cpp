#include "stdafx.h"
#include "SPUASMJITRecompiler.h"

#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/timers.hpp"

#include "SPUThread.h"
#include "SPUInterpreter.h"
#include "Crypto/sha1.h"

#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/sysinfo.hpp"

#include <cmath>
#include <thread>

#define SPU_OFF_128(x, ...) asmjit::x86::oword_ptr(*cpu, offset32(&spu_thread::x, ##__VA_ARGS__))
#define SPU_OFF_64(x, ...) asmjit::x86::qword_ptr(*cpu, offset32(&spu_thread::x, ##__VA_ARGS__))
#define SPU_OFF_32(x, ...) asmjit::x86::dword_ptr(*cpu, offset32(&spu_thread::x, ##__VA_ARGS__))
#define SPU_OFF_16(x, ...) asmjit::x86::word_ptr(*cpu, offset32(&spu_thread::x, ##__VA_ARGS__))
#define SPU_OFF_8(x, ...) asmjit::x86::byte_ptr(*cpu, offset32(&spu_thread::x, ##__VA_ARGS__))

const spu_decoder<spu_recompiler> s_spu_decoder;

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_asmjit_recompiler()
{
	return std::make_unique<spu_recompiler>();
}

spu_recompiler::spu_recompiler()
{
}

void spu_recompiler::init()
{
	// Initialize if necessary
	if (!m_spurt)
	{
		m_spurt = &g_fxo->get<spu_runtime>();
	}
}

spu_function_t spu_recompiler::compile(spu_program&& _func)
{
	const u32 start0 = _func.entry_point;

	const auto add_loc = m_spurt->add_empty(std::move(_func));

	if (!add_loc)
	{
		return nullptr;
	}

	if (add_loc->compiled)
	{
		return add_loc->compiled;
	}

	const spu_program& func = add_loc->data;

	if (func.entry_point != start0)
	{
		// Wait for the duplicate
		while (!add_loc->compiled)
		{
			add_loc->compiled.wait(nullptr);
		}

		return add_loc->compiled;
	}

	if (auto& cache = g_fxo->get<spu_cache>(); cache && g_cfg.core.spu_cache && !add_loc->cached.exchange(1))
	{
		cache.add(func);
	}

	{
		sha1_context ctx;
		u8 output[20];

		sha1_starts(&ctx);
		sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
		sha1_finish(&ctx, output);

		be_t<u64> hash_start;
		std::memcpy(&hash_start, output, sizeof(hash_start));
		m_hash_start = hash_start;
	}

	using namespace asmjit;

	StringLogger logger;
	logger.addFlags(FormatFlags::kMachineCode);

	std::string log;

	CodeHolder code;
	code.init(m_asmrt.environment());

	x86::Assembler compiler(&code);
	compiler.addEncodingOptions(EncodingOptions::kOptimizedAlign);
	this->c = &compiler;

	if (g_cfg.core.spu_debug && !add_loc->logged.exchange(1))
	{
		// Dump analyser data
		this->dump(func, log);
		fs::write_file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append, log);

		// Set logger
		code.setLogger(&logger);
	}

	// Initialize args
	this->cpu = &x86::r13;
	this->ls = &x86::rbp;
	this->rip = &x86::r12;

	this->pc0 = &x86::r15;
	this->addr = &x86::eax;

#ifdef _WIN32
	this->arg0 = &x86::rcx;
	this->arg1 = &x86::rdx;
	this->qw0 = &x86::r8;
	this->qw1 = &x86::r9;
#else
	this->arg0 = &x86::rdi;
	this->arg1 = &x86::rsi;
	this->qw0 = &x86::rdx;
	this->qw1 = &x86::rcx;
#endif

	const std::array<const x86::Xmm*, 16> vec_vars
	{
		&x86::xmm0,
		&x86::xmm1,
		&x86::xmm2,
		&x86::xmm3,
		&x86::xmm4,
		&x86::xmm5,
		&x86::xmm6,
		&x86::xmm7,
		&x86::xmm8,
		&x86::xmm9,
		&x86::xmm10,
		&x86::xmm11,
		&x86::xmm12,
		&x86::xmm13,
		&x86::xmm14,
		&x86::xmm15,
	};

	for (u32 i = 0; i < vec_vars.size(); i++)
	{
		vec[i] = vec_vars[i];
	}

	label_stop = c->newLabel();
	Label label_diff = c->newLabel();
	Label label_code = c->newLabel();
	std::vector<u32> words;
	u32 words_align = 8;

	// Start compilation
	m_pos = func.lower_bound;
	m_base = func.entry_point;
	m_size = ::size32(func.data) * 4;
	const u32 start = m_pos;
	const u32 end = start + m_size;

	// Create block labels
	for (u32 i = 0; i < func.data.size(); i++)
	{
		if (func.data[i] && m_block_info[i + start / 4])
		{
			instr_labels[i * 4 + start] = c->newLabel();
		}
	}

	// Load actual PC and check status
	c->sub(x86::rsp, 0x28);
	c->mov(pc0->r32(), SPU_OFF_32(pc));
	c->cmp(SPU_OFF_32(state), 0);
	c->jnz(label_stop);

	if (g_cfg.core.spu_prof && g_cfg.core.spu_verification)
	{
		c->mov(x86::rax, m_hash_start & -0xffff);
		c->mov(SPU_OFF_64(block_hash), x86::rax);
	}

	if (utils::has_avx())
	{
		// How to check dirty AVX state
		//c->pxor(x86::xmm0, x86::xmm0);
		//c->vptest(x86::ymm0, x86::ymm0);
		//c->jnz(label_stop);
	}

	// Get bit mask of valid code words for a given range (up to 128 bytes)
	auto get_code_mask = [&](u32 starta, u32 enda) -> u32
	{
		u32 result = 0;

		for (u32 addr = starta, m = 1; addr < enda && m; addr += 4, m <<= 1)
		{
			// Filter out if out of range, or is a hole
			if (addr >= start && addr < end && func.data[(addr - start) / 4])
			{
				result |= m;
			}
		}

		return result;
	};

	// Check code
	u32 starta = start;

	// Skip holes at the beginning (giga only)
	for (u32 j = start; j < end; j += 4)
	{
		if (!func.data[(j - start) / 4])
		{
			starta += 4;
		}
		else
		{
			break;
		}
	}

	auto get_pc_ptr = [&]()
	{
		// Get data start address
		if (starta != m_base)
		{
			c->lea(x86::rax, get_pc(starta));
			c->and_(x86::eax, 0x3fffc);
			return x86::qword_ptr(*ls, x86::rax);
		}
		else
		{
			return x86::qword_ptr(*ls, *pc0);
		}
	};

	if (!g_cfg.core.spu_verification)
	{
		// Disable check (unsafe)
		if (utils::has_avx())
		{
			c->vzeroupper();
		}
	}
	else if (m_size == 8)
	{
		c->mov(x86::rax, static_cast<u64>(func.data[1]) << 32 | func.data[0]);
		c->cmp(x86::rax, x86::qword_ptr(*ls, *pc0));
		c->jnz(label_diff);

		if (utils::has_avx())
		{
			c->vzeroupper();
		}
	}
	else if (m_size == 4)
	{
		c->cmp(x86::dword_ptr(*ls, *pc0), func.data[0]);
		c->jnz(label_diff);

		if (utils::has_avx())
		{
			c->vzeroupper();
		}
	}
	else if (utils::has_avx512() && false)
	{
		// AVX-512 optimized check using 512-bit registers (disabled)
		words_align = 64;

		const u32 starta = start & -64;
		const u32 enda = utils::align(end, 64);
		const u32 sizea = (enda - starta) / 64;
		ensure(sizea);

		// Initialize pointers
		c->lea(x86::rax, x86::qword_ptr(label_code));
		u32 code_off = 0;
		u32 ls_off = -8192;

		for (u32 j = starta; j < enda; j += 64)
		{
			const u32 cmask = get_code_mask(j, j + 64);

			if (cmask == 0) [[unlikely]]
			{
				continue;
			}

			const bool first = ls_off == u32{} - 8192;

			// Ensure small distance for disp8*N
			if (j - ls_off >= 8192)
			{
				c->lea(*qw1, x86::qword_ptr(*ls, j));
				ls_off = j;
			}

			if (code_off >= 8192)
			{
				c->lea(x86::rax, x86::qword_ptr(x86::rax, 8192));
				code_off -= 8192;
			}

			if (cmask != 0xffff)
			{
				// Generate k-mask for the block
				Label label = c->newLabel();
				c->kmovw(x86::k7, x86::word_ptr(label));

				consts.emplace_back([=, this]
				{
					c->bind(label);
					c->dq(cmask);
				});

				c->setExtraReg(x86::k7);
				c->z().vmovdqa32(x86::zmm0, x86::zmmword_ptr(*qw1, j - ls_off));
			}
			else
			{
				c->vmovdqa32(x86::zmm0, x86::zmmword_ptr(*qw1, j - ls_off));
			}

			if (first)
			{
				c->vpcmpud(x86::k1, x86::zmm0, x86::zmmword_ptr(x86::rax, code_off), 4);
			}
			else
			{
				c->vpcmpud(x86::k3, x86::zmm0, x86::zmmword_ptr(x86::rax, code_off), 4);
				c->korw(x86::k1, x86::k3, x86::k1);
			}

			for (u32 i = j; i < j + 64; i += 4)
			{
				words.push_back(i >= start && i < end ? func.data[(i - start) / 4] : 0);
			}

			code_off += 64;
		}

		c->ktestw(x86::k1, x86::k1);
		c->jnz(label_diff);
		c->vzeroupper();
	}
	else if (0 && utils::has_avx512())
	{
		// AVX-512 optimized check using 256-bit registers
		words_align = 32;

		const u32 starta = start & -32;
		const u32 enda = utils::align(end, 32);
		const u32 sizea = (enda - starta) / 32;
		ensure(sizea);

		if (sizea == 1)
		{
			const u32 cmask = get_code_mask(starta, enda);

			if (cmask == 0xff)
			{
				c->vmovdqa(x86::ymm0, x86::ymmword_ptr(*ls, starta));
			}
			else
			{
				c->vpxor(x86::ymm0, x86::ymm0, x86::ymm0);
				c->vpblendd(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta), cmask);
			}

			c->vpxor(x86::ymm0, x86::ymm0, x86::ymmword_ptr(label_code));
			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);

			for (u32 i = starta; i < enda; i += 4)
			{
				words.push_back(i >= start && i < end ? func.data[(i - start) / 4] : 0);
			}
		}
		else if (sizea == 2 && (end - start) <= 32)
		{
			const u32 cmask0 = get_code_mask(starta, starta + 32);
			const u32 cmask1 = get_code_mask(starta + 32, enda);

			c->vpxor(x86::ymm0, x86::ymm0, x86::ymm0);
			c->vpblendd(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta), cmask0);
			c->vpblendd(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta + 32), cmask1);
			c->vpxor(x86::ymm0, x86::ymm0, x86::ymmword_ptr(label_code));
			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);

			for (u32 i = starta; i < starta + 32; i += 4)
			{
				words.push_back(i >= start ? func.data[(i - start) / 4] : i + 32 < end ? func.data[(i + 32 - start) / 4] : 0);
			}
		}
		else
		{
			bool xmm2z = false;

			// Initialize pointers
			c->lea(x86::rax, x86::qword_ptr(label_code));
			u32 code_off = 0;
			u32 ls_off = -4096;

			for (u32 j = starta; j < enda; j += 32)
			{
				const u32 cmask = get_code_mask(j, j + 32);

				if (cmask == 0) [[unlikely]]
				{
					continue;
				}

				const bool first = ls_off == u32{0} - 4096;

				// Ensure small distance for disp8*N
				if (j - ls_off >= 4096)
				{
					c->lea(*qw1, x86::qword_ptr(*ls, j));
					ls_off = j;
				}

				if (code_off >= 4096)
				{
					c->lea(x86::rax, x86::qword_ptr(x86::rax, 4096));
					code_off -= 4096;
				}

				if (cmask != 0xff)
				{
					if (!xmm2z)
					{
						c->vpxor(x86::xmm2, x86::xmm2, x86::xmm2);
						xmm2z = true;
					}

					c->vpblendd(x86::ymm1, x86::ymm2, x86::ymmword_ptr(*qw1, j - ls_off), cmask);
				}
				else
				{
					c->vmovdqa32(x86::ymm1, x86::ymmword_ptr(*qw1, j - ls_off));
				}

				// Perform bitwise comparison and accumulate
				if (first)
				{
					c->vpxor(x86::ymm0, x86::ymm1, x86::ymmword_ptr(x86::rax, code_off));
				}
				else
				{
					c->vpternlogd(x86::ymm0, x86::ymm1, x86::ymmword_ptr(x86::rax, code_off), 0xf6 /* orAxorBC */);
				}

				for (u32 i = j; i < j + 32; i += 4)
				{
					words.push_back(i >= start && i < end ? func.data[(i - start) / 4] : 0);
				}

				code_off += 32;
			}

			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);
		}

		c->vzeroupper();
	}
	else if (0 && utils::has_avx())
	{
		// Mainstream AVX
		words_align = 32;

		const u32 starta = start & -32;
		const u32 enda = utils::align(end, 32);
		const u32 sizea = (enda - starta) / 32;
		ensure(sizea);

		if (sizea == 1)
		{
			const u32 cmask = get_code_mask(starta, enda);

			if (cmask == 0xff)
			{
				c->vmovaps(x86::ymm0, x86::ymmword_ptr(*ls, starta));
			}
			else
			{
				c->vxorps(x86::ymm0, x86::ymm0, x86::ymm0);
				c->vblendps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta), cmask);
			}

			c->vxorps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(label_code));
			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);

			for (u32 i = starta; i < enda; i += 4)
			{
				words.push_back(i >= start && i < end ? func.data[(i - start) / 4] : 0);
			}
		}
		else if (sizea == 2 && (end - start) <= 32)
		{
			const u32 cmask0 = get_code_mask(starta, starta + 32);
			const u32 cmask1 = get_code_mask(starta + 32, enda);

			c->vxorps(x86::ymm0, x86::ymm0, x86::ymm0);
			c->vblendps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta), cmask0);
			c->vblendps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(*ls, starta + 32), cmask1);
			c->vxorps(x86::ymm0, x86::ymm0, x86::ymmword_ptr(label_code));
			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);

			for (u32 i = starta; i < starta + 32; i += 4)
			{
				words.push_back(i >= start ? func.data[(i - start) / 4] : i + 32 < end ? func.data[(i + 32 - start) / 4] : 0);
			}
		}
		else
		{
			bool xmm2z = false;

			// Initialize pointers
			c->add(*ls, starta);
			c->lea(x86::rax, x86::qword_ptr(label_code));
			u32 code_off = 0;
			u32 ls_off = starta;
			u32 order0 = 0;
			u32 order1 = 0;

			for (u32 j = starta; j < enda; j += 32)
			{
				const u32 cmask = get_code_mask(j, j + 32);

				if (cmask == 0) [[unlikely]]
				{
					continue;
				}

				// Interleave two threads
				auto& order = order0 > order1 ? order1 : order0;
				const auto& reg0 = order0 > order1 ? x86::ymm3 : x86::ymm0;
				const auto& reg1 = order0 > order1 ? x86::ymm4 : x86::ymm1;

				// Ensure small distance for disp8
				if (j - ls_off >= 256)
				{
					c->add(*ls, j - ls_off);
					ls_off = j;
				}
				else if (j - ls_off >= 128)
				{
					c->sub(*ls, -128);
					ls_off += 128;
				}

				if (code_off >= 128)
				{
					c->sub(x86::rax, -128);
					code_off -= 128;
				}

				if (cmask != 0xff)
				{
					if (!xmm2z)
					{
						c->vxorps(x86::xmm2, x86::xmm2, x86::xmm2);
						xmm2z = true;
					}

					c->vblendps(reg1, x86::ymm2, x86::ymmword_ptr(*ls, j - ls_off), cmask);
				}
				else
				{
					c->vmovaps(reg1, x86::ymmword_ptr(*ls, j - ls_off));
				}

				// Perform bitwise comparison and accumulate
				if (!order++)
				{
					c->vxorps(reg0, reg1, x86::ymmword_ptr(x86::rax, code_off));
				}
				else
				{
					c->vxorps(reg1, reg1, x86::ymmword_ptr(x86::rax, code_off));
					c->vorps(reg0, reg1, reg0);
				}

				for (u32 i = j; i < j + 32; i += 4)
				{
					words.push_back(i >= start && i < end ? func.data[(i - start) / 4] : 0);
				}

				code_off += 32;
			}

			c->sub(*ls, ls_off);

			if (order1)
			{
				c->vorps(x86::ymm0, x86::ymm3, x86::ymm0);
			}

			c->vptest(x86::ymm0, x86::ymm0);
			c->jnz(label_diff);
		}

		c->vzeroupper();
	}
	else
	{
		if (utils::has_avx())
		{
			c->vzeroupper();
		}

		// Compatible SSE2
		words_align = 16;

		// Initialize pointers
		c->lea(x86::rcx, get_pc_ptr());
		c->lea(x86::rax, x86::qword_ptr(label_code));
		u32 code_off = 0;
		u32 ls_off = starta;
		u32 order0 = 0;
		u32 order1 = 0;

		for (u32 j = starta; j < end; j += 16)
		{
			const u32 cmask = get_code_mask(j, j + 16);

			if (cmask == 0) [[unlikely]]
			{
				continue;
			}

			// Interleave two threads
			auto& order = order0 > order1 ? order1 : order0;
			const auto& reg0 = order0 > order1 ? x86::xmm3 : x86::xmm0;
			const auto& reg1 = order0 > order1 ? x86::xmm4 : x86::xmm1;

			// Ensure small distance for disp8
			if (j - ls_off >= 256)
			{
				c->add(x86::rcx, j - ls_off);
				ls_off = j;
			}
			else if (j - ls_off >= 128)
			{
				c->sub(x86::rcx, -128);
				ls_off += 128;
			}

			if (code_off >= 128)
			{
				c->sub(x86::rax, -128);
				code_off -= 128;
			}

			// Determine which value will be duplicated at hole positions
			const u32 w3 = ::at32(func.data, (j - start + ~static_cast<u32>(std::countl_zero(cmask)) % 4 * 4) / 4);
			words.push_back(cmask & 1 ? func.data[(j - start + 0) / 4] : w3);
			words.push_back(cmask & 2 ? func.data[(j - start + 4) / 4] : w3);
			words.push_back(cmask & 4 ? func.data[(j - start + 8) / 4] : w3);
			words.push_back(w3);

			// PSHUFD immediate table for all possible hole mask values, holes repeat highest valid word
			static constexpr s32 s_pshufd_imm[16]
			{
				-1, // invalid index
				0b00000000, // copy 0
				0b01010101, // copy 1
				0b01010100, // copy 1
				0b10101010, // copy 2
				0b10101000, // copy 2
				0b10100110, // copy 2
				0b10100100, // copy 2
				0b11111111, // copy 3
				0b11111100, // copy 3
				0b11110111, // copy 3
				0b11110100, // copy 3
				0b11101111, // copy 3
				0b11101100, // copy 3
				0b11100111, // copy 3
				0b11100100, // full
			};

			const bool first = !order++;

			const auto& dest = first ? reg0 : reg1;

			// Load unaligned code block from LS
			if (cmask != 0xf)
			{
				if (utils::has_avx())
				{
					c->vpshufd(dest, x86::dqword_ptr(x86::rcx, j - ls_off), s_pshufd_imm[cmask]);
				}
				else
				{
					c->movups(dest, x86::dqword_ptr(x86::rcx, j - ls_off));
					c->pshufd(dest, dest, s_pshufd_imm[cmask]);
				}
			}
			else
			{
				c->movups(dest, x86::dqword_ptr(x86::rcx, j - ls_off));
			}

			// Perform bitwise comparison and accumulate
			c->xorps(dest, x86::dqword_ptr(x86::rax, code_off));

			if (!first)
			{
				c->orps(reg0, dest);
			}

			code_off += 16;
		}

		if (order1)
		{
			c->orps(x86::xmm0, x86::xmm3);
		}

		if (utils::has_sse41())
		{
			c->ptest(x86::xmm0, x86::xmm0);
			c->jnz(label_diff);
		}
		else
		{
			c->packssdw(x86::xmm0, x86::xmm0);
			c->movq(x86::rax, x86::xmm0);
			c->test(x86::rax, x86::rax);
			c->jne(label_diff);
		}
	}

	// Acknowledge success and add statistics
	c->add(SPU_OFF_64(block_counter), ::size32(words) / (words_align / 4));

	// Set block hash for profiling (if enabled)
	if (g_cfg.core.spu_prof)
	{
		c->mov(x86::rax, m_hash_start | 0xffff);
		c->mov(SPU_OFF_64(block_hash), x86::rax);
	}

	if (m_pos != start)
	{
		// Jump to the entry point if necessary
		c->jmp(instr_labels[m_pos]);
		m_pos = -1;
	}

	for (u32 i = 0; i < func.data.size(); i++)
	{
		const u32 pos = start + i * 4;
		const u32 op  = std::bit_cast<be_t<u32>>(func.data[i]);

		if (!op)
		{
			// Ignore hole
			if (m_pos + 1)
			{
				spu_log.error("Unexpected fallthrough to 0x%x", pos);
				branch_fixed(spu_branch_target(pos));
				m_pos = -1;
			}

			continue;
		}

		// Update position
		m_pos = pos;

		// Bind instruction label if necessary
		const auto found = instr_labels.find(pos);

		if (found != instr_labels.end())
		{
			if (m_preds.count(pos))
			{
				c->align(AlignMode::kCode, 16);
			}

			c->bind(found->second);
		}

		if (g_cfg.core.spu_debug)
		{
			// Write the instruction address inside the ASMJIT log
			compiler.comment(fmt::format("[0x%05x]", m_pos).c_str());
		}

		// Tracing
		//c->lea(x86::r14, get_pc(m_pos));

		// Execute recompiler function
		(this->*s_spu_decoder.decode(op))({op});

		// Collect allocated xmm vars
		for (u32 i = 0; i < vec_vars.size(); i++)
		{
			vec[i] = vec_vars[i];
		}
	}

	// Make fallthrough if necessary
	if (m_pos + 1)
	{
		branch_fixed(spu_branch_target(end));
	}

	// Simply return
	c->align(AlignMode::kCode, 16);
	c->bind(label_stop);
	c->add(x86::rsp, 0x28);
	c->ret();

	if (g_cfg.core.spu_verification)
	{
		// Dispatch
		c->align(AlignMode::kCode, 16);
		c->bind(label_diff);
		c->inc(SPU_OFF_64(block_failure));
		c->add(x86::rsp, 0x28);
		c->jmp(spu_runtime::tr_dispatch);
	}

	for (auto&& work : ::as_rvalue(std::move(after)))
	{
		work();
	}

	// Build instruction dispatch table
	if (instr_table.isValid())
	{
		c->align(AlignMode::kData, 8);
		c->bind(instr_table);

		// Get actual instruction table bounds
		const u32 start = instr_labels.begin()->first;
		const u32 end = instr_labels.rbegin()->first + 4;

		for (u32 addr = start; addr < end; addr += 4)
		{
			const auto found = instr_labels.find(addr);

			if (found != instr_labels.end())
			{
				c->embedLabel(found->second);
			}
			else
			{
				c->embedLabel(label_stop);
			}
		}
	}

	c->align(AlignMode::kData, words_align);
	c->bind(label_code);
	for (u32 d : words)
		c->dd(d);

	for (auto&& work : ::as_rvalue(std::move(consts)))
	{
		work();
	}

	label_stop.reset();
	instr_table.reset();
	instr_labels.clear();
	xmm_consts.clear();

	// Compile and get function address
	spu_function_t fn = reinterpret_cast<spu_function_t>(m_asmrt._add(&code));

	if (!fn)
	{
		spu_log.fatal("Failed to build a function");
	}
	else
	{
		jit_announce(fn, code.codeSize(), fmt::format("spu-b-%s", fmt::base57(be_t<u64>(m_hash_start))));
	}

	// Install compiled function pointer
	const bool added = !add_loc->compiled && add_loc->compiled.compare_and_swap_test(nullptr, fn);

	// Rebuild trampoline if necessary
	if (!m_spurt->rebuild_ubertrampoline(func.data[0]))
	{
		return nullptr;
	}

	if (added)
	{
		add_loc->compiled.notify_all();
	}

	if (g_cfg.core.spu_debug && added)
	{
		// Add ASMJIT logs
		fmt::append(log, "Address: %p\n\n", fn);
		log.append(logger._content.data(), logger._content.size());
		log += "\n\n\n";

		// Append log file
		fs::write_file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append, log);
	}

	return fn;
}

spu_recompiler::XmmLink spu_recompiler::XmmAlloc() // get empty xmm register
{
	for (auto& v : vec)
	{
		if (v) return{ v };
	}

	fmt::throw_exception("Out of Xmm Vars");
}

spu_recompiler::XmmLink spu_recompiler::XmmGet(s8 reg, XmmType type) // get xmm register with specific SPU reg
{
	XmmLink result = XmmAlloc();

	switch (type)
	{
	case XmmType::Int: c->movdqa(result, SPU_OFF_128(gpr, reg)); break;
	case XmmType::Float: c->movaps(result, SPU_OFF_128(gpr, reg)); break;
	case XmmType::Double: c->movapd(result, SPU_OFF_128(gpr, reg)); break;
	default: fmt::throw_exception("Invalid XmmType");
	}

	return result;
}

inline asmjit::x86::Mem spu_recompiler::XmmConst(const v128& data)
{
	// Find existing const
	auto& xmm_label = xmm_consts[std::make_pair(data._u64[0], data._u64[1])];

	if (!xmm_label.isValid())
	{
		xmm_label = c->newLabel();

		consts.emplace_back([=, this]
		{
			c->align(asmjit::AlignMode::kData, 16);
			c->bind(xmm_label);
			c->dq(data._u64[0]);
			c->dq(data._u64[1]);
		});
	}

	return asmjit::x86::oword_ptr(xmm_label);
}

inline asmjit::x86::Mem spu_recompiler::get_pc(u32 addr)
{
	return asmjit::x86::qword_ptr(*pc0, addr - m_base);
}

static void check_state(spu_thread* _spu)
{
	if (_spu->state && _spu->check_state())
	{
		spu_runtime::g_escape(_spu);
	}
}

void spu_recompiler::branch_fixed(u32 target, bool absolute)
{
	using namespace asmjit;

	// Check local branch
	const auto local = instr_labels.find(target);

	if (local != instr_labels.end() && local->second.isValid())
	{
		Label fail;

		if (absolute)
		{
			fail = c->newLabel();
			c->cmp(pc0->r32(), m_base);
			c->jne(fail);
		}

		c->cmp(SPU_OFF_32(state), 0);
		c->jz(local->second);
		c->lea(addr->r64(), get_pc(target));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(*arg0, *cpu);
		c->call(&check_state);
		c->jmp(local->second);

		if (absolute)
		{
			c->bind(fail);
		}
		else
		{
			return;
		}
	}

	const auto ppptr = !g_cfg.core.spu_verification ? nullptr : m_spurt->make_branch_patchpoint();

	if (absolute)
	{
		c->mov(SPU_OFF_32(pc), target);
	}
	else
	{
		c->lea(addr->r64(), get_pc(target));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
	}

	c->xor_(rip->r32(), rip->r32());
	c->cmp(SPU_OFF_32(state), 0);
	c->jnz(label_stop);

	if (ppptr)
	{
		c->add(x86::rsp, 0x28);
		c->jmp(ppptr);
	}
	else
	{
		c->jmp(label_stop);
	}
}

void spu_recompiler::branch_indirect(spu_opcode_t op, bool jt, bool ret)
{
	using namespace asmjit;

	// Initialize third arg to zero
	c->xor_(rip->r32(), rip->r32());

	if (op.d)
	{
		c->mov(SPU_OFF_8(interrupts_enabled), 0);
	}
	else if (op.e)
	{
		auto _throw = [](spu_thread* _spu)
		{
			_spu->state += cpu_flag::dbg_pause;
			spu_log.fatal("SPU Interrupts not implemented (mask=0x%x)", +_spu->ch_events.load().mask);
			spu_runtime::g_escape(_spu);
		};

		Label no_intr = c->newLabel();
		Label intr = c->newLabel();
		Label fail = c->newLabel();

		c->mov(*qw1, SPU_OFF_64(ch_events));
		c->ror(*qw1, 32);
		c->test(qw1->r32(), ~SPU_EVENT_INTR_IMPLEMENTED);
		c->ror(*qw1, 32);
		c->jnz(fail);
		c->mov(SPU_OFF_8(interrupts_enabled), 1);
		c->bt(qw1->r32(), 31);
		c->jc(intr);
		c->jmp(no_intr);
		c->bind(fail);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(*arg0, *cpu);
		c->add(x86::rsp, 0x28);
		c->jmp(+_throw);

		// Save addr in srr0 and disable interrupts
		c->bind(intr);
		c->mov(SPU_OFF_8(interrupts_enabled), 0);
		c->mov(SPU_OFF_32(srr0), *addr);

		// Test for BR/BRA instructions (they are equivalent at zero pc)
		c->mov(*addr, x86::dword_ptr(*ls));
		c->and_(*addr, 0xfffffffd);
		c->xor_(*addr, 0x30);
		c->bswap(*addr);
		c->test(*addr, 0xff80007f);
		c->cmovnz(*addr, rip->r32());
		c->shr(*addr, 5);
		c->align(AlignMode::kCode, 16);
		c->bind(no_intr);
	}

	c->mov(SPU_OFF_32(pc), *addr);
	c->cmp(SPU_OFF_32(state), 0);
	c->jnz(label_stop);

	if (g_cfg.core.spu_block_size != spu_block_size_type::safe && ret)
	{
		// Get stack pointer, try to use native return address (check SPU return address)
		Label fail = c->newLabel();
		c->mov(qw1->r32(), SPU_OFF_32(gpr, 1, &v128::_u32, 3));
		c->and_(qw1->r32(), 0x3fff0);
		c->lea(*qw1, x86::qword_ptr(*cpu, *qw1, 0, ::offset32(&spu_thread::stack_mirror)));
		c->cmp(x86::dword_ptr(*qw1, 8), *addr);
		c->jne(fail);
		c->mov(pc0->r32(), x86::dword_ptr(*qw1, 12));
		c->jmp(x86::qword_ptr(*qw1));
		c->bind(fail);
	}

	if (jt || g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		if (!instr_table.isValid())
		{
			// Request instruction table
			instr_table = c->newLabel();
		}

		// Get actual instruction table bounds
		const u32 start = instr_labels.begin()->first;
		const u32 end = instr_labels.rbegin()->first + 4;

		// Load local indirect jump address, check local bounds
		ensure(start == m_base);
		Label fail = c->newLabel();
		c->mov(qw1->r32(), *addr);
		c->sub(qw1->r32(), pc0->r32());
		c->cmp(qw1->r32(), end - start);
		c->jae(fail);
		c->lea(addr->r64(), x86::qword_ptr(instr_table));
		c->jmp(x86::qword_ptr(addr->r64(), *qw1, 1, 0));
		c->bind(fail);
	}

	// Simply external call (return or indirect call)
	const auto ppptr = !g_cfg.core.spu_verification ? nullptr : m_spurt->make_branch_patchpoint();

	if (ppptr)
	{
		c->add(x86::rsp, 0x28);
		c->jmp(ppptr);
	}
	else
	{
		c->jmp(label_stop);
	}
}

void spu_recompiler::branch_set_link(u32 target)
{
	using namespace asmjit;

	if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
	{
		// Find instruction at target
		const auto local = instr_labels.find(target);

		if (local != instr_labels.end() && local->second.isValid())
		{
			Label ret = c->newLabel();

			// Get stack pointer, write native and SPU return addresses into the stack mirror
			c->mov(qw1->r32(), SPU_OFF_32(gpr, 1, &v128::_u32, 3));
			c->and_(qw1->r32(), 0x3fff0);
			c->lea(*qw1, x86::qword_ptr(*cpu, *qw1, 0, ::offset32(&spu_thread::stack_mirror)));
			c->lea(x86::r10, x86::qword_ptr(ret));
			c->mov(x86::qword_ptr(*qw1, 0), x86::r10);
			c->lea(x86::r10, get_pc(target));
			c->and_(x86::r10d, 0x3fffc);
			c->mov(x86::dword_ptr(*qw1, 8), x86::r10d);
			c->mov(x86::dword_ptr(*qw1, 12), pc0->r32());

			after.emplace_back([=, this, target = local->second]
			{
				// Clear return info after use
				c->align(AlignMode::kCode, 16);
				c->bind(ret);
				c->mov(qw1->r32(), SPU_OFF_32(gpr, 1, &v128::_u32, 3));
				c->and_(qw1->r32(), 0x3fff0);
				c->pcmpeqd(x86::xmm0, x86::xmm0);
				c->movdqa(x86::dqword_ptr(*cpu, *qw1, 0, ::offset32(&spu_thread::stack_mirror)), x86::xmm0);

				// Set block hash for profiling (if enabled)
				if (g_cfg.core.spu_prof)
				{
					c->mov(x86::rax, m_hash_start | 0xffff);
					c->mov(SPU_OFF_64(block_hash), x86::rax);
				}

				c->jmp(target);
			});
		}
	}
}

void spu_recompiler::fall(spu_opcode_t op)
{
	auto gate = [](spu_thread* _spu, u32 opcode, spu_intrp_func_t _func)
	{
		if (!_func(*_spu, {opcode}))
		{
			_spu->state += cpu_flag::dbg_pause;
			spu_log.fatal("spu_recompiler::fall(): unexpected interpreter call (op=0x%08x)", opcode);
			spu_runtime::g_escape(_spu);
		}
	};

	c->lea(addr->r64(), get_pc(m_pos));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	c->mov(arg1->r32(), op.opcode);
	c->mov(*qw0, g_fxo->get<spu_interpreter_rt>().decode(op.opcode));
	c->mov(*arg0, *cpu);
	c->call(+gate);
}

void spu_recompiler::UNK(spu_opcode_t op)
{
	auto gate = [](spu_thread* _spu, u32 op)
	{
		_spu->state += cpu_flag::dbg_pause;
		spu_log.fatal("Unknown/Illegal instruction (0x%08x)", op);
		spu_runtime::g_escape(_spu);
	};

	c->lea(addr->r64(), get_pc(m_pos));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	c->mov(arg1->r32(), op.opcode);
	c->mov(*arg0, *cpu);
	c->add(asmjit::x86::rsp, 0x28);
	c->jmp(+gate);
	m_pos = -1;
}

void spu_stop(spu_thread* _spu, u32 code)
{
	if (!_spu->stop_and_signal(code) || _spu->state & cpu_flag::again)
	{
		spu_runtime::g_escape(_spu);
	}

	if (_spu->test_stopped())
	{
		_spu->pc += 4;
		spu_runtime::g_escape(_spu);
	}
}

void spu_recompiler::STOP(spu_opcode_t op)
{
	using namespace asmjit;

	Label ret = c->newLabel();
	c->lea(addr->r64(), get_pc(m_pos));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	c->mov(arg1->r32(), op.opcode & 0x3fff);
	c->mov(*arg0, *cpu);
	c->call(spu_stop);
	c->align(AlignMode::kCode, 16);
	c->bind(ret);

	c->add(SPU_OFF_32(pc), 4);

	if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
	{
		c->jmp(label_stop);
		m_pos = -1;
	}
}

void spu_recompiler::LNOP(spu_opcode_t)
{
}

void spu_recompiler::SYNC(spu_opcode_t)
{
	// This instruction must be used following a store instruction that modifies the instruction stream.
	c->lock().or_(asmjit::x86::dword_ptr(asmjit::x86::rsp), 0);

	if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
	{
		c->lea(addr->r64(), get_pc(m_pos + 4));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->jmp(label_stop);
		m_pos = -1;
	}
}

void spu_recompiler::DSYNC(spu_opcode_t)
{
	// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
	c->lock().or_(asmjit::x86::dword_ptr(asmjit::x86::rsp), 0);
}

void spu_recompiler::MFSPR(spu_opcode_t op)
{
	// Check SPUInterpreter for notes.
	const XmmLink& vr = XmmAlloc();
	c->pxor(vr, vr);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

static u32 spu_rdch(spu_thread* _spu, u32 ch)
{
	const s64 result = _spu->get_ch_value(ch);

	if (result < 0 || _spu->state & cpu_flag::again)
	{
		spu_runtime::g_escape(_spu);
	}

	if (_spu->test_stopped())
	{
		_spu->pc += 4;
		spu_runtime::g_escape(_spu);
	}

	return static_cast<u32>(result & 0xffffffff);
}

void spu_recompiler::RDCH(spu_opcode_t op)
{
	using namespace asmjit;

	auto read_channel = [&](x86::Mem channel_ptr, bool sync = true)
	{
		Label wait = c->newLabel();
		Label again = c->newLabel();
		Label ret = c->newLabel();
		c->mov(addr->r64(), channel_ptr);
		c->xor_(qw0->r32(), qw0->r32());
		c->align(AlignMode::kCode, 16);
		c->bind(again);
		c->bt(addr->r64(), spu_channel::off_count);
		c->jnc(wait);

		after.emplace_back([=, this, pos = m_pos]
		{
			c->bind(wait);
			c->lea(addr->r64(), get_pc(pos));
			c->and_(*addr, 0x3fffc);
			c->mov(SPU_OFF_32(pc), *addr);
			c->mov(arg1->r32(), +op.ra);
			c->mov(*arg0, *cpu);
			c->call(spu_rdch);
			c->jmp(ret);
		});

		if (sync)
		{
			// Channel is externally accessible
			c->lock().cmpxchg(channel_ptr, *qw0);
			c->jnz(again);
		}
		else
		{
			// Just write zero
			c->mov(channel_ptr, *qw0);
		}

		c->bind(ret);
		c->movd(x86::xmm0, *addr);
		c->pslldq(x86::xmm0, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), x86::xmm0);
	};

	switch (op.ra)
	{
	case SPU_RdSRR0:
	{
		const XmmLink& vr = XmmAlloc();
		c->movd(vr, SPU_OFF_32(srr0));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case SPU_RdInMbox:
	{
		// TODO
		break;
	}
	case MFC_RdTagStat:
	{
		read_channel(SPU_OFF_64(ch_tag_stat), false);
		return;
	}
	case MFC_RdTagMask:
	{
		const XmmLink& vr = XmmAlloc();
		c->movd(vr, SPU_OFF_32(ch_tag_mask));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case SPU_RdSigNotify1:
	{
		read_channel(SPU_OFF_64(ch_snr1));
		return;
	}
	case SPU_RdSigNotify2:
	{
		read_channel(SPU_OFF_64(ch_snr2));
		return;
	}
	case MFC_RdAtomicStat:
	{
		read_channel(SPU_OFF_64(ch_atomic_stat), false);
		return;
	}
	case MFC_RdListStallStat:
	{
		read_channel(SPU_OFF_64(ch_stall_stat), false);
		return;
	}
	case SPU_RdDec:
	{
		spu_log.warning("[0x%x] RDCH: RdDec", m_pos);

		auto sub1 = [](spu_thread* _spu, v128* _res)
		{
			const u32 out = _spu->ch_dec_value - static_cast<u32>(get_timebased_time() - _spu->ch_dec_start_timestamp);

			if (out > 1500)
			{
				_spu->state += cpu_flag::wait;
				std::this_thread::yield();

				if (_spu->test_stopped())
				{
					_spu->pc += 4;
					spu_runtime::g_escape(_spu);
				}
			}

			*_res = v128::from32r(out);
		};

		auto sub2 = [](spu_thread* _spu, v128* _res)
		{
			const u32 out = _spu->read_dec().first;

			*_res = v128::from32r(out);
		};

		c->lea(addr->r64(), get_pc(m_pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->lea(*arg1, SPU_OFF_128(gpr, op.rt));
		c->mov(*arg0, *cpu);
		c->call(g_cfg.core.spu_loop_detection ? +sub1 : +sub2);
		return;
	}
	case SPU_RdEventMask:
	{
		const XmmLink& vr = XmmAlloc();
		c->movq(vr, SPU_OFF_64(ch_events));
		c->psrldq(vr, 4);
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case SPU_RdEventStat:
	{
		spu_log.warning("[0x%x] RDCH: RdEventStat", m_pos);
		break; // TODO
	}
	case SPU_RdMachStat:
	{
		const XmmLink& vr = XmmAlloc();
		c->movzx(*addr, SPU_OFF_8(interrupts_enabled));
		c->mov(arg1->r32(), SPU_OFF_32(thread_type));
		c->and_(arg1->r32(), 2);
		c->or_(addr->r32(), arg1->r32());
		c->movd(vr, *addr);
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	default: break;
	}

	c->lea(addr->r64(), get_pc(m_pos));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	c->mov(arg1->r32(), +op.ra);
	c->mov(*arg0, *cpu);
	c->call(spu_rdch);
	c->movd(x86::xmm0, *addr);
	c->pslldq(x86::xmm0, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), x86::xmm0);
}

static u32 spu_rchcnt(spu_thread* _spu, u32 ch)
{
	return _spu->get_ch_count(ch);
}

void spu_recompiler::RCHCNT(spu_opcode_t op)
{
	using namespace asmjit;

	auto ch_cnt = [&](x86::Mem channel_ptr, bool inv = false)
	{
		// Load channel count
		const XmmLink& vr = XmmAlloc();
		c->movq(vr, channel_ptr);
		c->psrlq(vr, spu_channel::off_count);
		if (inv)
			c->pxor(vr, XmmConst(v128::from32p(1)));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	};

	switch (op.ra)
	{
	case SPU_WrOutMbox:       return ch_cnt(SPU_OFF_64(ch_out_mbox), true);
	case SPU_WrOutIntrMbox:   return ch_cnt(SPU_OFF_64(ch_out_intr_mbox), true);
	case MFC_RdTagStat:       return ch_cnt(SPU_OFF_64(ch_tag_stat));
	case MFC_RdListStallStat: return ch_cnt(SPU_OFF_64(ch_stall_stat));
	case SPU_RdSigNotify1:    return ch_cnt(SPU_OFF_64(ch_snr1));
	case SPU_RdSigNotify2:    return ch_cnt(SPU_OFF_64(ch_snr2));
	case MFC_RdAtomicStat:    return ch_cnt(SPU_OFF_64(ch_atomic_stat));

	case MFC_WrTagUpdate:
	{
		const XmmLink& vr = XmmAlloc();
		c->mov(addr->r32(), 1);
		c->movd(vr, addr->r32());
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}

	case MFC_Cmd:
	{
		const XmmLink& vr = XmmAlloc();
		const XmmLink& v1 = XmmAlloc();
		c->movdqa(vr, XmmConst(v128::from32p(16)));
		c->movd(v1, SPU_OFF_32(mfc_size));
		c->psubd(vr, v1);
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}

	case SPU_RdInMbox:
	{
		const XmmLink& vr = XmmAlloc();
		c->movdqa(vr, SPU_OFF_128(ch_in_mbox));
		c->pslldq(vr, 14);
		c->psrldq(vr, 3);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	// Channels with a constant count of 1:
	case SPU_WrEventMask:
	case SPU_WrEventAck:
	case SPU_WrDec:
	case SPU_RdDec:
	case SPU_RdEventMask:
	case SPU_RdMachStat:
	case SPU_WrSRR0:
	case SPU_RdSRR0:
	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	case MFC_RdTagMask:
	case MFC_LSA:
	case MFC_EAH:
	case MFC_EAL:
	case MFC_Size:
	case MFC_TagID:
	case MFC_WrTagMask:
	case MFC_WrListStallAck:
	{
		const XmmLink& vr = XmmAlloc();
		c->mov(addr->r32(), 1);
		c->movd(vr, addr->r32());
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case SPU_RdEventStat:
	{
		spu_log.warning("[0x%x] RCHCNT: RdEventStat", m_pos);
		[[fallthrough]]; // fallback
	}
	default:
	{
		c->lea(addr->r64(), get_pc(m_pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(arg1->r32(), +op.ra);
		c->mov(*arg0, *cpu);
		c->call(spu_rchcnt);
		break;
	}
	}

	// Use result from the third argument
	c->movd(x86::xmm0, *addr);
	c->pslldq(x86::xmm0, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), x86::xmm0);
}

void spu_recompiler::SF(spu_opcode_t op)
{
	// sub from
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubd(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::OR(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->por(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::BG(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();

	if (utils::has_avx512())
	{
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		c->vpsubd(vi, vb, va);
		c->vpternlogd(va, vb, vi, 0x4d /* B?nandAC:norAC */);
		c->psrld(va, 31);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtd(va, vi);
	c->paddd(va, XmmConst(v128::from32p(1)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SFH(spu_opcode_t op)
{
	// sub from (halfword)
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubw(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::NOR(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);

	if (utils::has_avx512())
	{
		c->vpternlogd(va, va, SPU_OFF_128(gpr, op.rb), 0x11 /* norCB */);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	c->por(va, SPU_OFF_128(gpr, op.rb));
	c->pxor(va, XmmConst(v128::from32p(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ABSDB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vm = XmmAlloc();
	c->movdqa(vm, va);
	c->pmaxub(va, vb);
	c->pminub(vb, vm);
	c->psubb(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROT(spu_opcode_t op)
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->vprolvd(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		c->movdqa(v4, XmmConst(v128::from32p(0x1f)));
		c->pand(vb, v4);
		c->vpsllvd(vt, va, vb);
		c->psubd(vb, XmmConst(v128::from32p(1)));
		c->pandn(vb, v4);
		c->vpsrlvd(va, va, vb);
		c->por(vt, va);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->vprotd(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 4; i++) // unrolled loop
	{
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
		c->mov(asmjit::x86::ecx, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
		c->rol(qw0->r32(), asmjit::x86::cl);
		c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
	}
}

void spu_recompiler::ROTM(spu_opcode_t op)
{
	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubd(vb, XmmConst(v128::from32p(1)));
		c->pandn(vb, XmmConst(v128::from32p(0x3f)));
		c->vpsrlvd(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubd(vb, XmmConst(v128::from32p(1)));
		c->pandn(vb, XmmConst(v128::from32p(0x3f)));
		c->pxor(vt, vt);
		c->psubd(vt, vb);
		c->pcmpgtd(vb, XmmConst(v128::from32p(31)));
		c->vpshld(vt, va, vt);
		c->vpandn(vt, vb, vt);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 4; i++) // unrolled loop
	{
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
		c->mov(asmjit::x86::ecx, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
		c->neg(asmjit::x86::ecx);
		c->shr(*qw0, asmjit::x86::cl);
		c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
	}
}

void spu_recompiler::ROTMA(spu_opcode_t op)
{
	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubd(vb, XmmConst(v128::from32p(1)));
		c->pandn(vb, XmmConst(v128::from32p(0x3f)));
		c->vpsravd(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubd(vb, XmmConst(v128::from32p(1)));
		c->pandn(vb, XmmConst(v128::from32p(0x3f)));
		c->pxor(vt, vt);
		c->pminud(vb, XmmConst(v128::from32p(31)));
		c->psubd(vt, vb);
		c->vpshad(vt, va, vt);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 4; i++) // unrolled loop
	{
		c->movsxd(*qw0, SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
		c->mov(asmjit::x86::ecx, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
		c->neg(asmjit::x86::ecx);
		c->sar(*qw0, asmjit::x86::cl);
		c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
	}
}

void spu_recompiler::SHL(spu_opcode_t op)
{
	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->pand(vb, XmmConst(v128::from32p(0x3f)));
		c->vpsllvd(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->pand(vb, XmmConst(v128::from32p(0x3f)));
		c->vpcmpgtd(vt, vb, XmmConst(v128::from32p(31)));
		c->vpshld(vb, va, vb);
		c->pandn(vt, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 4; i++) // unrolled loop
	{
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
		c->mov(asmjit::x86::ecx, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
		c->shl(*qw0, asmjit::x86::cl);
		c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
	}
}

void spu_recompiler::ROTH(spu_opcode_t op) //nf
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		c->vmovdqa(v4, XmmConst(v128::from32r(0x0d0c0d0c, 0x09080908, 0x05040504, 0x01000100)));
		c->vpshufb(vt, va, v4); // duplicate low word
		c->vpsrld(va, va, 16);
		c->vpshufb(va, va, v4);
		c->vpsrld(v4, vb, 16);
		c->vprolvd(va, va, v4);
		c->vprolvd(vb, vt, vb);
		c->vpblendw(vt, vb, va, 0xaa);
		c->vmovdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->vprotw(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 8; i++) // unrolled loop
	{
		c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
		c->movzx(asmjit::x86::ecx, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
		c->rol(qw0->r16(), asmjit::x86::cl);
		c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
	}
}

void spu_recompiler::ROTHM(spu_opcode_t op)
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->pandn(vb, XmmConst(v128::from16p(0x1f)));
		c->vpsrlvw(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		const XmmLink& v5 = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->pandn(vb, XmmConst(v128::from16p(0x1f)));
		c->movdqa(vt, XmmConst(v128::from32p(0xffff0000))); // mask: select high words
		c->vpsrld(v4, vb, 16);
		c->vpsubusw(v5, vb, vt); // clear high words (using saturation sub for throughput)
		c->vpandn(vb, vt, va); // clear high words
		c->vpsrlvd(va, va, v4);
		c->vpsrlvd(vb, vb, v5);
		c->vpblendw(vt, vb, va, 0xaa); // can use vpblendvb with 0xffff0000 mask (vt)
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->pandn(vb, XmmConst(v128::from16p(0x1f)));
		c->pxor(vt, vt);
		c->psubw(vt, vb);
		c->pcmpgtw(vb, XmmConst(v128::from16p(15)));
		c->vpshlw(vt, va, vt);
		c->vpandn(vt, vb, vt);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 8; i++) // unrolled loop
	{
		c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
		c->movzx(asmjit::x86::ecx, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
		c->neg(asmjit::x86::ecx);
		c->shr(qw0->r32(), asmjit::x86::cl);
		c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
	}
}

void spu_recompiler::ROTMAH(spu_opcode_t op)
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->pandn(vb, XmmConst(v128::from16p(0x1f)));
		c->vpsravw(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		const XmmLink& v5 = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->movdqa(vt, XmmConst(v128::from16p(0x1f)));
		c->vpandn(v4, vb, vt);
		c->vpand(v5, vb, vt);
		c->movdqa(vt, XmmConst(v128::from32p(0x2f)));
		c->vpsrld(v4, v4, 16);
		c->vpsubusw(v5, vt, v5); // clear high word and add 16 to low word
		c->vpslld(vb, va, 16);
		c->vpsravd(va, va, v4);
		c->vpsravd(vb, vb, v5);
		c->vpblendw(vt, vb, va, 0xaa);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->psubw(vb, XmmConst(v128::from16p(1)));
		c->pandn(vb, XmmConst(v128::from16p(0x1f)));
		c->pxor(vt, vt);
		c->pminuw(vb, XmmConst(v128::from16p(15)));
		c->psubw(vt, vb);
		c->vpshaw(vt, va, vt);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 8; i++) // unrolled loop
	{
		c->movsx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
		c->movzx(asmjit::x86::ecx, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
		c->neg(asmjit::x86::ecx);
		c->sar(qw0->r32(), asmjit::x86::cl);
		c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
	}
}

void spu_recompiler::SHLH(spu_opcode_t op)
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->pand(vb, XmmConst(v128::from16p(0x1f)));
		c->vpsllvw(vt, va, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_avx2())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		const XmmLink& v5 = XmmAlloc();
		c->pand(vb, XmmConst(v128::from16p(0x1f)));
		c->movdqa(vt, XmmConst(v128::from32p(0xffff0000))); // mask: select high words
		c->vpsrld(v4, vb, 16);
		c->vpsubusw(v5, vb, vt); // clear high words (using saturation sub for throughput)
		c->vpand(vb, vt, va); // clear low words
		c->vpsllvd(va, va, v5);
		c->vpsllvd(vb, vb, v4);
		c->vpblendw(vt, vb, va, 0x55);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->pand(vb, XmmConst(v128::from16p(0x1f)));
		c->vpcmpgtw(vt, vb, XmmConst(v128::from16p(15)));
		c->vpshlw(vb, va, vb);
		c->pandn(vt, vb);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	for (u32 i = 0; i < 8; i++) // unrolled loop
	{
		c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
		c->movzx(asmjit::x86::ecx, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
		c->shl(qw0->r32(), asmjit::x86::cl);
		c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
	}
}

void spu_recompiler::ROTI(spu_opcode_t op)
{
	// rotate left
	const int s = op.i7 & 0x1f;

	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		c->vprold(va, va, s);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	if (utils::has_xop())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		c->vprotd(va, va, s);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pslld(va, s);
	c->psrld(v1, 32 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMI(spu_opcode_t op)
{
	// shift right logical
	const int s = (0 - op.i7) & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrld(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMAI(spu_opcode_t op)
{
	// shift right arithmetical
	const int s = (0 - op.i7) & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrad(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLI(spu_opcode_t op)
{
	// shift left
	const int s = op.i7 & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTHI(spu_opcode_t op)
{
	// rotate left (halfword)
	const int s = op.i7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->psllw(va, s);
	c->psrlw(v1, 16 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTHMI(spu_opcode_t op)
{
	// shift right logical
	const int s = (0 - op.i7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrlw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMAHI(spu_opcode_t op)
{
	// shift right arithmetical (halfword)
	const int s = (0 - op.i7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psraw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLHI(spu_opcode_t op)
{
	// shift left (halfword)
	const int s = op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::A(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->paddd(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::AND(spu_opcode_t op)
{
	// and
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pand(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::CG(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();

	if (utils::has_avx512())
	{
		c->vpaddd(vi, vb, va);
		c->vpternlogd(vi, va, vb, 0x8e /* A?andBC:orBC */);
		c->psrld(vi, 31);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vi);
		return;
	}

	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->paddd(vb, va);
	c->pxor(va, vi);
	c->pxor(vb, vi);
	c->pcmpgtd(va, vb);
	c->psrld(va, 31);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::NAND(spu_opcode_t op)
{
	// nand
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);

	if (utils::has_avx512())
	{
		c->vpternlogd(va, va, SPU_OFF_128(gpr, op.rb), 0x77 /* nandCB */);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	c->pand(va, SPU_OFF_128(gpr, op.rb));
	c->pxor(va, XmmConst(v128::from32p(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AVGB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pavgb(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::MTSPR(spu_opcode_t)
{
	// Check SPUInterpreter for notes.
}

static void spu_wrch(spu_thread* _spu, u32 ch, u32 value)
{
	if (!_spu->set_ch_value(ch, value) || _spu->state & cpu_flag::again)
	{
		spu_runtime::g_escape(_spu);
	}

	if (_spu->test_stopped())
	{
		_spu->pc += 4;
		spu_runtime::g_escape(_spu);
	}
}

static void spu_wrch_mfc(spu_thread* _spu)
{
	if (!_spu->process_mfc_cmd() || _spu->state & cpu_flag::again)
	{
		spu_runtime::g_escape(_spu);
	}

	if (_spu->test_stopped())
	{
		_spu->pc += 4;
		spu_runtime::g_escape(_spu);
	}
}

void spu_recompiler::WRCH(spu_opcode_t op)
{
	using namespace asmjit;

	switch (op.ra)
	{
	case SPU_WrSRR0:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(srr0), *addr);
		return;
	}
	case SPU_WrOutIntrMbox:
	{
		// Can't seemingly be optimized
		break;
	}
	case SPU_WrOutMbox:
	{
		Label wait = c->newLabel();
		Label again = c->newLabel();
		Label ret = c->newLabel();
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(addr->r64(), SPU_OFF_64(ch_out_mbox));
		c->align(AlignMode::kCode, 16);
		c->bind(again);
		c->mov(qw0->r32(), qw0->r32());
		c->bt(addr->r64(), spu_channel::off_count);
		c->jc(wait);

		after.emplace_back([=, this, pos = m_pos]
		{
			c->bind(wait);
			c->lea(addr->r64(), get_pc(pos));
			c->and_(*addr, 0x3fffc);
			c->mov(SPU_OFF_32(pc), *addr);
			c->mov(arg1->r32(), +op.ra);
			c->mov(*arg0, *cpu);
			c->call(spu_wrch);
			c->jmp(ret);
		});

		c->bts(*qw0, spu_channel::off_count);
		c->lock().cmpxchg(SPU_OFF_64(ch_out_mbox), *qw0);
		c->jnz(again);
		c->bind(ret);
		return;
	}
	case MFC_WrTagMask:
	{
		Label upd = c->newLabel();
		Label ret = c->newLabel();
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_tag_mask), qw0->r32());
		c->cmp(SPU_OFF_32(ch_tag_upd), MFC_TAG_UPDATE_IMMEDIATE);
		c->jnz(upd);

		after.emplace_back([=, this, pos = m_pos]
		{
			c->bind(upd);
			c->lea(addr->r64(), get_pc(pos));
			c->and_(*addr, 0x3fffc);
			c->mov(SPU_OFF_32(pc), *addr);
			c->mov(arg1->r32(), MFC_WrTagMask);
			c->mov(*arg0, *cpu);
			c->call(spu_wrch);
			c->jmp(ret);
		});

		c->bind(ret);
		return;
	}
	case MFC_WrTagUpdate:
	{
		Label fail = c->newLabel();
		Label zero = c->newLabel();
		Label ret = c->newLabel();
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->cmp(qw0->r32(), 2);
		c->ja(fail);

		after.emplace_back([=, this, pos = m_pos]
		{
			c->bind(fail);
			c->lea(addr->r64(), get_pc(pos));
			c->and_(*addr, 0x3fffc);
			c->mov(SPU_OFF_32(pc), *addr);
			c->mov(arg1->r32(), +op.ra);
			c->mov(*arg0, *cpu);
			c->call(spu_wrch);
			c->jmp(ret);

			c->bind(zero);
			c->mov(SPU_OFF_32(ch_tag_upd), qw0->r32());
			c->jmp(ret);
		});

		// addr = completed mask, will be compared with qw1
		c->mov(*addr, SPU_OFF_32(mfc_fence));
		c->not_(*addr);
		c->and_(*addr, SPU_OFF_32(ch_tag_mask));
		c->mov(qw1->r32(), *addr);
		c->test(*addr, *addr);
		c->cmovz(qw1->r32(), qw0->r32());
		c->cmp(qw0->r32(), 1);
		c->cmovb(qw1->r32(), *addr);
		c->cmova(qw1->r32(), SPU_OFF_32(ch_tag_mask));
		c->cmp(*addr, qw1->r32());
		c->jne(zero);
		c->bts(addr->r64(), spu_channel::off_count);
		c->mov(SPU_OFF_32(ch_tag_upd), 0);
		c->mov(SPU_OFF_64(ch_tag_stat), addr->r64());
		c->bind(ret);
		return;
	}
	case MFC_LSA:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::lsa), *addr);
		return;
	}
	case MFC_EAH:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::eah), *addr);
		return;
	}
	case MFC_EAL:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::eal), *addr);
		return;
	}
	case MFC_Size:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->and_(*addr, 0x7fff);
		c->mov(SPU_OFF_16(ch_mfc_cmd, &spu_mfc_cmd::size), addr->r16());
		return;
	}
	case MFC_TagID:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->and_(*addr, 0x1f);
		c->mov(SPU_OFF_8(ch_mfc_cmd, &spu_mfc_cmd::tag), addr->r8());
		return;
	}
	case MFC_Cmd:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_8(ch_mfc_cmd, &spu_mfc_cmd::cmd), addr->r8());
		c->lea(addr->r64(), get_pc(m_pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(*arg0, *cpu);
		c->call(spu_wrch_mfc);
		return;
	}
	case MFC_WrListStallAck:
	{
		auto sub = [](spu_thread* _spu, u32 tag)
		{
			for (u32 i = 0; i < _spu->mfc_size; i++)
			{
				if (_spu->mfc_queue[i].tag == (tag | 0x80))
				{
					// Unset stall bit
					_spu->mfc_queue[i].tag &= 0x7f;
				}
			}

			_spu->do_mfc(true);
		};

		Label ret = c->newLabel();
		c->mov(arg1->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->and_(arg1->r32(), 0x1f);
		c->btr(SPU_OFF_32(ch_stall_mask), arg1->r32());
		c->jnc(ret);
		c->mov(*arg0, *cpu);
		c->call(+sub);
		c->bind(ret);
		return;
	}
	case SPU_WrDec:
	{
		auto sub = [](spu_thread* _spu)
		{
			_spu->get_events(SPU_EVENT_TM);
			_spu->ch_dec_start_timestamp = get_timebased_time();
		};

		c->mov(*arg0, *cpu);
		c->call(+sub);
		c->mov(qw0->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_dec_value), qw0->r32());
		c->mov(SPU_OFF_8(is_dec_frozen), 0);
		return;
	}
	case SPU_WrEventMask:
	{
		// TODO
		break;
	}
	case SPU_WrEventAck:
	{
		// TODO
		break;
	}
	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	{
		return;
	}
	default: break;
	}

	c->lea(addr->r64(), get_pc(m_pos));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	c->mov(arg1->r32(), +op.ra);
	c->mov(qw0->r32(), SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
	c->mov(*arg0, *cpu);
	c->call(spu_wrch);
}

void spu_recompiler::BIZ(spu_opcode_t op)
{
	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->je(branch_label);

	after.emplace_back([=, this, jt = m_targets[m_pos].size() > 1]
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
		c->and_(*addr, 0x3fffc);
		branch_indirect(op, jt);
	});
}

void spu_recompiler::BINZ(spu_opcode_t op)
{
	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->jne(branch_label);

	after.emplace_back([=, this, jt = m_targets[m_pos].size() > 1]
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
		c->and_(*addr, 0x3fffc);
		branch_indirect(op, jt);
	});
}

void spu_recompiler::BIHZ(spu_opcode_t op)
{
	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->je(branch_label);

	after.emplace_back([=, this, jt = m_targets[m_pos].size() > 1]
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
		c->and_(*addr, 0x3fffc);
		branch_indirect(op, jt);
	});
}

void spu_recompiler::BIHNZ(spu_opcode_t op)
{
	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->jne(branch_label);

	after.emplace_back([=, this, jt = m_targets[m_pos].size() > 1]
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
		c->and_(*addr, 0x3fffc);
		branch_indirect(op, jt);
	});
}

void spu_recompiler::STOPD(spu_opcode_t)
{
	STOP(spu_opcode_t{0x3fff});
}

void spu_recompiler::STQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(asmjit::x86::oword_ptr(*ls, addr->r64()), vt);
	}
	else
	{
		c->mov(*qw0, SPU_OFF_64(gpr, op.rt, &v128::_u64, 0));
		c->mov(*qw1, SPU_OFF_64(gpr, op.rt, &v128::_u64, 1));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0), *qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8), *qw0);
	}
}

void spu_recompiler::BI(spu_opcode_t op)
{
	const auto found = m_targets.find(m_pos);
	const auto is_jt = found == m_targets.end() || found->second.size() > 1;

	if (found == m_targets.end())
	{
		spu_log.error("[0x%x] BI: no targets", m_pos);
	}
	else if (op.d && found->second.size() == 1 && found->second[0] == spu_branch_target(m_pos, 1))
	{
		// Interrupts-disable pattern
		c->mov(SPU_OFF_8(interrupts_enabled), 0);
		return;
	}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	branch_indirect(op, is_jt, !is_jt);
	m_pos = -1;
}

void spu_recompiler::BISL(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	const XmmLink& vr = XmmAlloc();
	c->lea(*qw0, get_pc(m_pos + 4));
	c->and_(qw0->r32(), 0x3fffc);
	c->movd(vr, qw0->r32());
	c->pslldq(vr, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	branch_set_link(m_pos + 4);
	branch_indirect(op, true, false);
	m_pos = -1;
}

void spu_recompiler::IRET(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(srr0));
	branch_indirect(op);
	m_pos = -1;
}

void spu_recompiler::BISLED(spu_opcode_t op)
{
	auto get_events = [](spu_thread* _spu) -> u32
	{
		return _spu->get_events(_spu->ch_events.load().mask).count;
	};

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));

	const XmmLink& vr = XmmAlloc();
	c->lea(*qw0, get_pc(m_pos + 4));
	c->movd(vr, qw0->r32());
	c->pand(vr, XmmConst(v128::from32p(0x3fffc)));
	c->pslldq(vr, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);

	asmjit::Label branch_label = c->newLabel();
	c->mov(*arg0, *cpu);
	c->call(+get_events);
	c->test(*addr, 1);
	c->jne(branch_label);

	after.emplace_back([=, this]()
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		c->and_(*addr, 0x3fffc);
		branch_indirect(op, true, false);
	});
}

void spu_recompiler::HBR([[maybe_unused]] spu_opcode_t op)
{
}

void spu_recompiler::GB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, 31);
	c->movmskps(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::GBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, 15);
	c->packsswb(va, XmmConst({}));
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::GBB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FSM(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vm = XmmAlloc();
	c->pshufd(va, va, 0xff);
	c->movdqa(vm, XmmConst(v128::from32r(8, 4, 2, 1)));
	c->pand(va, vm);
	c->pcmpeqd(va, vm);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FSMH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vm = XmmAlloc();
	c->punpckhwd(va, va);
	c->pshufd(va, va, 0xaa);
	c->movdqa(vm, XmmConst(v128::from64r(0x0080004000200010, 0x0008000400020001)));
	c->pand(va, vm);
	c->pcmpeqw(va, vm);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FSMB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vm = XmmAlloc();

	if (utils::has_ssse3())
	{
		c->pshufb(va, XmmConst(v128::from64r(0x0d0d0d0d0d0d0d0d, 0x0c0c0c0c0c0c0c0c)));
	}
	else
	{
		c->punpckhbw(va, va);
		c->pshufhw(va, va, 0x50);
		c->pshufd(va, va, 0xfa);
	}

	c->movdqa(vm, XmmConst(v128::from64p(0x8040201008040201)));
	c->pand(va, vm);
	c->pcmpeqb(va, vm);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FREST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& v_fraction = XmmAlloc();
	const XmmLink& v_exponent = XmmAlloc();
	const XmmLink& v_sign = XmmAlloc();
	c->movdqa(v_fraction, va);
	c->movdqa(v_exponent, va);
	c->movdqa(v_sign, va);

	c->psrld(v_fraction, 18);
	c->psrld(v_exponent, 23);

	c->andps(v_fraction, XmmConst(v128::from32p(0x1F)));
	c->andps(v_exponent, XmmConst(v128::from32p(0xFF)));
	c->andps(v_sign, XmmConst(v128::from32p(0x80000000)));

	const u64 fraction_lut_addr = reinterpret_cast<u64>(spu_frest_fraction_lut);
	const u64 exponent_lut_addr = reinterpret_cast<u64>(spu_frest_exponent_lut);

	c->movabs(*arg0, fraction_lut_addr);
	c->movabs(*arg1, exponent_lut_addr);

	for (u32 index = 0; index < 4; index++)
	{
		c->pextrd(*qw0, v_fraction, index);
		c->mov(*qw1, asmjit::x86::dword_ptr(*arg0, *qw0, 2));
		c->pinsrd(v_fraction, *qw1, index);

		c->pextrd(*qw0, v_exponent, index);
		c->mov(*qw1, asmjit::x86::dword_ptr(*arg1, *qw0, 2));
		c->pinsrd(v_exponent, *qw1, index);
	}

	// AVX2(not working?)
	// c->mov(qw1->r64(),spu_frest_fraction_lut);
	// c->vpgatherdd(v_fraction, asmjit::x86::dword_ptr(*qw1));
	// c->mov(qw0->r64(),spu_frest_exponent_lut);
	// c->vpgatherdd(v_exponent, asmjit::x86::dword_ptr(*qw0));

	c->orps(v_fraction, v_exponent);
	c->orps(v_sign, v_fraction);

	c->movaps(SPU_OFF_128(gpr, op.rt), v_sign);
}

void spu_recompiler::FRSQEST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& v_fraction = XmmAlloc();
	const XmmLink& v_exponent = XmmAlloc();
	c->movdqa(v_fraction, va);
	c->movdqa(v_exponent, va);

	c->psrld(v_fraction, 18);
	c->psrld(v_exponent, 23);

	c->andps(v_fraction, XmmConst(v128::from32p(0x3F)));
	c->andps(v_exponent, XmmConst(v128::from32p(0xFF)));

	const u64 fraction_lut_addr = reinterpret_cast<u64>(spu_frsqest_fraction_lut);
	const u64 exponent_lut_addr = reinterpret_cast<u64>(spu_frsqest_exponent_lut);

	c->movabs(*arg0, fraction_lut_addr);
	c->movabs(*arg1, exponent_lut_addr);

	for (u32 index = 0; index < 4; index++)
	{
		c->pextrd(*qw0, v_fraction, index);
		c->mov(*qw1, asmjit::x86::dword_ptr(*arg0, *qw0, 2));
		c->pinsrd(v_fraction, *qw1, index);

		c->pextrd(*qw0, v_exponent, index);
		c->mov(*qw1, asmjit::x86::dword_ptr(*arg1, *qw0, 2));
		c->pinsrd(v_exponent, *qw1, index);
	}

	c->orps(v_fraction, v_exponent);

	c->movaps(SPU_OFF_128(gpr, op.rt), v_fraction);
}

void spu_recompiler::LQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmAlloc();
		c->movdqa(vt, asmjit::x86::oword_ptr(*ls, addr->r64()));
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	}
	else
	{
		c->mov(*qw0, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0));
		c->mov(*qw1, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw0);
	}
}

void spu_recompiler::ROTQBYBI(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.rldq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0xf << 3);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64(), 1));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTQMBYBI(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.srdq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f << 3);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64(), 1));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLQBYBI(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.sldq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f << 3);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64(), 1));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CBX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xf);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::byte_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x03);
}

void spu_recompiler::CHX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xe);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::word_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x0203);
}

void spu_recompiler::CWX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xc);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::dword_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x00010203);
}

void spu_recompiler::CDX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0x8);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(*qw0, asmjit::Imm(0x0001020304050607ull));
	c->mov(asmjit::x86::qword_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), *qw0);
}

void spu_recompiler::ROTQBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	const XmmLink& v4 = XmmAlloc();
	c->psrldq(vb, 12);
	c->pand(vb, XmmConst(v128::from64r(0, 7)));
	c->movdqa(v4, XmmConst(v128::from64r(0, 64)));
	c->pshufd(vt, va, 0x4e);
	c->psubq(v4, vb);
	c->psllq(va, vb);
	c->psrlq(vt, v4);
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ROTQMBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmAlloc();
	const XmmLink& vt = XmmGet(op.rb, XmmType::Int);
	const XmmLink& v4 = XmmAlloc();
	c->psrldq(vt, 12);
	c->pxor(vb, vb);
	c->psubq(vb, vt);
	c->pand(vb, XmmConst(v128::from64r(0, 7)));
	c->movdqa(v4, XmmConst(v128::from64r(0, 64)));
	c->movdqa(vt, va);
	c->psrldq(vt, 8);
	c->psubq(v4, vb);
	c->psrlq(va, vb);
	c->psllq(vt, v4);
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::SHLQBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	const XmmLink& v4 = XmmAlloc();
	c->psrldq(vb, 12);
	c->pand(vb, XmmConst(v128::from64r(0, 7)));
	c->movdqa(v4, XmmConst(v128::from64r(0, 64)));
	c->movdqa(vt, va);
	c->pslldq(vt, 8);
	c->psubq(v4, vb);
	c->psllq(va, vb);
	c->psrlq(vt, v4);
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ROTQBY(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.rldq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0xf);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64()));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTQMBY(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.srdq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64()));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLQBY(spu_opcode_t op)
{
	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, +g_spu_imm.sldq_pshufb);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, addr->r64()));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORX(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->pshufd(v1, va, 0xb1);
	c->por(va, v1);
	c->pshufd(v1, va, 0x4e);
	c->por(va, v1);
	c->pslldq(va, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CBD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u8r[op.i7 & 0xf] = 0x03;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, +op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xf);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::byte_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x03);
}

void spu_recompiler::CHD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u16r[(op.i7 >> 1) & 0x7] = 0x0203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, +op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xe);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::word_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x0203);
}

void spu_recompiler::CWD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u32r[(op.i7 >> 2) & 0x3] = 0x00010203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, +op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xc);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::dword_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), 0x00010203);
}

void spu_recompiler::CDD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u64r[(op.i7 >> 3) & 0x1] = 0x0001020304050607ull;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, +op.i7);
	c->not_(*addr);
	c->and_(*addr, 0x8);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32r(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(*qw0, asmjit::Imm(0x0001020304050607ull));
	c->mov(asmjit::x86::qword_ptr(*cpu, addr->r64(), 0, offset32(&spu_thread::gpr, op.rt)), *qw0);
}

void spu_recompiler::ROTQBII(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	c->pshufd(vt, va, 0x4e); // swap 64-bit parts
	c->psllq(va, (op.i7 & 0x7));
	c->psrlq(vt, 64 - (op.i7 & 0x7));
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ROTQMBII(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, va);
	c->psrldq(vt, 8);
	c->psrlq(va, ((0 - op.i7) & 0x7));
	c->psllq(vt, 64 - ((0 - op.i7) & 0x7));
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::SHLQBII(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, va);
	c->pslldq(vt, 8);
	c->psllq(va, (op.i7 & 0x7));
	c->psrlq(vt, 64 - (op.i7 & 0x7));
	c->por(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ROTQBYI(spu_opcode_t op)
{
	const int s = op.i7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v2 = XmmAlloc();

	if (s == 0)
	{
	}
	else if (s == 4 || s == 8 || s == 12)
	{
		c->pshufd(va, va, utils::rol8(0xE4, s / 2));
	}
	else if (utils::has_ssse3())
	{
		c->palignr(va, va, 16 - s);
	}
	else
	{
		c->movdqa(v2, va);
		c->psrldq(va, 16 - s);
		c->pslldq(v2, s);
		c->por(va, v2);
	}

	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTQMBYI(spu_opcode_t op)
{
	const int s = (0 - op.i7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrldq(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLQBYI(spu_opcode_t op)
{
	const int s = op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslldq(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::NOP(spu_opcode_t)
{
}

void spu_recompiler::CGT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XOR(spu_opcode_t op)
{
	// xor
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::EQV(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);

	if (utils::has_avx512())
	{
		c->vpternlogd(vb, vb, SPU_OFF_128(gpr, op.ra), 0x99 /* xnorCB */);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
		return;
	}

	c->pxor(vb, XmmConst(v128::from32p(0xffffffff)));
	c->pxor(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::CGTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SUMB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	c->movdqa(v2, XmmConst(v128::from16p(0xff)));
	c->movdqa(v1, va);
	c->psrlw(va, 8);
	c->pand(v1, v2);
	c->pand(v2, vb);
	c->psrlw(vb, 8);
	c->paddw(va, v1);
	c->paddw(vb, v2);
	c->movdqa(v2, XmmConst(v128::from32p(0xffff)));
	c->movdqa(v1, va);
	c->psrld(va, 16);
	c->pand(v1, v2);
	c->pandn(v2, vb);
	c->pslld(vb, 16);
	c->paddw(va, v1);
	c->paddw(vb, v2);
	c->por(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_s32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_s32, 3));

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->jg(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::CLZ(spu_opcode_t op)
{
	if (utils::has_avx512())
	{
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		c->vplzcntd(vt, va);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
		return;
	}

	c->mov(qw0->r32(), 32 + 31);
	for (u32 i = 0; i < 4; i++) // unrolled loop
	{
		c->bsr(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
		c->cmovz(*addr, qw0->r32());
		c->xor_(*addr, 31);
		c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), *addr);
	}
}

void spu_recompiler::XSWD(spu_opcode_t op)
{
	c->movsxd(*qw0, SPU_OFF_32(gpr, op.ra, &v128::_s32, 0));
	c->movsxd(*qw1, SPU_OFF_32(gpr, op.ra, &v128::_s32, 2));
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_s64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_s64, 1), *qw1);
}

void spu_recompiler::XSHW(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CNTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& vm = XmmAlloc();
	c->movdqa(vm, XmmConst(v128::from8p(0x55)));
	c->movdqa(v1, va);
	c->pand(va, vm);
	c->psrlq(v1, 1);
	c->pand(v1, vm);
	c->paddb(va, v1);
	c->movdqa(vm, XmmConst(v128::from8p(0x33)));
	c->movdqa(v1, va);
	c->pand(va, vm);
	c->psrlq(v1, 2);
	c->pand(v1, vm);
	c->paddb(va, v1);
	c->movdqa(vm, XmmConst(v128::from8p(0x0f)));
	c->movdqa(v1, va);
	c->pand(va, vm);
	c->psrlq(v1, 4);
	c->pand(v1, vm);
	c->paddb(va, v1);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XSBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, 8);
	c->psraw(va, 8);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGT(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtd(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDC(spu_opcode_t op)
{
	// and not
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::FCGT(spu_opcode_t op)
{
	const auto last_exp_bit = XmmConst(v128::from32p(0x00800000));
	const auto all_exp_bits = XmmConst(v128::from32p(0x7f800000));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmpv = XmmAlloc();

	c->pxor(tmp0, tmp0);
	c->pxor(tmp1, tmp1);
	c->cmpps(tmp0, SPU_OFF_128(gpr, op.ra), 3);  //tmp0 is true if a is extended (nan/inf)
	c->cmpps(tmp1, SPU_OFF_128(gpr, op.rb), 3);  //tmp1 is true if b is extended (nan/inf)

	//compute lower a and b
	c->movaps(tmp2, last_exp_bit);
	c->movaps(tmp3, last_exp_bit);
	c->pandn(tmp2, SPU_OFF_128(gpr, op.ra));  //tmp2 = lowered_a
	c->pandn(tmp3, SPU_OFF_128(gpr, op.rb));  //tmp3 = lowered_b

	//lower a if extended
	c->movaps(tmpv, tmp0);
	c->pand(tmpv, tmp2);
	c->pandn(tmp0, SPU_OFF_128(gpr, op.ra));
	c->orps(tmp0, tmpv);

	//lower b if extended
	c->movaps(tmpv, tmp1);
	c->pand(tmpv, tmp3);
	c->pandn(tmp1, SPU_OFF_128(gpr, op.rb));
	c->orps(tmp1, tmpv);

	//flush to 0 if denormalized
	c->pxor(tmpv, tmpv);
	c->movaps(tmp2, SPU_OFF_128(gpr, op.ra));
	c->movaps(tmp3, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp2, all_exp_bits);
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp2, tmpv, 0);
	c->cmpps(tmp3, tmpv, 0);
	c->pandn(tmp2, tmp0);
	c->pandn(tmp3, tmp1);

	c->cmpps(tmp3, tmp2, 1);
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp3);
}

void spu_recompiler::DFCGT(spu_opcode_t op)
{
	UNK(op);
}

void spu_recompiler::FA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->addps(va, SPU_OFF_128(gpr, op.rb));
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->subps(va, SPU_OFF_128(gpr, op.rb));
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FM(spu_opcode_t op)
{
	const auto sign_bits = XmmConst(v128::from32p(0x80000000));
	const auto all_exp_bits = XmmConst(v128::from32p(0x7f800000));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmp4 = XmmGet(op.ra, XmmType::Float);
	const XmmLink& tmp5 = XmmGet(op.rb, XmmType::Float);

	//check denormals
	c->pxor(tmp0, tmp0);
	c->movaps(tmp1, all_exp_bits);
	c->movaps(tmp2, all_exp_bits);
	c->andps(tmp1, tmp4);
	c->andps(tmp2, tmp5);
	c->cmpps(tmp1, tmp0, 0);
	c->cmpps(tmp2, tmp0, 0);
	c->orps(tmp1, tmp2);  //denormal operand mask

	//compute result with flushed denormal inputs
	c->movaps(tmp2, tmp4);
	c->mulps(tmp2, tmp5); //primary result
	c->movaps(tmp3, tmp2);
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp3, tmp0, 0); //denom mask from result
	c->orps(tmp3, tmp1);
	c->andnps(tmp3, tmp2); //flushed result

	//compute results for the extended path
	c->andps(tmp2, all_exp_bits);
	c->cmpps(tmp2, all_exp_bits, 0); //extended mask
	c->movaps(tmp4, sign_bits);
	c->movaps(tmp5, sign_bits);
	c->movaps(tmp0, sign_bits);
	c->andps(tmp4, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp5, SPU_OFF_128(gpr, op.rb));
	c->xorps(tmp4, tmp5); //sign mask
	c->pandn(tmp0, tmp2);
	c->orps(tmp4, tmp0); //add result sign back to original extended value
	c->movaps(tmp5, tmp1); //denormal mask (operands)
	c->andnps(tmp5, tmp4); //max_float with sign bit (nan/-nan) where not denormal or zero

	//select result
	c->movaps(tmp0, tmp2);
	c->andnps(tmp0, tmp3);
	c->andps(tmp2, tmp5);
	c->orps(tmp0, tmp2);
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp0);
}

void spu_recompiler::CLGTH(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from16p(0x8000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtw(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORC(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);

	if (utils::has_avx512())
	{
		c->vpternlogd(vb, vb, SPU_OFF_128(gpr, op.ra), 0xbb /* orC!B */);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
		return;
	}

	c->pxor(vb, XmmConst(v128::from32p(0xffffffff)));
	c->por(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::FCMGT(spu_opcode_t op)
{
	// reverted less-than
	// since comparison is absoulte, a > b if a is extended and b is not extended
	// flush denormals to zero to make zero == zero work
	const auto all_exp_bits = XmmConst(v128::from32p(0x7f800000));
	const auto remove_sign_bits = XmmConst(v128::from32p(0x7fffffff));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmpv = XmmAlloc();

	c->pxor(tmp0, tmp0);
	c->pxor(tmp1, tmp1);
	c->cmpps(tmp0, SPU_OFF_128(gpr, op.ra), 3);  //tmp0 is true if a is extended (nan/inf)
	c->cmpps(tmp1, SPU_OFF_128(gpr, op.rb), 3);  //tmp1 is true if b is extended (nan/inf)

	//flush to 0 if denormalized
	c->pxor(tmpv, tmpv);
	c->movaps(tmp2, SPU_OFF_128(gpr, op.ra));
	c->movaps(tmp3, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp2, all_exp_bits);
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp2, tmpv, 0);
	c->cmpps(tmp3, tmpv, 0);
	c->pandn(tmp2, SPU_OFF_128(gpr, op.ra));
	c->pandn(tmp3, SPU_OFF_128(gpr, op.rb));

	//Set tmp1 to true where a is extended but b is not extended
	//This is a simplification since absolute values remove necessity of lowering
	c->xorps(tmp0, tmp1);   //tmp0 is true when either a or b is extended
	c->pandn(tmp1, tmp0);   //tmp1 is true if b is not extended and a is extended

	c->andps(tmp2, remove_sign_bits);
	c->andps(tmp3, remove_sign_bits);
	c->cmpps(tmp3, tmp2, 1);
	c->orps(tmp3, tmp1);    //Force result to all true if a is extended but b is not
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp3);
}

void spu_recompiler::DFCMGT(spu_opcode_t op)
{
	UNK(op);
}

void spu_recompiler::DFA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->addpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->subpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFM(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTB(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from8p(0x80)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtb(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HLGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->ja(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::DFMA(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->addpd(vr, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::DFMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->subpd(va, vt);
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFNMS(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->subpd(vr, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::DFNMA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->addpd(va, vt);
	c->xorpd(va, XmmConst(v128::from64p(0x8000000000000000)));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQ(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYHHU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pand(va, XmmConst(v128::from32p(0xffff0000)));
	c->psrld(va2, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ADDX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pand(vt, XmmConst(v128::from32p(1)));
	c->paddd(vt, SPU_OFF_128(gpr, op.ra));
	c->paddd(vt, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::SFX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vt, XmmConst(v128::from32p(1)));
	c->psubd(vb, SPU_OFF_128(gpr, op.ra));
	c->psubd(vb, vt);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::CGX(spu_opcode_t op) //nf
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& res = XmmAlloc();
	const XmmLink& sign = XmmAlloc();

	c->pslld(vt, 31);
	c->psrad(vt, 31);

	if (utils::has_avx())
	{
		c->vpaddd(res, va, vb);
	}
	else
	{
		c->movdqa(res, va);
		c->paddd(res, vb);
	}

	c->movdqa(sign, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, sign);
	c->pxor(res, sign);
	c->pcmpgtd(va, res);
	c->pxor(res, sign);
	c->pcmpeqd(res, vt);
	c->pand(res, vt);
	c->por(res, va);
	c->psrld(res, 31);
	c->movdqa(SPU_OFF_128(gpr, op.rt), res);
}

void spu_recompiler::BGX(spu_opcode_t op) //nf
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& temp = XmmAlloc();
	const XmmLink& sign = XmmAlloc();

	c->pslld(vt, 31);

	if (utils::has_avx())
	{
		c->vpcmpeqd(temp, vb, va);
	}
	else
	{
		c->movdqa(temp, vb);
		c->pcmpeqd(temp, va);
	}

	c->pand(vt, temp);
	c->movdqa(sign, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, sign);
	c->pxor(vb, sign);
	c->pcmpgtd(vb, va);
	c->por(vt, vb);
	c->psrld(vt, 31);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::MPYHHA(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->psrld(vb, 16);
	c->pmaddwd(va, vb);
	c->paddd(vt, va);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::MPYHHAU(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pand(va, XmmConst(v128::from32p(0xffff0000)));
	c->psrld(va2, 16);
	c->paddd(vt, va);
	c->paddd(vt, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::FSCRRD(spu_opcode_t op)
{
	// zero (hack)
	const XmmLink& v0 = XmmAlloc();
	c->pxor(v0, v0);
	c->movdqa(SPU_OFF_128(gpr, op.rt), v0);
}

void spu_recompiler::FESD(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->shufps(va, va, 0x8d); // _f[0] = _f[1]; _f[1] = _f[3];
	c->cvtps2pd(va, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FRDS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->cvtpd2ps(va, va);
	c->shufps(va, va, 0x72); // _f[1] = _f[0]; _f[3] = _f[1]; _f[0] = _f[2] = 0;
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FSCRWR(spu_opcode_t /*op*/)
{
	// nop (not implemented)
}

void spu_recompiler::DFTSV(spu_opcode_t op)
{
	UNK(op);
}

void spu_recompiler::FCEQ(spu_opcode_t op)
{
	// compare equal
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->cmpps(vb, SPU_OFF_128(gpr, op.ra), 0);
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::DFCEQ(spu_opcode_t op)
{
	UNK(op);
}

void spu_recompiler::MPY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->pmullw(va, vb);
	c->pslld(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYHH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->psrld(vb, 16);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pmulhw(va, vb);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FCMEQ(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	c->movaps(vi, XmmConst(v128::from32p(0x7fffffff)));
	c->andps(vb, vi); // abs
	c->andps(vi, SPU_OFF_128(gpr, op.ra));
	c->cmpps(vb, vi, 0); // ==
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::DFCMEQ(spu_opcode_t op)
{
	UNK(op);
}

void spu_recompiler::MPYU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pslld(va, 16);
	c->pand(va2, XmmConst(v128::from32p(0xffff)));
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FI(spu_opcode_t op)
{
	// Floating Interpolate
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& vb_base = XmmAlloc();
	const XmmLink& ymul = XmmAlloc();
	const XmmLink& temp_reg = XmmAlloc();
	const XmmLink& temp_reg2 = XmmAlloc();

	c->movdqa(vb_base, vb);
	c->movdqa(ymul, vb);
	c->movdqa(temp_reg, va);

	c->pand(vb_base, XmmConst(v128::from32p(0x007ffc00u)));
	c->pslld(vb_base, 9);

	c->pand(temp_reg, XmmConst(v128::from32p(0x7ffff))); // va_fraction
	c->pand(ymul, XmmConst(v128::from32p(0x3ff)));
	c->pmulld(ymul, temp_reg);

	c->movdqa(temp_reg, vb_base);
	c->psubd(temp_reg, ymul);

	// Makes signed comparison unsigned and determines if we need to adjust exponent
	auto xor_const = XmmConst(v128::from32p(0x80000000));
	c->pxor(ymul, xor_const);
	c->pxor(vb_base, xor_const);
	c->pcmpgtd(ymul, vb_base);
	c->movdqa(vb_base, ymul);

	c->movdqa(temp_reg2, temp_reg);
	c->pand(temp_reg2, vb_base);
	c->psrld(temp_reg2, 8); // only shift right by 8 if exponent is adjusted
	c->xorps(vb_base, XmmConst(v128::from32p(0xFFFFFFFF))); // Invert the mask
	c->pand(temp_reg, vb_base);
	c->psrld(temp_reg, 9); // shift right by 9 if not adjusted
	c->por(temp_reg, temp_reg2);

	c->pand(vb, XmmConst(v128::from32p(0xff800000u)));
	c->pand(temp_reg, XmmConst(v128::from32p(~0xff800000u)));
	c->por(vb, temp_reg);

	c->pand(ymul, XmmConst(v128::from32p(1 << 23)));
	c->psubd(vb, ymul);

	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::HEQ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_s32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_s32, 3));

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->je(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::CFLTS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	if (op.i8 != 173) c->mulps(va, XmmConst(v128::fromf32p(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))))); // scale
	c->movaps(vi, XmmConst(v128::fromf32p(std::exp2(31.f))));
	c->cmpps(vi, va, 2);
	c->cvttps2dq(va, va); // convert to ints with truncation
	c->pxor(va, vi); // fix result saturation (0x80000000 -> 0x7fffffff)
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CFLTU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vs = XmmAlloc();
	const XmmLink& vs2 = XmmAlloc();
	const XmmLink& vs3 = XmmAlloc();
	if (op.i8 != 173) c->mulps(va, XmmConst(v128::fromf32p(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))))); // scale

	if (utils::has_avx512())
	{
		c->vcvttps2udq(vs, va);
		c->psrad(va, 31);
		c->pandn(va, vs);
		c->movdqa(SPU_OFF_128(gpr, op.rt), va);
		return;
	}

	c->movdqa(vs, va);
	c->psrad(va, 31);
	c->andnps(va, vs);
	c->movaps(vs, va); // copy scaled value
	c->movaps(vs2, va);
	c->movaps(vs3, XmmConst(v128::fromf32p(std::exp2(31.f))));
	c->subps(vs2, vs3);
	c->cmpps(vs3, vs, 2);
	c->andps(vs2, vs3);
	c->cvttps2dq(va, va);
	c->cmpps(vs, XmmConst(v128::fromf32p(std::exp2(32.f))), 5);
	c->cvttps2dq(vs2, vs2);
	c->por(va, vs);
	c->por(va, vs2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CSFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->cvtdq2ps(va, va); // convert to floats
	if (op.i8 != 155) c->mulps(va, XmmConst(v128::fromf32p(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CUFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();

	if (utils::has_avx512())
	{
		c->vcvtudq2ps(va, va);
	}
	else
	{
		c->movdqa(v1, va);
		c->pand(va, XmmConst(v128::from32p(0x7fffffff)));
		c->cvtdq2ps(va, va); // convert to floats
		c->psrad(v1, 31); // generate mask from sign bit
		c->andps(v1, XmmConst(v128::fromf32p(std::exp2(31.f)))); // generate correction component
		c->addps(va, v1); // add correction component
	}

	if (op.i8 != 155) c->mulps(va, XmmConst(v128::fromf32p(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::BRZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos + 4)
	{
		return;
	}

	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->je(branch_label);

	after.emplace_back([=, this]()
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		branch_fixed(target);
	});
}

void spu_recompiler::STQA(spu_opcode_t op)
{
	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(asmjit::x86::oword_ptr(*ls, spu_ls_target(0, op.i16)), vt);
	}
	else
	{
		c->mov(*qw0, SPU_OFF_64(gpr, op.rt, &v128::_u64, 0));
		c->mov(*qw1, SPU_OFF_64(gpr, op.rt, &v128::_u64, 1));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, spu_ls_target(0, op.i16) + 0), *qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, spu_ls_target(0, op.i16) + 8), *qw0);
	}
}

void spu_recompiler::BRNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos + 4)
	{
		return;
	}

	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->jne(branch_label);

	after.emplace_back([=, this]()
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		branch_fixed(target);
	});
}

void spu_recompiler::BRHZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos + 4)
	{
		return;
	}

	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->je(branch_label);

	after.emplace_back([=, this]()
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		branch_fixed(target);
	});
}

void spu_recompiler::BRHNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos + 4)
	{
		return;
	}

	asmjit::Label branch_label = c->newLabel();
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->jne(branch_label);

	after.emplace_back([=, this]()
	{
		c->align(asmjit::AlignMode::kCode, 16);
		c->bind(branch_label);
		branch_fixed(target);
	});
}

void spu_recompiler::STQR(spu_opcode_t op)
{
	c->lea(addr->r64(), get_pc(spu_ls_target(m_pos, op.i16)));
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(asmjit::x86::oword_ptr(*ls, addr->r64()), vt);
	}
	else
	{
		c->mov(*qw0, SPU_OFF_64(gpr, op.rt, &v128::_u64, 0));
		c->mov(*qw1, SPU_OFF_64(gpr, op.rt, &v128::_u64, 1));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0), *qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8), *qw0);
	}
}

void spu_recompiler::BRA(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	branch_fixed(target, true);
	m_pos = -1;
}

void spu_recompiler::LQA(spu_opcode_t op)
{
	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmAlloc();
		c->movdqa(vt, asmjit::x86::oword_ptr(*ls, spu_ls_target(0, op.i16)));
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	}
	else
	{
		c->mov(*qw0, asmjit::x86::qword_ptr(*ls, spu_ls_target(0, op.i16) + 0));
		c->mov(*qw1, asmjit::x86::qword_ptr(*ls, spu_ls_target(0, op.i16) + 8));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw0);
	}
}

void spu_recompiler::BRASL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	const XmmLink& vr = XmmAlloc();
	c->lea(addr->r64(), get_pc(m_pos + 4));
	c->and_(*addr, 0x3fffc);
	c->movd(vr, *addr);
	c->pslldq(vr, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);

	branch_set_link(m_pos + 4);
	branch_fixed(target, true);
	m_pos = -1;
}

void spu_recompiler::BR(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target != m_pos + 4)
	{
		branch_fixed(target);
		m_pos = -1;
	}
}

void spu_recompiler::FSMBI(spu_opcode_t op)
{
	v128 data;
	for (u32 i = 0; i < 16; i++)
		data._u8[i] = op.i16 & (1u << i) ? 0xff : 0;

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(data));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::BRSL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	const XmmLink& vr = XmmAlloc();
	c->lea(addr->r64(), get_pc(m_pos + 4));
	c->and_(*addr, 0x3fffc);
	c->movd(vr, *addr);
	c->pslldq(vr, 12);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);

	if (target != m_pos + 4)
	{
		branch_set_link(m_pos + 4);
		branch_fixed(target);
		m_pos = -1;
	}
}

void spu_recompiler::LQR(spu_opcode_t op)
{
	c->lea(addr->r64(), get_pc(spu_ls_target(m_pos, op.i16)));
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmAlloc();
		c->movdqa(vt, asmjit::x86::oword_ptr(*ls, addr->r64()));
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	}
	else
	{
		c->mov(*qw0, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0));
		c->mov(*qw1, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw0);
	}
}

void spu_recompiler::IL(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ILHU(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.i16 << 16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ILH(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from16p(op.i16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::IOHL(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->por(vt, XmmConst(v128::from32p(op.i16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	if (op.si10) c->por(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SFI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si10)));
	c->psubd(vr, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::SFHI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from16p(op.si10)));
	c->psubw(vr, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ANDI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AHI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::STQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.si10) c->add(*addr, op.si10 * 16);
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(asmjit::x86::oword_ptr(*ls, addr->r64()), vt);
	}
	else
	{
		c->mov(*qw0, SPU_OFF_64(gpr, op.rt, &v128::_u64, 0));
		c->mov(*qw1, SPU_OFF_64(gpr, op.rt, &v128::_u64, 1));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0), *qw1);
		c->mov(asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8), *qw0);
	}
}

void spu_recompiler::LQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.si10) c->add(*addr, op.si10 * 16);
	c->and_(*addr, 0x3fff0);

	if (utils::has_ssse3())
	{
		const XmmLink& vt = XmmAlloc();
		c->movdqa(vt, asmjit::x86::oword_ptr(*ls, addr->r64()));
		c->pshufb(vt, XmmConst(v128::from32r(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	}
	else
	{
		c->mov(*qw0, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 0));
		c->mov(*qw1, asmjit::x86::qword_ptr(*ls, addr->r64(), 0, 8));
		c->bswap(*qw0);
		c->bswap(*qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw1);
		c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw0);
	}
}

void spu_recompiler::XORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HGTI(spu_opcode_t op)
{
	c->cmp(SPU_OFF_32(gpr, op.ra, &v128::_s32, 3), +op.si10);

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->jg(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::CLGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from32p(0x80000000)));
	c->pcmpgtd(va, XmmConst(v128::from32p(op.si10 - 0x80000000)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from16p(0x8000)));
	c->pcmpgtw(va, XmmConst(v128::from16p(op.si10 - 0x8000)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psubb(va, XmmConst(v128::from8p(0x80)));
	c->pcmpgtb(va, XmmConst(v128::from8p(op.si10 - 0x80)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HLGTI(spu_opcode_t op)
{
	c->cmp(SPU_OFF_32(gpr, op.ra, &v128::_u32, 3), +op.si10);

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->ja(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::MPYI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pmaddwd(va, XmmConst(v128::from32p(op.si10 & 0xffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYUI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->movdqa(vi, XmmConst(v128::from32p(op.si10 & 0xffff)));
	c->pmulhuw(va, vi);
	c->pmullw(va2, vi);
	c->pslld(va, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HEQI(spu_opcode_t op)
{
	c->cmp(SPU_OFF_32(gpr, op.ra, &v128::_u32, 3), +op.si10);

	asmjit::Label label = c->newLabel();
	asmjit::Label ret = c->newLabel();
	c->je(label);

	after.emplace_back([=, this, pos = m_pos]
	{
		c->bind(label);
		c->lea(addr->r64(), get_pc(pos));
		c->and_(*addr, 0x3fffc);
		c->mov(SPU_OFF_32(pc), *addr);
		c->mov(addr->r64(), reinterpret_cast<u64>(vm::base(0xffdead00)));
		c->mov(asmjit::x86::dword_ptr(addr->r64()), "HALT"_u32);
		c->jmp(ret);
	});
}

void spu_recompiler::HBRA([[maybe_unused]] spu_opcode_t op)
{
}

void spu_recompiler::HBRR([[maybe_unused]] spu_opcode_t op)
{
}

void spu_recompiler::ILA(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.i18)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::SELB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vc = XmmGet(op.rc, XmmType::Int);

	if (utils::has_avx512())
	{
		c->vpternlogd(vc, vb, SPU_OFF_128(gpr, op.ra), 0xca /* A?B:C */);
		c->movdqa(SPU_OFF_128(gpr, op.rt4), vc);
		return;
	}

	if (utils::has_xop())
	{
		c->vpcmov(vc, vb, SPU_OFF_128(gpr, op.ra), vc);
		c->movdqa(SPU_OFF_128(gpr, op.rt4), vc);
		return;
	}

	c->pand(vb, vc);
	c->pandn(vc, SPU_OFF_128(gpr, op.ra));
	c->por(vb, vc);
	c->movdqa(SPU_OFF_128(gpr, op.rt4), vb);
}

void spu_recompiler::SHUFB(spu_opcode_t op)
{
	if (0 && utils::has_avx512())
	{
		// Deactivated due to poor performance of mask merge ops.
		const XmmLink& va = XmmGet(op.ra, XmmType::Int);
		const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
		const XmmLink& vc = XmmGet(op.rc, XmmType::Int);
		const XmmLink& vt = XmmAlloc();
		const XmmLink& vm = XmmAlloc();
		c->vpcmpub(asmjit::x86::k1, vc, XmmConst(v128::from8p(-0x40)), 5 /* GE */);
		c->vpxor(vm, vc, XmmConst(v128::from8p(0xf)));
		c->setExtraReg(asmjit::x86::k1);
		c->z().vpblendmb(vc, vc, XmmConst(v128::from8p(-1))); // {k1}
		c->vpcmpub(asmjit::x86::k2, vm, XmmConst(v128::from8p(-0x20)), 5 /* GE */);
		c->vptestmb(asmjit::x86::k1, vm, XmmConst(v128::from8p(0x10)));
		c->vpshufb(vt, va, vm);
		c->setExtraReg(asmjit::x86::k2);
		c->z().vpblendmb(va, va, XmmConst(v128::from8p(0x7f))); // {k2}
		c->setExtraReg(asmjit::x86::k1);
		c->vpshufb(vt, vb, vm); // {k1}
		c->vpternlogd(vt, va, vc, 0xf6 /* orAxorBC */);
		c->movdqa(SPU_OFF_128(gpr, op.rt4), vt);
		return;
	}

	if (!utils::has_ssse3())
	{
		return fall(op);
	}

	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vc = XmmGet(op.rc, XmmType::Int);
	const XmmLink& vt = XmmAlloc();
	const XmmLink& vm = XmmAlloc();
	const XmmLink& v5 = XmmAlloc();
	c->movdqa(vm, XmmConst(v128::from8p(static_cast<s8>(0xc0))));

	if (utils::has_avx())
	{
		c->vpand(v5, vc, XmmConst(v128::from8p(static_cast<s8>(0xe0))));
		c->vpxor(vc, vc, XmmConst(v128::from8p(0xf)));
		c->vpshufb(va, va, vc);
		c->vpslld(vt, vc, 3);
		c->vpcmpeqb(v5, v5, vm);
		c->vpshufb(vb, vb, vc);
		c->vpand(vc, vc, vm);
		c->vpblendvb(vb, va, vb, vt);
		c->vpcmpeqb(vt, vc, vm);
		c->vpavgb(vt, vt, v5);
		c->vpor(vt, vt, vb);
	}
	else
	{
		c->movdqa(v5, vc);
		c->pand(v5, XmmConst(v128::from8p(static_cast<s8>(0xe0))));
		c->movdqa(vt, vc);
		c->pand(vt, vm);
		c->pxor(vc, XmmConst(v128::from8p(0xf)));
		c->pshufb(va, vc);
		c->pshufb(vb, vc);
		c->pslld(vc, 3);
		c->pcmpeqb(v5, vm); // If true, result should become 0xFF
		c->pcmpeqb(vt, vm); // If true, result should become either 0xFF or 0x80
		c->pcmpeqb(vm, vm);
		c->pcmpgtb(vc, vm);
		c->pand(va, vc);
		c->pandn(vc, vb);
		c->por(vc, va); // Select result value from va or vb
		c->pavgb(vt, v5); // Generate result constant: AVG(0xff, 0x00) == 0x80
		c->por(vt, vc);
	}

	c->movdqa(SPU_OFF_128(gpr, op.rt4), vt);
}

void spu_recompiler::MPYA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->paddd(va, SPU_OFF_128(gpr, op.rc));
	c->movdqa(SPU_OFF_128(gpr, op.rt4), va);
}

void spu_recompiler::FNMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& mask = XmmAlloc();
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	c->movaps(mask, XmmConst(v128::from32p(0x7f800000)));
	c->movaps(v1, va);
	c->movaps(v2, vb);
	c->andps(va, mask);
	c->andps(vb, mask);
	c->cmpps(va, mask, 4); // va = ra == extended
	c->cmpps(vb, mask, 4); // vb = rb == extended
	c->andps(va, v1); // va = ra & ~ra_extended
	c->andps(vb, v2); // vb = rb & ~rb_extended

	c->mulps(va, vb);
	c->movaps(vb, SPU_OFF_128(gpr, op.rc));
	c->subps(vb, va);
	c->movaps(SPU_OFF_128(gpr, op.rt4), vb);
}

void spu_recompiler::FMA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& mask = XmmAlloc();
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	c->movaps(mask, XmmConst(v128::from32p(0x7f800000)));
	c->movaps(v1, va);
	c->movaps(v2, vb);
	c->andps(va, mask);
	c->andps(vb, mask);
	c->cmpps(va, mask, 4); // va = ra == extended
	c->cmpps(vb, mask, 4); // vb = rb == extended
	c->andps(va, v1); // va = ra & ~ra_extended
	c->andps(vb, v2); // vb = rb & ~rb_extended

	c->mulps(va, vb);
	c->addps(va, SPU_OFF_128(gpr, op.rc));
	c->movaps(SPU_OFF_128(gpr, op.rt4), va);
}

void spu_recompiler::FMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& mask = XmmAlloc();
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	c->movaps(mask, XmmConst(v128::from32p(0x7f800000)));
	c->movaps(v1, va);
	c->movaps(v2, vb);
	c->andps(va, mask);
	c->andps(vb, mask);
	c->cmpps(va, mask, 4); // va = ra == extended
	c->cmpps(vb, mask, 4); // vb = rb == extended
	c->andps(va, v1); // va = ra & ~ra_extended
	c->andps(vb, v2); // vb = rb & ~rb_extended

	c->mulps(va, vb);
	c->subps(va, SPU_OFF_128(gpr, op.rc));
	c->movaps(SPU_OFF_128(gpr, op.rt4), va);
}
