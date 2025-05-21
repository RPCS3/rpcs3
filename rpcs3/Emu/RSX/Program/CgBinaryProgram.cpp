#include "stdafx.h"
#include "CgBinaryProgram.h"

#ifndef WITHOUT_OPENGL
#include "Emu/RSX/GL/GLVertexProgram.h"
#include "Emu/RSX/GL/GLFragmentProgram.h"
#endif

CgBinaryDisasm::CgBinaryDisasm(const std::string& path)
	: m_path(path)
{
	fs::file f(path);
	if (!f)
	{
		return;
	}

	usz buffer_size = f.size();
	m_buffer.resize(buffer_size);
	f.read(m_buffer, buffer_size);
	fmt::append(m_arb_shader, "Loading... [%s]\n", path.c_str());
}

std::string CgBinaryDisasm::GetCgParamType(u32 type)
{
	switch (type)
	{
	case 1045: return "float";
	case 1046:
	case 1047:
	case 1048: return fmt::format("float%d", type - 1044);
	case 1064: return "float4x4";
	case 1066: return "sampler2D";
	case 1069: return "samplerCUBE";
	case 1091: return "float1";

	default: return fmt::format("!UnkCgType(%d)", type);
	}
}

std::string CgBinaryDisasm::GetCgParamName(u32 offset) const
{
	return std::string(&m_buffer[offset]);
}

std::string CgBinaryDisasm::GetCgParamRes(u32 /*offset*/) const
{
	// rsx_log.warning("GetCgParamRes offset 0x%x", offset);
	// TODO
	return "";
}

std::string CgBinaryDisasm::GetCgParamSemantic(u32 offset) const
{
	return std::string(&m_buffer[offset]);
}

std::string CgBinaryDisasm::GetCgParamValue(u32 offset, u32 end_offset) const
{
	std::string offsets = "offsets:";

	u32 num = 0;
	offset += 6;
	while (offset < end_offset)
	{
		fmt::append(offsets, " %d,", m_buffer[offset] << 8 | m_buffer[offset + 1]);
		offset += 4;
		num++;
	}

	if (num > 4)
	{
		return "";
	}

	offsets.pop_back();
	return fmt::format("num %d ", num) + offsets;
}

void CgBinaryDisasm::ConvertToLE(CgBinaryProgram& prog)
{
	// BE payload, requires that data be swapped
	const auto be_profile = prog.profile;

	auto swap_be32 = [&](u32 start_offset, size_t size_bytes)
	{
		auto start = reinterpret_cast<u32*>(m_buffer.data() + start_offset);
		auto end = reinterpret_cast<u32*>(m_buffer.data() + start_offset + size_bytes);

		for (auto data = start; data < end; ++data)
		{
			*data = std::bit_cast<be_t<u32>>(*data);
		}
	};

	// 1. Swap the header
	swap_be32(0, sizeof(CgBinaryProgram));

	// 2. Swap parameters
	swap_be32(prog.parameterArray, sizeof(CgBinaryParameter) * prog.parameterCount);

	// 3. Swap the ucode
	swap_be32(prog.ucode, m_buffer.size() - prog.ucode);

	// 4. Swap the domain header
	if (be_profile == 7004u)
	{
		// Need to swap each field individually
		auto& fprog = GetCgRef<CgBinaryFragmentProgram>(prog.program);
		fprog.instructionCount = std::bit_cast<be_t<u32>>(fprog.instructionCount);
		fprog.attributeInputMask = std::bit_cast<be_t<u32>>(fprog.attributeInputMask);
		fprog.partialTexType = std::bit_cast<be_t<u32>>(fprog.partialTexType);
		fprog.texCoordsInputMask = std::bit_cast<be_t<u16>>(fprog.texCoordsInputMask);
		fprog.texCoords2D = std::bit_cast<be_t<u16>>(fprog.texCoords2D);
		fprog.texCoordsCentroid = std::bit_cast<be_t<u16>>(fprog.texCoordsCentroid);
	}
	else
	{
		// Swap entire header block as all fields are u32
		swap_be32(prog.program, sizeof(CgBinaryVertexProgram));
	}
}

