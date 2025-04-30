#include "stdafx.h"
#include "PPUAnalyser.h"

#include "lv2/sys_sync.h"

#include "PPUOpcodes.h"
#include "PPUThread.h"

#include <unordered_set>
#include "util/yaml.hpp"
#include "util/asm.hpp"

LOG_CHANNEL(ppu_validator);

extern const ppu_decoder<ppu_itype> g_ppu_itype;

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

static constexpr struct const_tag{} is_const;
/*static constexpr*/ struct range_tag{} /*is_range*/;
static constexpr struct min_value_tag{} minv;
static constexpr struct max_value_tag{} maxv;
static constexpr struct sign_bit_tag{} sign_bitv;
static constexpr struct load_addr_tag{} load_addrv;

struct reg_state_t
{
	u64 ge_than;
	u64 value_range;
	u64 bit_range;
	bool is_loaded; // Is loaded from memory(?) (this includes offsetting from that address)
	u32 tag;

	// Check if state is a constant value
	bool operator()(const_tag) const
	{
		return !is_loaded && value_range == 1 && bit_range == 0;
	}

	// Check if state is a ranged value
	bool operator()(range_tag) const
	{
		return !is_loaded && bit_range == 0;
	}

	// Get minimum bound
	u64 operator()(min_value_tag) const
	{
		return value_range ? ge_than : 0;
	}

	// Get maximum bound
	u64 operator()(max_value_tag) const
	{
		return value_range ? (ge_than | bit_range) + value_range : 0;
	}

	u64 operator()(sign_bit_tag) const
	{
		return value_range == 0 || (bit_range >> 63) || (ge_than + value_range - 1) >> 63 != (ge_than >> 63) ? u64{umax} : (ge_than >> 63);
	}

	u64 operator()(load_addr_tag) const
	{
		return is_loaded ? ge_than : 0;
	}

	// Check if value is of the same origin
	bool is_equals(const reg_state_t& rhs) const
	{
		return (rhs.tag && rhs.tag == this->tag) || (rhs(is_const) && (*this)(is_const) && rhs.ge_than == ge_than);
	}

	void set_lower_bound(u64 value)
	{
		const u64 prev_max = ge_than + value_range;
		ge_than = value;
		value_range = prev_max - ge_than;
	}

	// lower_bound = Max(bounds)
	void lift_lower_bound(u64 value)
	{
		const u64 prev_max = ge_than + value_range;

		// Get the value closer to upper bound (may be lower in value)
		// Make 0 underflow (it is actually not 0 but UINT64_MAX + 1 in this context)
		ge_than = value_range - 1 > prev_max - value - 1 ? value : ge_than;

		value_range = prev_max - ge_than;
	}

	// Upper bound is not inclusive
	void set_upper_bound(u64 value)
	{
		value_range = value - ge_than;
	}

	// upper_bound = Min(bounds)
	void limit_upper_bound(u64 value)
	{
		// Make 0 underflow (it is actually not 0 but UINT64_MAX + 1 in this context)
		value_range = std::min(value - ge_than - 1, value_range - 1) + 1;
	}

	// Clear bits using mask
	// May fail if ge_than(+)value_range is modified by the operation 
	bool clear_mask(u64 bit_mask, u32& reg_tag_allocator)
	{
		if (bit_mask == umax)
		{
			return true;
		}

		if ((ge_than & bit_mask) > ~value_range)
		{
			// Discard data: mask clears the carry bit
			value_range = 0;
			tag = reg_tag_allocator++;
			return false;
		}

		if (((ge_than & bit_mask) + value_range) & ~bit_mask)
		{
			// Discard data: mask clears range bits
			value_range = 0;
			tag = reg_tag_allocator++;
			return false;
		}

		ge_than &= bit_mask;
		bit_range &= bit_mask;
		return true;
	}

	bool clear_lower_bits(u64 bits, u32& reg_tag_allocator)
	{
		const u64 bit_mask = ~((u64{1} << bits) - 1);
		return clear_mask(bit_mask, reg_tag_allocator);
	}

	bool clear_higher_bits(u64 bits, u32& reg_tag_allocator)
	{
		const u64 bit_mask = (u64{1} << ((64 - bits) % 64)) - 1;
		return clear_mask(bit_mask, reg_tag_allocator);
	}

	// Undefine bits using mask
	void undef_mask(u64 bit_mask)
	{
		ge_than &= bit_mask;
		bit_range |= ~bit_mask;
	}

	void undef_lower_bits(u64 bits)
	{
		const u64 bit_mask = ~((u64{1} << bits) - 1);
		undef_mask(bit_mask);
	}

	void undef_higher_bits(u64 bits)
	{
		const u64 bit_mask = (u64{1} << ((64 - bits) % 64)) - 1;
		undef_mask(bit_mask);
	}

	// Add value to state, respecting of bit_range
	void add(u64 value)
	{
		const u64 old_ge = ge_than;
		ge_than += value;

		// Adjust bit_range to undefine bits that their state may not be defined anymore
		// No need to adjust value_range at the moment (wrapping around is implied)
		bit_range |= ((old_ge | bit_range) + value) ^ ((old_ge) + value);

		ge_than &= ~bit_range;
	}

	void sub(u64 value)
	{
		// This function should be perfect, so it handles subtraction as well
		add(~value + 1);
	}

	// TODO: For CMP/CMPD value fixup
	bool can_subtract_from_both_without_loss_of_meaning(const reg_state_t& rhs, u64 value)
	{
		const reg_state_t& lhs = *this;

		return lhs.ge_than >= value && rhs.ge_than >= value && !!((lhs.ge_than - value) & lhs.bit_range) && !!((rhs.ge_than - value) & rhs.bit_range);
	}

