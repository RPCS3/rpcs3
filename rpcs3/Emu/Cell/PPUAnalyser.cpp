#include "stdafx.h"
#include "PPUAnalyser.h"

#include "lv2/sys_sync.h"

#include "PPUOpcodes.h"
#include "PPUThread.h"

#include <unordered_set>
#include "util/yaml.hpp"
#include "util/asm.hpp"

LOG_CHANNEL(ppu_validator);

const ppu_decoder<ppu_itype> s_ppu_itype;

template<>
void fmt_class_string<ppu_attr>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_attr value)
	{
		switch (value)
		{
		case ppu_attr::known_size: return "known_size";
		case ppu_attr::no_return: return "no_return";
		case ppu_attr::no_size: return "no_size";
		case ppu_attr::has_mfvscr: return "has_mfvscr";
		case ppu_attr::__bitset_enum_max: break;
		}

		return unknown;
	});
}

template<>
void fmt_class_string<bs_t<ppu_attr>>::format(std::string& out, u64 arg)
{
	format_bitset(out, arg, "[", ",", "]", &fmt_class_string<ppu_attr>::format);
}

template <>
void ppu_module<lv2_obj>::validate(u32 reloc)
{
	// Load custom PRX configuration if available
	if (fs::file yml{path + ".yml"})
	{
		const auto [cfg, error] = yaml_load(yml.to_string());

		if (!error.empty())
		{
			ppu_validator.error("Failed to load %s.yml: %s", path, error);
			return;
		}

		u32 index = 0;

		// Validate detected functions using information provided
		for (const auto func : cfg["functions"])
		{
			const u32 addr = func["addr"].as<u32>(-1);
			const u32 size = func["size"].as<u32>(0);

			if (addr != umax && index < funcs.size())
			{
				u32 found = funcs[index].addr - reloc;

				while (addr > found && index + 1 < funcs.size())
				{
					ppu_validator.warning("%s.yml : unexpected function at 0x%x (0x%x, 0x%x)", path, found, addr, size);
					index++;
					found = funcs[index].addr - reloc;
				}

				if (addr < found)
				{
					ppu_validator.error("%s.yml : function not found (0x%x, 0x%x)", path, addr, size);
					continue;
				}

				if (size && size != funcs[index].size)
				{
					if (size + 4 != funcs[index].size || get_ref<u32>(addr + size) != ppu_instructions::NOP())
					{
						ppu_validator.error("%s.yml : function size mismatch at 0x%x(size=0x%x) (0x%x, 0x%x)", path, found, funcs[index].size, addr, size);
					}
				}

				index++;
			}
			else
			{
				ppu_validator.error("%s.yml : function not found at the end (0x%x, 0x%x)", path, addr, size);
				break;
			}
		}

		if (!index)
		{
			return; // ???
		}

		while (index < funcs.size())
		{
			if (funcs[index].size)
			{
				ppu_validator.error("%s.yml : function not covered at 0x%x (size=0x%x)", path, funcs[index].addr, funcs[index].size);
			}

			index++;
		}

		ppu_validator.success("%s.yml : validation completed", path);
	}
}

static u32 ppu_test(const be_t<u32>* ptr, const void* fend, ppu_pattern_array pat)
{
	const be_t<u32>* cur = ptr;

	for (auto& p : pat)
	{
		if (cur >= fend)
		{
			return 0;
		}

		if (*cur == ppu_instructions::NOP())
		{
			cur++;

			if (cur >= fend)
			{
				return 0;
			}
		}

		if ((*cur & p.mask) != p.opcode)
		{
			return 0;
		}

		cur++;
	}

	return ::narrow<u32>((cur - ptr) * sizeof(*ptr));
}

static u32 ppu_test(const be_t<u32>* ptr, const void* fend, ppu_pattern_matrix pats)
{
	for (auto pat : pats)
	{
		if (const u32 len = ppu_test(ptr, fend, pat))
		{
			return len;
		}
	}

	return 0;
}

namespace ppu_patterns
{
	using namespace ppu_instructions;

	const ppu_pattern abort1[]
	{
		{ STDU(r1, r1, -0xc0) },
		{ MFLR(r0) },
		{ STD(r26, r1, 0x90) },
		{ STD(r27, r1, 0x98) },
		{ STD(r28, r1, 0xa0) },
		{ STD(r29, r1, 0xa8) },
		{ STD(r30, r1, 0xb0) },
		{ STD(r31, r1, 0xb8) },
		{ STD(r0, r1, 0xd0) },
		{ LI(r3, 4) },
		{ LI(r4, 0) },
		{ LI(r11, 0x3dc) },
		{ SC(0) },
		{ MR(r29, r1) },
		{ CLRLDI(r29, r29, 32) },
		{ LWZ(r4, r2, 0), 0xffff },
		{ ADDI(r31, r1, 0x70) },
		{ LI(r3, 1) },
		{ LI(r5, 0x19) },
		{ MR(r6, r31) },
		{ LWZ(r28, r29, 4) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ ADDI(r26, r1, 0x78) },
		{ LD(r3, r28, 0x10) },
		{ MR(r4, r26) },
		{ B(0, false, true), 0x3fffffc }, // .hex2str
		{ LI(r5, 0x10) },
		{ CLRLDI(r4, r3, 32) },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LWZ(r27, r2, 0), 0xffff },
		{ LI(r3, 1) },
		{ LI(r5, 1) },
		{ MR(r4, r27) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LD(r28, r28, 0) },
		{ CMPDI(cr7, r28, 0) },
		{ BEQ(cr7, +0x6c) },
		{ LWZ(r30, r2, 0), 0xffff },
		{ LI(r3, 1) },
		{ MR(r4, r30) },
		{ LI(r5, 0x19) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ CLRLDI(r29, r28, 32) },
		{ CLRLDI(r4, r26, 32) },
		{ LD(r3, r29, 0x10) },
		{ 0, 0xffffffff }, // .hex2str
		{ LI(r5, 0x10) },
		{ CLRLDI(r4, r3, 32) },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LI(r3, 1) },
		{ MR(r4, r27) },
		{ LI(r5, 1) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LD(r28, r29, 0) },
		{ CMPDI(cr7, r28, 0) },
		{ BNE(cr7, -0x60) },
		{ LWZ(r4, r2, 0), 0xffff },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r5, 0x27) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LI(r3, 1) },
		{ B(0, false, true), 0x3fffffc }, // .sys_process_exit
		{ LD(r2, r1, 0x28) },
		{ LI(r3, 1) },
		{ B(0, false, true), 0x3fffffc }, // .exit
	};

	const ppu_pattern abort2[]
	{
		{ STDU(r1, r1, -0xc0) },
		{ MFLR(r0) },
		{ STD(r27, r1, 0x98) },
		{ STD(r28, r1, 0xa0) },
		{ STD(r29, r1, 0xa8) },
		{ STD(r30, r1, 0xb0) },
		{ STD(r31, r1, 0xb8) },
		{ STD(r0, r1, 0xd0) },
		{ MR(r9, r1) },
		{ CLRLDI(r9, r9, 32) },
		{ LWZ(r4, r2, 0), 0xffff },
		{ ADDI(r31, r1, 0x70) },
		{ LI(r3, 1) },
		{ LI(r5, 0x19) },
		{ MR(r6, r31) },
		{ LWZ(r29, r9, 4) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ ADDI(r27, r1, 0x78) },
		{ LD(r3, r29, 0x10) },
		{ MR(r4, r27) },
		{ B(0, false, true), 0x3fffffc }, // .hex2str
		{ LI(r5, 0x10) },
		{ CLRLDI(r4, r3, 32) },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LWZ(r28, r2, 0), 0xffff },
		{ LI(r3, 1) },
		{ LI(r5, 1) },
		{ MR(r4, r28) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LD(r29, r29, 0) },
		{ CMPDI(cr7, r29, 0) },
		{ BEQ(cr7, +0x6c) },
		{ LWZ(r30, r2, 0), 0xffff },
		{ LI(r3, 1) },
		{ MR(r4, r30) },
		{ LI(r5, 0x19) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ CLRLDI(r29, r29, 32) },
		{ CLRLDI(r4, r27, 32) },
		{ LD(r3, r29, 0x10) },
		{ 0, 0xffffffff }, // .hex2str
		{ LI(r5, 0x10) },
		{ CLRLDI(r4, r3, 32) },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LI(r3, 1) },
		{ MR(r4, r28) },
		{ LI(r5, 1) },
		{ MR(r6, r31) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LD(r29, r29, 0) },
		{ CMPDI(cr7, r29, 0) },
		{ BNE(cr7, -0x60) },
		{ LWZ(r4, r2, 0), 0xffff },
		{ MR(r6, r31) },
		{ LI(r3, 1) },
		{ LI(r5, 0x27) },
		{ LI(r11, 0x193) },
		{ SC(0) },
		{ LI(r3, 1) },
		{ B(0, false, true), 0x3fffffc }, // .sys_process_exit
		{ LD(r2, r1, 0x28) },
		{ LI(r3, 1) },
		{ B(0, false, true), 0x3fffffc }, // .exit
	};

	const ppu_pattern_array abort[]
	{
		abort1,
		abort2,
	};

	const ppu_pattern get_context[]
	{
		ADDI(r3, r3, 0xf),
		CLRRDI(r3, r3, 4),
		STD(r1, r3, 0),
		STD(r2, r3, 8),
		STD(r14, r3, 0x18),
		STD(r15, r3, 0x20),
		STD(r16, r3, 0x28),
		STD(r17, r3, 0x30),
		STD(r18, r3, 0x38),
		STD(r19, r3, 0x40),
		STD(r20, r3, 0x48),
		STD(r21, r3, 0x50),
		STD(r22, r3, 0x58),
		STD(r23, r3, 0x60),
		STD(r24, r3, 0x68),
		STD(r25, r3, 0x70),
		STD(r26, r3, 0x78),
		STD(r27, r3, 0x80),
		STD(r28, r3, 0x88),
		STD(r29, r3, 0x90),
		STD(r30, r3, 0x98),
		STD(r31, r3, 0xa0),
		MFLR(r0),
		STD(r0, r3, 0xa8),
		0x7c000026, // mfcr r0
		STD(r0, r3, 0xb0),
		STFD(f14, r3, 0xb8),
		STFD(f15, r3, 0xc0),
		STFD(F16, r3, 0xc8),
		STFD(f17, r3, 0xd0),
		STFD(f18, r3, 0xd8),
		STFD(f19, r3, 0xe0),
		STFD(f20, r3, 0xe8),
		STFD(f21, r3, 0xf0),
		STFD(f22, r3, 0xf8),
		STFD(f23, r3, 0x100),
		STFD(f24, r3, 0x108),
		STFD(f25, r3, 0x110),
		STFD(f26, r3, 0x118),
		STFD(f27, r3, 0x120),
		STFD(f28, r3, 0x128),
		STFD(f29, r3, 0x130),
		STFD(f30, r3, 0x138),
		STFD(f31, r3, 0x140),
		0x7c0042A6, // mfspr r0, vrsave
		STD(r0, r3, 0x148),
		ADDI(r4, r3, 0x150),
		ADDI(r5, r3, 0x160),
		ADDI(r6, r3, 0x170),
		ADDI(r7, r3, 0x180),
		STVX(v20, r0, r4),
		STVX(v21, r0, r5),
		STVX(v22, r0, r6),
		STVX(v23, r0, r7),
		ADDI(r4, r4, 0x40),
		ADDI(r5, r5, 0x40),
		ADDI(r6, r6, 0x40),
		ADDI(r7, r7, 0x40),
		STVX(v24, r0, r4),
		STVX(v25, r0, r5),
		STVX(v26, r0, r6),
		STVX(v27, r0, r7),
		ADDI(r4, r4, 0x40),
		ADDI(r5, r5, 0x40),
		ADDI(r6, r6, 0x40),
		ADDI(r7, r7, 0x40),
		STVX(v28, r0, r4),
		STVX(v29, r0, r5),
		STVX(v30, r0, r6),
		STVX(v31, r0, r7),
		LI(r3, 0),
		BLR(),
	};

	const ppu_pattern set_context[]
	{
		ADDI(r3, r3, 0xf),
		CLRRDI(r3, r3, 4),
		LD(r1, r3, 0),
		LD(r2, r3, 8),
		LD(r14, r3, 0x18),
		LD(r15, r3, 0x20),
		LD(r16, r3, 0x28),
		LD(r17, r3, 0x30),
		LD(r18, r3, 0x38),
		LD(r19, r3, 0x40),
		LD(r20, r3, 0x48),
		LD(r21, r3, 0x50),
		LD(r22, r3, 0x58),
		LD(r23, r3, 0x60),
		LD(r24, r3, 0x68),
		LD(r25, r3, 0x70),
		LD(r26, r3, 0x78),
		LD(r27, r3, 0x80),
		LD(r28, r3, 0x88),
		LD(r29, r3, 0x90),
		LD(r30, r3, 0x98),
		LD(r31, r3, 0xa0),
		LD(r0, r3, 0xa8),
		MTLR(r0),
		LD(r0, r3, 0xb0),
		0x7c101120, // mtocrf 1, r0
		0x7c102120, // mtocrf 2, r0
		0x7c104120, // mtocrf 4, r0
		0x7c108120, // mtocrf 8, r0
		0x7c110120, // mtocrf 0x10, r0
		0x7c120120, // mtocrf 0x20, r0
		0x7c140120, // mtocrf 0x40, r0
		0x7c180120, // mtocrf 0x80, r0
		LFD(f14, r3, 0xb8),
		LFD(f15, r3, 0xc0),
		LFD(F16, r3, 0xc8),
		LFD(f17, r3, 0xd0),
		LFD(f18, r3, 0xd8),
		LFD(f19, r3, 0xe0),
		LFD(f20, r3, 0xe8),
		LFD(f21, r3, 0xf0),
		LFD(f22, r3, 0xf8),
		LFD(f23, r3, 0x100),
		LFD(f24, r3, 0x108),
		LFD(f25, r3, 0x110),
		LFD(f26, r3, 0x118),
		LFD(f27, r3, 0x120),
		LFD(f28, r3, 0x128),
		LFD(f29, r3, 0x130),
		LFD(f30, r3, 0x138),
		LFD(f31, r3, 0x140),
		LD(r0, r3, 0x148),
		0x7c0043A6, //mtspr vrsave, r0
		ADDI(r5, r3, 0x150),
		ADDI(r6, r3, 0x160),
		ADDI(r7, r3, 0x170),
		ADDI(r8, r3, 0x180),
		LVX(v20, r0, r5),
		LVX(v21, r0, r6),
		LVX(v22, r0, r7),
		LVX(v23, r0, r8),
		ADDI(r5, r5, 0x40),
		ADDI(r6, r6, 0x40),
		ADDI(r7, r7, 0x40),
		ADDI(r8, r8, 0x40),
		LVX(v24, r0, r5),
		LVX(v25, r0, r6),
		LVX(v26, r0, r7),
		LVX(v27, r0, r8),
		ADDI(r5, r5, 0x40),
		ADDI(r6, r6, 0x40),
		ADDI(r7, r7, 0x40),
		ADDI(r8, r8, 0x40),
		LVX(v28, r0, r5),
		LVX(v29, r0, r6),
		LVX(v30, r0, r7),
		LVX(v31, r0, r8),
		LI(r3, 0),
		0x7c041810, // subfc r0, r4, r3
		0x7c640194, // addze r3, r4
		BLR(),
	};

	const ppu_pattern x26c[]
	{
		LI(r9, 0),
		STD(r9, r6, 0),
		MR(r1, r6),
		STDU(r1, r1, -0x70),
		STD(r9, r1, 0),
		CLRLDI(r7, r3, 32),
		LWZ(r0, r7, 0),
		MTCTR(r0),
		LWZ(r2, r7, 4),
		MR(r3, r4),
		MR(r4, r5),
		BCTRL(),
	};

	const ppu_pattern x2a0[]
	{
		MR(r8, r1),
		0x7d212850, // subf r9, r1, r5
		0x7c21496a, // stdux r1, r1, r9
		MFLR(r0),
		STD(r0, r8, 0x10),
		STD(r2, r1, 0x28),
		CLRLDI(r7, r3, 32),
		LWZ(r0, r7, 0),
		MTCTR(r0),
		LWZ(r2, r7, 4),
		MR(r3, r4),
		BCTRL(),
		LD(r2, r1, 0x28),
		LD(r9, r1, 0x0),
		LD(r0, r9, 0x10),
		MTLR(r0),
		MR(r1, r9),
		BLR(),
	};
}

