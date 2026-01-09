#include "stdafx.h"
#include "FPOpcodes.h"

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <unordered_set>

namespace rsx::assembler::FP
{
	static const char* s_opcode_names[RSX_FP_OPCODE_ENUM_MAX + 1] =
	{
		"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DP4", "DST", "MIN", "MAX", "SLT", "SGE", "SLE", "SGT", "SNE", "SEQ",                                   // 0x00 - 0x0F
		"FRC", "FLR", "KIL", "PK4", "UP4", "DDX", "DDY", "TEX", "TXP", "TXD", "RCP", "RSQ", "EX2", "LG2", "LIT", "LRP",                                   // 0x10 - 0x1F
		"STR", "SFL", "COS", "SIN", "PK2", "UP2", "POW", "PKB", "UPB", "PK16", "UP16", "BEM", "PKG", "UPG", "DP2A", "TXL",                                // 0x20 - 0x2F
		"UNK_30", "TXB", "UNK_32", "TEXBEM", "TXPBEM", "BEMLUM", "REFL", "TIMESWTEX", "DP2", "NRM", "DIV", "DIVSQ", "LIF", "FENCT", "FENCB", "UNK_3F",    // 0x30 - 0x3F
		"BRK", "CAL", "IFE", "LOOP", "REP", "RET",                                                                                                        // 0x40 - 0x45 (Flow control)
		"OR16_LO", "OR16_HI"                                                                                                                              // Custom instructions for RPCS3 use
	};

	const char* get_opcode_name(FP_opcode opcode)
	{
		if (opcode > RSX_FP_OPCODE_ENUM_MAX)
		{
			return "invalid";
		}
		return s_opcode_names[opcode];
	}

	bool is_instruction_valid(FP_opcode opcode)
	{
		switch (opcode)
		{
		case RSX_FP_OPCODE_POW:
		case RSX_FP_OPCODE_BEM:
		case RSX_FP_OPCODE_TEXBEM:
		case RSX_FP_OPCODE_TXPBEM:
		case RSX_FP_OPCODE_BEMLUM:
		case RSX_FP_OPCODE_TIMESWTEX:
			return false;
		default:
			// This isn't necessarily true
			return opcode <= RSX_FP_OPCODE_ENUM_MAX;
		}
	}