	// Bitwise shift left
	bool shift_left(u64 value, u32& reg_tag_allocator)
	{
		if (!value || !value_range)
		{
			return true;
		}

		const u64 mask_out = u64{umax} >> value;

		if ((ge_than & mask_out) > ~value_range)
		{
			// Discard data: shift clears the carry bit
			value_range = 0;
			tag = reg_tag_allocator++;
			return false;
		}

		if (((ge_than & mask_out) + value_range) & ~mask_out)
		{
			// Discard data: shift clears range bits
			value_range = 0;
			tag = reg_tag_allocator++;
			return false;
		}

		ge_than <<= value;
		bit_range <<= value;
		value_range = ((value_range - 1) << value) + 1;
		return true;
	}

	void load_const(u64 value)
	{
		ge_than = value;
		value_range = 1;
		is_loaded = false;
		tag = 0;
		bit_range = 0;
	}

	u64 upper_bound() const
	{
		return ge_than + value_range;
	}

	// Using comparison evaluation data to bound data to our needs
	void declare_ordering(reg_state_t& right_register, bool lt, bool eq, bool gt, bool so, bool nso)
	{
		if (lt && gt)
		{
			// We don't care about inequality at the moment

			// Except for 0
			if (right_register.value_range == 1 && right_register.ge_than == 0)
			{
				lift_lower_bound(1);
			}
			else if (value_range == 1 && ge_than == 0)
			{
				right_register.lift_lower_bound(1);
			}

			return;
		}

		if (lt || gt)
		{
			reg_state_t* rhs = &right_register;
			reg_state_t* lhs = this;

			// This is written as if for operator<
			if (gt)
			{
				std::swap(rhs, lhs);
			}

			if (lhs->value_range == 1)
			{
				rhs->lift_lower_bound(lhs->upper_bound() - (eq ? 1 : 0));
			}
			else if (rhs->value_range == 1)
			{
				lhs->limit_upper_bound(rhs->ge_than + (eq ? 1 : 0));
			}
		}
		else if (eq)
		{
			if (value_range == 1)
			{
				right_register = *this;
			}
			else if (right_register.value_range == 1)
			{
				*this = right_register;
			}
			else if (0)
			{
				// set_lower_bound(std::max(ge_than, right_register.ge_than));
				// set_upper_bound(std::min(value_range + ge_than, right_register.ge_than + right_register.value_range));
				// right_register.ge_than = ge_than;
				// right_register.value_range = value_range;
			}
		}
		else if (so || nso)
		{
			// TODO: Implement(?)
		}
	}
};

static constexpr reg_state_t s_reg_const_0{ 0, 1 };

