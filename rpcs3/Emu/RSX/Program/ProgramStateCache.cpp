#include "stdafx.h"
#include "ProgramStateCache.h"
#include "Emu/system_config.h"
#include "Emu/RSX/Core/RSXDriverState.h"
#include "util/sysinfo.hpp"

#include <stack>

#if defined(ARCH_X64)
#include "emmintrin.h"
#include "immintrin.h"
#endif

#ifdef ARCH_ARM64
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "Emu/CPU/sse2neon.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif

#ifdef _MSC_VER
#define AVX512_ICL_FUNC
#else
#define AVX512_ICL_FUNC __attribute__((__target__("avx512f,avx512bw,avx512dq,avx512cd,avx512vl,avx512bitalg,avx512ifma,avx512vbmi,avx512vbmi2,avx512vnni,avx512vpopcntdq")))
#endif


using namespace program_hash_util;

#ifdef ARCH_X64
AVX512_ICL_FUNC usz get_vertex_program_ucode_hash_512(const RSXVertexProgram &program)
{
	// Load all elements of the instruction_mask bitset
	const __m512i* instMask512 = utils::bless<const __m512i>(&program.instruction_mask);
	const __m128i* instMask128 = utils::bless<const __m128i>(&program.instruction_mask);

	const __m512i lowerMask = _mm512_loadu_si512(instMask512);
	const __m128i upper128 = _mm_loadu_si128(instMask128 + 4);
	const __m512i upperMask = _mm512_zextsi128_si512(upper128);
	
	__m512i maskIndex = _mm512_setzero_si512();
	const __m512i negativeOnes = _mm512_set1_epi64(-1);

	// Special masks to test against bitset 
	const __m512i testMask0 = _mm512_set_epi64(
	0x0808080808080808,
	0x0808080808080808,
	0x0404040404040404,
	0x0404040404040404,
	0x0202020202020202,
	0x0202020202020202,
	0x0101010101010101,
	0x0101010101010101);

	const __m512i testMask1 = _mm512_set_epi64(
	0x8080808080808080,
	0x8080808080808080,
	0x4040404040404040,
	0x4040404040404040,
	0x2020202020202020,
	0x2020202020202020,
	0x1010101010101010,
	0x1010101010101010);

	const __m512i* instBuffer = reinterpret_cast<const __m512i*>(program.data.data());
	__m512i acc0 = _mm512_setzero_si512();
	__m512i acc1 = _mm512_setzero_si512();

	__m512i rotMask0 = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
	__m512i rotMask1 = _mm512_set_epi64(15, 14, 13, 12, 11, 10, 9, 8);
	const __m512i rotMaskAdd = _mm512_set_epi64(16, 16, 16, 16, 16, 16, 16, 16);

	u32 instIndex = 0;

	// If there is remainder, add an extra (masked) iteration
	const u32 extraIteration = (program.data.size() % 32 != 0) ? 1 : 0;
	const u32 length = static_cast<u32>(program.data.size() / 32) + extraIteration;

	// The instruction mask will prevent us from reading out of bounds, we do not need a seperate masked loop
	// for the remainder, or a scalar loop.
	while (instIndex < (length))
	{
		const __m512i masks = _mm512_permutex2var_epi8(lowerMask, maskIndex, upperMask);
		const __mmask8 result0 = _mm512_test_epi64_mask(masks, testMask0);
		const __mmask8 result1 = _mm512_test_epi64_mask(masks, testMask1);
		const __m512i load0 = _mm512_maskz_loadu_epi64(result0, (instBuffer + instIndex * 2));
		const __m512i load1 = _mm512_maskz_loadu_epi64(result1, (instBuffer + (instIndex * 2)+ 1));

		const __m512i rotated0 = _mm512_rorv_epi64(load0, rotMask0);
		const __m512i rotated1 = _mm512_rorv_epi64(load1, rotMask1);

		acc0 = _mm512_add_epi64(acc0, rotated0);
		acc1 = _mm512_add_epi64(acc1, rotated1);

		rotMask0 = _mm512_add_epi64(rotMask0, rotMaskAdd);
		rotMask1 = _mm512_add_epi64(rotMask1, rotMaskAdd);
		maskIndex = _mm512_sub_epi8(maskIndex, negativeOnes);

		instIndex++;
	}

	const __m512i result = _mm512_add_epi64(acc0, acc1);
	return _mm512_reduce_add_epi64(result);
}
#endif

