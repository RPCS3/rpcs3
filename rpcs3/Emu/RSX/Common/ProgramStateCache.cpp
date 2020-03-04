#include "stdafx.h"
#include "ProgramStateCache.h"
#include "Emu/system_config.h"

#include <stack>

using namespace program_hash_util;

size_t vertex_program_utils::get_vertex_program_ucode_hash(const RSXVertexProgram &program)
{
	// 64-bit Fowler/Noll/Vo FNV-1a hash code
	size_t hash = 0xCBF29CE484222325ULL;
	const qword* instbuffer = reinterpret_cast<const qword*>(program.data.data());
	size_t instIndex = 0;
	bool end = false;
	for (unsigned i = 0; i < program.data.size() / 4; i++)
	{
		if (program.instruction_mask[i])
		{
			const qword inst = instbuffer[instIndex];
			hash ^= inst.dword[0];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			hash ^= inst.dword[1];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
		}

		instIndex++;
	}
	return hash;
}

vertex_program_utils::vertex_program_metadata vertex_program_utils::analyse_vertex_program(const u32* data, u32 entry, RSXVertexProgram& dst_prog)
{
	vertex_program_utils::vertex_program_metadata result{};
	u32 last_instruction_address = 0;
	u32 first_instruction_address = entry;

	std::stack<u32> call_stack;
	std::pair<u32, u32> instruction_range = { UINT32_MAX, 0 };
	std::bitset<512> instructions_to_patch;
	bool has_branch_instruction = false;

	D3  d3;
	D2  d2;
	D1  d1;
	D0  d0;

	std::function<void(u32, bool)> walk_function = [&](u32 start, bool fast_exit)
	{
		u32 current_instrution = start;
		std::set<u32> conditional_targets;

		while (true)
		{
			verify(HERE), current_instrution < 512;

			if (result.instruction_mask[current_instrution])
			{
				if (!fast_exit)
				{
					// This can be harmless if a dangling RET was encountered before
					rsx_log.error("vp_analyser: Possible infinite loop detected");
					current_instrution++;
					continue;
				}
				else
				{
					// Block walk, looking for earliest exit
					break;
				}
			}

			const qword* instruction = reinterpret_cast<const qword*>(&data[current_instrution * 4]);
			d1.HEX = instruction->word[1];
			d3.HEX = instruction->word[3];

			// Touch current instruction
			result.instruction_mask[current_instrution] = true;
			instruction_range.first = std::min(current_instrution, instruction_range.first);
			instruction_range.second = std::max(current_instrution, instruction_range.second);

			// Basic vec op analysis, must be done before flow analysis
			switch (d1.vec_opcode)
			{
			case RSX_VEC_OPCODE_TXL:
			{
				d2.HEX = instruction->word[2];
				result.referenced_textures_mask |= (1 << d2.tex_num);
				break;
			}
			}

			bool static_jump = false;
			bool function_call = true;

			switch (d1.sca_opcode)
			{
			case RSX_SCA_OPCODE_BRI:
			{
				d0.HEX = instruction->word[0];
				static_jump = (d0.cond == 0x7);
				// Fall through
			}
			case RSX_SCA_OPCODE_BRB:
			{
				function_call = false;
				// Fall through
			}
			case RSX_SCA_OPCODE_CAL:
			case RSX_SCA_OPCODE_CLI:
			case RSX_SCA_OPCODE_CLB:
			{
				// Need to patch the jump address to be consistent wherever the program is located
				instructions_to_patch[current_instrution] = true;
				has_branch_instruction = true;

				d2.HEX = instruction->word[2];
				const u32 jump_address = ((d2.iaddrh << 3) | d3.iaddrl);

				if (function_call)
				{
					call_stack.push(current_instrution + 1);
					current_instrution = jump_address;
					continue;
				}
				else if (static_jump)
				{
					// NOTE: This will skip potential jump target blocks between current->target
					current_instrution = jump_address;
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
					current_instrution = call_stack.top();
					call_stack.pop();
					continue;
				}

				break;
			}
			}

			if ((d3.end && (fast_exit || current_instrution >= instruction_range.second)) ||
				(current_instrution + 1) == 512)
			{
				break;
			}

			current_instrution++;
		}

		for (const u32 target : conditional_targets)
		{
			if (!result.instruction_mask[target])
			{
				walk_function(target, true);
			}
		}
	};

	if (g_cfg.video.log_programs)
	{
		fs::file dump(fs::get_cache_dir() + "shaderlog/vp_analyser.bin", fs::rewrite);
		dump.write(&entry, 4);
		dump.write(data, 512 * 16);
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
		verify(HERE), instruction_range.first == entry;
		std::memcpy(dst_prog.data.data(), data + (instruction_range.first * 4), result.ucode_length);
	}
	else
	{
		for (u32 i = instruction_range.first, count = 0; i <= instruction_range.second; ++i, ++count)
		{
			const qword* instruction = reinterpret_cast<const qword*>(&data[i * 4]);
			qword* dst = reinterpret_cast<qword*>(&dst_prog.data[count * 4]);

			if (result.instruction_mask[i])
			{
				dst->dword[0] = instruction->dword[0];
				dst->dword[1] = instruction->dword[1];

				if (instructions_to_patch[i])
				{
					d2.HEX = dst->word[2];
					d3.HEX = dst->word[3];

					u32 address = ((d2.iaddrh << 3) | d3.iaddrl);
					address -= instruction_range.first;

					d2.iaddrh = (address >> 3);
					d3.iaddrl = (address & 0x7);
					dst->word[2] = d2.HEX;
					dst->word[3] = d3.HEX;

					dst_prog.jump_table.emplace(address);
				}
			}
			else
			{
				dst->dword[0] = 0ull;
				dst->dword[1] = 0ull;
			}
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

	return result;
}

size_t vertex_program_storage_hash::operator()(const RSXVertexProgram &program) const
{
	size_t hash = vertex_program_utils::get_vertex_program_ucode_hash(program);
	hash ^= program.output_mask;
	hash ^= program.texture_dimensions;
	return hash;
}

bool vertex_program_compare::operator()(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2) const
{
	if (binary1.output_mask != binary2.output_mask)
		return false;
	if (binary1.texture_dimensions != binary2.texture_dimensions)
		return false;
	if (binary1.data.size() != binary2.data.size())
		return false;
	if (binary1.jump_table != binary2.jump_table)
		return false;
	if (!binary1.skip_vertex_input_check && !binary2.skip_vertex_input_check && binary1.rsx_vertex_inputs != binary2.rsx_vertex_inputs)
		return false;

	const qword* instBuffer1 = reinterpret_cast<const qword*>(binary1.data.data());
	const qword* instBuffer2 = reinterpret_cast<const qword*>(binary2.data.data());
	size_t instIndex = 0;
	for (unsigned i = 0; i < binary1.data.size() / 4; i++)
	{
		const auto active = binary1.instruction_mask[instIndex];
		if (active != binary2.instruction_mask[instIndex])
		{
			return false;
		}

		if (active)
		{
			const qword& inst1 = instBuffer1[instIndex];
			const qword& inst2 = instBuffer2[instIndex];
			if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
			{
				return false;
			}
		}

		instIndex++;
	}

	return true;
}


bool fragment_program_utils::is_constant(u32 sourceOperand)
{
	return ((sourceOperand >> 8) & 0x3) == 2;
}

size_t fragment_program_utils::get_fragment_program_ucode_size(void *ptr)
{
	const qword* instBuffer = reinterpret_cast<const qword*>(ptr);
	size_t instIndex = 0;
	while (true)
	{
		const qword& inst = instBuffer[instIndex];
		bool isSRC0Constant = is_constant(inst.word[1]);
		bool isSRC1Constant = is_constant(inst.word[2]);
		bool isSRC2Constant = is_constant(inst.word[3]);
		bool end = (inst.word[0] >> 8) & 0x1;

		if (isSRC0Constant || isSRC1Constant || isSRC2Constant)
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

fragment_program_utils::fragment_program_metadata fragment_program_utils::analyse_fragment_program(void *ptr)
{
	const qword* instBuffer = reinterpret_cast<const qword*>(ptr);
	s32 index = 0;
	s32 program_offset = -1;
	u32 ucode_size = 0;
	u32 constants_size = 0;
	u16 textures_mask = 0;

	while (true)
	{
		const qword& inst = instBuffer[index];
		const u32 opcode = (inst.word[0] >> 16) & 0x3F;

		if (opcode)
		{
			if (program_offset < 0)
				program_offset = index * 16;

			switch(opcode)
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
				const u32 tex_num = (inst.word[0] >> 25) & 15;
				textures_mask |= (1 << tex_num);
				break;
			}
			}

			if (is_constant(inst.word[1]) || is_constant(inst.word[2]) || is_constant(inst.word[3]))
			{
				//Instruction references constant, skip one slot occupied by data
				index++;
				ucode_size += 16;
				constants_size += 16;
			}
		}

		if (program_offset >= 0)
		{
			ucode_size += 16;
		}

		if ((inst.word[0] >> 8) & 0x1)
		{
			if (program_offset < 0)
			{
				program_offset = index * 16;
				ucode_size = 16;
			}

			break;
		}

		index++;
	}

	return{ static_cast<u32>(program_offset), ucode_size, constants_size, textures_mask };
}

size_t fragment_program_utils::get_fragment_program_ucode_hash(const RSXFragmentProgram& program)
{
	// 64-bit Fowler/Noll/Vo FNV-1a hash code
	size_t hash = 0xCBF29CE484222325ULL;
	const qword* instbuffer = reinterpret_cast<const qword*>(program.addr);
	size_t instIndex = 0;
	while (true)
	{
		const qword& inst = instbuffer[instIndex];
		hash ^= inst.dword[0];
		hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
		hash ^= inst.dword[1];
		hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
		instIndex++;
		// Skip constants
		if (fragment_program_utils::is_constant(inst.word[1]) ||
			fragment_program_utils::is_constant(inst.word[2]) ||
			fragment_program_utils::is_constant(inst.word[3]))
			instIndex++;

		bool end = (inst.word[0] >> 8) & 0x1;
		if (end)
			return hash;
	}
	return 0;
}

size_t fragment_program_storage_hash::operator()(const RSXFragmentProgram& program) const
{
	size_t hash = fragment_program_utils::get_fragment_program_ucode_hash(program);
	hash ^= program.ctrl;
	hash ^= program.texture_dimensions;
	hash ^= program.unnormalized_coords;
	hash ^= +program.two_sided_lighting;
	hash ^= program.shadow_textures;
	hash ^= program.redirected_textures;

	return hash;
}

bool fragment_program_compare::operator()(const RSXFragmentProgram& binary1, const RSXFragmentProgram& binary2) const
{
	if (binary1.ctrl != binary2.ctrl || binary1.texture_dimensions != binary2.texture_dimensions || binary1.unnormalized_coords != binary2.unnormalized_coords ||
		binary1.two_sided_lighting != binary2.two_sided_lighting ||
		binary1.shadow_textures != binary2.shadow_textures || binary1.redirected_textures != binary2.redirected_textures)
		return false;

	for (u8 index = 0; index < 16; ++index)
	{
		if (binary1.textures_alpha_kill[index] != binary2.textures_alpha_kill[index])
			return false;

		if (binary1.textures_zfunc[index] != binary2.textures_zfunc[index])
			return false;
	}

	const qword* instBuffer1 = reinterpret_cast<const qword*>(binary1.addr);
	const qword* instBuffer2 = reinterpret_cast<const qword*>(binary2.addr);
	size_t instIndex = 0;
	while (true)
	{
		const qword& inst1 = instBuffer1[instIndex];
		const qword& inst2 = instBuffer2[instIndex];

		if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
			return false;

		instIndex++;
		// Skip constants
		if (fragment_program_utils::is_constant(inst1.word[1]) ||
			fragment_program_utils::is_constant(inst1.word[2]) ||
			fragment_program_utils::is_constant(inst1.word[3]))
			instIndex++;

		bool end = ((inst1.word[0] >> 8) & 0x1) && ((inst2.word[0] >> 8) & 0x1);
		if (end)
			return true;
	}
}
