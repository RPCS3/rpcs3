#include "stdafx.h"
#include "RegisterDependencyPass.h"
#include "Emu/RSX/Program/Assembler/FPOpcodes.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <unordered_set>

namespace rsx::assembler::FP
{
	static constexpr u32 register_file_length = 48 * 8; // 24 F32 or 48 F16 registers
	static constexpr char content_unknown = 0;
	static constexpr char content_float32 = 'R';
	static constexpr char content_float16 = 'H';
	static constexpr char content_dual = 'D';

	std::vector<RegisterRef> decode_lanes16(const std::unordered_set<u32>& lanes)
	{
		std::vector<RegisterRef> result;

		for (u32 index = 0, file_offset = 0; index < 48; ++index, file_offset += 8)
		{
			// Each register has 4 16-bit lanes
			u32 mask = 0;
			if (lanes.contains(file_offset + 0)) mask |= (1u << 0);
			if (lanes.contains(file_offset + 2)) mask |= (1u << 1);
			if (lanes.contains(file_offset + 4)) mask |= (1u << 2);
			if (lanes.contains(file_offset + 6)) mask |= (1u << 3);

			if (mask == 0)
			{
				continue;
			}

			RegisterRef ref{ .reg{.id = static_cast<int>(index), .f16 = true } };
			ref.mask = mask;
			result.push_back(ref);
		}
		return result;
	}

	std::vector<RegisterRef> decode_lanes32(const std::unordered_set<u32>& lanes)
	{
		std::vector<RegisterRef> result;

		for (u32 index = 0, file_offset = 0; index < 48; ++index, file_offset += 16)
		{
			// Each register has 8 16-bit lanes

			u32 mask = 0;
			if (lanes.contains(file_offset + 0)  || lanes.contains(file_offset + 2))  mask |= (1u << 0);
			if (lanes.contains(file_offset + 4)  || lanes.contains(file_offset + 6))  mask |= (1u << 1);
			if (lanes.contains(file_offset + 8)  || lanes.contains(file_offset + 10)) mask |= (1u << 2);
			if (lanes.contains(file_offset + 12) || lanes.contains(file_offset + 14)) mask |= (1u << 3);

			if (mask == 0)
			{
				continue;
			}

			RegisterRef ref{ .reg{.id = static_cast<int>(index), .f16 = false } };
			ref.mask = mask;
			result.push_back(ref);
		}

		return result;
	}

	std::vector<Instruction> build_barrier32(const RegisterRef& reg)
	{
		// Upto 4 instructions are needed per 32-bit register
		// R0.x = packHalf2x16(H0.xy)
		// R0.y = packHalf2x16(H0.zw);
		// R0.z = packHalf2x16(H1.xy);
		// R0.w = packHalf2x16(H1.zw);

		std::vector<Instruction> result;

		for (u32 mask = reg.mask, ch = 0; mask > 0; mask >>= 1, ++ch)
		{
			if (!(mask & 1))
			{
				continue;
			}

			Instruction instruction{};
			OPDEST dst{};
			dst.opcode = RSX_FP_OPCODE_PK2;
			dst.prec = RSX_FP_PRECISION_REAL;
			dst.fp16 = 0;
			dst.dest_reg = reg.reg.id;
			dst.write_mask = (1u << ch);

			const u32 src_reg_id = (ch / 2) + (reg.reg.id * 2);
			const bool is_word0 = !(ch & 1); // Only even

			SRC0 src0{};
			src0.exec_if_eq = src0.exec_if_gr = src0.exec_if_lt = 1;
			src0.fp16 = 1;

			if (is_word0)
			{
				src0.swizzle_x = 0;
				src0.swizzle_y = 1;
			}
			else
			{
				src0.swizzle_x = 2;
				src0.swizzle_y = 3;
			}

			src0.swizzle_z = 2;
			src0.swizzle_w = 3;
			src0.reg_type = RSX_FP_REGISTER_TYPE_TEMP;
			src0.tmp_reg_index = src_reg_id;

			instruction.opcode = dst.opcode;
			instruction.bytecode[0] = dst.HEX;
			instruction.bytecode[1] = src0.HEX;

			Register src_reg{ .id = static_cast<int>(src_reg_id), .f16 = true };
			instruction.srcs.push_back({ .reg=src_reg, .mask=0xF });
			instruction.dsts.push_back({ .reg{ .id = reg.reg.id, .f16 = false }, .mask = (1u << ch) });
			result.push_back(instruction);
		}

		return result;
	}