usz vertex_program_utils::get_vertex_program_ucode_hash(const RSXVertexProgram &program)
{
 	// Checksum as hash with rotated data
 	const void* instbuffer = program.data.data();
 	u32 instIndex = 0;
 	usz acc0 = 0;
 	usz acc1 = 0;

 	do
 	{
 		if (program.instruction_mask[instIndex])
 		{
 			const auto inst = v128::loadu(instbuffer, instIndex);
 			const usz tmp0 = std::rotr(inst._u64[0], instIndex * 2);
 			acc0 += tmp0;
 			const usz tmp1 = std::rotr(inst._u64[1], (instIndex * 2) + 1);
 			acc1 += tmp1;
 		}

 		instIndex++;
 	} while (instIndex < (program.data.size() / 4));
	return acc0 + acc1;
 }

vertex_program_utils::vertex_program_metadata vertex_program_utils::analyse_vertex_program(const u32* data, u32 entry, RSXVertexProgram& dst_prog)
{
	vertex_program_utils::vertex_program_metadata result{};
	//u32 last_instruction_address = 0;
	//u32 first_instruction_address = entry;

	std::bitset<rsx::max_vertex_program_instructions> instructions_to_patch;
	std::pair<u32, u32> instruction_range{ umax, 0 };
	bool has_branch_instruction = false;
	std::stack<u32> call_stack;

	D3 d3{};
	D2 d2{};
	D1 d1{};
	D0 d0{};

	std::function<void(u32, bool)> walk_function = [&](u32 start, bool fast_exit)
	{
		u32 current_instruction = start;
		std::set<u32> conditional_targets;

		while (true)
		{
			ensure(current_instruction < rsx::max_vertex_program_instructions);

			if (result.instruction_mask[current_instruction])
			{
				if (!fast_exit)
				{
					// This can be harmless if a dangling RET was encountered before.
					// This can also be legal in case of BRB...BRI loops since BRIs are conditional. Might just be a loop with exit cond.
					rsx_log.warning("vp_analyser: Possible infinite loop detected");
				}

				// There is never any reason to continue scanning after self-intersecting on the control-flow tree.
				break;
			}

			const auto instruction = v128::loadu(&data[current_instruction * 4]);
			d1.HEX = instruction._u32[1];
			d2.HEX = instruction._u32[2];
			d3.HEX = instruction._u32[3];

			// Touch current instruction
			result.instruction_mask[current_instruction] = true;
			instruction_range.first = std::min(current_instruction, instruction_range.first);
			instruction_range.second = std::max(current_instruction, instruction_range.second);

			// Whether to check if the current instruction references an input stream
			auto input_attribute_ref = [&]()
			{
				if (!d1.input_src)
				{
					// It is possible to reference ATTR0, but this is mandatory anyway. No need to explicitly test for it
					return;
				}

				const auto ref_mask = (1u << d1.input_src);
				if ((result.referenced_inputs_mask & ref_mask) == 0)
				{
					// Type is encoded in the first 2 bits of each block
					const auto src0 = d2.src0l & 0x3;
					const auto src1 = d2.src1  & 0x3;
					const auto src2 = d3.src2l & 0x3;

					if ((src0 == RSX_VP_REGISTER_TYPE_INPUT) ||
						(src1 == RSX_VP_REGISTER_TYPE_INPUT) ||
						(src2 == RSX_VP_REGISTER_TYPE_INPUT))
					{
						result.referenced_inputs_mask |= ref_mask;
					}
				}
			};

			auto branch_to = [&](const u32 target)
			{
				input_attribute_ref();
				current_instruction = target;
			};

			// Basic vec op analysis, must be done before flow analysis
			switch (d1.vec_opcode)
			{
			case RSX_VEC_OPCODE_NOP:
			{
				break;
			}
			case RSX_VEC_OPCODE_TXL:
			{
				result.referenced_textures_mask |= (1 << d2.tex_num);
				break;
			}
			default:
			{
				input_attribute_ref();
				break;
			}
			}

			bool static_jump = false;
			bool function_call = true;

			switch (d1.sca_opcode)
			{
			case RSX_SCA_OPCODE_NOP:
			{
				break;
			}
			case RSX_SCA_OPCODE_BRI:
			{
				d0.HEX = instruction._u32[0];
				static_jump = (d0.cond == 0x7);
				[[fallthrough]];
			}
			case RSX_SCA_OPCODE_BRB:
			{
				function_call = false;
				[[fallthrough]];
			}
			case RSX_SCA_OPCODE_CAL:
			case RSX_SCA_OPCODE_CLI:
			case RSX_SCA_OPCODE_CLB:
			{
				// Need to patch the jump address to be consistent wherever the program is located
				instructions_to_patch[current_instruction] = true;
				has_branch_instruction = true;

				d0.HEX = instruction._u32[0];
				const u32 jump_address = (d0.iaddrh2 << 9) | (d2.iaddrh << 3) | d3.iaddrl;

				if (function_call)
				{
					call_stack.push(current_instruction + 1);
					branch_to(jump_address);
					continue;
				}
				else if (static_jump)
				{
					// NOTE: This will skip potential jump target blocks between current->target
					branch_to(jump_address);
					continue;
				}
				else
				{
					// Set possible end address and proceed as usual
					conditional_targets.emplace(jump_address);
					instruction_range.second = std::max(jump_address, instruction_range.second);
				}

				break;
			}
			case RSX_SCA_OPCODE_RET:
			{
				if (call_stack.empty())
				{
					rsx_log.error("vp_analyser: RET found outside subroutine call");
				}
				else
				{
					branch_to(call_stack.top());
					call_stack.pop();
					continue;
				}

				break;
			}
			default:
			{
				input_attribute_ref();
				break;
			}
			}

			// Check exit conditions...
			if (d3.end)
			{
				// We have seen an end of instructions marker.
				// Multiple exits may exist, usually skipped over by branching. Do not exit on end unless there is no branching.
				if (!has_branch_instruction || fast_exit || current_instruction >= instruction_range.second)
				{
					// Conditions:
					// 1. No branching so far. This will always be the exit.
					// 2. Fast exit flag is set. This happens when walking through subroutines.
					// 3. We've gone beyond the known instruction range. In this scenario, this is the furthest end marker seen so far. It has to be reached by some earlier branch.
					break;
				}
			}
			else if ((current_instruction + 1) == rsx::max_vertex_program_instructions)
			{
				// No more instructions to read.
				break;
			}

			current_instruction++;
		}

		for (const u32 target : conditional_targets)
		{
			if (!result.instruction_mask[target])
			{
				walk_function(target, true);
			}
		}
	};

	if (g_cfg.video.debug_program_analyser)
	{
		fs::file dump(fs::get_cache_dir() + "shaderlog/vp_analyser.bin", fs::rewrite);
		dump.write(&entry, 4);
		dump.write(data, rsx::max_vertex_program_instructions * 16);
		dump.close();
	}

	walk_function(entry, false);

	const u32 instruction_count = (instruction_range.second - instruction_range.first + 1);
	result.ucode_length = instruction_count * 16;

	dst_prog.base_address = instruction_range.first;
	dst_prog.entry = entry;
	dst_prog.data.resize(instruction_count * 4);
	dst_prog.instruction_mask = (result.instruction_mask >> instruction_range.first);

	if (!has_branch_instruction)
	{
		ensure(instruction_range.first == entry);
		std::memcpy(dst_prog.data.data(), data + (instruction_range.first * 4), result.ucode_length);
	}
	else
	{
		for (u32 i = instruction_range.first, count = 0; i <= instruction_range.second; ++i, ++count)
		{
			const u32* instruction = &data[i * 4];
			u32* dst = &dst_prog.data[count * 4];

			if (result.instruction_mask[i])
			{
				v128::storeu(v128::loadu(instruction), dst);

				if (instructions_to_patch[i])
				{
					d0.HEX = dst[0];
					d2.HEX = dst[2];
					d3.HEX = dst[3];

					u32 address = (d0.iaddrh2 << 9) | (d2.iaddrh << 3) | d3.iaddrl;
					address -= instruction_range.first;

					d0.iaddrh2 = (address >> 9) & 0x1;
					d2.iaddrh = (address >> 3) & 0x3F;
					d3.iaddrl = (address & 0x7);
					dst[0] = d0.HEX;
					dst[2] = d2.HEX;
					dst[3] = d3.HEX;

					dst_prog.jump_table.emplace(address);
				}
			}
			else
			{
				v128::storeu({}, dst);
			}
		}

		// Typical ubershaders have the dispatch at the top with subroutines following. However...
		// some games have the dispatch block at the end and the subroutines above them.
		// We need to simulate a jump-to-entry in this situation
		// Normally this condition is handled by the conditional_targets walk, but sometimes this doesn't work due to cyclic branches
		if (instruction_range.first < dst_prog.entry)
		{
			// Is there a subroutine that jumps into the entry? If not, add to jump table
			const auto target = dst_prog.entry - instruction_range.first;
			dst_prog.jump_table.insert(target);
		}

		// Verification
		for (const u32 target : dst_prog.jump_table)
		{
			if (!dst_prog.instruction_mask[target])
			{
				rsx_log.error("vp_analyser: Failed, branch target 0x%x was not resolved", target);
			}
		}
	}

	result.referenced_inputs_mask |= 1u; // VPOS is always enabled, else no rendering can happen
	return result;
}