	u8 get_operand_count(FP_opcode opcode)
	{
		switch (opcode)
		{
		case RSX_FP_OPCODE_NOP:
			return 0;
		case RSX_FP_OPCODE_MOV:
			return 1;
		case RSX_FP_OPCODE_MUL:
		case RSX_FP_OPCODE_ADD:
			return 2;
		case RSX_FP_OPCODE_MAD:
			return 3;
		case RSX_FP_OPCODE_DP3:
		case RSX_FP_OPCODE_DP4:
			return 2;
		case RSX_FP_OPCODE_DST:
			return 2;
		case RSX_FP_OPCODE_MIN:
		case RSX_FP_OPCODE_MAX:
			return 2;
		case RSX_FP_OPCODE_SLT:
		case RSX_FP_OPCODE_SGE:
		case RSX_FP_OPCODE_SLE:
		case RSX_FP_OPCODE_SGT:
		case RSX_FP_OPCODE_SNE:
		case RSX_FP_OPCODE_SEQ:
			return 2;
		case RSX_FP_OPCODE_FRC:
		case RSX_FP_OPCODE_FLR:
			return 1;
		case RSX_FP_OPCODE_KIL:
			return 0;
		case RSX_FP_OPCODE_PK4:
		case RSX_FP_OPCODE_UP4:
			return 1;
		case RSX_FP_OPCODE_DDX:
		case RSX_FP_OPCODE_DDY:
			return 1;
		case RSX_FP_OPCODE_TEX:
		case RSX_FP_OPCODE_TXD:
		case RSX_FP_OPCODE_TXP:
			return 1;
		case RSX_FP_OPCODE_RCP:
		case RSX_FP_OPCODE_RSQ:
		case RSX_FP_OPCODE_EX2:
		case RSX_FP_OPCODE_LG2:
			return 1;
		case RSX_FP_OPCODE_LIT:
			return 1;
		case RSX_FP_OPCODE_LRP:
			return 3;
		case RSX_FP_OPCODE_STR:
		case RSX_FP_OPCODE_SFL:
			return 0;
		case RSX_FP_OPCODE_COS:
		case RSX_FP_OPCODE_SIN:
			return 1;
		case RSX_FP_OPCODE_PK2:
		case RSX_FP_OPCODE_UP2:
			return 1;
		case RSX_FP_OPCODE_PKB:
		case RSX_FP_OPCODE_UPB:
		case RSX_FP_OPCODE_PK16:
		case RSX_FP_OPCODE_UP16:
		case RSX_FP_OPCODE_PKG:
		case RSX_FP_OPCODE_UPG:
			return 1;
		case RSX_FP_OPCODE_DP2A:
			return 3;
		case RSX_FP_OPCODE_TXL:
		case RSX_FP_OPCODE_TXB:
			return 2;
		case RSX_FP_OPCODE_DP2:
			return 2;
		case RSX_FP_OPCODE_NRM:
			return 1;
		case RSX_FP_OPCODE_DIV:
		case RSX_FP_OPCODE_DIVSQ:
			return 2;
		case RSX_FP_OPCODE_LIF:
			return 1;
		case RSX_FP_OPCODE_REFL:
			return 2;
		case RSX_FP_OPCODE_FENCT:
		case RSX_FP_OPCODE_FENCB:
		case RSX_FP_OPCODE_BRK:
		case RSX_FP_OPCODE_CAL:
		case RSX_FP_OPCODE_IFE:
		case RSX_FP_OPCODE_LOOP:
		case RSX_FP_OPCODE_REP:
		case RSX_FP_OPCODE_RET:
			// Flow control. Special registers are provided for these outside the common file
			return 0;

		// The rest are unimplemented and not encountered in real software.
		// TODO: Probe these on real PS3 and figure out what they actually do.
		case RSX_FP_OPCODE_POW:
			fmt::throw_exception("Unimplemented POW instruction."); // Unused
		case RSX_FP_OPCODE_BEM:
		case RSX_FP_OPCODE_TEXBEM:
		case RSX_FP_OPCODE_TXPBEM:
		case RSX_FP_OPCODE_BEMLUM:
			fmt::throw_exception("Unimplemented BEM class instruction"); // Unused
		case RSX_FP_OPCODE_TIMESWTEX:
			fmt::throw_exception("Unimplemented TIMESWTEX instruction"); // Unused
		default:
			break;
		}

		return 0;
	}

