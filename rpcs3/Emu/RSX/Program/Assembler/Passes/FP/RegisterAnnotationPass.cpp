#include "stdafx.h"

#include "RegisterAnnotationPass.h"
#include "Emu/RSX/Program/Assembler/FPOpcodes.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <span>
#include <unordered_map>

namespace rsx::assembler::FP
{
	using namespace constants;

	bool is_delay_slot(const Instruction& instruction)
	{
		const OPDEST dst{ .HEX = instruction.bytecode[0] };
		const SRC0 src0{ .HEX = instruction.bytecode[1] };
		const SRC1 src1{ .HEX = instruction.bytecode[2] };

		if (dst.opcode != RSX_FP_OPCODE_MOV ||            // These slots are always populated with MOV
			dst.no_dest ||                                // Must have a sink
			src0.reg_type != RSX_FP_REGISTER_TYPE_TEMP || // Must read from reg
			dst.dest_reg != src0.tmp_reg_index ||         // Must be a write-to-self
			dst.fp16 != src0.fp16 ||                      // Must really be the same register
			src0.abs || src0.neg ||
			dst.saturate)                                 // Precision modifier
		{
			return false;
		}

		switch (dst.prec)
		{
		case RSX_FP_PRECISION_REAL:
		case RSX_FP_PRECISION_UNKNOWN:
			break;
		case RSX_FP_PRECISION_HALF:
			if (!src0.fp16) return false;
			break;
		case RSX_FP_PRECISION_FIXED12:
		case RSX_FP_PRECISION_FIXED9:
		case RSX_FP_PRECISION_SATURATE:
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

	std::vector<RegisterRef> compile_register_file(const register_file_t& file)
	{
		std::vector<RegisterRef> results;

		// F16 register processing
		for (int reg16 = 0; reg16 < 48; ++reg16)
		{
			const u32 offset = reg16 * 8;
			auto word = *reinterpret_cast<const u64*>(&file[offset]);

			if (!word) [[ likely ]]
			{
				// Trivial rejection, very commonly hit.
				continue;
			}

			RegisterRef ref{ .reg {.id = reg16, .f16 = true } };
			ref.x = (file[offset] == content_dual || file[offset] == content_float16);
			ref.y = (file[offset + 2] == content_dual || file[offset + 2] == content_float16);
			ref.z = (file[offset + 4] == content_dual || file[offset + 4] == content_float16);
			ref.w = (file[offset + 6] == content_dual || file[offset + 6] == content_float16);

			if (ref)
			{
				results.push_back(std::move(ref));
			}
		}

		// Helper to check a span for 32-bit access
		auto match_any_32 = [](const std::span<const char> lanes)
		{
			return std::any_of(lanes.begin(), lanes.end(), FN(x == content_dual || x == content_float32));
		};

		// F32 register processing
		for (int reg32 = 0; reg32 < 24; ++reg32)
		{
			const u32 offset = reg32 * 16;
			auto word0 = *reinterpret_cast<const u64*>(&file[offset]);
			auto word1 = *reinterpret_cast<const u64*>(&file[offset + 8]);

			if (!word0 && !word1) [[ likely ]]
			{
				// Trivial rejection, very commonly hit.
				continue;
			}

			RegisterRef ref{ .reg {.id = reg32, .f16 = false } };
			if (word0)
			{
				ref.x = match_any_32({ &file[offset], 4 });
				ref.y = match_any_32({ &file[offset + 4], 4 });
			}

			if (word1)
			{
				ref.z = match_any_32({ &file[offset + 8], 4 });
				ref.w = match_any_32({ &file[offset + 12], 4 });
			}

			if (ref)
			{
				results.push_back(std::move(ref));
			}
		}

		return results;
	}

	// Decay instructions into register references
	void annotate_instructions(BasicBlock* block, const RSXFragmentProgram& prog, bool skip_delay_slots)
	{
		for (auto& instruction : block->instructions)
		{
			if (skip_delay_slots && is_delay_slot(instruction))
			{
				continue;
			}

			const u32 operand_count = get_operand_count(static_cast<FP_opcode>(instruction.opcode));
			for (u32 i = 0; i < operand_count; i++)
			{
				RegisterRef reg = get_src_register(prog, &instruction, i);
				if (!reg.mask)
				{
					// Likely a literal constant
					continue;
				}

				instruction.srcs.push_back(std::move(reg));
			}

			RegisterRef dst = get_dst_register(&instruction);
			if (dst)
			{
				instruction.dsts.push_back(std::move(dst));
			}
		}
	}

	// Annotate each block with input and output lanes (read and clobber list)
	void annotate_block_io(BasicBlock* block)
	{
		alignas(16) register_file_t output_register_file;
		alignas(16) register_file_t input_register_file;      // We'll eventually replace with a bitfield mask, but for ease of debugging, we use char for now

		std::memset(output_register_file.data(), content_unknown, register_file_max_len);
		std::memset(input_register_file.data(), content_unknown, register_file_max_len);

		for (const auto& instruction : block->instructions)
		{
			for (const auto& src : instruction.srcs)
			{
				const auto read_bytes = get_register_file_range(src);
				const char expected_type = src.reg.f16 ? content_float16 : content_float32;
				for (const auto& index : read_bytes)
				{
					if (output_register_file[index] != content_unknown)
					{
						// Something already wrote to this lane
						continue;
					}

					if (input_register_file[index] == expected_type)
					{
						// We already know about this input
						continue;
					}

					if (input_register_file[index] == 0)
					{
						// Not known, tag as input
						input_register_file[index] = expected_type;
						continue;
					}

					// Collision on the lane
					input_register_file[index] = content_dual;
				}
			}

			if (!instruction.dsts.empty())
			{
				const auto& dst = instruction.dsts.front();
				const auto write_bytes = get_register_file_range(dst);
				const char expected_type = dst.reg.f16 ? content_float16 : content_float32;

				for (const auto& index : write_bytes)
				{
					output_register_file[index] = expected_type;
				}
			}
		}

		// Compile the input and output refs into register references
		block->clobber_list = compile_register_file(output_register_file);
		block->input_list = compile_register_file(input_register_file);
	}

	void RegisterAnnotationPass::run(FlowGraph& graph)
	{
		for (auto& block : graph.blocks)
		{
			annotate_instructions(&block, m_prog, m_config.skip_delay_slots);
			annotate_block_io(&block);
		}
	}
}