usz vertex_program_storage_hash::operator()(const RSXVertexProgram &program) const
{
#ifdef ARCH_X64
	usz ucode_hash;

		if (utils::has_avx512_icl())
		{
			ucode_hash = get_vertex_program_ucode_hash_512(program);
		}
		else
		{
			ucode_hash = vertex_program_utils::get_vertex_program_ucode_hash(program);
		}
#else
	const usz ucode_hash = vertex_program_utils::get_vertex_program_ucode_hash(program);
#endif
	const u32 state_params[] =
	{
		program.ctrl,
		program.output_mask,
		program.texture_state.texture_dimensions,
		program.texture_state.multisampled_textures,
	};
	const usz metadata_hash = rpcs3::hash_array(state_params);
	return rpcs3::hash64(ucode_hash, metadata_hash);
}

#ifdef ARCH_X64
AVX512_ICL_FUNC bool vertex_program_compare_512(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2)
	{
		// Load all elements of the instruction_mask bitset
		const __m512i* instMask512 = utils::bless<const __m512i>(&binary1.instruction_mask);
		const __m128i* instMask128 = utils::bless<const __m128i>(&binary1.instruction_mask);

		const __m512i lowerMask = _mm512_loadu_si512(instMask512);
		const __m128i upper128 = _mm_loadu_si128(instMask128 + 4);
		const __m512i upperMask = _mm512_zextsi128_si512(upper128);
		
		__m512i maskIndex = _mm512_setzero_si512();
		const __m512i negativeOnes = _mm512_set1_epi64(-1);

		// Special masks to test against bitset 
		const __m512i testMask0 = _mm512_set_epi64(
		0x0808080808080808,
		0x0808080808080808,
		0x0404040404040404,
		0x0404040404040404,
		0x0202020202020202,
		0x0202020202020202,
		0x0101010101010101,
		0x0101010101010101);

		const __m512i testMask1 = _mm512_set_epi64(
		0x8080808080808080,
		0x8080808080808080,
		0x4040404040404040,
		0x4040404040404040,
		0x2020202020202020,
		0x2020202020202020,
		0x1010101010101010,
		0x1010101010101010);

		const __m512i* instBuffer1 = reinterpret_cast<const __m512i*>(binary1.data.data());
		const __m512i* instBuffer2 = reinterpret_cast<const __m512i*>(binary2.data.data());

		// If there is remainder, add an extra (masked) iteration
		const u32 extraIteration = (binary1.data.size() % 32 != 0) ? 1 : 0;
		const u32 length = static_cast<u32>(binary1.data.size() / 32) + extraIteration;

		u32 instIndex = 0;

		// The instruction mask will prevent us from reading out of bounds, we do not need a seperate masked loop
		// for the remainder, or a scalar loop.
		while (instIndex < (length))
		{
			const __m512i masks = _mm512_permutex2var_epi8(lowerMask, maskIndex, upperMask);

			const __mmask8 result0 = _mm512_test_epi64_mask(masks, testMask0);
			const __mmask8 result1 = _mm512_test_epi64_mask(masks, testMask1);

			const __m512i load0 = _mm512_maskz_loadu_epi64(result0, (instBuffer1 + (instIndex * 2)));
			const __m512i load1 = _mm512_maskz_loadu_epi64(result0, (instBuffer2 + (instIndex * 2)));
			const __m512i load2 = _mm512_maskz_loadu_epi64(result1, (instBuffer1 + (instIndex * 2) + 1));
			const __m512i load3 = _mm512_maskz_loadu_epi64(result1, (instBuffer2 + (instIndex * 2)+ 1));

			const __mmask8 res0 = _mm512_cmpneq_epi64_mask(load0, load1);
			const __mmask8 res1 = _mm512_cmpneq_epi64_mask(load2, load3);

			const u8 result = _kortestz_mask8_u8(res0, res1);

			//kortestz will set result to 1 if all bits are zero, so invert the check for result
			if (!result)
			{
				return false;
			}

			maskIndex = _mm512_sub_epi8(maskIndex, negativeOnes);

			instIndex++;
		}

		return true;
	}
