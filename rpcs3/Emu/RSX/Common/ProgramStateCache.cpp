#include "stdafx.h"
#include "ProgramStateCache.h"

using namespace program_hash_util;

size_t vertex_program_utils::get_vertex_program_ucode_hash(const RSXVertexProgram &program)
{
	// 64-bit Fowler/Noll/Vo FNV-1a hash code
	size_t hash = 0xCBF29CE484222325ULL;
	const qword *instbuffer = (const qword*)program.data.data();
	size_t instIndex = 0;
	bool end = false;
	for (unsigned i = 0; i < program.data.size() / 4; i++)
	{
		const qword inst = instbuffer[instIndex];
		hash ^= inst.dword[0];
		hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
		hash ^= inst.dword[1];
		hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
		instIndex++;
	}
	return hash;
}

vertex_program_utils::vertex_program_metadata vertex_program_utils::analyse_vertex_program(const std::vector<u32>& data)
{
	u32 ucode_size = 0;
	u32 current_instrution = 0;
	u32 last_instruction_address = 0;
	D3  d3;
	D2  d2;
	D1  d1;

	for (; ucode_size < data.size(); ucode_size += 4)
	{
		d1.HEX = data[ucode_size + 1];

		switch (d1.sca_opcode)
		{
		case RSX_SCA_OPCODE_BRI:
		case RSX_SCA_OPCODE_BRB:
		case RSX_SCA_OPCODE_CAL:
		case RSX_SCA_OPCODE_CLI:
		case RSX_SCA_OPCODE_CLB:
		{
			d2.HEX = data[ucode_size + 2];
			d3.HEX = data[ucode_size + 3];

			u32 jump_address = ((d2.iaddrh << 3) | d3.iaddrl) * 4;
			last_instruction_address = std::max(last_instruction_address, jump_address);
			break;
		}
		}

		if (ucode_size >= last_instruction_address)
		{
			d3.HEX = data[ucode_size + 3];
			if (d3.end)
			{
				break;
			}
		}
	}

	return{ ucode_size + 4 };
}

size_t vertex_program_storage_hash::operator()(const RSXVertexProgram &program) const
{
	size_t hash = vertex_program_utils::get_vertex_program_ucode_hash(program);
	hash ^= program.output_mask;
	return hash;
}

bool vertex_program_compare::operator()(const RSXVertexProgram &binary1, const RSXVertexProgram &binary2) const
{
	if (binary1.output_mask != binary2.output_mask)
		return false;
	if (binary1.data.size() != binary2.data.size())
		return false;
	if (!binary1.skip_vertex_input_check && !binary2.skip_vertex_input_check && binary1.rsx_vertex_inputs != binary2.rsx_vertex_inputs)
		return false;

	const qword *instBuffer1 = (const qword*)binary1.data.data();
	const qword *instBuffer2 = (const qword*)binary2.data.data();
	size_t instIndex = 0;
	for (unsigned i = 0; i < binary1.data.size() / 4; i++)
	{
		const qword& inst1 = instBuffer1[instIndex];
		const qword& inst2 = instBuffer2[instIndex];
		if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
			return false;
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
	const qword *instBuffer = (const qword*)ptr;
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
	const qword *instBuffer = (const qword*)ptr;
	size_t instIndex = 0;
	s32 program_offset = -1;
	u16 textures_mask = 0;

	while (true)
	{
		const qword& inst = instBuffer[instIndex];
		const u32 opcode = (inst.word[0] >> 16) & 0x3F;

		if (opcode)
		{
			if (program_offset < 0)
				program_offset = instIndex * 16;

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
				instIndex++;
			}

			if ((inst.word[0] >> 8) & 0x1)
				break;
		}

		instIndex++;
	}

	return{ (u32)program_offset, textures_mask };
}

size_t fragment_program_utils::get_fragment_program_ucode_hash(const RSXFragmentProgram& program)
{
	// 64-bit Fowler/Noll/Vo FNV-1a hash code
	size_t hash = 0xCBF29CE484222325ULL;
	const qword *instbuffer = (const qword*)program.addr;
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
	hash ^= program.back_color_diffuse_output;
	hash ^= program.back_color_specular_output;
	hash ^= program.front_back_color_enabled;
	hash ^= program.shadow_textures;
	hash ^= program.redirected_textures;

	return hash;
}

bool fragment_program_compare::operator()(const RSXFragmentProgram& binary1, const RSXFragmentProgram& binary2) const
{
	if (binary1.ctrl != binary2.ctrl || binary1.texture_dimensions != binary2.texture_dimensions || binary1.unnormalized_coords != binary2.unnormalized_coords ||
		binary1.back_color_diffuse_output != binary2.back_color_diffuse_output || binary1.back_color_specular_output != binary2.back_color_specular_output ||
		binary1.front_back_color_enabled != binary2.front_back_color_enabled ||
		binary1.shadow_textures != binary2.shadow_textures || binary1.redirected_textures != binary2.redirected_textures)
		return false;

	for (u8 index = 0; index < 16; ++index)
	{
		if (binary1.textures_alpha_kill[index] != binary2.textures_alpha_kill[index])
			return false;

		if (binary1.textures_zfunc[index] != binary2.textures_zfunc[index])
			return false;
	}

	const qword *instBuffer1 = (const qword*)binary1.addr;
	const qword *instBuffer2 = (const qword*)binary2.addr;
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