template <>
bool ppu_module<lv2_obj>::analyse(u32 lib_toc, u32 entry, const u32 sec_end, const std::vector<u32>& applied, const std::vector<u32>& exported_funcs, std::function<bool()> check_aborted)
{
	if (segs.empty())
	{
		return false;
	}

	// Assume first segment is executable
	const u32 start = segs[0].addr;

	// End of executable segment (may change)
	u32 end = sec_end ? sec_end : segs[0].addr + segs[0].size;

	// Known TOCs (usually only 1)
	std::unordered_set<u32> TOCs;

	// Known functions
	std::map<u32, ppu_function> fmap;
	std::set<u32> known_functions;

	// Function analysis workload
	std::vector<std::reference_wrapper<ppu_function>> func_queue;

	// Known references (within segs, addr and value alignment = 4)
	std::set<u32> addr_heap;

	if (entry)
	{
		addr_heap.emplace(entry);
	}

	auto verify_func = [&](u32 addr)
	{
		if (entry)
		{
			// Fixed addresses
			return true;
		}

		// Check if the storage address exists within relocations

		for (auto& rel : this->relocs)
		{
			if ((rel.addr & -8) == (addr & -8))
			{
				if (rel.type != 38 && rel.type != 44 && (rel.addr & -4) != (addr & -4))
				{
					continue;
				}

				return true;
			}
		}

		return false;
	};

	auto can_trap_continue = [](ppu_opcode_t op, ppu_itype::type type)
	{
		if ((op.bo & 0x1c) == 0x1c || (op.bo & 0x7) == 0x7)
		{
			// All signed or unsigned <=>, must be true
			return false;
		}

		if (op.simm16 == 0 && (type == ppu_itype::TWI || type == ppu_itype::TDI) && (op.bo & 0x5) == 0x5)
		{
			// Logically greater or equal to 0
			return false;
		}

		return true;
	};

	// Register new function
	auto add_func = [&](u32 addr, u32 toc, u32 caller) -> ppu_function&
	{
		ppu_function& func = fmap[addr];

		if (caller)
		{
			// Register caller
			func.callers.emplace(caller);
		}

		if (func.addr)
		{
			if (toc && func.toc && func.toc != umax && func.toc != toc)
			{
				func.toc = -1;
			}
			else if (toc && func.toc == 0)
			{
				// Must then update TOC recursively
				func.toc = toc;
				func_queue.emplace_back(func);
			}

			return func;
		}

		func_queue.emplace_back(func);
		func.addr = addr;
		func.toc = toc;
		ppu_log.trace("Function 0x%x added (toc=0x%x)", addr, toc);
		return func;
	};

	static const auto advance = [](auto& _ptr, auto& ptr, u32 count)
	{
		const auto old_ptr = ptr;
		_ptr += count;
		ptr += count;
		return old_ptr;
	};

	// Register new TOC and find basic set of functions
	auto add_toc = [&](u32 toc)
	{
		if (!toc || toc == umax || !TOCs.emplace(toc).second)
		{
			return;
		}

		// Grope for OPD section (TODO: optimization, better constraints)
		for (const auto& seg : segs)
		{
			if (seg.size < 8) continue;

			const vm::cptr<void> seg_end = vm::cast(seg.addr + seg.size - 8);
			vm::cptr<u32> _ptr = vm::cast(seg.addr);
			auto ptr = get_ptr<u32>(_ptr);

			for (; _ptr <= seg_end;)
			{
				if (ptr[1] == toc && FN(x >= start && x < end && x % 4 == 0)(ptr[0]) && verify_func(_ptr.addr()))
				{
					// New function
					ppu_log.trace("OPD*: [0x%x] 0x%x (TOC=0x%x)", _ptr, ptr[0], ptr[1]);
					add_func(*ptr, addr_heap.count(_ptr.addr()) ? toc : 0, 0);
					advance(_ptr, ptr, 2);
				}
				else
				{
					advance(_ptr, ptr, 1);
				}
			}
		}
	};

	// Get next reliable function address
	auto get_limit = [&](u32 addr) -> u32
	{
		auto it = known_functions.lower_bound(addr);
		return it == known_functions.end() ? end : *it;
	};

	// Find references indiscriminately
	for (const auto& seg : segs)
	{
		if (seg.size < 4) continue;

		vm::cptr<u32> _ptr = vm::cast(seg.addr);
		const vm::cptr<void> seg_end = vm::cast(seg.addr + seg.size - 4);
		auto ptr = get_ptr<u32>(_ptr);

		for (; _ptr <= seg_end; advance(_ptr, ptr, 1))
		{
			const u32 value = *ptr;

			if (value % 4)
			{
				continue;
			}

			for (const auto& _seg : segs)
			{
				if (!_seg.addr) continue;

				if (value >= start && value < end)
				{
					addr_heap.emplace(value);
					break;
				}
			}
		}
	}

	// Find OPD section
	for (const auto& sec : secs)
	{
		if (sec.size % 8)
		{
			continue;
		}

		vm::cptr<void> sec_end = vm::cast(sec.addr + sec.size);

		// Probe
		for (vm::cptr<u32> _ptr = vm::cast(sec.addr); _ptr < sec_end; _ptr += 2)
		{
			auto ptr = get_ptr<u32>(_ptr);

			if (_ptr + 6 <= sec_end && !ptr[0] && !ptr[2] && ptr[1] == ptr[4] && ptr[3] == ptr[5])
			{
				// Special OPD format case (some homebrews)
				advance(_ptr, ptr, 4);
			}

			if (_ptr + 2 > sec_end)
			{
				sec_end.set(0);
				break;
			}

			const u32 addr = ptr[0];
			const u32 _toc = ptr[1];

			// Rough Table of Contents borders
			const u32 toc_begin = _toc - 0x8000;
			//const u32 toc_end = _toc + 0x7ffc;

			// TODO: improve TOC constraints
			if (toc_begin % 4 || !get_ptr<u8>(toc_begin) || toc_begin >= 0x40000000 || (toc_begin >= start && toc_begin < end))
			{
				sec_end.set(0);
				break;
			}

			if (addr % 4 || addr < start || addr >= end || !verify_func(_ptr.addr()))
			{
				sec_end.set(0);
				break;
			}
		}

		if (sec_end) ppu_log.notice("Reading OPD section at 0x%x...", sec.addr);

		// Mine
		for (vm::cptr<u32> _ptr = vm::cast(sec.addr); _ptr < sec_end; _ptr += 2)
		{
			auto ptr = get_ptr<u32>(_ptr);

			// Special case: see "Probe"
			if (!ptr[0]) advance(_ptr, ptr, 4);

			// Add function and TOC
			const u32 addr = ptr[0];
			const u32 toc = ptr[1];
			ppu_log.trace("OPD: [0x%x] 0x%x (TOC=0x%x)", _ptr, addr, toc);

			TOCs.emplace(toc);
			auto& func = add_func(addr, addr_heap.count(_ptr.addr()) ? toc : 0, 0);
			known_functions.emplace(addr);
		}
	}

	// Register TOC from entry point
	if (entry && !lib_toc)
	{
		lib_toc = get_ref<u32>(entry) ? get_ref<u32>(entry + 4) : get_ref<u32>(entry + 20);
	}

	// Secondary attempt
	if (TOCs.empty() && lib_toc)
	{
		add_toc(lib_toc);
	}

	// Clean TOCs
	for (auto&& pair : fmap)
	{
		if (pair.second.toc == umax)
		{
			pair.second.toc = 0;
		}
	}

	// Find .eh_frame section
	for (const auto& sec : secs)
	{
		if (sec.size % 4)
		{
			continue;
		}

		vm::cptr<void> sec_end = vm::cast(sec.addr + sec.size);

		// Probe
		for (vm::cptr<u32> _ptr = vm::cast(sec.addr); _ptr < sec_end;)
		{
			if (!_ptr.aligned() || _ptr.addr() < sec.addr || _ptr >= sec_end)
			{
				sec_end.set(0);
				break;
			}

			const auto ptr = get_ptr<u32>(_ptr);

			const u32 size = ptr[0] + 4;

			if (size == 4 && _ptr + 1 == sec_end)
			{
				// Null terminator
				break;
			}

			if (size % 4 || size < 0x10 || _ptr + size / 4 > sec_end)
			{
				sec_end.set(0);
				break;
			}

			if (ptr[1])
			{
				const u32 cie_off = _ptr.addr() - ptr[1] + 4;

				if (cie_off % 4 || cie_off < sec.addr || cie_off >= sec_end.addr())
				{
					sec_end.set(0);
					break;
				}
			}

			_ptr = vm::cast(_ptr.addr() + size);
		}

		if (sec_end && sec.size > 4) ppu_log.notice("Reading .eh_frame section at 0x%x...", sec.addr);

		// Mine
		for (vm::cptr<u32> _ptr = vm::cast(sec.addr); _ptr < sec_end; _ptr = vm::cast(_ptr.addr() + *get_ptr<u32>(_ptr) + 4))
		{
			const auto ptr = get_ptr<u32>(_ptr);

			if (ptr[0] == 0u)
			{
				// Null terminator
				break;
			}

			if (ptr[1] == 0u)
			{
				// CIE
				ppu_log.trace(".eh_frame: [0x%x] CIE 0x%x", ptr, ptr[0]);
			}
			else
			{
				// Get associated CIE (currently unused)
				const vm::cptr<u32> cie = vm::cast(_ptr.addr() - ptr[1] + 4);

				u32 addr = 0;
				u32 size = 0;

				// TODO: 64 bit or 32 bit values (approximation)
				if (ptr[2] == 0u && ptr[3] == 0u)
				{
					size = ptr[5];
				}
				else if ((ptr[2] + 1 == 0u || ptr[2] == 0u) && ptr[4] == 0u && ptr[5])
				{
					addr = ptr[3];
					size = ptr[5];
				}
				else if (ptr[2] + 1 && ptr[3])
				{
					addr = ptr[2];
					size = ptr[3];
				}
				else
				{
					ppu_log.error(".eh_frame: [0x%x] 0x%x, 0x%x, 0x%x, 0x%x, 0x%x", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
					continue;
				}

				// TODO: absolute/relative offset (approximation)
				if (addr > 0xc0000000)
				{
					addr += _ptr.addr() + 8;
				}

				ppu_log.trace(".eh_frame: [0x%x] FDE 0x%x (cie=*0x%x, addr=0x%x, size=0x%x)", ptr, ptr[0], cie, addr, size);

				// TODO: invalid offsets, zero offsets (removed functions?)
				if (addr % 4 || size % 4 || size > (end - start) || addr < start || addr + size > end)
				{
					if (addr) ppu_log.error(".eh_frame: Invalid function 0x%x", addr);
					continue;
				}

				//auto& func = add_func(addr, 0, 0);
				//func.attr += ppu_attr::known_size;
				//func.size = size;
				//known_functions.emplace(func);
			}
		}
	}

	bool used_fallback = false;

	if (func_queue.empty())
	{
		for (u32 addr : exported_funcs)
		{
			const u32 faddr = get_ref<u32>(addr);

			if (addr < start || addr >= start + segs[0].size)
			{
				// TODO: Reverse engineer how it works (maybe some flag in exports)

				if (faddr < start || faddr >= start + segs[0].size)
				{
					ppu_log.notice("Export not usable at 0x%x / 0x%x (0x%x...0x%x)", addr, faddr, start, start + segs[0].size);
					continue;
				}

				addr = faddr;
			}

			ppu_log.trace("Enqueued exported PPU function 0x%x for analysis", addr);

			add_func(addr, 0, 0);
			used_fallback = true;
		}
	}

	if (func_queue.empty() && segs[0].size >= 4u)
	{
		// Fallback, identify functions using callers (no jumptable detection, tail calls etc)
		ppu_log.warning("Looking for PPU functions using callers. ('%s')", name);

		vm::cptr<u32> _ptr = vm::cast(start);
		const vm::cptr<void> seg_end = vm::cast(end - 4);

		for (auto ptr = get_ptr<u32>(_ptr); _ptr <= seg_end; advance(_ptr, ptr, 1))
		{
			const u32 iaddr = _ptr.addr();

			const ppu_opcode_t op{*ptr};
			const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

			if ((type == ppu_itype::B || type == ppu_itype::BC) && op.lk && (!op.aa || verify_func(iaddr)))
			{
				const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

				if (target >= start && target < end && target != iaddr && target != iaddr + 4)
				{
					// TODO: Check full executability
					if (s_ppu_itype.decode(get_ref<u32>(target)) != ppu_itype::UNK)
					{
						ppu_log.trace("Enqueued PPU function 0x%x using a caller at 0x%x", target, iaddr);
						add_func(target, 0, 0);
						used_fallback = true;
					}
				}
			}
		}
	}

	// Main loop (func_queue may grow)
	for (usz i = 0; i < func_queue.size(); i++)
	{
		if (check_aborted && check_aborted())
		{
			return false;
		}

		ppu_function& func = func_queue[i];

		// Fixup TOCs
		if (func.toc && func.toc != umax)
		{
			for (u32 addr : func.callers)
			{
				ppu_function& caller = fmap[addr];

				if (!caller.toc)
				{
					add_func(addr, func.toc - caller.trampoline, 0);
				}
			}

			for (u32 addr : func.calls)
			{
				ppu_function& callee = fmap[addr];

				if (!callee.toc)
				{
					add_func(addr, func.toc + func.trampoline, 0);
				}
			}
		}

		if (func.blocks.empty())
		{
			// Special function analysis
			const vm::cptr<u32> _ptr = vm::cast(func.addr);
			const vm::cptr<void> fend = vm::cast(end);
			const auto ptr = get_ptr<u32>(_ptr);

			using namespace ppu_instructions;

			if (_ptr + 1 <= fend && (ptr[0] & 0xfc000001) == B({}, {}))
			{
				// Simple trampoline
				const u32 target = (ptr[0] & 0x2 ? 0 : _ptr.addr()) + ppu_opcode_t{ptr[0]}.bt24;

				if (target == func.addr)
				{
					// Special case
					func.size = 0x4;
					func.blocks.emplace(func.addr, func.size);
					func.attr += ppu_attr::no_return;
					continue;
				}

				if (target >= start && target < end && (~ptr[0] & 0x2 || verify_func(_ptr.addr())))
				{
					auto& new_func = add_func(target, func.toc, func.addr);

					if (new_func.blocks.empty())
					{
						func_queue.emplace_back(func);
						continue;
					}

					func.size = 0x4;
					func.blocks.emplace(func.addr, func.size);
					func.attr += new_func.attr & ppu_attr::no_return;
					func.calls.emplace(target);
					func.trampoline = 0;
					continue;
				}
			}

			if (_ptr + 0x4 <= fend &&
				(ptr[0] & 0xffff0000) == LIS(r11, 0) &&
				(ptr[1] & 0xffff0000) == ADDI(r11, r11, 0) &&
				ptr[2] == MTCTR(r11) &&
				ptr[3] == BCTR())
			{
				// Simple trampoline
				const u32 target = (ptr[0] << 16) + ppu_opcode_t{ptr[1]}.simm16;

				if (target >= start && target < end && verify_func(_ptr.addr()))
				{
					auto& new_func = add_func(target, func.toc, func.addr);

					if (new_func.blocks.empty())
					{
						func_queue.emplace_back(func);
						continue;
					}

					func.size = 0x10;
					func.blocks.emplace(func.addr, func.size);
					func.attr += new_func.attr & ppu_attr::no_return;
					func.calls.emplace(target);
					func.trampoline = 0;
					continue;
				}
			}

			if (_ptr + 0x7 <= fend &&
				ptr[0] == STD(r2, r1, 0x28) &&
				(ptr[1] & 0xffff0000) == ADDIS(r12, r2, {}) &&
				(ptr[2] & 0xffff0000) == LWZ(r11, r12, {}) &&
				(ptr[3] & 0xffff0000) == ADDIS(r2, r2, {}) &&
				(ptr[4] & 0xffff0000) == ADDI(r2, r2, {}) &&
				ptr[5] == MTCTR(r11) &&
				ptr[6] == BCTR())
			{
				func.toc = -1;
				func.size = 0x1C;
				func.blocks.emplace(func.addr, func.size);
				func.attr += ppu_attr::known_size;

				// Look for another imports to fill gaps (hack)
				auto _p2 = _ptr + 7;
				auto p2 = get_ptr<u32>(_p2);

				while (_p2 + 0x7 <= fend &&
					p2[0] == STD(r2, r1, 0x28) &&
					(p2[1] & 0xffff0000) == ADDIS(r12, r2, {}) &&
					(p2[2] & 0xffff0000) == LWZ(r11, r12, {}) &&
					(p2[3] & 0xffff0000) == ADDIS(r2, r2, {}) &&
					(p2[4] & 0xffff0000) == ADDI(r2, r2, {}) &&
					p2[5] == MTCTR(r11) &&
					p2[6] == BCTR())
				{
					auto& next = add_func(_p2.addr(), -1, func.addr);
					next.size = 0x1C;
					next.blocks.emplace(next.addr, next.size);
					next.attr += ppu_attr::known_size;
					advance(_p2, p2, 7);
				}

				continue;
			}

			if (_ptr + 0x7 <= fend &&
				ptr[0] == STD(r2, r1, 0x28) &&
				(ptr[1] & 0xffff0000) == ADDIS(r2, r2, {}) &&
				(ptr[2] & 0xffff0000) == ADDI(r2, r2, {}) &&
				(ptr[3] & 0xffff0000) == LIS(r11, {}) &&
				(ptr[4] & 0xffff0000) == ADDI(r11, r11, {}) &&
				ptr[5] == MTCTR(r11) &&
				ptr[6] == BCTR())
			{
				// Trampoline with TOC
				const u32 target = (ptr[3] << 16) + s16(ptr[4]);
				const u32 toc_add = (ptr[1] << 16) + s16(ptr[2]);
				constexpr u32 func_size = 0x1C;

				if (target >= start && target < end && verify_func((_ptr + 3).addr()) && target - func.addr >= func_size)
				{
					auto& new_func = add_func(target, 0, func.addr);

					if (func.toc && func.toc != umax && new_func.toc == 0)
					{
						const u32 toc = func.toc + toc_add;
						add_toc(toc);
						add_func(new_func.addr, toc, 0);
					}
					else if (new_func.toc && new_func.toc != umax && func.toc == 0)
					{
						const u32 toc = new_func.toc - toc_add;
						add_toc(toc);
						add_func(func.addr, toc, 0);
					}
					//else if (new_func.toc - func.toc != toc_add)
					//{
					//	func.toc = -1;
					//	new_func.toc = -1;
					//}

					if (new_func.blocks.empty())
					{
						func_queue.emplace_back(func);
						continue;
					}

					func.size = 0x1C;
					func.blocks.emplace(func.addr, func.size);
					func.attr += new_func.attr & ppu_attr::no_return;
					func.calls.emplace(target);
					func.trampoline = toc_add;
					continue;
				}
			}

			if (_ptr + 4 <= fend &&
				ptr[0] == STD(r2, r1, 0x28) &&
				(ptr[1] & 0xffff0000) == ADDIS(r2, r2, {}) &&
				(ptr[2] & 0xffff0000) == ADDI(r2, r2, {}) &&
				(ptr[3] & 0xfc000001) == B({}, {}))
			{
				// Trampoline with TOC
				const u32 toc_add = (ptr[1] << 16) + s16(ptr[2]);
				const u32 target = (ptr[3] & 0x2 ? 0 : (_ptr + 3).addr()) + ppu_opcode_t{ptr[3]}.bt24;

				if (target >= start && target < end && (~ptr[3] & 0x2 || verify_func((_ptr + 3).addr())))
				{
					auto& new_func = add_func(target, 0, func.addr);

					if (func.toc && func.toc != umax && new_func.toc == 0)
					{
						const u32 toc = func.toc + toc_add;
						add_toc(toc);
						add_func(new_func.addr, toc, 0);
					}
					else if (new_func.toc && new_func.toc != umax && func.toc == 0)
					{
						const u32 toc = new_func.toc - toc_add;
						add_toc(toc);
						add_func(func.addr, toc, 0);
					}
					//else if (new_func.toc - func.toc != toc_add)
					//{
					//	func.toc = -1;
					//	new_func.toc = -1;
					//}

					if (new_func.blocks.empty())
					{
						func_queue.emplace_back(func);
						continue;
					}

					func.size = 0x10;
					func.blocks.emplace(func.addr, func.size);
					func.attr += new_func.attr & ppu_attr::no_return;
					func.calls.emplace(target);
					func.trampoline = toc_add;
					continue;
				}
			}

			if (_ptr + 8 <= fend &&
				(ptr[0] & 0xffff0000) == LI(r12, 0) &&
				(ptr[1] & 0xffff0000) == ORIS(r12, r12, 0) &&
				(ptr[2] & 0xffff0000) == LWZ(r12, r12, 0) &&
				ptr[3] == STD(r2, r1, 0x28) &&
				ptr[4] == LWZ(r0, r12, 0) &&
				ptr[5] == LWZ(r2, r12, 4) &&
				ptr[6] == MTCTR(r0) &&
				ptr[7] == BCTR())
			{
				// The most used simple import stub
				func.toc = -1;
				func.size = 0x20;
				func.blocks.emplace(func.addr, func.size);
				known_functions.emplace(func.addr);
				func.attr += ppu_attr::known_size;

				// Look for another imports to fill gaps (hack)
				auto _p2 = _ptr + 8;
				auto p2 = get_ptr<u32>(_p2);

				while (_p2 + 8 <= fend &&
					(p2[0] & 0xffff0000) == LI(r12, 0) &&
					(p2[1] & 0xffff0000) == ORIS(r12, r12, 0) &&
					(p2[2] & 0xffff0000) == LWZ(r12, r12, 0) &&
					p2[3] == STD(r2, r1, 0x28) &&
					p2[4] == LWZ(r0, r12, 0) &&
					p2[5] == LWZ(r2, r12, 4) &&
					p2[6] == MTCTR(r0) &&
					p2[7] == BCTR())
				{
					auto& next = add_func(_p2.addr(), -1, func.addr);
					next.size = 0x20;
					next.blocks.emplace(next.addr, next.size);
					next.attr += ppu_attr::known_size;
					advance(_p2, p2, 8);
					known_functions.emplace(next.addr);
				}

				continue;
			}

			if (_ptr + 3 <= fend &&
				ptr[0] == 0x7c0004acu &&
				ptr[1] == 0x00000000u &&
				ptr[2] == BLR())
			{
				// Weird function (illegal instruction)
				func.size = 0xc;
				func.blocks.emplace(func.addr, func.size);
				//func.attr += ppu_attr::no_return;
				continue;
			}

			if (const u32 len = ppu_test(ptr, get_ptr<void>(fend), ppu_patterns::abort))
			{
				// Function "abort"
				ppu_log.notice("Function [0x%x]: 'abort'", func.addr);
				func.attr += ppu_attr::no_return;
				func.attr += ppu_attr::known_size;
				func.size = len;
			}

			// TODO: detect no_return, scribe more TODOs

			// Acknowledge completion
			func.blocks.emplace(vm::cast(func.addr), 0);
		}

		// Get function limit
		const u32 func_end = std::min<u32>(get_limit(func.addr + 1), func.attr & ppu_attr::known_size ? func.addr + func.size : end);

		// Block analysis workload
		std::vector<std::reference_wrapper<std::pair<const u32, u32>>> block_queue;

		// Add new block for analysis
		auto add_block = [&](u32 addr) -> bool
		{
			if (addr < func.addr || addr >= func_end)
			{
				return false;
			}

			const auto _pair = func.blocks.emplace(addr, 0);

			if (_pair.second)
			{
				block_queue.emplace_back(*_pair.first);
				return true;
			}

			return false;
		};

		for (auto& block : func.blocks)
		{
			if (!block.second && block.first < func_end)
			{
				block_queue.emplace_back(block);
			}
		}

		// TODO: lower priority?
		if (func.attr & ppu_attr::no_size)
		{
			// Get next function
			const auto _next = fmap.lower_bound(func.blocks.crbegin()->first + 1);

			// Get limit
			const u32 func_end2 = _next == fmap.end() ? func_end : std::min<u32>(_next->first, func_end);

			// Set more block entries
			std::for_each(addr_heap.lower_bound(func.addr), addr_heap.lower_bound(func_end2), add_block);
		}

		const bool was_empty = block_queue.empty();

		// Block loop (block_queue may grow, may be aborted via clearing)
		for (usz j = 0; j < block_queue.size(); j++)
		{
			auto& block = block_queue[j].get();

			vm::cptr<u32> _ptr = vm::cast(block.first);
			auto ptr = ensure(get_ptr<u32>(_ptr));

			for (; _ptr.addr() < func_end;)
			{
				const u32 iaddr = _ptr.addr();
				const ppu_opcode_t op{*advance(_ptr, ptr, 1)};
				const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

				if (type == ppu_itype::UNK)
				{
					// Invalid blocks will remain empty
					break;
				}
				else if (type == ppu_itype::B || type == ppu_itype::BC)
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target < start || target >= end)
					{
						ppu_log.warning("[0x%x] Invalid branch at 0x%x -> 0x%x", func.addr, iaddr, target);
						continue;
					}

					if (!op.aa && target == _ptr.addr() && _ptr.addr() < func_end)
					{
						ppu_log.notice("[0x%x] Branch to next at 0x%x -> 0x%x", func.addr, iaddr, target);
					}

					const bool is_call = op.lk && target != iaddr && target != _ptr.addr() && _ptr.addr() < func_end;
					const auto pfunc = is_call ? &add_func(target, 0, 0) : nullptr;

					if (pfunc && pfunc->blocks.empty())
					{
						// Postpone analysis (no info)
						block_queue.clear();
						break;
					}

					// Add next block if necessary
					if ((is_call && !(pfunc->attr & ppu_attr::no_return)) || (type == ppu_itype::BC && (op.bo & 0x14) != 0x14))
					{
						add_block(_ptr.addr());
					}

					if (is_call && pfunc->attr & ppu_attr::no_return)
					{
						// Nothing
					}
					else if (is_call || target < func.addr || target >= func_end)
					{
						// Add function call (including obvious tail call)
						add_func(target, 0, 0);
					}
					else
					{
						// Add block
						add_block(target);
					}

					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type == ppu_itype::BCLR)
				{
					if (op.lk || (op.bo & 0x14) != 0x14)
					{
						add_block(_ptr.addr());
					}

					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type == ppu_itype::BCCTR)
				{
					if (op.lk || (op.bo & 0x10) != 0x10)
					{
						add_block(_ptr.addr());
					}
					else
					{
						// Analyse jumptable (TODO)
						const u32 jt_addr = _ptr.addr();
						const u32 jt_end = func_end;

						for (; _ptr.addr() < jt_end; advance(_ptr, ptr, 1))
						{
							const u32 addr = jt_addr + *ptr;

							if (addr == jt_addr)
							{
								// TODO (cannot branch to jumptable itself)
								break;
							}

							if (addr % 4 || addr < func.addr || addr >= jt_end)
							{
								break;
							}

							add_block(addr);
						}

						if (jt_addr != jt_end && _ptr.addr() == jt_addr)
						{
							// Acknowledge jumptable detection failure
							if (!(func.attr & ppu_attr::no_size))
							{
								ppu_log.warning("[0x%x] Jump table not found! 0x%x-0x%x", func.addr, jt_addr, jt_end);
							}

							func.attr += ppu_attr::no_size;
							add_block(jt_addr);
							block_queue.clear();
						}
						else
						{
							ppu_log.trace("[0x%x] Jump table found: 0x%x-0x%x", func.addr, jt_addr, _ptr);
						}
					}

					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type & ppu_itype::trap && op.bo)
				{
					if (can_trap_continue(op, type))
					{
						add_block(_ptr.addr());
					}

					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type == ppu_itype::SC)
				{
					add_block(_ptr.addr());
					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type == ppu_itype::STDU && func.attr & ppu_attr::no_size && (op.opcode == *ptr || *ptr == ppu_instructions::BLR()))
				{
					// Hack
					ppu_log.success("[0x%x] Instruction repetition: 0x%08x", iaddr, op.opcode);
					add_block(_ptr.addr());
					block.second = _ptr.addr() - block.first;
					break;
				}
			}
		}

		if (block_queue.empty() && !was_empty)
		{
			// Block aborted: abort function, postpone
			func_queue.emplace_back(func);
			continue;
		}

		// Finalization: determine function size
		if (!(func.attr & ppu_attr::known_size))
		{
			const auto last = func.blocks.crbegin();

			if (last != func.blocks.crend())
			{
				func.size = last->first + last->second - func.addr;
			}
		}

		// Finalization: normalize blocks
		for (auto& block : func.blocks)
		{
			const auto next = func.blocks.upper_bound(block.first);

			// Normalize block if necessary
			if (next != func.blocks.end() && block.second > next->first - block.first)
			{
				block.second = next->first - block.first;
			}

			// Invalidate blocks out of the function
			const u32 fend = func.addr + func.size;
			const u32 bend = block.first + block.second;

			if (block.first >= fend)
			{
				block.second = 0;
			}
			else if (bend > fend)
			{
				block.second -= bend - fend;
			}
		}

		// Finalization: process remaining tail calls
		for (const auto& block : func.blocks)
		{
			for (vm::cptr<u32> _ptr = vm::cast(block.first); _ptr.addr() < block.first + block.second;)
			{
				const u32 iaddr = _ptr.addr();
				const ppu_opcode_t op{get_ref<u32>(_ptr++)};
				const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

				if (type == ppu_itype::B || type == ppu_itype::BC)
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target >= start && target < end && (!op.aa || verify_func(iaddr)))
					{
						if (target < func.addr || target >= func.addr + func.size)
						{
							func.calls.emplace(target);
							add_func(target, func.toc ? func.toc + func.trampoline : 0, func.addr);
						}
					}
				}
				else if (type == ppu_itype::BCCTR && !op.lk)
				{
					// Jumptable (do not touch entries)
					break;
				}
			}
		}

		// Finalization: decrease known function size (TODO)
		if (func.attr & ppu_attr::known_size)
		{
			const auto last = func.blocks.crbegin();

			if (last != func.blocks.crend())
			{
				func.size = std::min<u32>(func.size, last->first + last->second - func.addr);
			}
		}
	}

	// Function shrinkage, disabled (TODO: it's potentially dangerous but improvable)
	for (auto& _pair : fmap)
	{
		auto& func = _pair.second;

		// Get next function addr
		const auto _next = fmap.lower_bound(_pair.first + 1);

		const u32 next = _next == fmap.end() ? end : _next->first;

		// Just ensure that functions don't overlap
		if (func.addr + func.size > next)
		{
			ppu_log.trace("Function overlap: [0x%x] 0x%x -> 0x%x", func.addr, func.size, next - func.addr);
			continue; //func.size = next - func.addr;

			// Also invalidate blocks
			for (auto& block : func.blocks)
			{
				if (block.first + block.second > next)
				{
					block.second = block.first >= next ? 0 : next - block.first;
				}
			}
		}

		// Suspicious block start
		u32 start = func.addr + func.size;

		if (next == end)
		{
			continue;
		}

		// Analyse gaps between functions
		for (vm::cptr<u32> _ptr = vm::cast(start); _ptr.addr() < next;)
		{
			const u32 addr = _ptr.addr();
			const ppu_opcode_t op{get_ref<u32>(_ptr++)};
			const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

			if (type == ppu_itype::UNK)
			{
				break;
			}

			if (addr == start && op.opcode == ppu_instructions::NOP())
			{
				if (start == func.addr + func.size)
				{
					// Extend function with tail NOPs (hack)
					func.size += 4;
				}

				start += 4;
				continue;
			}

			if (type == ppu_itype::SC && op.opcode != ppu_instructions::SC(0))
			{
				break;
			}

			if (addr == start && op.opcode == ppu_instructions::BLR())
			{
				start += 4;
				continue;
			}

			if (type == ppu_itype::B || type == ppu_itype::BC)
			{
				const u32 target = (op.aa ? 0 : addr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

				if (target == addr)
				{
					break;
				}

				_ptr.set(next);
			}
			else if (type == ppu_itype::BCLR || type == ppu_itype::BCCTR)
			{
				_ptr.set(next);
			}

			if (_ptr.addr() >= next)
			{
				ppu_log.trace("Function gap: [0x%x] 0x%x bytes at 0x%x", func.addr, next - start, start);
				break;
			}
		}
	}

	// Fill TOCs for trivial case
	if (TOCs.size() == 1)
	{
		lib_toc = *TOCs.begin();

		for (auto&& pair : fmap)
		{
			if (pair.second.toc == 0)
			{
				pair.second.toc = lib_toc;
			}
		}
	}

	(fmap.empty() ? ppu_log.error : ppu_log.notice)("Function analysis: %zu functions (%zu enqueued)", fmap.size(), func_queue.size());

	// Decompose functions to basic blocks
	if (!entry && !sec_end)
	{
		// Regenerate end from blocks
		end = 0;
	}

	u32 per_instruction_bytes = 0;

	for (auto&& [_, func] : fmap)
	{
		if (func.attr & ppu_attr::no_size && entry)
		{
			// Disabled for PRX for now
			const u32 lim = get_limit(func.addr);

			ppu_log.warning("Function 0x%x will be compiled on per-instruction basis (next=0x%x)", func.addr, lim);

			for (u32 addr = func.addr; addr < lim; addr += 4)
			{
				auto& block = fmap[addr];
				block.addr = addr;
				block.size = 4;
				block.toc  = func.toc;
				block.attr = ppu_attr::no_size;
			}

			per_instruction_bytes += utils::sub_saturate<u32>(lim, func.addr);
			continue;
		}

		for (const auto& [addr, size] : func.blocks)
		{
			if (!size)
			{
				continue;
			}

			auto& block = fmap[addr];

			if (block.addr || block.size)
			{
				ppu_log.trace("Block __0x%x exists (size=0x%x)", block.addr, block.size);
				continue;
			}

			block.addr = addr;
			block.size = size;
			block.toc  = func.toc;
			ppu_log.trace("Block __0x%x added (func=0x%x, size=0x%x, toc=0x%x)", block.addr, _, block.size, block.toc);

			if (!entry && !sec_end)
			{
				// Workaround for SPRX: update end to the last found function
				end = std::max<u32>(end, block.addr + block.size);
			}
		}
	}

	// Simple callable block analysis
	std::vector<std::pair<u32, u32>> block_queue;
	block_queue.reserve(128000);

	std::unordered_set<u32> block_set;

	// Check relocations which may involve block addresses (usually it's type 1)
	for (auto& rel : this->relocs)
	{
		// Disabled (TODO)
		//if (!vm::check_addr<4>(rel.addr))
		{
			continue;
		}

		const u32 target = get_ref<u32>(rel.addr);

		if (target % 4 || target < start || target >= end)
		{
			continue;
		}

		switch (rel.type)
		{
		case 1:
		case 24:
		case 26:
		case 27:
		case 28:
		case 107:
		case 108:
		case 109:
		case 110:
		{
			ppu_log.trace("Added block from reloc: 0x%x (0x%x, %u) (heap=%d)", target, rel.addr, rel.type, addr_heap.count(target));
			block_queue.emplace_back(target, 0);
			block_set.emplace(target);
			continue;
		}
		default:
		{
			continue;
		}
		}
	}

	u32 exp = start;
	u32 lim = end;

	// Start with full scan
	block_queue.emplace_back(exp, lim);

	// Add entries from patches (on per-instruction basis)
	for (u32 addr : applied)
	{
		if (addr % 4 == 0 && addr >= start && addr < segs[0].addr + segs[0].size && !block_set.count(addr))
		{
			block_queue.emplace_back(addr, addr + 4);
			block_set.emplace(addr);
		}
	}

	// block_queue may grow
	for (usz i = 0; i < block_queue.size(); i++)
	{
		std::tie(exp, lim) = block_queue[i];

		if (lim == 0)
		{
			// Find next function
			const auto found = fmap.upper_bound(exp);

			if (found != fmap.cend())
			{
				lim = found->first;
			}

			ppu_log.trace("Block rescan: addr=0x%x, lim=0x%x", exp, lim);
		}

		while (exp < lim)
		{
			u32 i_pos = exp;

			u32 block_edges[16];
			u32 edge_count = 0;

			bool is_good = true;
			bool is_fallback = true;

			for (; i_pos < lim; i_pos += 4)
			{
				const ppu_opcode_t op{get_ref<u32>(i_pos)};

				switch (auto type = s_ppu_itype.decode(op.opcode))
				{
				case ppu_itype::UNK:
				case ppu_itype::ECIWX:
				case ppu_itype::ECOWX:
				{
					// Seemingly bad instruction, skip this block
					is_good = false;
					break;
				}
				case ppu_itype::TDI:
				case ppu_itype::TWI:
				{
					if (op.bo && (op.ra == 1u || op.ra == 13u || op.ra == 2u))
					{
						// Non-user registers, checking them against a constant value makes no sense
						is_good = false;
						break;
					}

					[[fallthrough]];
				}
				case ppu_itype::TD:
				case ppu_itype::TW:
				{
					if (!op.bo)
					{
						continue;
					}

					if (!can_trap_continue(op, type))
					{
						is_fallback = false;
					}

					[[fallthrough]];
				}
				case ppu_itype::B:
				case ppu_itype::BC:
				{
					if (type == ppu_itype::B || (type == ppu_itype::BC && (op.bo & 0x14) == 0x14))
					{
						is_fallback = false;
					}

					if (type == ppu_itype::B || type == ppu_itype::BC)
					{
						if (type == ppu_itype::BC && (op.bo & 0x14) == 0x14 && op.bo & 0xB)
						{
							// Invalid form
							is_good = false;
							break;
						}

						if (entry == 0 && op.aa)
						{
							// Ignore absolute branches in PIC (PRX)
							is_good = false;
							break;
						}

						const u32 target = (op.aa ? 0 : i_pos) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

						if (target < segs[0].addr || target >= segs[0].addr + segs[0].size)
						{
							// Sanity check
							is_good = false;
							break;
						}

						const ppu_opcode_t test_op{get_ref<u32>(target)};
						const auto type0 = s_ppu_itype.decode(test_op.opcode);

						if (type0 == ppu_itype::UNK)
						{
							is_good = false;
							break;
						}

						// Test another instruction just in case (testing more is unlikely to improve results by much)

						if (type0 == ppu_itype::SC || (type0 & ppu_itype::trap && !can_trap_continue(test_op, type0)))
						{
							// May not be followed by a valid instruction
						}
						else if (!(type0 & ppu_itype::branch))
						{
							if (target + 4 >= segs[0].addr + segs[0].size)
							{
								is_good = false;
								break;
							}

							const auto type1 = s_ppu_itype.decode(get_ref<u32>(target + 4));

							if (type1 == ppu_itype::UNK)
							{
								is_good = false;
								break;
							}
						}
						else if (u32 target0 = (test_op.aa ? 0 : target) + (type0 == ppu_itype::B ? +test_op.bt24 : +test_op.bt14);
							(type0 == ppu_itype::B || type0 == ppu_itype::BC) && (target0 < segs[0].addr || target0 >= segs[0].addr + segs[0].size))
						{
							// Sanity check
							is_good = false;
							break;
						}

						if (target != i_pos && !fmap.contains(target))
						{
							if (block_set.count(target) == 0)
							{
								ppu_log.trace("Block target found: 0x%x (i_pos=0x%x)", target, i_pos);
								block_queue.emplace_back(target, 0);
								block_set.emplace(target);
							}
						}
					}

					[[fallthrough]];
				}
				case ppu_itype::BCCTR:
				case ppu_itype::BCLR:
				case ppu_itype::SC:
				{
					if (type == ppu_itype::SC && op.opcode != ppu_instructions::SC(0) && op.opcode != ppu_instructions::SC(1))
					{
						// Strict garbage filter
						is_good = false;
						break;
					}

					if (type == ppu_itype::BCCTR && op.opcode & 0xe000)
					{
						// Garbage filter
						is_good = false;
						break;
					}

					if (type == ppu_itype::BCLR && op.opcode & 0xe000)
					{
						// Garbage filter
						is_good = false;
						break;
					}

					if (type == ppu_itype::BCCTR || type == ppu_itype::BCLR)
					{
						is_fallback = false;
					}

					if ((type & ppu_itype::branch && op.lk) || is_fallback)
					{
						// if farther instructions are valid: register all blocks
						// Otherwise, register none (all or nothing)
						if (edge_count < std::size(block_edges) && i_pos + 4 < lim)
						{
							block_edges[edge_count++] = i_pos + 4;
							is_fallback = true; // Reset value
							continue;
						}
					}

					// Good block terminator found, add single block
					break;
				}
				default:
				{
					// Normal instruction: keep scanning
					continue;
				}
				}

				break;
			}

			if (i_pos < lim)
			{
				i_pos += 4;
			}
			else if (is_good && is_fallback && lim < end)
			{
				// Register fallback target
				if (!fmap.contains(lim) && !block_set.contains(lim))
				{
					ppu_log.trace("Block target found: 0x%x (i_pos=0x%x)", lim, i_pos);
					block_queue.emplace_back(lim, 0);
					block_set.emplace(lim);
				}
			}

			if (is_good)
			{
				for (u32 it = 0, prev_addr = exp; it <= edge_count; it++)
				{
					const u32 block_end = it < edge_count ? block_edges[it] : i_pos;
					const u32 block_begin = std::exchange(prev_addr, block_end);

					auto& block = fmap[block_begin];

					if (!block.addr)
					{
						block.addr = block_begin;
						block.size = block_end - block_begin;
						ppu_log.trace("Block __0x%x added (size=0x%x)", block.addr, block.size);

						if (get_limit(block_begin) == end)
						{
							block.attr += ppu_attr::no_size;
						}
					}
				}
			}

			exp = i_pos;
		}
	}

	// Remove overlaps in blocks
	for (auto it = fmap.begin(), end = fmap.end(); it != fmap.end(); it++)
	{
		const auto next = std::next(it);

		if (next != end && next->first < it->first + it->second.size)
		{
			it->second.size = next->first - it->first;
		}
	}

	// Convert map to vector (destructive)
	for (auto it = fmap.begin(); it != fmap.end(); it = fmap.begin())
	{
		ppu_function block = std::move(fmap.extract(it).mapped());

		if (block.attr & ppu_attr::no_size && block.size > 4 && !used_fallback)
		{
			ppu_log.warning("Block 0x%x will be compiled on per-instruction basis (size=0x%x)", block.addr, block.size);

			for (u32 addr = block.addr; addr < block.addr + block.size; addr += 4)
			{
				auto& i = funcs.emplace_back();
				i.addr = addr;
				i.size = 4;
				i.toc  = block.toc;
				i.attr = ppu_attr::no_size;
			}

			per_instruction_bytes += block.size;
			continue;
		}

		funcs.emplace_back(std::move(block));
	}

	if (per_instruction_bytes)
	{
		const bool error = per_instruction_bytes >= 200 && per_instruction_bytes / 4 >= utils::aligned_div<u32>(::size32(funcs), 128);
		(error ? ppu_log.error : ppu_log.notice)("%d instructions will be compiled on per-instruction basis in total", per_instruction_bytes / 4);
	}

	ppu_log.notice("Block analysis: %zu blocks (%zu enqueued)", funcs.size(), block_queue.size());
	return true;
}