#endif

bool vertex_program_compare::operator()(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2) const
{
	if (!compare_properties(binary1, binary2))
	{
		return false;
	}

	if (binary1.data.size() != binary2.data.size() ||
		binary1.jump_table != binary2.jump_table)
	{
		return false;
	}

#ifdef ARCH_X64
	if (utils::has_avx512_icl())
	{
		return vertex_program_compare_512(binary1, binary2);
	}
#endif

	const void* instBuffer1 = binary1.data.data();
	const void* instBuffer2 = binary2.data.data();
	usz instIndex = 0;
	while (instIndex < (binary1.data.size() / 4))
	{
		if (binary1.instruction_mask[instIndex])
		{
			const auto inst1 = v128::loadu(instBuffer1, instIndex);
			const auto inst2 = v128::loadu(instBuffer2, instIndex);
			if (inst1._u ^ inst2._u)
			{
				return false;
			}
		}

		instIndex++;
	}

	return true;
}

bool vertex_program_compare::compare_properties(const RSXVertexProgram& binary1, const RSXVertexProgram& binary2)
{
	return binary1.output_mask == binary2.output_mask &&
		binary1.ctrl == binary2.ctrl &&
		binary1.texture_state == binary2.texture_state;
}

bool fragment_program_utils::is_any_src_constant(v128 sourceOperand)
{
	const u64 masked = sourceOperand._u64[1] & 0x30000000300;
	return (sourceOperand._u32[1] & 0x300) == 0x200 || (static_cast<u32>(masked) == 0x200 || static_cast<u32>(masked >> 32) == 0x200);
}