template <>
bool ppu_module<lv2_obj>::analyse(u32 lib_toc, u32 entry, const u32 sec_end, const std::vector<u32>& applied, const std::vector<u32>& exported_funcs, std::function<bool()> check_aborted)
{
	if (segs.empty() || !segs[0].addr)
	{
		return false;
	}

	// Assume first segment is executable
	const u32 start = segs[0].addr;

	// End of executable segment (may change)
	u32 end = sec_end ? sec_end : segs[0].addr + segs[0].size;

	// End of all segments
	u32 segs_end = end;

	for (const auto& s : segs)
	{
		if (s.size && s.addr != start)
		{
			segs_end = std::max(segs_end, s.addr + s.size);
		}
	}

	// Known TOCs (usually only 1)
	std::unordered_set<u32> TOCs;

	struct ppu_function_ext : ppu_function
	{
		//u32 stack_frame = 0;
		u32 single_target = 0;
		u32 trampoline = 0;
		bs_t<ppu_attr> attr{};

		std::set<u32> callers{};
	};

	// Known functions
	std::map<u32, ppu_function_ext> fmap;
	std::set<u32> known_functions;

	// Function analysis workload
	std::vector<std::reference_wrapper<ppu_function_ext>> func_queue;

	// Known references (within segs, addr and value alignment = 4)
	// For seg0, must be valid code
	// Value is a sample of an address that refernces it
	std::map<u32, u32> addr_heap;

	if (entry)
	{
		addr_heap.emplace(entry, 0);
	}

	auto verify_ref = [&](u32 addr)
	{
		if (!is_relocatable)
		{
			// Fixed addresses
			return true;
		}

		// Check if the storage address exists within relocations
		constexpr auto compare = [](const ppu_reloc& a, u32 addr) { return a.addr < addr; };
		auto it = std::lower_bound(this->relocs.begin(), this->relocs.end(), (addr & -8), compare);
		auto end = std::lower_bound(it, this->relocs.end(), (addr & -8) + 8, compare);

		for (; it != end; it++)
		{
			const ppu_reloc& rel = *it;

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

	auto is_valid_code = [](std::span<const be_t<u32>> range, bool is_fixed_addr, u32 /*cia*/)
	{
		for (usz index = 0; index < std::min<usz>(range.size(), 10); index++)
		{
			const ppu_opcode_t op{+range[index]};

			switch (g_ppu_itype.decode(op.opcode))
			{
			case ppu_itype::UNK:
			{
				return false;
			}
			case ppu_itype::BC:
			case ppu_itype::B:
			{
				if (!is_fixed_addr && op.aa)
				{
					return false;
				}

				return true;
			}
			case ppu_itype::BCCTR:
			case ppu_itype::BCLR:
			{
				if (op.opcode & 0xe000)
				{
					// Garbage filter
					return false;
				}

				return true;
			}
			default:
			{
				continue;
			}
			}
		}

		return true;
	};

	// Register new function
	auto add_func = [&](u32 addr, u32 toc, u32 caller) -> ppu_function_ext&
	{
		if (addr < start || addr >= end || g_ppu_itype.decode(*get_ptr<u32>(addr)) == ppu_itype::UNK)
		{
			if (!fmap.contains(addr))
			{
				ppu_log.error("Potentially invalid function has been added: 0x%x", addr);
			}
		}

		ppu_function_ext& func = fmap[addr];

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
				if (ptr[1] == toc && FN(x >= start && x < end && x % 4 == 0)(ptr[0]) && verify_ref(_ptr.addr()))
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
	// For seg0, must be valid code
	for (const auto& seg : segs)
	{
		if (seg.size < 4) continue;

		vm::cptr<u32> _ptr = vm::cast(seg.addr);
		const vm::cptr<void> seg_end = vm::cast(seg.addr + seg.size - 4);
		auto ptr = get_ptr<u32>(_ptr);

		for (; _ptr <= seg_end; advance(_ptr, ptr, 1))
		{
			const u32 value = *ptr;

			if (value % 4 || !verify_ref(_ptr.addr()))
			{
				continue;
			}

			for (const auto& _seg : segs)
			{
				if (!_seg.size) continue;

				if (value >= start && value < end)
				{
					if (is_valid_code({ ptr, ptr + (end - value) }, !is_relocatable, _ptr.addr()))
					{
						continue;
					}

					addr_heap.emplace(value, _ptr.addr());
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

			if (addr % 4 || addr < start || addr >= end || !verify_ref(_ptr.addr()))
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
			add_func(addr, addr_heap.count(_ptr.addr()) ? toc : 0, 0);
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
			const ppu_itype::type type = g_ppu_itype.decode(op.opcode);

			if ((type == ppu_itype::B || type == ppu_itype::BC) && op.lk && (!op.aa || verify_ref(iaddr)))
			{
				const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

				if (target >= start && target < end && target != iaddr && target != iaddr + 4)
				{
					if (is_valid_code({ get_ptr<u32>(target), get_ptr<u32>(end - 4) }, !is_relocatable, target))
					{
						ppu_log.trace("Enqueued PPU function 0x%x using a caller at 0x%x", target, iaddr);
						add_func(target, 0, 0);
						used_fallback = true;
					}
				}
			}
		}
	}

	// Register state (preallocated)
	// Make sure to re-initialize this for every function!
	std::vector<reg_state_t> reg_state_storage(64 * (is_relocatable ? 256 : 1024));

	struct block_local_info_t
	{
		u32 addr = 0;
		u32 size = 0;
		u32 parent_block_idx = umax;
		ppua_reg_mask_t mapped_registers_mask{0};
		ppua_reg_mask_t moved_registers_mask{0};
	};

	// Block analysis workload
	std::vector<block_local_info_t> block_queue_storage;

	bool is_function_caller_analysis = false;

	// Main loop (func_queue may grow)
	for (usz i = 0; i <= func_queue.size(); i++)
	{
		if (i == func_queue.size())
		{
			if (is_function_caller_analysis)
			{
				break;
			}

			// Add callers of imported functions to be analyzed
			std::set<u32> added;

			for (const auto& [stub_addr, _] : stub_addr_to_constant_state_of_registers)
			{
				auto it = fmap.upper_bound(stub_addr);

				if (it == fmap.begin())
				{
					continue;
				}

				auto stub_func = std::prev(it);

				for (u32 caller : stub_func->second.callers)
				{
					ppu_function_ext& func = ::at32(fmap, caller);

					if (func.attr.none_of(ppu_attr::no_size) && !func.blocks.empty() && !added.contains(caller))
					{
						added.emplace(caller);
						func_queue.emplace_back(::at32(fmap, caller));
					}
				}
			}

			if (added.empty())
			{
				break;
			}

			is_function_caller_analysis = true;
		}

		if (check_aborted && check_aborted())
		{
			return false;
		}

		ppu_function_ext& func = func_queue[i];

		// Fixup TOCs
		if (!is_function_caller_analysis && func.toc && func.toc != umax)
		{
			// Fixup callers
			for (u32 addr : func.callers)
			{
				ppu_function_ext& caller = fmap[addr];

				if (!caller.toc)
				{
					add_func(addr, func.toc - caller.trampoline, 0);
				}
			}

			// Fixup callees
			auto for_callee = [&](u32 addr)
			{
				if (addr < func.addr || addr >= func.addr + func.size)
				{
					return;
				}

				const u32 iaddr = addr;

				const ppu_opcode_t op{get_ref<u32>(iaddr)};
				const ppu_itype::type type = g_ppu_itype.decode(op.opcode);

				if (type == ppu_itype::B || type == ppu_itype::BC)
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target >= start && target < end && (!op.aa || verify_ref(iaddr)))
					{
						if (target < func.addr || target >= func.addr + func.size)
						{
							ppu_function_ext& callee = fmap[target];

							if (!callee.toc)
							{
								add_func(target, func.toc + func.trampoline, 0);
							}
						}
					}
				}
			};

			for (const auto& [addr, size] : func.blocks)
			{
				if (size)
				{
					for_callee(addr + size - 4);
				}
			}

			if (func.size)
			{
				for_callee(func.addr + func.size - 4);
			}

			// For trampoline functions
			if (func.single_target)
			{
				ppu_function_ext& callee = fmap[func.single_target];

				if (!callee.toc)
				{
					add_func(func.single_target, func.toc + func.trampoline, 0);
				}
			}
		}

		if (!is_function_caller_analysis && func.blocks.empty())
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

				if (target >= start && target < end && (~ptr[0] & 0x2 || verify_ref(_ptr.addr())))
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
					func.single_target = target;
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

				if (target >= start && target < end && verify_ref(_ptr.addr()))
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
					func.single_target = target;
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
				known_functions.emplace(func.addr);

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
					known_functions.emplace(_p2.addr());
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

				if (target >= start && target < end && verify_ref((_ptr + 3).addr()))
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
					func.single_target = target;
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

				if (target >= start && target < end && (~ptr[3] & 0x2 || verify_ref((_ptr + 3).addr())))
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
					func.single_target = target;
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

		auto& block_queue = block_queue_storage;
		block_queue.clear();
		u32 reg_tag_allocator = 1;

		auto make_unknown_reg_state = [&]()
		{
			reg_state_t s{};
			s.tag = reg_tag_allocator++;
			return s;
		};

		// Add new block for analysis
		auto add_block = [&](u32 addr, u32 parent_block) -> u32
		{
			if (addr < func.addr || addr >= func_end)
			{
				return umax;
			}

			const auto _pair = func.blocks.emplace(addr, 0);

			if (_pair.second)
			{
				auto& block = block_queue.emplace_back(block_local_info_t{addr});
				block.parent_block_idx = parent_block;
	
				if (parent_block != umax)
				{
					// Inherit loaded registers mask (lazily)
					block.mapped_registers_mask.mask = ::at32(block_queue, parent_block).mapped_registers_mask.mask;
				}

				return static_cast<u32>(block_queue.size() - 1);
			}

			return umax;
		};

		std::map<u32, u32> preserve_blocks;

		if (is_function_caller_analysis)
		{
			preserve_blocks = std::move(func.blocks);
			func.blocks.clear();
			func.blocks.emplace(preserve_blocks.begin()->first, 0);
		}

		for (auto& block : func.blocks)
		{
			if (!block.second && block.first < func_end)
			{
				block_queue.emplace_back(block_local_info_t{block.first});
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
			std::for_each(addr_heap.lower_bound(func.addr), addr_heap.lower_bound(func_end2), [&](auto a) { add_block(a.first, umax); });
		}

		bool postpone_analysis = false;

		// Block loop (block_queue may grow, may be aborted via clearing)
		for (u32 j = 0; !postpone_analysis && j < block_queue.size(); [&]()
		{
			if (u32 size = block_queue[j].size)
			{
				// Update size
				func.blocks[block_queue[j].addr] = size;
			}

			j++;
		}())
		{
			block_local_info_t* block = &block_queue[j];

			const u32 block_addr = block->addr;

			vm::cptr<u32> _ptr = vm::cast(block_addr);
			auto ptr = ensure(get_ptr<u32>(_ptr));

			auto is_reg_mapped = [&](u32 index)
			{
				return !!(block_queue[j].mapped_registers_mask.mask & (u64{1} << index));
			};

			reg_state_t dummy_state{};

			const auto get_reg = [&](usz index) -> reg_state_t&
			{
				block_local_info_t* block = &block_queue[j];

				const usz reg_mask = u64{1} << index;

				if (~block->moved_registers_mask.mask & reg_mask)
				{
					if ((j + 1) * 64 >= reg_state_storage.size())
					{
						dummy_state.value_range = 0;
						dummy_state.tag = reg_tag_allocator++;
						return dummy_state;
					}

					usz begin_block = umax;

					// Try searching for register origin
					if (block->mapped_registers_mask.mask & reg_mask)
				 	{
				 		for (u32 i = block->parent_block_idx; i != umax; i = block_queue[i].parent_block_idx)
						{
							if (~block_queue[i].moved_registers_mask.mask & reg_mask)
							{
								continue;
							}

							begin_block = i;
							break;
						}
					}

					// Initialize register or copy from origin
					if (begin_block != umax)
					{
						reg_state_storage[64 * j + index] = reg_state_storage[64 * begin_block + index];
					}
					else
					{
						reg_state_storage[64 * j + index] = make_unknown_reg_state();
					}

					block->mapped_registers_mask.mask |= reg_mask;
					block->moved_registers_mask.mask |= reg_mask;
				}

				return reg_state_storage[64 * j + index];
			};

			const auto store_block_reg = [&](u32 block_index, u32 index, const reg_state_t& rhs)
			{
				if ((block_index + 1) * 64 >= reg_state_storage.size())
				{
					return;
				}

				reg_state_storage[64 * block_index + index] = rhs;

				const usz reg_mask = u64{1} << index;
				block_queue[block_index].mapped_registers_mask.mask |= reg_mask;
				block_queue[block_index].moved_registers_mask.mask |= reg_mask;
			};

			const auto unmap_reg = [&](u32 index)
			{
				block_local_info_t* block = &block_queue[j];

				const usz reg_mask = u64{1} << index;

				block->mapped_registers_mask.mask &= ~reg_mask;
				block->moved_registers_mask.mask &= ~reg_mask;
			};

			enum : u32
			{
				c_reg_lr = 32,
				c_reg_ctr = 33,
				c_reg_vrsave = 34,
				c_reg_xer = 35,
				c_reg_cr0_arg_rhs = 36,
				c_reg_cr7_arg_rhs = c_reg_cr0_arg_rhs + 7,
				c_reg_cr0_arg_lhs = 44,
				c_reg_cr7_arg_lhs = c_reg_cr0_arg_lhs + 7,
			};

			for (; _ptr.addr() < func_end;)
			{
				const u32 iaddr = _ptr.addr();
				const ppu_opcode_t op{*advance(_ptr, ptr, 1)};
				const ppu_itype::type type = g_ppu_itype.decode(op.opcode);

				switch (type)
				{
				case ppu_itype::UNK:
				{
					// Invalid blocks will remain empty
					break;
				}
				case ppu_itype::B:
				case ppu_itype::BC:
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target < start || target >= end)
					{
						ppu_log.warning("[0x%x] Invalid branch at 0x%x -> 0x%x", func.addr, iaddr, target);
						continue;
					}

					if (!op.aa && target == _ptr.addr() && _ptr.addr() < func_end)
					{
						(!!op.lk ? ppu_log.notice : ppu_log.trace)("[0x%x] Branch to next at 0x%x -> 0x%x", func.addr, iaddr, target);
					}

					const bool is_call = op.lk && target != iaddr && target != _ptr.addr() && _ptr.addr() < func_end;
					const auto pfunc = is_call ? &add_func(target, 0, 0) : nullptr;

					if (pfunc && pfunc->blocks.empty() && !is_function_caller_analysis)
					{
						// Postpone analysis (no info)
						postpone_analysis = true;
						break;
					}

					if (is_function_caller_analysis && is_call && !(pfunc->attr & ppu_attr::no_return))
					{
						while (is_function_caller_analysis)
						{
							// Verify that it is the call to the imported function (may be more than one)
							const auto it = stub_addr_to_constant_state_of_registers.lower_bound(target);

							if (it == stub_addr_to_constant_state_of_registers.end())
							{
								break;
							}

							const auto next_func = fmap.upper_bound(it->first);

							if (next_func == fmap.begin())
							{
								break;
							}

							const auto stub_func = std::prev(next_func);

							if (stub_func->first == target)
							{
								// It is
								// Now, mine register state
								// Currently only of R3

								if (is_reg_mapped(3))
								{
									const reg_state_t& value = get_reg(3);

									if (value(is_const))
									{
										it->second.emplace_back(ppua_reg_mask_t{ 1u << 3 }, value(minv) );
									}
								}
							}

							break;
						}
					}

					// Add next block if necessary
					if ((is_call && !(pfunc->attr & ppu_attr::no_return)) || (type == ppu_itype::BC && (op.bo & 0x14) != 0x14))
					{
						const u32 next_idx = add_block(_ptr.addr(), j);

						if (next_idx != umax && target != iaddr + 4 && type == ppu_itype::BC && (op.bo & 0b10100) == 0b00100)
						{
							bool lt{};
							bool gt{};
							bool eq{};
							bool so{};
							bool nso{};

							// Because this is the following instruction, reverse the flags
							if ((op.bo & 0b01000) != 0)
							{
								switch (op.bi % 4)
								{
								case 0x0: gt = true; eq = true; break;
								case 0x1: lt = true; eq = true; break;
								case 0x2: gt = true; lt = true; break;
								case 0x3: nso = true; break;
								default: fmt::throw_exception("Unreachable");
								}
							}
							else
							{
								switch (op.bi % 4)
								{
								case 0x0: lt = true; break;
								case 0x1: gt = true; break;
								case 0x2: eq = true; break;
								case 0x3: so = true; break;
								default: fmt::throw_exception("Unreachable");
								}
							}

							const u32 rhs_cr_state = c_reg_cr0_arg_rhs + op.bi / 4;
							const u32 lhs_cr_state = c_reg_cr0_arg_lhs + op.bi / 4;

							// Complete the block information based on the data that the jump is NOT taken
							// This information would be inherited to next block

							auto lhs_state = get_reg(lhs_cr_state);
							auto rhs_state = get_reg(rhs_cr_state);

							lhs_state.declare_ordering(rhs_state, lt, eq, gt, so, nso);

							store_block_reg(next_idx, lhs_cr_state, lhs_state);
							store_block_reg(next_idx, rhs_cr_state, rhs_state);

							const u64 reg_mask = block_queue[j].mapped_registers_mask.mask;

							for (u32 bit = std::countr_zero(reg_mask); bit < 64 && reg_mask & (u64{1} << bit);
								bit += 1, bit = std::countr_zero(reg_mask >> (bit % 64)) + bit)
							{
								if (bit == lhs_cr_state || bit == rhs_cr_state)
								{
									continue;
								}

								auto& reg_state = get_reg(bit);

								if (reg_state.is_equals(lhs_state))
								{
									store_block_reg(next_idx, bit, lhs_state);
								}
								else if (reg_state.is_equals(rhs_state))
								{
									store_block_reg(next_idx, bit, rhs_state);
								}
							}
						}
					}

					if (is_call && pfunc->attr & ppu_attr::no_return)
					{
						// Nothing
					}
					else if (is_call || target < func.addr || target >= func_end)
					{
						// Add function call (including obvious tail call)
						add_func(target, 0, func.addr);
					}
					else
					{
						// Add block
						add_block(target, umax);
					}

					block_queue[j].size = _ptr.addr() - block_addr;
					break;
				}
				case ppu_itype::BCLR:
				{
					if (op.lk || (op.bo & 0x14) != 0x14)
					{
						add_block(_ptr.addr(), j);
					}

					block_queue[j].size = _ptr.addr() - block_addr;
					break;
				}
				case ppu_itype::BCCTR:
				{
					if (op.lk || (op.bo & 0x10) != 0x10)
					{
						add_block(_ptr.addr(), j);
					}
					else if (get_reg(c_reg_ctr)(is_const))
					{
						ppu_log.todo("[0x%x] BCTR to constant destination at 0x%x: 0x%x", func.addr, iaddr, get_reg(c_reg_ctr)(minv));
					}
					else
					{
						// Analyse jumptable (TODO)
						u32 jt_addr = _ptr.addr();
						u32 jt_end = func_end;
						const auto code_end = get_ptr<u32>(func_end);

						auto get_jumptable_end = [&](vm::cptr<u32>& _ptr, be_t<u32>*& ptr, bool is_relative)
						{
							for (; _ptr.addr() < jt_end; advance(_ptr, ptr, 1))
							{
								const u32 addr = (is_relative ? jt_addr : 0) + *ptr;

								if (addr == jt_addr)
								{
									// TODO (cannot branch to jumptable itself)
									return;
								}

								if (addr % 4 || addr < func.addr || addr >= func_end || !is_valid_code({ get_ptr<u32>(addr), code_end }, !is_relocatable, addr))
								{
									return;
								}

								add_block(addr, j);
							}
						};

						get_jumptable_end(_ptr, ptr, true);

						bool found_jt = jt_addr == jt_end || _ptr.addr() != jt_addr;

						if (!found_jt && is_reg_mapped(c_reg_ctr))
						{
							// Fallback: try to extract jumptable address from registers
							const reg_state_t ctr = get_reg(c_reg_ctr);

							if (ctr.is_loaded)
							{
								vm::cptr<u32> jumpatble_off = vm::cast(static_cast<u32>(ctr(load_addrv)));

								if (be_t<u32>* jumpatble_ptr_begin = get_ptr<u32>(jumpatble_off))
								{
									be_t<u32>* jumpatble_ptr = jumpatble_ptr_begin;

									jt_addr = jumpatble_off.addr();

									jt_end = segs_end;

									for (const auto& seg : segs)
									{
										if (seg.size)
										{
											if (jt_addr < seg.addr + seg.size && jt_addr >= seg.addr)
											{
												jt_end = seg.addr + seg.size;
												break;
											}
										}
									}

									jt_end = utils::align<u32>(static_cast<u32>(std::min<u64>(jt_end - 1, ctr(maxv) - 1) + 1), 4);

									get_jumptable_end(jumpatble_off, jumpatble_ptr, false);

									// If we do not have jump-table bounds, try manually extract reference address (last resort: may be inaccurate in theory)
									if (jumpatble_ptr == jumpatble_ptr_begin && addr_heap.contains(_ptr.addr()))
									{
										// See who is referencing it
										const u32 ref_addr = addr_heap.at(_ptr.addr());

										if (ref_addr > jt_addr && ref_addr < jt_end)
										{
											ppu_log.todo("[0x%x] Jump table not found through GPR search, retrying using addr_heap. (iaddr=0x%x, ref_addr=0x%x)", func.addr, iaddr, ref_addr);

											jumpatble_off = vm::cast(ref_addr);
											jumpatble_ptr_begin = get_ptr<u32>(jumpatble_off);
											jumpatble_ptr = jumpatble_ptr_begin;
											jt_addr = ref_addr;

											get_jumptable_end(jumpatble_off, jumpatble_ptr, false);
										}
									}

									if (jumpatble_ptr != jumpatble_ptr_begin)
									{
										found_jt = true;

										for (be_t<u32> addr : std::span<const be_t<u32>>{ jumpatble_ptr_begin , jumpatble_ptr })
										{
											if (addr == _ptr.addr())
											{
												ppu_log.success("[0x%x] Found address of next code in jump table at 0x%x! 0x%x-0x%x", func.addr, iaddr, jt_addr, jt_addr + 4 * (jumpatble_ptr - jumpatble_ptr_begin));
												break;
											}
										}
									}
								}
							}
						}

						if (!found_jt)
						{
							// Acknowledge jumptable detection failure
							if (!(func.attr & ppu_attr::no_size))
							{
								ppu_log.warning("[0x%x] Jump table not found! 0x%x-0x%x", func.addr, jt_addr, jt_end);
							}

							func.attr += ppu_attr::no_size;
							add_block(jt_addr, j);
							postpone_analysis = true;
						}
						else
						{
							ppu_log.trace("[0x%x] Jump table found: 0x%x-0x%x", func.addr, jt_addr, _ptr);
						}
					}

					block_queue[j].size = _ptr.addr() - block_addr;
					break;
				}
				case ppu_itype::TD:
				case ppu_itype::TW:
				case ppu_itype::TDI:
				case ppu_itype::TWI:
				{
					if (!op.bo)
					{
						// No-op
						continue;
					}

					if (can_trap_continue(op, type))
					{
						add_block(_ptr.addr(), j);
					}

					block_queue[j].size = _ptr.addr() - block_addr;
					break;
				}
				case ppu_itype::SC:
				{
					add_block(_ptr.addr(), j);
					block_queue[j].size = _ptr.addr() - block_addr;
					break;
				}
				case ppu_itype::STDU:
				{
					if (func.attr & ppu_attr::no_size && (op.opcode == *ptr || *ptr == ppu_instructions::BLR()))
					{
						// Hack
						ppu_log.success("[0x%x] Instruction repetition: 0x%08x", iaddr, op.opcode);
						add_block(_ptr.addr(), j);
						block_queue[j].size = _ptr.addr() - block_addr;
					}

					continue;
				}
				case ppu_itype::ADDI:
				case ppu_itype::ADDIC:
				case ppu_itype::ADDIS:
				{
					const s64 to_add = type == ppu_itype::ADDIS ? op.simm16 * 65536 : op.simm16;

					const reg_state_t ra = type == ppu_itype::ADDIC || op.ra ? get_reg(op.ra) : s_reg_const_0;

					if (ra(is_const))
					{
						reg_state_t rd{};
						rd.load_const(ra(minv) + to_add);
						store_block_reg(j, op.rd, rd);
					}
					else
					{
						unmap_reg(op.rd);
					}

					continue;
				}
				case ppu_itype::ORI:
				case ppu_itype::ORIS:
				{
					const u64 to_or = type == ppu_itype::ORIS ? op.uimm16 * 65536 : op.uimm16;

					const reg_state_t rs = get_reg(op.rs);

					if (!to_or && is_reg_mapped(op.rs))
					{
						store_block_reg(j, op.ra, get_reg(op.rs));
					}
					else if (rs(is_const))
					{
						reg_state_t ra{};
						ra.load_const(rs(minv) | to_or);
						store_block_reg(j, op.ra, ra);
					}
					else
					{
						unmap_reg(op.ra);
					}

					continue;
				}
				case ppu_itype::MTSPR:
				{
					const u32 spr_idx = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

					switch (spr_idx)
					{
					case 0x001: // MTXER
					{
						break;
					}
					case 0x008: // MTLR
					{
						//store_block_reg(j, c_reg_lr, get_reg(op.rs));
						break;
					}
					case 0x009: // MTCTR
					{
						store_block_reg(j, c_reg_ctr, get_reg(op.rs));
						break;
					}
					case 0x100:
					{
						// get_reg(c_reg_vrsave) = get_reg(op.rs);
						// get_reg(c_reg_vrsave).value &= u32{umax};
						break;
					}
					default:
					{
						break;
					}
					}

					continue;
				}
				case ppu_itype::LWZ:
				{
					const bool is_load_from_toc = (is_function_caller_analysis && op.ra == 2u && func.toc && func.toc != umax);

					if (is_load_from_toc || is_reg_mapped(op.rd) || is_reg_mapped(op.ra))
					{
						const reg_state_t ra = get_reg(op.ra);
						auto& rd = get_reg(op.rd);

						rd = {};
						rd.tag = reg_tag_allocator++;
						rd.is_loaded = true;

						reg_state_t const_offs{};
						const_offs.load_const(op.simm16);

						reg_state_t toc_offset{};
						toc_offset.load_const(func.toc);

						const reg_state_t& off_ra = is_load_from_toc ? toc_offset : ra;

						rd.ge_than = const_offs(minv);

						const bool is_negative = const_offs(sign_bitv) == 1u;

						const bool is_offset_test_ok = is_negative
							? (0 - const_offs(minv) <= off_ra(minv) && off_ra(minv) + const_offs(minv) < segs_end)
							: (off_ra(minv) < segs_end && const_offs(minv) < segs_end - off_ra(minv));

						if (off_ra(minv) < off_ra(maxv) && is_offset_test_ok)
						{
							rd.ge_than += off_ra(minv);

							const bool is_range_end_test_ok = is_negative
								? (off_ra(maxv) + const_offs(minv) <= segs_end)
								: (off_ra(maxv) - 1 < segs_end - 1 && const_offs(minv) <= segs_end - off_ra(maxv));

							if (is_range_end_test_ok)
							{
								rd.value_range = off_ra.value_range;
							}
						}

						if (is_load_from_toc)
						{
							if (rd.value_range == 1)
							{
								// Try to load a constant value from data segment
								if (auto val_ptr = get_ptr<u32>(static_cast<u32>(rd.ge_than)))
								{
									rd = {};
									rd.load_const(*val_ptr);
								}
							}
						}
					}

					continue;
				}
				case ppu_itype::LWZX:
				case ppu_itype::LDX: // TODO: Confirm if LDX can appear in jumptable branching (probably in LV1 applications such as ps2_emu)
				{
					if (is_reg_mapped(op.ra) || is_reg_mapped(op.rb))
					{
						const reg_state_t ra = get_reg(op.ra);
						const reg_state_t rb = get_reg(op.rb);

						const bool is_ra = ra(is_const) && (ra(minv) >= start && ra(minv) < segs_end);

						if (ra(is_const) == rb(is_const))
						{
							unmap_reg(op.rd);
						}
						else
						{
							// Register possible jumptable offset
							auto& rd = get_reg(op.rd);
							rd = {};
							rd.tag = reg_tag_allocator++;
							rd.is_loaded = true;

							const reg_state_t& const_reg = is_ra ? ra : rb;
							const reg_state_t& off_reg = is_ra ? rb : ra;

							rd.ge_than = const_reg(minv);

							if (off_reg.value_range != 0 && off_reg(minv) < segs_end && const_reg(minv) < segs_end - off_reg(minv))
							{
								rd.ge_than += off_reg(minv);

								if (off_reg(maxv) - 1 < segs_end - 1 && const_reg(minv) <= segs_end - off_reg(maxv))
								{
									rd.value_range = off_reg.value_range;
								}
							}
						}
					}

					continue;
				}
				case ppu_itype::RLWINM:
				{
					//const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);

					if (!is_reg_mapped(op.rs) && !is_reg_mapped(op.ra))
					{
						continue;
					}

					if (op.mb32 <= op.me32 && op.sh32 == 31 - op.me32)
					{
						// SLWI mnemonic (mb32 == 0) or generic form

						reg_state_t rs = get_reg(op.rs);

						if (!rs.shift_left(op.sh32, reg_tag_allocator) || !rs.clear_higher_bits(32 + op.mb32, reg_tag_allocator))
						{
							unmap_reg(op.ra);
						}
						else
						{
							store_block_reg(j, op.ra, rs);
						}
					}
					else
					{
						unmap_reg(op.ra);
					}

					continue;
				}
				case ppu_itype::RLDICR:
				{
					const u32 sh = op.sh64;
					const u32 me = op.mbe64;
					//const u64 mask = ~0ull << (63 - me);

					if (sh == 63 - me)
					{
						// SLDI mnemonic
						reg_state_t rs = get_reg(op.rs);

						if (!rs.shift_left(op.sh32, reg_tag_allocator))
						{
							unmap_reg(op.ra);
						}
						else
						{
							store_block_reg(j, op.ra, rs);
						}
					}
					else
					{
						unmap_reg(op.ra);
					}

					continue;
				}
				case ppu_itype::CMPLI:
				case ppu_itype::CMPI:
				case ppu_itype::CMP:
				case ppu_itype::CMPL:
				{
					const bool is_wbits = op.l10 == 0;
					const bool is_signed = type == ppu_itype::CMPI || type == ppu_itype::CMP;
					const bool is_rhs_register = type == ppu_itype::CMP || type == ppu_itype::CMPL;

					reg_state_t rhs{};
					reg_state_t lhs{};

					lhs = get_reg(op.ra);

					if (is_rhs_register)
					{
						rhs = get_reg(op.rb);
					}
					else
					{
						const u64 compared = is_signed ? op.simm16 : op.uimm16;
						rhs.load_const(is_wbits ? (compared & u32{umax}) : compared);
					}

					if (is_wbits)
					{
						lhs.undef_higher_bits(32);
						rhs.undef_higher_bits(32);
					}

					if (is_signed)
					{
						// Force the comparison to become unsigned
						// const u64 sign_bit = u64{1} << (is_wbits ? 31 : 63);
						// lhs.add(sign_bit);
						// rhs.add(sign_bit);
					}

					const u32 cr_idx = op.crfd;
					store_block_reg(j, c_reg_cr0_arg_rhs + cr_idx, rhs);
					store_block_reg(j, c_reg_cr0_arg_lhs + cr_idx, lhs);
					continue;
				}
				default:
				{
					if (type & ppu_itype::store)
					{
						continue;
					}

					// TODO: Tell if instruction modified RD or RA
					unmap_reg(op.rd);
					unmap_reg(op.ra);
					continue;
				}
				}

				break;
			}
		}

		if (!preserve_blocks.empty())
		{
			ensure(func.blocks.size() == preserve_blocks.size());

			for (auto fit = func.blocks.begin(), pit = preserve_blocks.begin(); fit != func.blocks.end(); fit++, pit++)
			{
				// Ensure block addresses match
				ensure(fit->first == pit->first);
			}

			func.blocks = std::move(preserve_blocks);
		}

		if (postpone_analysis)
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
				const ppu_itype::type type = g_ppu_itype.decode(op.opcode);

				if (type == ppu_itype::B || type == ppu_itype::BC)
				{
					const u32 target = (op.aa ? 0 : iaddr) + (type == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target >= start && target < end && (!op.aa || verify_ref(iaddr)))
					{
						if (target < func.addr || target >= func.addr + func.size)
						{
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
			const ppu_itype::type type = g_ppu_itype.decode(op.opcode);

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

	// Iterate by address (fmap may grow)
	for (u32 addr_next = start; addr_next != umax;)
	{
		// Get next iterator
		const auto it = fmap.lower_bound(addr_next);

		if (it == fmap.end())
		{
			break;
		}

		// Save next function address as is as of this moment (ignoring added functions)
		const auto it_next = std::next(it);
		addr_next = it_next == fmap.end() ? u32{umax} : it_next->first;

		const ppu_function_ext& func = it->second;

		if (func.attr & ppu_attr::no_size && !is_relocatable)
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
			addr_next = std::max<u32>(addr_next, lim);
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
			ppu_log.trace("Block __0x%x added (func=0x%x, size=0x%x, toc=0x%x)", block.addr, it->first, block.size, block.toc);

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

				switch (auto type = g_ppu_itype.decode(op.opcode))
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
						const auto type0 = g_ppu_itype.decode(test_op.opcode);

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

							const auto type1 = g_ppu_itype.decode(get_ref<u32>(target + 4));

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

			if (is_good)
			{
				// Special opting-out: range of "instructions" is likely to be embedded floats
				// Test this by testing if the lower 23 bits are 0
				vm::cptr<u32> _ptr = vm::cast(exp);
				auto ptr = get_ptr<const u32>(_ptr);

				bool ok = false;

				for (u32 it = exp; it < std::min(i_pos + 4, lim); it += 4, advance(_ptr, ptr, 1))
				{
					const u32 value_of_supposed_opcode = *ptr;

					if (value_of_supposed_opcode == ppu_instructions::NOP() || (value_of_supposed_opcode << (32 - 23)))
					{
						ok = true;
						break;
					}
				}

				if (!ok)
				{
					is_good = false;
				}
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
		ppu_function_ext block = std::move(fmap.extract(it).mapped());

		if (block.attr & ppu_attr::no_size && block.size > 4 && !used_fallback)
		{
			ppu_log.warning("Block 0x%x will be compiled on per-instruction basis (size=0x%x)", block.addr, block.size);

			for (u32 addr = block.addr; addr < block.addr + block.size; addr += 4)
			{
				auto& i = funcs.emplace_back();
				i.addr = addr;
				i.size = 4;
				i.toc  = block.toc;
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
