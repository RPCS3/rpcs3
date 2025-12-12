#include "stdafx.h"
#include "CFG.h"

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <util/asm.hpp>
#include <util/v128.hpp>
#include <span>

#if defined(ARCH_ARM64)
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#undef FORCE_INLINE
#include "Emu/CPU/sse2neon.h"
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
#endif

namespace rsx::assembler
{
	inline v128 decode_instruction(const v128& raw_inst)
	{
		// Fixup of RSX's weird half-word shuffle for FP instructions
		// Convert input stream into LE u16 array
		__m128i _mask0 = _mm_set1_epi32(0xff00ff00);
		__m128i _mask1 = _mm_set1_epi32(0x00ff00ff);
		__m128i a = _mm_slli_epi32(static_cast<__m128i>(raw_inst), 8);
		__m128i b = _mm_srli_epi32(static_cast<__m128i>(raw_inst), 8);
		__m128i ret = _mm_or_si128(
			_mm_and_si128(_mask0, a),
			_mm_and_si128(_mask1, b)
		);
		return v128::loadu(&ret);
	}

	FlowGraph deconstruct_fragment_program(const RSXFragmentProgram& prog)
	{
		// For a flowgraph, we don't care at all about the actual contents, just flow control instructions.
		OPDEST dst{};
		SRC0 src0{};
		SRC1 src1{};
		SRC2 src2{};

		u32 pc = 0; // Program counter
		bool end = false;

		// Flow control data
		rsx::simple_array<BasicBlock*> end_blocks;
		rsx::simple_array<BasicBlock*> else_blocks;

		// Data block
		u32* data = static_cast<u32*>(prog.get_data());

		// Output
		FlowGraph graph{};
		BasicBlock* bb = graph.push();

		auto find_block_for_pc = [&](u32 id) -> BasicBlock*
		{
			auto found = std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == id));
			if (found != graph.blocks.end())
			{
				return &(*found);
			}
			return nullptr;
		};

		auto safe_insert_block = [&](BasicBlock* parent, u32 id, EdgeType edge_type) -> BasicBlock*
		{
			if (auto found = find_block_for_pc(id))
			{
				auto succ = found;
				if (found->is_of_type(EdgeType::ELSE) &&
					(edge_type == EdgeType::ENDIF || edge_type == EdgeType::ENDLOOP))
				{
					// If we landed on an "ELSE" node, link to its "ENDIF" counterpart
					auto if_parent = found->pred.front().from;
					auto endif_edge = std::find_if(if_parent->succ.begin(), if_parent->succ.end(), FN(x.type == EdgeType::ENDIF));
					ensure(endif_edge != if_parent->succ.end(), "CFG: Invalid ELSE node");
					succ = endif_edge->to;
				}

				parent->insert_succ(succ, edge_type);
				succ->insert_pred(parent, edge_type);
				return found;
			}

			return graph.push(parent, id, edge_type);
		};

		auto includes_literal_constant = [&]()
		{
			return src0.reg_type == RSX_FP_REGISTER_TYPE_CONSTANT ||
				src1.reg_type == RSX_FP_REGISTER_TYPE_CONSTANT ||
				src2.reg_type == RSX_FP_REGISTER_TYPE_CONSTANT;
		};

		while (!end)
		{
			BasicBlock** found = end_blocks.find_if(FN(x->id == pc));

			if (!found)
			{
				found = else_blocks.find_if(FN(x->id == pc));
			}

			if (found)
			{
				auto front_edge = std::find_if(bb->pred.begin(), bb->pred.end(), FN(x.type != EdgeType::ENDIF && x.type != EdgeType::ENDLOOP));
				if (front_edge != bb->pred.end())
				{
					auto parent = ensure(front_edge->from);
					switch (front_edge->type)
					{
					case EdgeType::IF:
					case EdgeType::ELSE:
					{
						// Find the merge node from the parent.
						auto succ = std::find_if(parent->succ.begin(), parent->succ.end(), FN(x.type == EdgeType::ENDIF));
						ensure(succ != parent->succ.end(), "CFG: Broken IF linkage. Please report to developers.");
						bb->insert_succ(succ->to, EdgeType::ENDIF);
						succ->to->insert_pred(bb, EdgeType::ENDIF);
						break;
					}
					case EdgeType::LOOP:
					{
						// Find the merge node from the parent
						auto succ = std::find_if(parent->succ.begin(), parent->succ.end(), FN(x.type == EdgeType::ENDLOOP));
						ensure(succ != parent->succ.end(), "CFG: Broken LOOP linkage. Please report to developers.");
						bb->insert_succ(succ->to, EdgeType::ENDLOOP);
						succ->to->insert_pred(bb, EdgeType::ENDLOOP);
						break;
					}
					default:
						// Missing an edge type?
						rsx_log.error("CFG: Unexpected block exit. Report to developers.");
						break;
					}
				}
				else if (bb->pred.empty())
				{
					// Impossible situation.
					rsx_log.error("CFG: Child block has no parent but has successor! Report to developers.");
				}

				bb = *found;
			}

			const v128 raw_inst = v128::loadu(data, pc);
			v128 decoded = decode_instruction(raw_inst);

			dst.HEX = decoded._u32[0];
			src0.HEX = decoded._u32[1];
			src1.HEX = decoded._u32[2];
			src2.HEX = decoded._u32[3];

			end = !!dst.end;
			const u32 opcode = dst.opcode | (src1.opcode_hi << 6);

			if (opcode == RSX_FP_OPCODE_NOP)
			{
				pc++;
				continue;
			}

			bb->instructions.push_back({});
			auto& ir_inst = bb->instructions.back();
			std::memcpy(ir_inst.bytecode, &decoded._u32[0], 16);
			ir_inst.length = 4;
			ir_inst.addr = pc * 16;
			ir_inst.opcode = opcode;

			switch (opcode)
			{
			case RSX_FP_OPCODE_BRK:
				break;
			case RSX_FP_OPCODE_CAL:
				// Unimplemented. Also unused by the RSX compiler
				fmt::throw_exception("Unimplemented FP CAL instruction.");
				break;
			case RSX_FP_OPCODE_FENCT:
				break;
			case RSX_FP_OPCODE_FENCB:
				break;
			case RSX_FP_OPCODE_RET:
				// Outside a subroutine, this doesn't mean much. The main block can conditionally return to stop execution early.
				// This will not alter flow control.
				break;
			case RSX_FP_OPCODE_IFE:
			{
				// Inserts if and else and end blocks
				const u32 end_addr = src2.end_offset >> 2u;
				const u32 else_addr = src1.else_offset >> 2u;
				if (end_addr == pc + 1u)
				{
					// NOP. Empty IF block
					bb->instructions.pop_back();
					break;
				}

				if (else_addr > end_addr)
				{
					// Our systems support this, but it is not verified on real hardware.
					rsx_log.error("CFG: Non-contiguous branch detected. Report to developers.");
				}

				auto parent = bb;
				bb = safe_insert_block(parent, pc + 1u, EdgeType::IF);
				if (end_addr != else_addr)
				{
					else_blocks.push_back(safe_insert_block(parent, else_addr, EdgeType::ELSE));
				}
				end_blocks.push_back(safe_insert_block(parent, end_addr, EdgeType::ENDIF));
				break;
			}
			case RSX_FP_OPCODE_LOOP:
			case RSX_FP_OPCODE_REP:
			{
				// Inserts for and end blocks
				const u32 end_addr = src2.end_offset >> 2u;
				if (end_addr == pc + 1u)
				{
					// NOP. Empty LOOP block
					bb->instructions.pop_back();
					break;
				}

				auto parent = bb;
				bb = safe_insert_block(parent, pc + 1u, EdgeType::LOOP);
				end_blocks.push_back(safe_insert_block(parent, end_addr, EdgeType::ENDLOOP));
				break;
			}
			default:
				if (includes_literal_constant())
				{
					const v128 constant_literal = v128::loadu(data, pc);
					v128 decoded_literal = decode_instruction(constant_literal);

					std::memcpy(ir_inst.bytecode + 4, &decoded_literal._u32[0], 16);
					ir_inst.length += 4;
					pc++;
				}
				break;
			}

			pc++;
		}

		// Sort edges for each block by distance
		for (auto& block : graph.blocks)
		{
			std::sort(block.pred.begin(), block.pred.end(), FN(x.from->id > y.from->id));
			std::sort(block.succ.begin(), block.succ.end(), FN(x.to->id < y.to->id));
		}

		// Sort block nodes by distance
		graph.blocks.sort(FN(x.id < y.id));
		return graph;
	}
}