usz fragment_program_utils::get_fragment_program_ucode_size(const void* ptr)
{
	const auto instBuffer = ptr;
	usz instIndex = 0;
	while (true)
	{
		const v128 inst = v128::loadu(instBuffer, instIndex);
		const bool end = (inst._u32[0] >> 8) & 0x1;

		if (is_any_src_constant(inst))
		{
			instIndex += 2;
			if (end)
				return instIndex * 4 * 4;
			continue;
		}
		instIndex++;
		if (end)
			return (instIndex)* 4 * 4;
	}
}

fragment_program_utils::fragment_program_metadata fragment_program_utils::analyse_fragment_program(const void* ptr)
{
	fragment_program_utils::fragment_program_metadata result{};
	result.program_start_offset = -1;
	const auto instBuffer = ptr;
	s32 index = 0;

	// Find the start of the program
	while (true)
	{
		const auto inst = v128::loadu(instBuffer, index);

		const u32 opcode = (inst._u32[0] >> 16) & 0x3F;
		if (opcode)
		{
			// We found the start of the program, don't advance the index
			result.program_start_offset = index * 16;
			break;
		}

		if ((inst._u32[0] >> 8) & 0x1)
		{
			result.program_start_offset = index * 16;
			result.program_ucode_length = 16;
			result.is_nop_shader = true;
			return result;
		}

		index++;
	}

	while (true)
	{
		const auto inst = v128::loadu(instBuffer, index);

		// Check for opcode high bit which indicates a branch instructions (opcode 0x40...0x45)
		if (inst._u32[2] & (1 << 23))
		{
			// NOTE: Jump instructions are not yet proved to work outside of loops and if/else blocks
			// Otherwise we would need to follow the execution chain
			result.has_branch_instructions = true;
		}
		else
		{
			const u32 opcode = (inst._u32[0] >> 16) & 0x3F;
			if (opcode)
			{
				switch (opcode)
				{
				case RSX_FP_OPCODE_TEX:
				case RSX_FP_OPCODE_TEXBEM:
				case RSX_FP_OPCODE_TXP:
				case RSX_FP_OPCODE_TXPBEM:
				case RSX_FP_OPCODE_TXD:
				case RSX_FP_OPCODE_TXB:
				case RSX_FP_OPCODE_TXL:
				{
					//Bits 17-20 of word 1, swapped within u16 sections
					//Bits 16-23 are swapped into the upper 8 bits (24-31)
					const u32 tex_num = (inst._u32[0] >> 25) & 15;
					result.referenced_textures_mask |= (1 << tex_num);
					break;
				}
				case RSX_FP_OPCODE_PK4:
				case RSX_FP_OPCODE_UP4:
				case RSX_FP_OPCODE_PK2:
				case RSX_FP_OPCODE_UP2:
				case RSX_FP_OPCODE_PKB:
				case RSX_FP_OPCODE_UPB:
				case RSX_FP_OPCODE_PK16:
				case RSX_FP_OPCODE_UP16:
				case RSX_FP_OPCODE_PKG:
				case RSX_FP_OPCODE_UPG:
				{
					result.has_pack_instructions = true;
					break;
				}
				}
			}

			if (is_any_src_constant(inst))
			{
				//Instruction references constant, skip one slot occupied by data
				index++;
				result.program_constants_buffer_length += 16;
			}
		}

		index++;

		if ((inst._u32[0] >> 8) & 0x1)
		{
			break;
		}
	}

	result.program_ucode_length = (index - (result.program_start_offset / 16)) * 16;
	return result;
}