	// Returns a lane mask for the given operand.
	// The lane mask is the fixed function hardware lane so swizzles need to be applied on top to resolve the real data channel.
	u32 get_src_vector_lane_mask(const RSXFragmentProgram& prog, const Instruction* instruction, u32 operand)
	{
		constexpr u32 x = 0b0001;
		constexpr u32 y = 0b0010;
		constexpr u32 z = 0b0100;
		constexpr u32 w = 0b1000;
		constexpr u32 xy = 0b0011;
		constexpr u32 xyz = 0b0111;
		constexpr u32 xyzw = 0b1111;

		const auto decode = [&](const rsx::simple_array<u32>& masks) -> u32
		{
			return operand < masks.size()
				? masks[operand]
				: 0u;
		};

		auto opcode = static_cast<FP_opcode>(instruction->opcode);
		if (operand >= get_operand_count(opcode))
		{
			return 0;
		}

		OPDEST d0 { .HEX = instruction->bytecode[0] };
		const u32 dst_write_mask = d0.no_dest ? 0 : d0.write_mask;

		switch (opcode)
		{
		case RSX_FP_OPCODE_NOP:
			return 0;
		case RSX_FP_OPCODE_MOV:
		case RSX_FP_OPCODE_MUL:
		case RSX_FP_OPCODE_ADD:
		case RSX_FP_OPCODE_MAD:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_DP3:
			return xyz;
		case RSX_FP_OPCODE_DP4:
			return xyzw;
		case RSX_FP_OPCODE_DST:
			return decode({ y | z, y | w });
		case RSX_FP_OPCODE_MIN:
		case RSX_FP_OPCODE_MAX:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_SLT:
		case RSX_FP_OPCODE_SGE:
		case RSX_FP_OPCODE_SLE:
		case RSX_FP_OPCODE_SGT:
		case RSX_FP_OPCODE_SNE:
		case RSX_FP_OPCODE_SEQ:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_FRC:
		case RSX_FP_OPCODE_FLR:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_KIL:
			return 0;
		case RSX_FP_OPCODE_PK4:
			return xyzw;
		case RSX_FP_OPCODE_UP4:
			return x;
		case RSX_FP_OPCODE_DDX:
		case RSX_FP_OPCODE_DDY:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_TEX:
		case RSX_FP_OPCODE_TXD:
			switch (prog.get_texture_dimension(d0.tex_num))
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				return x;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				return xy;
			case rsx::texture_dimension_extended::texture_dimension_3d:
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				return xyz;
			default:
				return 0;
			}
		case RSX_FP_OPCODE_TXP:
			switch (prog.get_texture_dimension(d0.tex_num))
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				return xy;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				return xyz;
			case rsx::texture_dimension_extended::texture_dimension_3d:
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				return xyzw;
			default:
				return 0;
			}
		case RSX_FP_OPCODE_RCP:
		case RSX_FP_OPCODE_RSQ:
		case RSX_FP_OPCODE_EX2:
		case RSX_FP_OPCODE_LG2:
			return x;
		case RSX_FP_OPCODE_LIT:
			return xyzw;
		case RSX_FP_OPCODE_LRP:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_STR:
		case RSX_FP_OPCODE_SFL:
			return xyzw & dst_write_mask;
		case RSX_FP_OPCODE_COS:
		case RSX_FP_OPCODE_SIN:
			return x;
		case RSX_FP_OPCODE_PK2:
			return xy;
		case RSX_FP_OPCODE_UP2:
			return x;
		case RSX_FP_OPCODE_PKB:
			return xyzw;
		case RSX_FP_OPCODE_UPB:
			return x;
		case RSX_FP_OPCODE_PK16:
			return xy;
		case RSX_FP_OPCODE_UP16:
			return x;
		case RSX_FP_OPCODE_PKG:
			return xyzw;
		case RSX_FP_OPCODE_UPG:
			return x;
		case RSX_FP_OPCODE_DP2A:
			return decode({ xy, xy, x });
		case RSX_FP_OPCODE_TXL:
		case RSX_FP_OPCODE_TXB:
			return decode({ xy, x });
		case RSX_FP_OPCODE_REFL:
			return xyzw;
		case RSX_FP_OPCODE_DP2:
			return xy;
		case RSX_FP_OPCODE_NRM:
			return xyz;
		case RSX_FP_OPCODE_DIV:
		case RSX_FP_OPCODE_DIVSQ:
			return decode({ xyzw, x }) & dst_write_mask;
		case RSX_FP_OPCODE_LIF:
			return decode({ y | w });
		case RSX_FP_OPCODE_FENCT:
		case RSX_FP_OPCODE_FENCB:
		case RSX_FP_OPCODE_BRK:
		case RSX_FP_OPCODE_CAL:
		case RSX_FP_OPCODE_IFE:
		case RSX_FP_OPCODE_LOOP:
		case RSX_FP_OPCODE_REP:
		case RSX_FP_OPCODE_RET:
			// Flow control. Special registers are provided for these outside the common file
			return 0;