// Temporarily
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

void ppu_acontext::UNK(ppu_opcode_t op)
{
	std::fill_n(gpr, 32, spec_gpr{});
	ppu_log.error("Unknown/Illegal opcode: 0x%08x at 0x%x", op.opcode, cia);
}

void ppu_acontext::MFVSCR(ppu_opcode_t op)
{
}

void ppu_acontext::MTVSCR(ppu_opcode_t op)
{
}

void ppu_acontext::VADDCUW(ppu_opcode_t op)
{
}

void ppu_acontext::VADDFP(ppu_opcode_t op)
{
}

void ppu_acontext::VADDSBS(ppu_opcode_t op)
{
}

void ppu_acontext::VADDSHS(ppu_opcode_t op)
{
}

void ppu_acontext::VADDSWS(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUBM(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUBS(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUHM(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUHS(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUWM(ppu_opcode_t op)
{
}

void ppu_acontext::VADDUWS(ppu_opcode_t op)
{
}

void ppu_acontext::VAND(ppu_opcode_t op)
{
}

void ppu_acontext::VANDC(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGSB(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGSH(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGSW(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGUB(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGUH(ppu_opcode_t op)
{
}

void ppu_acontext::VAVGUW(ppu_opcode_t op)
{
}

void ppu_acontext::VCFSX(ppu_opcode_t op)
{
}

void ppu_acontext::VCFUX(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPBFP(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPEQFP(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPEQUB(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPEQUH(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPEQUW(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGEFP(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTFP(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTSB(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTSH(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTSW(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTUB(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTUH(ppu_opcode_t op)
{
}

void ppu_acontext::VCMPGTUW(ppu_opcode_t op)
{
}

void ppu_acontext::VCTSXS(ppu_opcode_t op)
{
}

void ppu_acontext::VCTUXS(ppu_opcode_t op)
{
}

void ppu_acontext::VEXPTEFP(ppu_opcode_t op)
{
}

void ppu_acontext::VLOGEFP(ppu_opcode_t op)
{
}

void ppu_acontext::VMADDFP(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXFP(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXSB(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXSH(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXSW(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXUB(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXUH(ppu_opcode_t op)
{
}

void ppu_acontext::VMAXUW(ppu_opcode_t op)
{
}

void ppu_acontext::VMHADDSHS(ppu_opcode_t op)
{
}

void ppu_acontext::VMHRADDSHS(ppu_opcode_t op)
{
}

void ppu_acontext::VMINFP(ppu_opcode_t op)
{
}

void ppu_acontext::VMINSB(ppu_opcode_t op)
{
}

void ppu_acontext::VMINSH(ppu_opcode_t op)
{
}

void ppu_acontext::VMINSW(ppu_opcode_t op)
{
}

void ppu_acontext::VMINUB(ppu_opcode_t op)
{
}

void ppu_acontext::VMINUH(ppu_opcode_t op)
{
}

void ppu_acontext::VMINUW(ppu_opcode_t op)
{
}

void ppu_acontext::VMLADDUHM(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGHB(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGHH(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGHW(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGLB(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGLH(ppu_opcode_t op)
{
}

void ppu_acontext::VMRGLW(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMMBM(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMSHM(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMSHS(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMUBM(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMUHM(ppu_opcode_t op)
{
}

void ppu_acontext::VMSUMUHS(ppu_opcode_t op)
{
}

void ppu_acontext::VMULESB(ppu_opcode_t op)
{
}

void ppu_acontext::VMULESH(ppu_opcode_t op)
{
}

void ppu_acontext::VMULEUB(ppu_opcode_t op)
{
}

void ppu_acontext::VMULEUH(ppu_opcode_t op)
{
}

void ppu_acontext::VMULOSB(ppu_opcode_t op)
{
}

void ppu_acontext::VMULOSH(ppu_opcode_t op)
{
}

void ppu_acontext::VMULOUB(ppu_opcode_t op)
{
}

void ppu_acontext::VMULOUH(ppu_opcode_t op)
{
}

void ppu_acontext::VNMSUBFP(ppu_opcode_t op)
{
}

void ppu_acontext::VNOR(ppu_opcode_t op)
{
}

void ppu_acontext::VOR(ppu_opcode_t op)
{
}

void ppu_acontext::VPERM(ppu_opcode_t op)
{
}

void ppu_acontext::VPKPX(ppu_opcode_t op)
{
}

void ppu_acontext::VPKSHSS(ppu_opcode_t op)
{
}

void ppu_acontext::VPKSHUS(ppu_opcode_t op)
{
}

void ppu_acontext::VPKSWSS(ppu_opcode_t op)
{
}

void ppu_acontext::VPKSWUS(ppu_opcode_t op)
{
}

void ppu_acontext::VPKUHUM(ppu_opcode_t op)
{
}

void ppu_acontext::VPKUHUS(ppu_opcode_t op)
{
}

void ppu_acontext::VPKUWUM(ppu_opcode_t op)
{
}

void ppu_acontext::VPKUWUS(ppu_opcode_t op)
{
}

void ppu_acontext::VREFP(ppu_opcode_t op)
{
}

void ppu_acontext::VRFIM(ppu_opcode_t op)
{
}

void ppu_acontext::VRFIN(ppu_opcode_t op)
{
}

void ppu_acontext::VRFIP(ppu_opcode_t op)
{
}

void ppu_acontext::VRFIZ(ppu_opcode_t op)
{
}

void ppu_acontext::VRLB(ppu_opcode_t op)
{
}

void ppu_acontext::VRLH(ppu_opcode_t op)
{
}

void ppu_acontext::VRLW(ppu_opcode_t op)
{
}

void ppu_acontext::VRSQRTEFP(ppu_opcode_t op)
{
}

void ppu_acontext::VSEL(ppu_opcode_t op)
{
}

void ppu_acontext::VSL(ppu_opcode_t op)
{
}

void ppu_acontext::VSLB(ppu_opcode_t op)
{
}

void ppu_acontext::VSLDOI(ppu_opcode_t op)
{
}

void ppu_acontext::VSLH(ppu_opcode_t op)
{
}

void ppu_acontext::VSLO(ppu_opcode_t op)
{
}

void ppu_acontext::VSLW(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTB(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTH(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTISB(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTISH(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTISW(ppu_opcode_t op)
{
}

void ppu_acontext::VSPLTW(ppu_opcode_t op)
{
}

void ppu_acontext::VSR(ppu_opcode_t op)
{
}

void ppu_acontext::VSRAB(ppu_opcode_t op)
{
}

void ppu_acontext::VSRAH(ppu_opcode_t op)
{
}

void ppu_acontext::VSRAW(ppu_opcode_t op)
{
}

void ppu_acontext::VSRB(ppu_opcode_t op)
{
}

void ppu_acontext::VSRH(ppu_opcode_t op)
{
}

void ppu_acontext::VSRO(ppu_opcode_t op)
{
}

void ppu_acontext::VSRW(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBCUW(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBFP(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBSBS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBSHS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBSWS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUBM(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUBS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUHM(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUHS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUWM(ppu_opcode_t op)
{
}

void ppu_acontext::VSUBUWS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUMSWS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUM2SWS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUM4SBS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUM4SHS(ppu_opcode_t op)
{
}

void ppu_acontext::VSUM4UBS(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKHPX(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKHSB(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKHSH(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKLPX(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKLSB(ppu_opcode_t op)
{
}

void ppu_acontext::VUPKLSH(ppu_opcode_t op)
{
}

void ppu_acontext::VXOR(ppu_opcode_t op)
{
}

void ppu_acontext::TDI(ppu_opcode_t op)
{
}

void ppu_acontext::TWI(ppu_opcode_t op)
{
}

void ppu_acontext::MULLI(ppu_opcode_t op)
{
	const s64 amin = gpr[op.ra].imin;
	const s64 amax = gpr[op.ra].imax;

	// Undef or mixed range (default)
	s64 min = 0;
	s64 max = -1;

	// Special cases like powers of 2 and their negations are not handled
	if (amin <= amax)
	{
		min = amin * op.simm16;
		max = amax * op.simm16;

		// Check overflow
		if (min >> 63 != utils::mulh64(amin, op.simm16) || max >> 63 != utils::mulh64(amax, op.simm16))
		{
			min = 0;
			max = -1;
		}
		else if (min > max)
		{
			std::swap(min, max);
		}
	}

	gpr[op.rd] = spec_gpr::range(min, max, gpr[op.ra].tz() + std::countr_zero<u64>(op.simm16));
}

void ppu_acontext::SUBFIC(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + spec_gpr::fixed(op.simm16) + spec_gpr::fixed(1);
}

void ppu_acontext::CMPLI(ppu_opcode_t op)
{
}

void ppu_acontext::CMPI(ppu_opcode_t op)
{
}

void ppu_acontext::ADDIC(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + spec_gpr::fixed(op.simm16);
}

void ppu_acontext::ADDI(ppu_opcode_t op)
{
	gpr[op.rd] = op.ra ? gpr[op.ra] + spec_gpr::fixed(op.simm16) : spec_gpr::fixed(op.simm16);
}

void ppu_acontext::ADDIS(ppu_opcode_t op)
{
	gpr[op.rd] = op.ra ? gpr[op.ra] + spec_gpr::fixed(op.simm16 * 65536) : spec_gpr::fixed(op.simm16 * 65536);
}

void ppu_acontext::BC(ppu_opcode_t op)
{
}

void ppu_acontext::SC(ppu_opcode_t op)
{
}

void ppu_acontext::B(ppu_opcode_t op)
{
}

void ppu_acontext::MCRF(ppu_opcode_t op)
{
}

void ppu_acontext::BCLR(ppu_opcode_t op)
{
}

void ppu_acontext::CRNOR(ppu_opcode_t op)
{
}

void ppu_acontext::CRANDC(ppu_opcode_t op)
{
}

void ppu_acontext::ISYNC(ppu_opcode_t op)
{
}

void ppu_acontext::CRXOR(ppu_opcode_t op)
{
}

void ppu_acontext::CRNAND(ppu_opcode_t op)
{
}

void ppu_acontext::CRAND(ppu_opcode_t op)
{
}

void ppu_acontext::CREQV(ppu_opcode_t op)
{
}

void ppu_acontext::CRORC(ppu_opcode_t op)
{
}

void ppu_acontext::CROR(ppu_opcode_t op)
{
}

void ppu_acontext::BCCTR(ppu_opcode_t op)
{
}

void ppu_acontext::RLWIMI(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (op.mb32 <= op.me32)
	{
		// 32-bit op, including mnemonics: INSLWI, INSRWI (TODO)
		min = utils::rol32(static_cast<u32>(min), op.sh32) & mask;
		max = utils::rol32(static_cast<u32>(max), op.sh32) & mask;
	}
	else
	{
		// Full 64-bit op with duplication
		min = utils::rol64(static_cast<u32>(min) | min << 32, op.sh32) & mask;
		max = utils::rol64(static_cast<u32>(max) | max << 32, op.sh32) & mask;
	}

	if (mask != umax)
	{
		// Insertion
		min |= gpr[op.ra].bmin & ~mask;
		max |= gpr[op.ra].bmax & ~mask;
	}

	gpr[op.rs] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLWINM(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (op.mb32 <= op.me32)
	{
		if (op.sh32 == 0)
		{
			// CLRLWI, CLRRWI mnemonics
			gpr[op.ra] = gpr[op.ra] & spec_gpr::fixed(mask);
			return;
		}
		else if (op.mb32 == 0 && op.me32 == 31)
		{
			// ROTLWI, ROTRWI mnemonics
		}
		else if (op.mb32 == 0 && op.sh32 == 31 - op.me32)
		{
			// SLWI mnemonic
		}
		else if (op.me32 == 31 && op.sh32 == 32 - op.mb32)
		{
			// SRWI mnemonic
		}
		else if (op.mb32 == 0 && op.sh32 < 31 - op.me32)
		{
			// EXTLWI and other possible mnemonics
		}
		else if (op.me32 == 31 && 32 - op.sh32 < op.mb32)
		{
			// EXTRWI and other possible mnemonics
		}

		min = utils::rol32(static_cast<u32>(min), op.sh32) & mask;
		max = utils::rol32(static_cast<u32>(max), op.sh32) & mask;
	}
	else
	{
		// Full 64-bit op with duplication
		min = utils::rol64(static_cast<u32>(min) | min << 32, op.sh32) & mask;
		max = utils::rol64(static_cast<u32>(max) | max << 32, op.sh32) & mask;
	}

	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLWNM(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (op.mb32 <= op.me32)
	{
		if (op.mb32 == 0 && op.me32 == 31)
		{
			// ROTLW mnemonic
		}

		// TODO
		min = 0;
		max = mask;
	}
	else
	{
		// Full 64-bit op with duplication
		min = 0;
		max = mask;
	}

	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::ORI(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] | spec_gpr::fixed(op.uimm16);
}

void ppu_acontext::ORIS(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] | spec_gpr::fixed(op.uimm16 << 16);
}

void ppu_acontext::XORI(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] ^ spec_gpr::fixed(op.uimm16);
}

void ppu_acontext::XORIS(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] ^ spec_gpr::fixed(op.uimm16 << 16);
}

void ppu_acontext::ANDI(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] & spec_gpr::fixed(op.uimm16);
}

void ppu_acontext::ANDIS(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] & spec_gpr::fixed(op.uimm16 << 16);
}

void ppu_acontext::RLDICL(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ~0ull >> mb;

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (64 - sh < mb)
	{
		// EXTRDI mnemonic
	}
	else if (64 - sh == mb)
	{
		// SRDI mnemonic
	}
	else if (sh == 0)
	{
		// CLRLDI mnemonic
		gpr[op.ra] = gpr[op.rs] & spec_gpr::fixed(mask);
		return;
	}

	min = utils::rol64(min, sh) & mask;
	max = utils::rol64(max, sh) & mask;
	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLDICR(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 me = op.mbe64;
	const u64 mask = ~0ull << (63 - me);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (sh < 63 - me)
	{
		// EXTLDI mnemonic
	}
	else if (sh == 63 - me)
	{
		// SLDI mnemonic
	}
	else if (sh == 0)
	{
		// CLRRDI mnemonic
		gpr[op.ra] = gpr[op.rs] & spec_gpr::fixed(mask);
		return;
	}

	min = utils::rol64(min, sh) & mask;
	max = utils::rol64(max, sh) & mask;
	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLDIC(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ppu_rotate_mask(mb, 63 - sh);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (mb == 0 && sh == 0)
	{
		gpr[op.ra] = gpr[op.rs];
		return;
	}
	else if (mb <= 63 - sh)
	{
		// CLRLSLDI
		//gpr[op.ra] = (gpr[op.rs] & spec_gpr::fixed(ppu_rotate_mask(0, sh + mb))) << spec_gpr::fixed(sh);
		return;
	}

	min = utils::rol64(min, sh) & mask;
	max = utils::rol64(max, sh) & mask;
	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLDIMI(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ppu_rotate_mask(mb, 63 - sh);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	if (mb == 0 && sh == 0)
	{
		// Copy
	}
	else if (mb <= 63 - sh)
	{
		// INSRDI mnemonic
	}

	min = utils::rol64(min, sh) & mask;
	max = utils::rol64(max, sh) & mask;

	if (mask != umax)
	{
		// Insertion
		min |= gpr[op.ra].bmin & ~mask;
		max |= gpr[op.ra].bmax & ~mask;
	}

	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLDCL(ppu_opcode_t op)
{
	const u32 mb = op.mbe64;
	const u64 mask = ~0ull >> mb;

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	// TODO
	min = 0;
	max = mask;
	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::RLDCR(ppu_opcode_t op)
{
	const u32 me = op.mbe64;
	const u64 mask = ~0ull << (63 - me);

	u64 min = gpr[op.rs].bmin;
	u64 max = gpr[op.rs].bmax;

	// TODO
	min = 0;
	max = mask;
	gpr[op.ra] = spec_gpr::approx(min, max);
}

void ppu_acontext::CMP(ppu_opcode_t op)
{
}

void ppu_acontext::TW(ppu_opcode_t op)
{
}

void ppu_acontext::LVSL(ppu_opcode_t op)
{
}

void ppu_acontext::LVEBX(ppu_opcode_t op)
{
}

void ppu_acontext::SUBFC(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + gpr[op.rb] + spec_gpr::fixed(1);
}

void ppu_acontext::ADDC(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + gpr[op.rb];
}

void ppu_acontext::MULHDU(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::MULHWU(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::MFOCRF(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LWARX(ppu_opcode_t op)
{
	gpr[op.rd] = spec_gpr::range(0, u32{umax});
}

void ppu_acontext::LDX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LWZX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::SLW(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::CNTLZW(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::SLD(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::AND(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] & gpr[op.rb];
}

void ppu_acontext::CMPL(ppu_opcode_t op)
{
}

void ppu_acontext::LVSR(ppu_opcode_t op)
{
}

void ppu_acontext::LVEHX(ppu_opcode_t op)
{
}

void ppu_acontext::SUBF(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + gpr[op.rb] + spec_gpr::fixed(1);
}

void ppu_acontext::LDUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::DCBST(ppu_opcode_t op)
{
}

void ppu_acontext::LWZUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::CNTLZD(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::ANDC(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] & ~gpr[op.rb];
}

void ppu_acontext::TD(ppu_opcode_t op)
{
}

void ppu_acontext::LVEWX(ppu_opcode_t op)
{
}

void ppu_acontext::MULHD(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::MULHW(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LDARX(ppu_opcode_t op)
{
	gpr[op.rd] = {};
}

void ppu_acontext::DCBF(ppu_opcode_t op)
{
}

void ppu_acontext::LBZX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LVX(ppu_opcode_t op)
{
}

void ppu_acontext::NEG(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + spec_gpr::fixed(1);
}

void ppu_acontext::LBZUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::NOR(ppu_opcode_t op)
{
	gpr[op.ra] = ~(gpr[op.rs] | gpr[op.rb]);
}

void ppu_acontext::STVEBX(ppu_opcode_t op)
{
}

void ppu_acontext::SUBFE(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + gpr[op.rb] + spec_gpr::range(0, 1);
}

void ppu_acontext::ADDE(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + gpr[op.rb] + spec_gpr::range(0, 1);
}

void ppu_acontext::MTOCRF(ppu_opcode_t op)
{
}

void ppu_acontext::STDX(ppu_opcode_t op)
{
}

void ppu_acontext::STWCX(ppu_opcode_t op)
{
}

void ppu_acontext::STWX(ppu_opcode_t op)
{
}

void ppu_acontext::STVEHX(ppu_opcode_t op)
{
}

void ppu_acontext::STDUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STWUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STVEWX(ppu_opcode_t op)
{
}

void ppu_acontext::SUBFZE(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + spec_gpr::range(0, 1);
}

void ppu_acontext::ADDZE(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + spec_gpr::range(0, 1);
}

void ppu_acontext::STDCX(ppu_opcode_t op)
{
}

void ppu_acontext::STBX(ppu_opcode_t op)
{
}

void ppu_acontext::STVX(ppu_opcode_t op)
{
}

void ppu_acontext::SUBFME(ppu_opcode_t op)
{
	gpr[op.rd] = ~gpr[op.ra] + spec_gpr::fixed(-1) + spec_gpr::range(0, 1);
}

void ppu_acontext::MULLD(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::ADDME(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + spec_gpr::fixed(-1) + spec_gpr::range(0, 1);
}

void ppu_acontext::MULLW(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::DCBTST(ppu_opcode_t op)
{
}

void ppu_acontext::STBUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::ADD(ppu_opcode_t op)
{
	gpr[op.rd] = gpr[op.ra] + gpr[op.rd];
}

void ppu_acontext::DCBT(ppu_opcode_t op)
{
}

void ppu_acontext::LHZX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::EQV(ppu_opcode_t op)
{
	gpr[op.ra] = ~(gpr[op.rs] ^ gpr[op.rb]);
}

void ppu_acontext::ECIWX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LHZUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::XOR(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] ^ gpr[op.rb];
}

void ppu_acontext::MFSPR(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LWAX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::DST(ppu_opcode_t op)
{
}

void ppu_acontext::LHAX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LVXL(ppu_opcode_t op)
{
}

void ppu_acontext::MFTB(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LWAUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::DSTST(ppu_opcode_t op)
{
}

void ppu_acontext::LHAUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::STHX(ppu_opcode_t op)
{
}

void ppu_acontext::ORC(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] | ~gpr[op.rb];
}

void ppu_acontext::ECOWX(ppu_opcode_t op)
{
}

void ppu_acontext::STHUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::OR(ppu_opcode_t op)
{
	gpr[op.ra] = gpr[op.rs] | gpr[op.rb];
}

void ppu_acontext::DIVDU(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::DIVWU(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::MTSPR(ppu_opcode_t op)
{
}

void ppu_acontext::DCBI(ppu_opcode_t op)
{
}

void ppu_acontext::NAND(ppu_opcode_t op)
{
	gpr[op.ra] = ~(gpr[op.rs] & gpr[op.rb]);
}

void ppu_acontext::STVXL(ppu_opcode_t op)
{
}

void ppu_acontext::DIVD(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::DIVW(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LVLX(ppu_opcode_t op)
{
}

void ppu_acontext::LDBRX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LSWX(ppu_opcode_t op)
{
}

void ppu_acontext::LWBRX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LFSX(ppu_opcode_t op)
{
}

void ppu_acontext::SRW(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::SRD(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::LVRX(ppu_opcode_t op)
{
}

void ppu_acontext::LSWI(ppu_opcode_t op)
{
	std::fill_n(gpr, 32, spec_gpr{});
}

void ppu_acontext::LFSUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::SYNC(ppu_opcode_t op)
{
}

void ppu_acontext::LFDX(ppu_opcode_t op)
{
}

void ppu_acontext::LFDUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STVLX(ppu_opcode_t op)
{
}

void ppu_acontext::STDBRX(ppu_opcode_t op)
{
}

void ppu_acontext::STSWX(ppu_opcode_t op)
{
}

void ppu_acontext::STWBRX(ppu_opcode_t op)
{
}

void ppu_acontext::STFSX(ppu_opcode_t op)
{
}

void ppu_acontext::STVRX(ppu_opcode_t op)
{
}

void ppu_acontext::STFSUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STSWI(ppu_opcode_t op)
{
}

void ppu_acontext::STFDX(ppu_opcode_t op)
{
}

void ppu_acontext::STFDUX(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::LVLXL(ppu_opcode_t op)
{
}

void ppu_acontext::LHBRX(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::SRAW(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::SRAD(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::LVRXL(ppu_opcode_t op)
{
}

void ppu_acontext::DSS(ppu_opcode_t op)
{
}

void ppu_acontext::SRAWI(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::SRADI(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::EIEIO(ppu_opcode_t op)
{
}

void ppu_acontext::STVLXL(ppu_opcode_t op)
{
}

void ppu_acontext::STHBRX(ppu_opcode_t op)
{
}

void ppu_acontext::EXTSH(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::STVRXL(ppu_opcode_t op)
{
}

void ppu_acontext::EXTSB(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::STFIWX(ppu_opcode_t op)
{
}

void ppu_acontext::EXTSW(ppu_opcode_t op)
{
	gpr[op.ra].set_undef();
}

void ppu_acontext::ICBI(ppu_opcode_t op)
{
}

void ppu_acontext::DCBZ(ppu_opcode_t op)
{
}

void ppu_acontext::LWZ(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LWZU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::LBZ(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LBZU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::STW(ppu_opcode_t op)
{
}

void ppu_acontext::STWU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STB(ppu_opcode_t op)
{
}

void ppu_acontext::STBU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::LHZ(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LHZU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::LHA(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LHAU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::STH(ppu_opcode_t op)
{
}

void ppu_acontext::STHU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::LMW(ppu_opcode_t op)
{
	std::fill_n(gpr, 32, spec_gpr{});
}

void ppu_acontext::STMW(ppu_opcode_t op)
{
}

void ppu_acontext::LFS(ppu_opcode_t op)
{
}

void ppu_acontext::LFSU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::LFD(ppu_opcode_t op)
{
}

void ppu_acontext::LFDU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STFS(ppu_opcode_t op)
{
}

void ppu_acontext::STFSU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::STFD(ppu_opcode_t op)
{
}

void ppu_acontext::STFDU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::LD(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::LDU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.rd].set_undef();
	gpr[op.ra] = addr;
}

void ppu_acontext::LWA(ppu_opcode_t op)
{
	gpr[op.rd].set_undef();
}

void ppu_acontext::STD(ppu_opcode_t op)
{
}

void ppu_acontext::STDU(ppu_opcode_t op)
{
	const auto addr = gpr[op.ra] + gpr[op.rb];
	gpr[op.ra] = addr;
}

void ppu_acontext::FDIVS(ppu_opcode_t op)
{
}

void ppu_acontext::FSUBS(ppu_opcode_t op)
{
}

void ppu_acontext::FADDS(ppu_opcode_t op)
{
}

void ppu_acontext::FSQRTS(ppu_opcode_t op)
{
}

void ppu_acontext::FRES(ppu_opcode_t op)
{
}

void ppu_acontext::FMULS(ppu_opcode_t op)
{
}

void ppu_acontext::FMADDS(ppu_opcode_t op)
{
}

void ppu_acontext::FMSUBS(ppu_opcode_t op)
{
}

void ppu_acontext::FNMSUBS(ppu_opcode_t op)
{
}

void ppu_acontext::FNMADDS(ppu_opcode_t op)
{
}

void ppu_acontext::MTFSB1(ppu_opcode_t op)
{
}

void ppu_acontext::MCRFS(ppu_opcode_t op)
{
}

void ppu_acontext::MTFSB0(ppu_opcode_t op)
{
}

void ppu_acontext::MTFSFI(ppu_opcode_t op)
{
}

void ppu_acontext::MFFS(ppu_opcode_t op)
{
}

void ppu_acontext::MTFSF(ppu_opcode_t op)
{
}

void ppu_acontext::FCMPU(ppu_opcode_t op)
{
}

void ppu_acontext::FRSP(ppu_opcode_t op)
{
}

void ppu_acontext::FCTIW(ppu_opcode_t op)
{
}

void ppu_acontext::FCTIWZ(ppu_opcode_t op)
{
}

void ppu_acontext::FDIV(ppu_opcode_t op)
{
}

void ppu_acontext::FSUB(ppu_opcode_t op)
{
}

void ppu_acontext::FADD(ppu_opcode_t op)
{
}

void ppu_acontext::FSQRT(ppu_opcode_t op)
{
}

void ppu_acontext::FSEL(ppu_opcode_t op)
{
}

void ppu_acontext::FMUL(ppu_opcode_t op)
{
}

void ppu_acontext::FRSQRTE(ppu_opcode_t op)
{
}

void ppu_acontext::FMSUB(ppu_opcode_t op)
{
}

void ppu_acontext::FMADD(ppu_opcode_t op)
{
}

void ppu_acontext::FNMSUB(ppu_opcode_t op)
{
}

void ppu_acontext::FNMADD(ppu_opcode_t op)
{
}

void ppu_acontext::FCMPO(ppu_opcode_t op)
{
}

void ppu_acontext::FNEG(ppu_opcode_t op)
{
}

void ppu_acontext::FMR(ppu_opcode_t op)
{
}

void ppu_acontext::FNABS(ppu_opcode_t op)
{
}

void ppu_acontext::FABS(ppu_opcode_t op)
{
}

void ppu_acontext::FCTID(ppu_opcode_t op)
{
}

void ppu_acontext::FCTIDZ(ppu_opcode_t op)
{
}

void ppu_acontext::FCFID(ppu_opcode_t op)
{
}

#include <random>

const bool s_tes = []()
{
	return true;

	std::mt19937_64 rnd{123};

	for (u32 i = 0; i < 10000; i++)
	{
		ppu_acontext::spec_gpr r1, r2, r3;
		r1 = ppu_acontext::spec_gpr::approx(rnd(), rnd());
		r2 = ppu_acontext::spec_gpr::range(rnd(), rnd());
		r3 = r1 | r2;

		for (u32 j = 0; j < 10000; j++)
		{
			u64 v1 = rnd(), v2 = rnd();
			v1 &= r1.mask();
			v1 |= r1.ones();
			if (!r2.test(v2))
			{
				v2 = r2.imin;
			}

			if (r1.test(v1) && r2.test(v2))
			{
				if (!r3.test(v1 | v2))
				{
					auto exp = ppu_acontext::spec_gpr::approx(r1.ones() & r2.ones(), r1.mask() & r2.mask());

					ppu_log.error("ppu_acontext failure:"
						"\n\tr1 = 0x%016x..0x%016x, 0x%016x:0x%016x"
						"\n\tr2 = 0x%016x..0x%016x, 0x%016x:0x%016x"
						"\n\tr3 = 0x%016x..0x%016x, 0x%016x:0x%016x"
						"\n\tex = 0x%016x..0x%016x"
						"\n\tv1 = 0x%016x, v2 = 0x%016x, v3 = 0x%016x",
						r1.imin, r1.imax, r1.bmin, r1.bmax, r2.imin, r2.imax, r2.bmin, r2.bmax, r3.imin, r3.imax, r3.bmin, r3.bmax, exp.imin, exp.imax, v1, v2, v1 | v2);
					break;
				}
			}
		}
	}

	ppu_acontext::spec_gpr r1;
	r1 = ppu_acontext::spec_gpr::range(0x13311, 0x1fe22);
	r1 = r1 ^ ppu_acontext::spec_gpr::approx(0x000, 0xf00);
	ppu_log.success("0x%x..0x%x", r1.imin, r1.imax);

	return true;
}();