usz fragment_program_utils::get_fragment_program_ucode_hash(const RSXFragmentProgram& program)
{
	// Checksum as hash with rotated data
	const void* instbuffer = program.get_data();
	usz acc0 = 0;
	usz acc1 = 0;
	for (int instIndex = 0; instIndex < static_cast<int>(program.ucode_length / 16); instIndex++)
	{
		const auto inst = v128::loadu(instbuffer, instIndex);
		const usz tmp0 = std::rotr(inst._u64[0], instIndex * 2);
		acc0 += tmp0;
		const usz tmp1 = std::rotr(inst._u64[1], (instIndex * 2) + 1);
		acc1 += tmp1;
		// Skip constants
		if (fragment_program_utils::is_any_src_constant(inst))
			instIndex++;

	}
	return acc0 + acc1;
}

usz fragment_program_storage_hash::operator()(const RSXFragmentProgram& program) const
{
	const usz ucode_hash = fragment_program_utils::get_fragment_program_ucode_hash(program);
	const u32 state_params[] =
	{
		program.ctrl,
		program.two_sided_lighting ? 1u : 0u,
		program.texture_state.texture_dimensions,
		program.texture_state.shadow_textures,
		program.texture_state.redirected_textures,
		program.texture_state.multisampled_textures,
		program.texcoord_control_mask,
		program.mrt_buffers_count
	};
	const usz metadata_hash = rpcs3::hash_array(state_params);
	return rpcs3::hash64(ucode_hash, metadata_hash);
}

bool fragment_program_compare::operator()(const RSXFragmentProgram& binary1, const RSXFragmentProgram& binary2) const
{
	if (!compare_properties(binary1, binary2))
	{
		return false;
	}

	const void* instBuffer1 = binary1.get_data();
	const void* instBuffer2 = binary2.get_data();
	for (usz instIndex = 0; instIndex < (binary1.ucode_length / 16); instIndex++)
	{
		const auto inst1 = v128::loadu(instBuffer1, instIndex);
		const auto inst2 = v128::loadu(instBuffer2, instIndex);

		if (inst1._u ^ inst2._u)
		{
			return false;
		}

		// Skip constants
		if (fragment_program_utils::is_any_src_constant(inst1))
			instIndex++;
	}
	
	return true;
}

bool fragment_program_compare::compare_properties(const RSXFragmentProgram& binary1, const RSXFragmentProgram& binary2)
{
	return binary1.ucode_length == binary2.ucode_length &&
		binary1.ctrl == binary2.ctrl &&
		binary1.texture_state == binary2.texture_state &&
		binary1.texcoord_control_mask == binary2.texcoord_control_mask &&
		binary1.two_sided_lighting == binary2.two_sided_lighting &&
		binary1.mrt_buffers_count == binary2.mrt_buffers_count;
}