		case RSX_FP_OPCODE_POW:
			fmt::throw_exception("Unimplemented POW instruction."); // Unused ??
		case RSX_FP_OPCODE_BEM:
		case RSX_FP_OPCODE_TEXBEM:
		case RSX_FP_OPCODE_TXPBEM:
		case RSX_FP_OPCODE_BEMLUM:
			fmt::throw_exception("Unimplemented BEM class instruction"); // Unused
		case RSX_FP_OPCODE_TIMESWTEX:
			fmt::throw_exception("Unimplemented TIMESWTEX instruction"); // Unused
		default:
			break;
		}

		return 0;
	}

	// Resolved vector lane mask with swizzles applied.
	u32 get_src_vector_lane_mask_shuffled(const RSXFragmentProgram& prog, const Instruction* instruction, u32 operand)
	{
		// Brute-force this. There's only 16 permutations.
		constexpr u32 x = 0b0001;
		constexpr u32 y = 0b0010;
		constexpr u32 z = 0b0100;
		constexpr u32 w = 0b1000;

		const u32 lane_mask = get_src_vector_lane_mask(prog, instruction, operand);
		if (!lane_mask)
		{
			return lane_mask;
		}

		// Now we resolve matching lanes.
		// This sequence can be drastically sped up using lookup tables but that will come later.
		std::unordered_set<u32> inputs;
		SRC_Common src { .HEX = instruction->bytecode[operand + 1] };

		if (src.reg_type != RSX_FP_REGISTER_TYPE_TEMP)
		{
			return 0;
		}

		if (lane_mask & x) inputs.insert(src.swizzle_x);
		if (lane_mask & y) inputs.insert(src.swizzle_y);
		if (lane_mask & z) inputs.insert(src.swizzle_z);
		if (lane_mask & w) inputs.insert(src.swizzle_w);

		u32 result = 0;
		if (inputs.contains(0)) result |= x;
		if (inputs.contains(1)) result |= y;
		if (inputs.contains(2)) result |= z;
		if (inputs.contains(3)) result |= w;

		return result;
	}

	bool is_delay_slot(const Instruction* instruction)
	{
		OPDEST dst { .HEX = instruction->bytecode[0] };
		SRC0 src0 { .HEX = instruction->bytecode[1] };
		SRC1 src1{ .HEX = instruction->bytecode[2] };

		if (dst.opcode != RSX_FP_OPCODE_MOV ||            // These slots are always populated with MOV
			dst.no_dest ||                                // Must have a sink
			src0.reg_type != RSX_FP_REGISTER_TYPE_TEMP || // Must read from reg
			dst.dest_reg != src0.tmp_reg_index ||         // Must be a write-to-self
			dst.fp16 ||                                   // Always full lane. We need to collect more data on this but it won't matter
			dst.saturate ||                               // Precision modifier
			(dst.prec != RSX_FP_PRECISION_REAL &&
				dst.prec != RSX_FP_PRECISION_UNKNOWN))    // Cannot have precision modifiers
		{
			return false;
		}

		// Check if we have precision modifiers on the source
		if (src0.abs || src0.neg || src1.scale)
		{
			return false;
		}

		if (dst.mask_x && src0.swizzle_x != 0) return false;
		if (dst.mask_y && src0.swizzle_y != 1) return false;
		if (dst.mask_z && src0.swizzle_z != 2) return false;
		if (dst.mask_w && src0.swizzle_w != 3) return false;

		return true;
	}

	RegisterRef get_src_register(const RSXFragmentProgram& prog, const Instruction* instruction, u32 operand)
	{
		SRC_Common src{ .HEX = instruction->bytecode[operand + 1] };
		if (src.reg_type != RSX_FP_REGISTER_TYPE_TEMP)
		{
			return {};
		}

		const u32 read_lanes = get_src_vector_lane_mask_shuffled(prog, instruction, operand);
		if (!read_lanes)
		{
			return {};
		}

		RegisterRef ref{ .mask = read_lanes };
		Register& reg = ref.reg;

		reg.f16 = !!src.fp16;
		reg.id = src.tmp_reg_index;
		return ref;
	}

	RegisterRef get_dst_register(const Instruction* instruction)
	{
		OPDEST dst { .HEX = instruction->bytecode[0] };
		if (dst.no_dest)
		{
			return {};
		}

		RegisterRef ref{ .mask = dst.write_mask };
		ref.reg.f16 = dst.fp16;
		ref.reg.id = dst.dest_reg;
		return ref;
	}

	// Convert vector mask to file range
	rsx::simple_array<u32> get_register_file_range(const RegisterRef& reg)
	{
		if (!reg.mask || reg.reg.id >= 48)
		{
			return {};
		}

		const u32 lane_width = reg.reg.f16 ? 2 : 4;
		const u32 file_offset = reg.reg.id * lane_width * 4;

		ensure(file_offset < constants::register_file_max_len, "Invalid register index");

		rsx::simple_array<u32> result{};
		auto insert_lane = [&](u32 word_offset)
		{
			for (u32 i = 0; i < lane_width; ++i)
			{
				result.push_back(file_offset + (word_offset * lane_width) + i);
			}
		};

		if (reg.x) insert_lane(0);
		if (reg.y) insert_lane(1);
		if (reg.z) insert_lane(2);
		if (reg.w) insert_lane(3);

		return result;
	}
}