	std::vector<Instruction> build_barrier16(const RegisterRef& reg)
	{
		// H0.xy = unpackHalf2x16(R0.x)
		// H0.zw = unpackHalf2x16(R0.y)
		// H1.xy = unpackHalf2x16(R0.z)
		// H1.zw = unpackHalf2x16(R0.w)

		std::vector<Instruction> result;

		for (u32 mask = reg.mask, ch = 0; mask > 0; mask >>= 1, ++ch)
		{
			if (!(mask & 1))
			{
				continue;
			}

			Instruction instruction{};
			OPDEST dst{};
			dst.opcode = RSX_FP_OPCODE_UP2;
			dst.prec = RSX_FP_PRECISION_HALF;
			dst.fp16 = 1;
			dst.dest_reg = reg.reg.id;
			dst.write_mask = 1u << ch;

			const u32 src_reg_id = reg.reg.id / 2;
			const bool is_odd_reg = !!(reg.reg.id & 1);
			const bool is_word0 = ch < 2;

			// If we're a non-odd register, we should also write the next channel (y/w)
			if (!is_odd_reg && (mask & 2))
			{
				mask >>= 1;
				++ch;
				dst.write_mask |= (1u << ch);
			}

			SRC0 src0{};
			src0.exec_if_eq = src0.exec_if_gr = src0.exec_if_lt = 1;

			if (is_word0)
			{
				src0.swizzle_x = is_odd_reg ? 2 : 0;
			}
			else
			{
				src0.swizzle_x = is_odd_reg ? 3 : 1;
			}

			src0.swizzle_y = 1;
			src0.swizzle_z = 2;
			src0.swizzle_w = 3;
			src0.reg_type = RSX_FP_REGISTER_TYPE_TEMP;
			src0.tmp_reg_index = src_reg_id;

			instruction.opcode = dst.opcode;
			instruction.bytecode[0] = dst.HEX;
			instruction.bytecode[1] = src0.HEX;

			Register src_reg{ .id = static_cast<int>(src_reg_id), .f16 = true };
			instruction.srcs.push_back({ .reg = src_reg, .mask = 0xF });
			instruction.dsts.push_back({ .reg{.id = reg.reg.id, .f16 = false }, .mask = dst.write_mask });
			result.push_back(instruction);
		}

		return result;
	}

	void insert_dependency_barriers(BasicBlock* block)
	{
		std::array<char, register_file_length> register_file;
		std::memset(register_file.data(), content_unknown, register_file_length);

		std::unordered_set<u32> barrier16;
		std::unordered_set<u32> barrier32;

		// This subpass does not care about the prologue and epilogue and assumes each block is unique.
		for (auto it = block->instructions.begin(); it != block->instructions.end();)
		{
			auto& inst = *it;

			barrier16.clear();
			barrier32.clear();

			for (const auto& src : inst.srcs)
			{
				const auto read_bytes = get_register_file_range(src);
				const char expected_type = src.reg.f16 ? content_float16 : content_float32;
				for (const auto& index : read_bytes)
				{
					if (register_file[index] == content_unknown)
					{
						// Skip input
						continue;
					}

					if (register_file[index] == expected_type || register_file[index] == content_dual)
					{
						// Match - nothing to do
						continue;
					}

					// Collision on the lane
					register_file[index] = content_dual;
					(src.reg.f16 ? barrier16 : barrier32).insert(index);
				}
			}

			for (const auto& dst : inst.dsts)
			{
				const auto write_bytes = get_register_file_range(dst);
				const char expected_type = dst.reg.f16 ? content_float16 : content_float32;

				for (const auto& index : write_bytes)
				{
					register_file[index] = expected_type;
				}
			}

			if (barrier16.empty() && barrier32.empty())
			{
				++it;
				continue;
			}

			// We need to inject some barrier instructions
			if (!barrier16.empty())
			{
				auto barrier16_in = decode_lanes16(barrier16);
				for (const auto& reg : barrier16_in)
				{
					auto instructions = build_barrier16(reg);
					it = block->instructions.insert(it, instructions.begin(), instructions.end());
					std::advance(it, instructions.size() + 1);
				}
			}

			if (!barrier32.empty())
			{
				auto barrier32_in = decode_lanes32(barrier32);
				for (const auto& reg : barrier32_in)
				{
					auto instructions = build_barrier32(reg);
					it = block->instructions.insert(it, instructions.begin(), instructions.end());
					std::advance(it, instructions.size() + 1);
				}
			}
		}
	}

	void RegisterDependencyPass::run(FlowGraph& graph)
	{
		// First, run intra-block dependency
		for (auto& block : graph.blocks)
		{
			insert_dependency_barriers(&block);
		}

		// TODO: Create prologue/epilogue instructions
	}
}
