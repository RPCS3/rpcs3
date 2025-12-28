#include "stdafx.h"

#include "RegisterDependencyPass.h"
#include "Emu/RSX/Program/Assembler/FPOpcodes.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <unordered_map>
#include <unordered_set>

namespace rsx::assembler::FP
{
	using namespace constants;

	struct DependencyPassContext
	{
		std::unordered_map<BasicBlock*, register_file_t> exec_register_map;
		std::unordered_map<BasicBlock*, register_file_t> sync_register_map;
	};

	enum Register32BarrierFlags
	{
		NONE = 0,
		OR_WORD0 = 1,
		OR_WORD1 = 2,
		DEFAULT = OR_WORD0 | OR_WORD1
	};

	struct RegisterBarrier32
	{
		RegisterRef ref;
		u32 flags[4];
	};

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
			result.push_back(std::move(ref));
		}
		return result;
	}

	std::vector<RegisterBarrier32> decode_lanes32(const std::unordered_set<u32>& lanes)
	{
		std::vector<RegisterBarrier32> result;

		for (u32 index = 0, file_offset = 0; index < 48; ++index, file_offset += 16)
		{
			// Each register has 8 16-bit lanes
			RegisterBarrier32 barrier{};
			auto& ref = barrier.ref;

			for (u32 lane = 0; lane < 16; lane += 2)
			{
				if (!lanes.contains(file_offset + lane))
				{
					continue;
				}

				const u32 ch = (lane / 4);
				const u32 flags = (lane & 3)
					? Register32BarrierFlags::OR_WORD1
					: Register32BarrierFlags::OR_WORD0;

				ref.mask |= (1u << ch);
				barrier.flags[ch] |= flags;
			}

			if (ref.mask == 0)
			{
				continue;
			}

			ref.reg = {.id = static_cast<int>(index), .f16 = false };
			result.push_back(std::move(barrier));
		}

		return result;
	}

	std::vector<Instruction> build_barrier32(const RegisterBarrier32& barrier)
	{
		// Upto 4 instructions are needed per 32-bit register
		// R0.x = packHalf2x16(H0.xy)
		// R0.y = packHalf2x16(H0.zw);
		// R0.z = packHalf2x16(H1.xy);
		// R0.w = packHalf2x16(H1.zw);

		std::vector<Instruction> result;

		for (u32 mask = barrier.ref.mask, ch = 0; mask > 0; mask >>= 1, ++ch)
		{
			if (!(mask & 1))
			{
				continue;
			}

			const auto& reg = barrier.ref.reg;
			const auto reg_id = reg.id;

			Instruction instruction{};
			OPDEST dst{};
			dst.prec = RSX_FP_PRECISION_REAL;
			dst.fp16 = 0;
			dst.dest_reg = reg_id;
			dst.write_mask = (1u << ch);

			const u32 src_reg_id = (ch / 2) + (reg_id * 2);
			const bool is_word0 = !(ch & 1); // Only even

			SRC0 src0{};
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
			src0.fp16 = 1;

			// Prepare source 1 to match the output in case we need to encode an OR
			SRC1 src1{};
			src1.reg_type = RSX_FP_REGISTER_TYPE_TEMP;
			src1.tmp_reg_index = reg_id;
			src1.swizzle_x = ch;
			src1.swizzle_y = ch;
			src1.swizzle_z = ch;
			src1.swizzle_w = ch;

			u32 opcode = 0;
			switch (barrier.flags[ch])
			{
			case Register32BarrierFlags::DEFAULT:
				opcode = RSX_FP_OPCODE_PK2;
				break;
			case Register32BarrierFlags::OR_WORD0:
				opcode = RSX_FP_OPCODE_OR16_LO;
				// Swap inputs
				std::swap(src0.HEX, src1.HEX);
				break;
			case Register32BarrierFlags::OR_WORD1:
				opcode = RSX_FP_OPCODE_OR16_HI;
				src0.swizzle_x = src0.swizzle_y;
				std::swap(src0.HEX, src1.HEX);
				break;
			case Register32BarrierFlags::NONE:
			default:
				fmt::throw_exception("Unexpected lane barrier with no mask.");
			}

			dst.opcode = opcode & 0x3F;
			src1.opcode_hi = (opcode > 0x3F) ? 1 : 0;
			src0.exec_if_eq = src0.exec_if_gr = src0.exec_if_lt = 1;

			instruction.opcode = opcode;
			instruction.bytecode[0] = dst.HEX;
			instruction.bytecode[1] = src0.HEX;
			instruction.bytecode[2] = src1.HEX;

			Register src_reg{ .id = static_cast<int>(src_reg_id), .f16 = true };
			instruction.srcs.push_back({ .reg = src_reg, .mask = 0xF });
			instruction.dsts.push_back({ .reg{ .id = reg_id, .f16 = false }, .mask = (1u << ch) });
			result.push_back(std::move(instruction));
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
			const bool is_odd_ch = !!(ch & 1);
			const bool is_word0 = ch < 2;

			// If we're an even channel, we should also write the next channel (y/w)
			if (!is_odd_ch && (mask & 2))
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
			result.push_back(std::move(instruction));
		}

		return result;
	}

	std::vector<Instruction> resolve_dependencies(const std::unordered_set<u32>& lanes, bool f16)
	{
		std::vector<Instruction> result;

		if (f16)
		{
			const auto regs = decode_lanes16(lanes);
			for (const auto& ref : regs)
			{
				auto instructions = build_barrier16(ref);
				result.insert(result.end(), instructions.begin(), instructions.end());
			}

			return result;
		}

		const auto barriers = decode_lanes32(lanes);
		for (const auto& barrier : barriers)
		{
			auto instructions = build_barrier32(barrier);
			result.insert(result.end(), std::make_move_iterator(instructions.begin()), std::make_move_iterator(instructions.end()));
		}

		return result;
	}

	void insert_dependency_barriers(DependencyPassContext& ctx, BasicBlock* block)
	{
		register_file_t& register_file = ctx.exec_register_map[block];
		std::memset(register_file.data(), content_unknown, register_file_max_len);

		std::unordered_set<u32> barrier16;
		std::unordered_set<u32> barrier32;

		// This subpass does not care about the prologue and epilogue and assumes each block is unique.
		for (auto it = block->instructions.begin(); it != block->instructions.end(); ++it)
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

			// We need to inject some barrier instructions
			if (!barrier16.empty())
			{
				auto barrier16_in = decode_lanes16(barrier16);
				std::vector<Instruction> instructions;
				instructions.reserve(barrier16_in.size());

				for (const auto& reg : barrier16_in)
				{
					auto barrier = build_barrier16(reg);
					instructions.insert(instructions.end(), std::make_move_iterator(barrier.begin()), std::make_move_iterator(barrier.end()));
				}

				it = block->instructions.insert(it, std::make_move_iterator(instructions.begin()), std::make_move_iterator(instructions.end()));
				std::advance(it, instructions.size());
			}

			if (!barrier32.empty())
			{
				auto barrier32_in = decode_lanes32(barrier32);
				std::vector<Instruction> instructions;
				instructions.reserve(barrier32_in.size());

				for (const auto& reg : barrier32_in)
				{
					auto barrier = build_barrier32(reg);
					instructions.insert(instructions.end(), std::make_move_iterator(barrier.begin()), std::make_move_iterator(barrier.end()));
				}

				it = block->instructions.insert(it, std::make_move_iterator(instructions.begin()), std::make_move_iterator(instructions.end()));
				std::advance(it, instructions.size());
			}
		}
	}

	void insert_block_register_dependency(DependencyPassContext& ctx, BasicBlock* block, const std::unordered_set<u32>& lanes, bool f16)
	{
		std::unordered_set<u32> clobbered_lanes;
		std::unordered_set<u32> lanes_to_search;

		for (auto& back_edge : block->pred)
		{
			auto target = back_edge.from;

			// Quick check - if we've reached an IF-ELSE anchor, don't traverse upwards.
			// The IF and ELSE edges are already a complete set and will bre processed before this node.
			if (back_edge.type == EdgeType::ENDIF &&
				&back_edge == &block->pred.back() &&
				target->succ.size() == 3 &&
				target->succ[1].type == EdgeType::ELSE &&
				target->succ[2].type == EdgeType::ENDIF &&
				target->succ[2].to == block)
			{
				return;
			}

			// Did this target even clobber our register?
			ensure(ctx.exec_register_map.find(target) != ctx.exec_register_map.end(), "Block has not been pre-processed");

			if (ctx.sync_register_map.find(target) == ctx.sync_register_map.end())
			{
				auto& blob = ctx.sync_register_map[target];
				std::memset(blob.data(), content_unknown, register_file_max_len);
			}

			auto& sync_register_file = ctx.sync_register_map[target];
			const auto& exec_register_file = ctx.exec_register_map[target];
			const auto clobber_type = f16 ? content_float32 : content_float16;

			lanes_to_search.clear();
			clobbered_lanes.clear();

			for (auto& lane : lanes)
			{
				if (exec_register_file[lane] == clobber_type &&
					sync_register_file[lane] == content_unknown)
				{
					clobbered_lanes.insert(lane);
					sync_register_file[lane] = content_dual;
					continue;
				}

				if (exec_register_file[lane] == content_unknown)
				{
					lanes_to_search.insert(lane);
				}
			}

			if (!clobbered_lanes.empty())
			{
				auto instructions = resolve_dependencies(clobbered_lanes, f16);
				target->epilogue.insert(target->epilogue.end(), std::make_move_iterator(instructions.begin()), std::make_move_iterator(instructions.end()));
			}

			if (lanes_to_search.empty())
			{
				continue;
			}

			// We have some missing lanes. Search upwards
			if (!target->pred.empty())
			{
				// We only need to search the last predecessor which is the true "root" of the branch
				insert_block_register_dependency(ctx, target, lanes_to_search, f16);
			}
		}
	}

	void insert_block_dependencies(DependencyPassContext& ctx, BasicBlock* block)
	{
		auto range_from_ref = [](const RegisterRef& ref)
		{
			const auto range = get_register_file_range(ref);

			std::unordered_set<u32> result;
			for (const auto& value : range)
			{
				result.insert(value);
			}
			return result;
		};

		for (auto& ref : block->input_list)
		{
			const auto range = range_from_ref(ref);
			insert_block_register_dependency(ctx, block, range, ref.reg.f16);
		}
	}

	void RegisterDependencyPass::run(FlowGraph& graph)
	{
		DependencyPassContext ctx{};

		// First, run intra-block dependency
		for (auto& block : graph.blocks)
		{
			insert_dependency_barriers(ctx, &block);
		}

		// Then, create prologue/epilogue instructions
		// Traverse the list in reverse order to bubble up dependencies correctly.
		for (auto it = graph.blocks.rbegin(); it != graph.blocks.rend(); ++it)
		{
			insert_block_dependencies(ctx, &(*it));
		}
	}
}