void CgBinaryDisasm::BuildShaderBody(bool include_glsl)
{
	ParamArray param_array;

	auto& prog = GetCgRef<CgBinaryProgram>(0);

	if (const u32 be_profile = std::bit_cast<be_t<u32>>(prog.profile);
		be_profile == 7003u || be_profile == 7004u)
	{
		ConvertToLE(prog);
		ensure(be_profile == prog.profile);
	}

	if (prog.profile == 7004u)
	{
		auto& fprog = GetCgRef<CgBinaryFragmentProgram>(prog.program);
		m_arb_shader += "\n";
		fmt::append(m_arb_shader, "# binaryFormatRevision 0x%x\n", prog.binaryFormatRevision);
		fmt::append(m_arb_shader, "# profile sce_fp_rsx\n");
		fmt::append(m_arb_shader, "# parameterCount %d\n", prog.parameterCount);
		fmt::append(m_arb_shader, "# instructionCount %d\n", fprog.instructionCount);
		fmt::append(m_arb_shader, "# attributeInputMask 0x%x\n", fprog.attributeInputMask);
		fmt::append(m_arb_shader, "# registerCount %d\n\n", fprog.registerCount);

		CgBinaryParameterOffset offset = prog.parameterArray;
		for (u32 i = 0; i < prog.parameterCount; i++)
		{
			auto& fparam = GetCgRef<CgBinaryParameter>(offset);

			std::string param_type = GetCgParamType(fparam.type) + " ";
			std::string param_name = GetCgParamName(fparam.name) + " ";
			std::string param_res = GetCgParamRes(fparam.res) + " ";
			std::string param_semantic = GetCgParamSemantic(fparam.semantic) + " ";
			std::string param_const = GetCgParamValue(fparam.embeddedConst, fparam.name);

			fmt::append(m_arb_shader, "#%d%s%s%s%s\n", i, param_type, param_name, param_semantic, param_const);

			offset += u32{sizeof(CgBinaryParameter)};
		}

		m_arb_shader += "\n";
		m_offset = prog.ucode;
		TaskFP();

		if (!include_glsl)
		{
			return;
		}

		u32 unused;
		std::vector<u32> be_data;

		// Swap bytes. FP decompiler expects input in BE
		for (u32* ptr = reinterpret_cast<u32*>(m_buffer.data() + m_offset),
			*end = reinterpret_cast<u32*>(m_buffer.data() + m_buffer.size());
			ptr < end; ++ptr)
		{
			be_data.push_back(std::bit_cast<be_t<u32>>(*ptr));
		}

		RSXFragmentProgram rsx_prog;
		auto metadata = program_hash_util::fragment_program_utils::analyse_fragment_program(be_data.data());
		rsx_prog.ctrl = (fprog.outputFromH0 ? 0 : 0x40) | (fprog.depthReplace ? 0xe : 0);
		rsx_prog.offset = metadata.program_start_offset;
		rsx_prog.ucode_length = metadata.program_ucode_length;
		rsx_prog.total_length = metadata.program_ucode_length + metadata.program_start_offset;
		rsx_prog.data = reinterpret_cast<u8*>(be_data.data()) + metadata.program_start_offset;
		for (u32 i = 0; i < 16; ++i) rsx_prog.texture_state.set_dimension(rsx::texture_dimension_extended::texture_dimension_2d, i);
#ifndef WITHOUT_OPENGL
		GLFragmentDecompilerThread(m_glsl_shader, param_array, rsx_prog, unused).Task();
#endif
	}

	else
	{
		const auto& vprog = GetCgRef<CgBinaryVertexProgram>(prog.program);
		m_arb_shader += "\n";
		fmt::append(m_arb_shader, "# binaryFormatRevision 0x%x\n", prog.binaryFormatRevision);
		fmt::append(m_arb_shader, "# profile sce_vp_rsx\n");
		fmt::append(m_arb_shader, "# parameterCount %d\n", prog.parameterCount);
		fmt::append(m_arb_shader, "# instructionCount %d\n", vprog.instructionCount);
		fmt::append(m_arb_shader, "# registerCount %d\n", vprog.registerCount);
		fmt::append(m_arb_shader, "# attributeInputMask 0x%x\n", vprog.attributeInputMask);
		fmt::append(m_arb_shader, "# attributeOutputMask 0x%x\n\n", vprog.attributeOutputMask);

		CgBinaryParameterOffset offset = prog.parameterArray;
		for (u32 i = 0; i < prog.parameterCount; i++)
		{
			auto& vparam = GetCgRef<CgBinaryParameter>(offset);

			std::string param_type = GetCgParamType(vparam.type) + " ";
			std::string param_name = GetCgParamName(vparam.name) + " ";
			std::string param_res = GetCgParamRes(vparam.res) + " ";
			std::string param_semantic = GetCgParamSemantic(vparam.semantic) + " ";
			std::string param_const = GetCgParamValue(vparam.embeddedConst, vparam.name);

			fmt::append(m_arb_shader, "#%d%s%s%s%s\n", i, param_type, param_name, param_semantic, param_const);

			offset += u32{sizeof(CgBinaryParameter)};
		}

		m_arb_shader += "\n";
		m_offset = prog.ucode;
		ensure((m_buffer.size() - m_offset) % sizeof(u32) == 0);

		u32* vdata = reinterpret_cast<u32*>(&m_buffer[m_offset]);
		m_data.resize(prog.ucodeSize / sizeof(u32));
		std::memcpy(m_data.data(), vdata, prog.ucodeSize);
		TaskVP();

		if (!include_glsl)
		{
			return;
		}

		RSXVertexProgram rsx_prog;
		program_hash_util::vertex_program_utils::analyse_vertex_program(vdata, 0, rsx_prog);
		for (u32 i = 0; i < 4; ++i) rsx_prog.texture_state.set_dimension(rsx::texture_dimension_extended::texture_dimension_2d, i);
#ifndef WITHOUT_OPENGL
		GLVertexDecompilerThread(rsx_prog, m_glsl_shader, param_array).Task();
#endif
	}
}