namespace rsx
{
#if defined(ARCH_X64) || defined(ARCH_ARM64)
	static inline void write_fragment_constants_to_buffer_sse2(const std::span<f32>& buffer, const RSXFragmentProgram& rsx_prog, const std::vector<usz>& offsets_cache, bool sanitize)
	{
		f32* dst = buffer.data();
		for (usz offset_in_fragment_program : offsets_cache)
		{
			const char* data = static_cast<const char*>(rsx_prog.get_data()) + offset_in_fragment_program;

			const __m128i vector = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
			const __m128i shuffled_vector = _mm_or_si128(_mm_slli_epi16(vector, 8), _mm_srli_epi16(vector, 8));

			if (sanitize)
			{
				//Convert NaNs and Infs to 0
				const auto masked = _mm_and_si128(shuffled_vector, _mm_set1_epi32(0x7fffffff));
				const auto valid = _mm_cmplt_epi32(masked, _mm_set1_epi32(0x7f800000));
				const auto result = _mm_and_si128(shuffled_vector, valid);
				_mm_stream_si128(utils::bless<__m128i>(dst), result);
			}
			else
			{
				_mm_stream_si128(utils::bless<__m128i>(dst), shuffled_vector);
			}

			dst += 4;
		}
	}
#else
	static inline void write_fragment_constants_to_buffer_fallback(const std::span<f32>& buffer, const RSXFragmentProgram& rsx_prog, const std::vector<usz>& offsets_cache, bool sanitize)
	{
		f32* dst = buffer.data();

		for (usz offset_in_fragment_program : offsets_cache)
		{
			const char* data = static_cast<const char*>(rsx_prog.get_data()) + offset_in_fragment_program;

			for (u32 i = 0; i < 4; i++)
			{
				const u32 value = reinterpret_cast<const u32*>(data)[i];
				const u32 shuffled = ((value >> 8) & 0xff00ff) | ((value << 8) & 0xff00ff00);

				if (sanitize && (shuffled & 0x7fffffff) >= 0x7f800000)
				{
					dst[i] = 0.f;
				}
				else
				{
					dst[i] = std::bit_cast<f32>(shuffled);
				}
			}

			dst += 4;
		}
	}
#endif

	void write_fragment_constants_to_buffer(const std::span<f32>& buffer, const RSXFragmentProgram& rsx_prog, const std::vector<usz>& offsets_cache, bool sanitize)
	{
#if defined(ARCH_X64) || defined(ARCH_ARM64)
		write_fragment_constants_to_buffer_sse2(buffer, rsx_prog, offsets_cache, sanitize);
#else
		write_fragment_constants_to_buffer_fallback(buffer, rsx_prog, offsets_cache, sanitize);
#endif
	}

	void program_cache_hint_t::invalidate(u32 flags)
	{
		if (flags & rsx::vertex_program_dirty)
		{
			m_cached_vertex_program = nullptr;
		}

		if (flags & rsx::fragment_program_dirty)
		{
			m_cached_fragment_program = nullptr;
		}
	}

	void program_cache_hint_t::invalidate_vertex_program(const RSXVertexProgram& p)
	{
		if (!m_cached_vertex_program)
		{
			return;
		}

		if (!vertex_program_compare::compare_properties(m_cached_vp_properties, p))
		{
			m_cached_vertex_program = nullptr;
		}
	}

	void program_cache_hint_t::invalidate_fragment_program(const RSXFragmentProgram& p)
	{
		if (!m_cached_fragment_program)
		{
			return;
		}

		if (!fragment_program_compare::compare_properties(m_cached_fp_properties, p))
		{
			m_cached_fragment_program = nullptr;
		}
	}

	void program_cache_hint_t::cache_vertex_program(program_cache_hint_t* cache, const RSXVertexProgram& ref, void* vertex_program)
	{
		if (!cache)
		{
			return;
		}

		cache->m_cached_vertex_program = vertex_program;
		cache->m_cached_vp_properties.output_mask = ref.output_mask;
		cache->m_cached_vp_properties.ctrl = ref.ctrl;
		cache->m_cached_vp_properties.texture_state = ref.texture_state;
	}

	void program_cache_hint_t::cache_fragment_program(program_cache_hint_t* cache, const RSXFragmentProgram& ref, void* fragment_program)
	{
		if (!cache)
		{
			return;
		}

		cache->m_cached_fragment_program = fragment_program;
		cache->m_cached_fp_properties.ucode_length = ref.ucode_length;
		cache->m_cached_fp_properties.ctrl = ref.ctrl;
		cache->m_cached_fp_properties.texture_state = ref.texture_state;
		cache->m_cached_fp_properties.texcoord_control_mask = ref.texcoord_control_mask;
		cache->m_cached_fp_properties.two_sided_lighting = ref.two_sided_lighting;
		cache->m_cached_fp_properties.mrt_buffers_count = ref.mrt_buffers_count;
	}

}
