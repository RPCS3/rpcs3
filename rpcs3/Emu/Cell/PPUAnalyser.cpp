#include "stdafx.h"
#include "PPUOpcodes.h"
#include "PPUModule.h"
#include "PPUAnalyser.h"

#include <unordered_set>

#include "yaml-cpp/yaml.h"

namespace vm { using namespace ps3; }

const ppu_decoder<ppu_itype> s_ppu_itype;
const ppu_decoder<ppu_iname> s_ppu_iname;

template<>
void fmt_class_string<ppu_attr>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_attr value)
	{
		switch (value)
		{
		case ppu_attr::known_addr: return "known_addr";
		case ppu_attr::known_size: return "known_size";
		case ppu_attr::no_return: return "no_return";
		case ppu_attr::no_size: return "no_size";
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

void ppu_validate(const std::string& fname, const std::vector<ppu_function>& funcs, u32 reloc)
{
	// Load custom PRX configuration if available
	if (fs::file yml{fname + ".yml"})
	{
		const auto cfg = YAML::Load(yml.to_string());

		u32 index = 0;

		// Validate detected functions using information provided
		for (const auto func : cfg["functions"])
		{
			const u32 addr = func["addr"].as<u32>(-1);
			const u32 size = func["size"].as<u32>(0);

			if (addr != -1 && index < funcs.size())
			{
				u32 found = funcs[index].addr - reloc;

				while (addr > found && index + 1 < funcs.size())
				{
					LOG_WARNING(LOADER, "%s.yml : unexpected function at 0x%x (0x%x, 0x%x)", fname, found, addr, size);
					index++;
					found = funcs[index].addr - reloc;
				}

				if (addr < found)
				{
					LOG_ERROR(LOADER, "%s.yml : function not found (0x%x, 0x%x)", fname, addr, size);
					continue;
				}

				if (size && size != funcs[index].size)
				{
					if (size + 4 != funcs[index].size || vm::read32(addr + size) != ppu_instructions::NOP())
					{
						LOG_ERROR(LOADER, "%s.yml : function size mismatch at 0x%x(size=0x%x) (0x%x, 0x%x)", fname, found, funcs[index].size, addr, size);
					}
				}

				index++;
			}
			else
			{
				LOG_ERROR(LOADER, "%s.yml : function not found at the end (0x%x, 0x%x)", fname, addr, size);
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
				LOG_ERROR(LOADER, "%s.yml : function not covered at 0x%x (size=0x%x)", fname, funcs[index].addr, funcs[index].size);
			}

			index++;
		}

		LOG_SUCCESS(LOADER, "%s.yml : validation completed", fname);
	}
}

static u32 ppu_test(const vm::cptr<u32> ptr, vm::cptr<void> fend, ppu_pattern_array pat)
{
	vm::cptr<u32> cur = ptr;

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

	return cur.addr() - ptr.addr();
}

static u32 ppu_test(vm::cptr<u32> ptr, vm::cptr<void> fend, ppu_pattern_matrix pats)
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

std::vector<ppu_function> ppu_analyse(const std::vector<std::pair<u32, u32>>& segs, const std::vector<std::pair<u32, u32>>& secs, u32 lib_toc, u32 entry)
{
	// Assume first segment is executable
	const u32 start = segs[0].first;
	const u32 end = segs[0].first + segs[0].second;

	// Known TOCs (usually only 1)
	std::unordered_set<u32> TOCs;

	// Known functions
	std::map<u32, ppu_function> funcs;

	// Function analysis workload
	std::vector<std::reference_wrapper<ppu_function>> func_queue;

	// Known references (within segs, addr and value alignment = 4)
	std::set<u32> addr_heap{entry};

	// Register new function
	auto add_func = [&](u32 addr, u32 toc, u32 caller) -> ppu_function&
	{
		ppu_function& func = funcs[addr];

		if (caller)
		{
			// Register caller
			func.callers.emplace(caller);
		}

		if (func.addr)
		{
			if (toc && func.toc && func.toc != -1 && func.toc != toc)
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
		func.name = fmt::format("__0x%x", func.addr);
		LOG_TRACE(PPU, "Function 0x%x added (toc=0x%x)", addr, toc);
		return func;
	};

	// Register new TOC and find basic set of functions
	auto add_toc = [&](u32 toc)
	{
		if (!toc || toc == -1 || !TOCs.emplace(toc).second)
		{
			return;
		}

		// Grope for OPD section (TODO: optimization, better constraints)
		for (const auto& seg : segs)
		{
			for (vm::cptr<u32> ptr = vm::cast(seg.first); ptr.addr() < seg.first + seg.second; ptr++)
			{
				if (ptr[0] >= start && ptr[0] < end && ptr[0] % 4 == 0 && ptr[1] == toc)
				{
					// New function
					LOG_TRACE(PPU, "OPD*: [0x%x] 0x%x (TOC=0x%x)", ptr, ptr[0], ptr[1]);
					add_func(*ptr, addr_heap.count(ptr.addr()) ? toc : 0, 0);
					ptr++;
				}
			}
		}
	};

	// Get next reliable function address
	auto get_limit = [&](u32 addr) -> u32
	{
		for (auto it = funcs.lower_bound(addr), end = funcs.end(); it != end; it++)
		{
			if (test(it->second.attr, ppu_attr::known_addr))
			{
				return it->first;
			}
		}

		return end;
	};

	// Find references indiscriminately
	for (const auto& seg : segs)
	{
		for (vm::cptr<u32> ptr = vm::cast(seg.first); ptr.addr() < seg.first + seg.second; ptr++)
		{
			const u32 value = *ptr;

			if (value % 4)
			{
				continue;
			}

			for (const auto& _seg : segs)
			{
				if (value >= _seg.first && value < _seg.first + _seg.second)
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
		vm::cptr<void> sec_end = vm::cast(sec.first + sec.second);

		// Probe
		for (vm::cptr<u32> ptr = vm::cast(sec.first); ptr < sec_end; ptr += 2)
		{
			if (ptr + 6 <= sec_end && !ptr[0] && !ptr[2] && ptr[1] == ptr[4] && ptr[3] == ptr[5])
			{
				// Special OPD format case (some homebrews)
				ptr += 4;
			}

			if (ptr + 2 > sec_end)
			{
				sec_end.set(0);
				break;
			}

			const u32 addr = ptr[0];
			const u32 _toc = ptr[1];

			// Rough Table of Contents borders
			const u32 _toc_begin = _toc - 0x8000;
			const u32 _toc_end = _toc + 0x8000;

			// TODO: improve TOC constraints
			if (_toc % 4 || _toc == 0 || _toc >= 0x40000000 || (_toc >= start && _toc < end))
			{
				sec_end.set(0);
				break;
			}

			if (addr % 4 || addr < start || addr >= end || addr == _toc)
			{
				sec_end.set(0);
				break;
			}
		}

		if (sec_end) LOG_NOTICE(PPU, "Reading OPD section at 0x%x...", sec.first);

		// Mine
		for (vm::cptr<u32> ptr = vm::cast(sec.first); ptr < sec_end; ptr += 2)
		{
			// Special case: see "Probe"
			if (!ptr[0]) ptr += 4;

			// Add function and TOC
			const u32 addr = ptr[0];
			const u32 toc = ptr[1];
			LOG_TRACE(PPU, "OPD: [0x%x] 0x%x (TOC=0x%x)", ptr, addr, toc);

			TOCs.emplace(toc);
			auto& func = add_func(addr, addr_heap.count(ptr.addr()) ? toc : 0, 0);
			func.attr += ppu_attr::known_addr;
		}
	}

	// Register TOC from entry point
	if (entry && !lib_toc)
	{
		lib_toc = vm::read32(entry) ? vm::read32(entry + 4) : vm::read32(entry + 20);
	}

	// Secondary attempt
	if (TOCs.empty() && lib_toc)
	{
		add_toc(lib_toc);
	}

	// Clean TOCs
	for (auto&& pair : funcs)
	{
		if (pair.second.toc == -1)
		{
			pair.second.toc = 0;
		}
	}

	// Find .eh_frame section
	for (const auto& sec : secs)
	{
		vm::cptr<void> sec_end = vm::cast(sec.first + sec.second);

		// Probe
		for (vm::cptr<u32> ptr = vm::cast(sec.first); ptr < sec_end;)
		{
			if (ptr % 4 || ptr.addr() < sec.first || ptr >= sec_end)
			{
				sec_end.set(0);
				break;
			}

			const u32 size = ptr[0] + 4;

			if (size == 4 && ptr + 1 == sec_end)
			{
				// Null terminator
				break;
			}

			if (size % 4 || size < 0x10 || ptr + size / 4 > sec_end)
			{
				sec_end.set(0);
				break;
			}

			if (ptr[1])
			{
				const u32 cie_off = ptr.addr() - ptr[1] + 4;

				if (cie_off % 4 || cie_off < sec.first || cie_off >= sec_end.addr())
				{
					sec_end.set(0);
					break;
				}
			}

			ptr = vm::cast(ptr.addr() + size);
		}

		if (sec_end && sec.second > 4) LOG_NOTICE(PPU, "Reading .eh_frame section at 0x%x...", sec.first);

		// Mine
		for (vm::cptr<u32> ptr = vm::cast(sec.first); ptr < sec_end; ptr = vm::cast(ptr.addr() + ptr[0] + 4))
		{
			if (ptr[0] == 0)
			{
				// Null terminator
				break;
			}

			if (ptr[1] == 0)
			{
				// CIE
				LOG_TRACE(PPU, ".eh_frame: [0x%x] CIE 0x%x", ptr, ptr[0]);
			}
			else
			{
				// Get associated CIE (currently unused)
				const vm::cptr<u32> cie = vm::cast(ptr.addr() - ptr[1] + 4);

				u32 addr = 0;
				u32 size = 0;

				// TODO: 64 bit or 32 bit values (approximation)
				if (ptr[2] == 0 && ptr[3] == 0)
				{
					size = ptr[5];
				}
				else if ((ptr[2] == -1 || ptr[2] == 0) && ptr[4] == 0 && ptr[5])
				{
					addr = ptr[3];
					size = ptr[5];
				}
				else if (ptr[2] != -1 && ptr[3])
				{
					addr = ptr[2];
					size = ptr[3];
				}
				else
				{
					LOG_ERROR(PPU, ".eh_frame: [0x%x] 0x%x, 0x%x, 0x%x, 0x%x, 0x%x", ptr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
					continue;
				}

				// TODO: absolute/relative offset (approximation)
				if (addr > 0xc0000000)
				{
					addr += ptr.addr() + 8;
				}

				LOG_TRACE(PPU, ".eh_frame: [0x%x] FDE 0x%x (cie=*0x%x, addr=0x%x, size=0x%x)", ptr, ptr[0], cie, addr, size);

				// TODO: invalid offsets, zero offsets (removed functions?)
				if (addr % 4 || size % 4 || size > (end - start) || addr < start || addr + size > end)
				{
					if (addr) LOG_ERROR(PPU, ".eh_frame: Invalid function 0x%x", addr);
					continue;
				}

				//auto& func = add_func(addr, 0, 0);
				//func.attr += ppu_attr::known_addr;
				//func.attr += ppu_attr::known_size;
				//func.size = size;
			}
		}
	}

	// Main loop (func_queue may grow)
	for (std::size_t i = 0; i < func_queue.size(); i++)
	{
		ppu_function& func = func_queue[i];

		// Fixup TOCs
		if (func.toc && func.toc != -1)
		{
			for (u32 addr : func.callers)
			{
				ppu_function& caller = funcs[addr];

				if (!caller.toc)
				{
					add_func(addr, func.toc - caller.trampoline, 0);
				}
			}

			for (u32 addr : func.calls)
			{
				ppu_function& callee = funcs[addr];

				if (!callee.toc)
				{
					add_func(addr, func.toc + func.trampoline, 0);
				}
			}
		}

		if (func.blocks.empty())
		{
			// Special function analysis
			const vm::cptr<u32> ptr = vm::cast(func.addr);
			const vm::cptr<void> fend = vm::cast(end);

			using namespace ppu_instructions;

			if (ptr + 1 <= fend && (ptr[0] & 0xfc000001) == B({}, {}))
			{
				// Simple trampoline
				const u32 target = (ptr[0] & 0x2 ? 0 : ptr.addr()) + ppu_opcode_t{ptr[0]}.bt24;

				if (target == func.addr)
				{
					// Special case
					func.size = 0x4;
					func.blocks.emplace(func.addr, func.size);
					func.attr += ppu_attr::no_return;
					continue;
				}
				
				if (target >= start && target < end)
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

			if (ptr + 0x4 <= fend &&
				(ptr[0] & 0xffff0000) == LIS(r11, 0) &&
				(ptr[1] & 0xffff0000) == ADDI(r11, r11, 0) &&
				ptr[2] == MTCTR(r11) &&
				ptr[3] == BCTR())
			{
				// Simple trampoline
				const u32 target = (ptr[0] << 16) + ppu_opcode_t{ptr[1]}.simm16;

				if (target >= start && target < end)
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

			if (ptr + 4 <= fend &&
				ptr[0] == STD(r2, r1, 0x28) &&
				(ptr[1] & 0xffff0000) == ADDIS(r2, r2, {}) &&
				(ptr[2] & 0xffff0000) == ADDI(r2, r2, {}) &&
				(ptr[3] & 0xfc000001) == B({}, {}))
			{
				// Trampoline with TOC
				const u32 toc_add = (ptr[1] << 16) + s16(ptr[2]);
				const u32 target = (ptr[3] & 0x2 ? 0 : (ptr + 3).addr()) + ppu_opcode_t{ptr[3]}.bt24;

				if (target >= start && target < end)
				{
					auto& new_func = add_func(target, 0, func.addr);

					if (func.toc && func.toc != -1 && new_func.toc == 0)
					{
						const u32 toc = func.toc + toc_add;
						add_toc(toc);
						add_func(new_func.addr, toc, 0);
					}
					else if (new_func.toc && new_func.toc != -1 && func.toc == 0)
					{
						const u32 toc = new_func.toc - toc_add;
						add_toc(toc);
						add_func(func.addr, toc, 0);
					}
					else if (new_func.toc - func.toc != toc_add)
					{
						//func.toc = -1;
						//new_func.toc = -1;
					}

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

			if (ptr + 8 <= fend &&
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
				func.attr += ppu_attr::known_addr;
				func.attr += ppu_attr::known_size;

				// Look for another imports to fill gaps (hack)
				auto p2 = ptr + 8;

				while (p2 + 8 <= fend &&
					(p2[0] & 0xffff0000) == LI(r12, 0) &&
					(p2[1] & 0xffff0000) == ORIS(r12, r12, 0) &&
					(p2[2] & 0xffff0000) == LWZ(r12, r12, 0) &&
					p2[3] == STD(r2, r1, 0x28) &&
					p2[4] == LWZ(r0, r12, 0) &&
					p2[5] == LWZ(r2, r12, 4) &&
					p2[6] == MTCTR(r0) &&
					p2[7] == BCTR())
				{
					auto& next = add_func(p2.addr(), -1, func.addr);
					next.size = 0x20;
					next.blocks.emplace(next.addr, next.size);
					next.attr += ppu_attr::known_addr;
					next.attr += ppu_attr::known_size;
					p2 += 8;
				}

				continue;
			}

			if (ptr + 3 <= fend &&
				ptr[0] == 0x7c0004ac &&
				ptr[1] == 0x00000000 &&
				ptr[2] == BLR())
			{
				// Weird function (illegal instruction)
				func.size = 0xc;
				func.blocks.emplace(func.addr, func.size);
				//func.attr += ppu_attr::no_return;
				continue;
			}

			if (const u32 len = ppu_test(ptr, fend, ppu_patterns::abort))
			{
				// Function "abort"
				LOG_NOTICE(PPU, "Function [0x%x]: 'abort'", func.addr);
				func.attr += ppu_attr::no_return;
				func.attr += ppu_attr::known_size;
				func.size = len;
			}

			// TODO: detect no_return, scribe more TODOs

			// Acknowledge completion
			func.blocks.emplace(vm::cast(func.addr), 0);
		}

		// Get function limit
		const u32 func_end = std::min<u32>(get_limit(func.addr + 1), test(func.attr, ppu_attr::known_size) ? func.addr + func.size : end);

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
		if (test(func.attr, ppu_attr::no_size))
		{
			// Get next function
			const auto _next = funcs.lower_bound(func.blocks.crbegin()->first + 1);

			// Get limit
			const u32 func_end2 = _next == funcs.end() ? func_end : std::min<u32>(_next->first, func_end);

			// Set more block entries
			std::for_each(addr_heap.lower_bound(func.addr), addr_heap.lower_bound(func_end2), add_block);
		}

		const bool was_empty = block_queue.empty();

		// Block loop (block_queue may grow, may be aborted via clearing)
		for (std::size_t j = 0; j < block_queue.size(); j++)
		{
			auto& block = block_queue[j].get();

			for (vm::cptr<u32> _ptr = vm::cast(block.first); _ptr.addr() < func_end;)
			{
				const u32 iaddr = _ptr.addr();
				const ppu_opcode_t op{*_ptr++};
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
						LOG_WARNING(PPU, "[0x%x] Invalid branch at 0x%x -> 0x%x", func.addr, iaddr, target);
						continue;
					}

					const bool is_call = op.lk && target != iaddr;
					const auto pfunc = is_call ? &add_func(target, 0, 0) : nullptr;

					if (pfunc && pfunc->blocks.empty())
					{
						// Postpone analysis (no info)
						block_queue.clear();
						break;
					}
					
					// Add next block if necessary
					if ((is_call && !test(pfunc->attr, ppu_attr::no_return)) || (type == ppu_itype::BC && (op.bo & 0x14) != 0x14))
					{
						add_block(_ptr.addr());
					}

					if (is_call && test(pfunc->attr, ppu_attr::no_return))
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

						for (; _ptr.addr() < jt_end; _ptr++)
						{
							const u32 addr = jt_addr + *_ptr;

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
							if (!test(func.attr, ppu_attr::no_size))
							{
								LOG_WARNING(PPU, "[0x%x] Jump table not found! 0x%x-0x%x", func.addr, jt_addr, jt_end);
							}

							func.attr += ppu_attr::no_size;
							add_block(jt_addr);
							block_queue.clear();
						}
						else
						{
							LOG_TRACE(PPU, "[0x%x] Jump table found: 0x%x-0x%x", func.addr, jt_addr, _ptr);
						}
					}

					block.second = _ptr.addr() - block.first;
					break;
				}
				else if (type == ppu_itype::TW || type == ppu_itype::TWI || type == ppu_itype::TD || type == ppu_itype::TDI)
				{
					if (op.opcode != ppu_instructions::TRAP())
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
				else if (type == ppu_itype::STDU && test(func.attr, ppu_attr::no_size) && (op.opcode == *_ptr || *_ptr == ppu_instructions::BLR()))
				{
					// Hack
					LOG_SUCCESS(PPU, "[0x%x] Instruction repetition: 0x%08x", iaddr, op.opcode);
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
		if (!test(func.attr, ppu_attr::known_size))
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
				const ppu_opcode_t op{*_ptr++};
				const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

				if (type == ppu_itype::B || type == ppu_itype::BC)
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target >= start && target < end)
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
		if (test(func.attr, ppu_attr::known_size))
		{
			const auto last = func.blocks.crbegin();

			if (last != func.blocks.crend())
			{
				func.size = std::min<u32>(func.size, last->first + last->second - func.addr);
			}
		}
	}

	// Function shrinkage, disabled (TODO: it's potentially dangerous but improvable)
	for (auto& _pair : funcs)
	{
		auto& func = _pair.second;

		// Get next function addr
		const auto _next = funcs.lower_bound(_pair.first + 1);

		const u32 next = _next == funcs.end() ? end : _next->first;

		// Just ensure that functions don't overlap
		if (func.addr + func.size > next)
		{
			LOG_WARNING(PPU, "Function overlap: [0x%x] 0x%x -> 0x%x", func.addr, func.size, next - func.addr);
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
			const ppu_opcode_t op{*_ptr++};
			const ppu_itype::type type = s_ppu_itype.decode(op.opcode);

			if (type == ppu_itype::UNK)
			{
				break;
			}
			else if (addr == start && op.opcode == ppu_instructions::NOP())
			{
				if (start == func.addr + func.size)
				{
					// Extend function with tail NOPs (hack)
					func.size += 4;
				}

				start += 4;
				continue;
			}
			else if (type == ppu_itype::SC && op.opcode != ppu_instructions::SC(0))
			{
				break;
			}
			else if (addr == start && op.opcode == ppu_instructions::BLR())
			{
				start += 4;
				continue;
			}
			else if (type == ppu_itype::B || type == ppu_itype::BC)
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
				LOG_WARNING(PPU, "Function gap: [0x%x] 0x%x bytes at 0x%x", func.addr, next - start, start);
				break;
			}
		}
	}

	// Fill TOCs for trivial case
	if (TOCs.size() == 1)
	{
		lib_toc = *TOCs.begin();

		for (auto&& pair : funcs)
		{
			if (pair.second.toc == 0)
			{
				pair.second.toc = lib_toc;
			}
		}
	}
	
	// Convert map to vector (destructive)
	std::vector<ppu_function> result;

	for (auto&& pair : funcs)
	{
		auto& func = pair.second;
		LOG_TRACE(PPU, "Function %s (size=0x%x, toc=0x%x, attr %#x)", func.name, func.size, func.toc, func.attr);
		result.emplace_back(std::move(func));
	}

	LOG_NOTICE(PPU, "Function analysis: %zu functions (%zu enqueued)", result.size(), func_queue.size());

	return result;
}
