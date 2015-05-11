#include "stdafx.h"
#if defined (DX12_SUPPORT)

#include "D3D12PipelineState.h"
#include "D3D12ProgramDisassembler.h"
#include "Emu/Memory/vm.h"
#include "Utilities/Log.h"
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <unordered_map>

#pragma comment (lib, "d3dcompiler.lib")



struct GLBufferInfo
{
	ID3D12PipelineState *prog_id;
	u32 fp_id;
	u32 vp_id;
	std::vector<u8> fp_data;
	std::vector<u32> vp_data;
	std::string fp_shader;
	std::string vp_shader;
	Microsoft::WRL::ComPtr<ID3DBlob> fp_bytecode;
	Microsoft::WRL::ComPtr<ID3DBlob> vp_bytecode;
};

enum class SHADER_TYPE
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
};

/** Storage for a shader
*   Embeds the D3DBlob corresponding to 
*/
class Shader
{
public:
	Shader() : bytecode(nullptr) {}
	~Shader() {}

	Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
	std::vector<u8> RSXBinary;

	/**
	* Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
//	void Decompile(RSXFragmentProgram& prog)

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile(SHADER_TYPE st)
	{
		static const char VSstring[] =
			"float4 main(float4 pos : POSITION) : SV_POSITION"
			"{"
			"	return pos;"
			"}";
		static const char FSstring[] =
			"float4 main() : SV_TARGET"
			"{"
			"return float4(1.0f, 1.0f, 1.0f, 1.0f);"
			"}";
		HRESULT hr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		switch (st)
		{
		case SHADER_TYPE::SHADER_TYPE_VERTEX:
			hr = D3DCompile(VSstring, sizeof(VSstring), "test", nullptr, nullptr, "main", "vs_5_0", 0, 0, bytecode.GetAddressOf(), errorBlob.GetAddressOf());
			if (hr != S_OK)
				LOG_ERROR(RSX, "VS build failed:%s", errorBlob->GetBufferPointer());
			break;
		case SHADER_TYPE::SHADER_TYPE_FRAGMENT:
			hr = D3DCompile(FSstring, sizeof(FSstring), "test", nullptr, nullptr, "main", "ps_5_0", 0, 0, bytecode.GetAddressOf(), errorBlob.GetAddressOf());
			if (hr != S_OK)
				LOG_ERROR(RSX, "FS build failed:%s", errorBlob->GetBufferPointer());
			break;
		}
	}
};

// Based on
// https://github.com/AlexAltea/nucleus/blob/master/nucleus/gpu/rsx_pgraph.cpp
union qword
{
	u64 dword[2];
	u32 word[4];
};

size_t getFPBinarySize(void *ptr)
{
	const qword *instBuffer = (const qword*)ptr;
	size_t instIndex = 0;
	while (true)
	{
		const qword& inst = instBuffer[instIndex];
		bool end = (inst.word[0] >> 8) & 0x1;
		if (end)
			return (instIndex + 1) * 4;
		instIndex++;
	}
}

struct HashVertexProgram
{
	size_t operator()(const void *program) const
	{
		// 64-bit Fowler/Noll/Vo FNV-1a hash code
		size_t hash = 0xCBF29CE484222325ULL;
		const qword *instbuffer = (const qword*)program;
		size_t instIndex = 0;
		bool end = false;
		return 0;
		while (true)
		{
			const qword inst = instbuffer[instIndex];
			bool end = inst.word[0] >> 31;
			if (end)
				return hash;
			hash ^= inst.dword[0];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			hash ^= inst.dword[1];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			instIndex++;
		}
		return 0;
	}
};

struct HashFragmentProgram
{
	size_t operator()(const void *program) const
	{
		// 64-bit Fowler/Noll/Vo FNV-1a hash code
		size_t hash = 0xCBF29CE484222325ULL;
		const qword *instbuffer = (const qword*)program;
		size_t instIndex = 0;
		while (true)
		{
			const qword& inst = instbuffer[instIndex];
			bool end = (inst.word[0] >> 8) & 0x1;
			if (end)
				return hash;
			hash ^= inst.dword[0];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			hash ^= inst.dword[1];
			hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			instIndex++;
		}
		return 0;
	}
};

struct VertexProgramCompare
{
	bool operator()(const void *binary1, const void *binary2) const
	{
		const qword *instBuffer1 = (const qword*)binary1;
		const qword *instBuffer2 = (const qword*)binary2;
		size_t instIndex = 0;
		return true;
		while (true)
		{
			const qword& inst1 = instBuffer1[instIndex];
			const qword& inst2 = instBuffer2[instIndex];
			bool end = (inst1.word[0] >> 31) && (inst2.word[0] >> 31);
			if (end)
				return true;
			if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
				return false;
			instIndex++;
		}
	}
};

struct FragmentProgramCompare
{
	bool operator()(const void *binary1, const void *binary2) const
	{
		const qword *instBuffer1 = (const qword*)binary1;
		const qword *instBuffer2 = (const qword*)binary2;
		size_t instIndex = 0;
		while (true)
		{
			const qword& inst1 = instBuffer1[instIndex];
			const qword& inst2 = instBuffer2[instIndex];
			bool end = ((inst1.word[0] >> 8) & 0x1) && ((inst2.word[0] >> 8) & 0x1);
			if (end)
				return true;
			if (inst1.dword[0] != inst2.dword[0] || inst1.dword[1] != inst2.dword[1])
				return false;
			instIndex++;
		}
	}
};

typedef std::unordered_map<void *, Shader, HashVertexProgram, VertexProgramCompare> binary2VS;
typedef std::unordered_map<void *, Shader, HashFragmentProgram, FragmentProgramCompare> binary2FS;
static int tmp = 0;
class ProgramBuffer
{
public:
	binary2VS cacheVS;
	binary2FS cacheFS;

	bool SearchFp(const RSXFragmentProgram& rsx_fp, Shader& shader)
	{
		binary2FS::const_iterator It = cacheFS.find(vm::get_ptr<void>(rsx_fp.addr));
		if (It != cacheFS.end())
		{
			shader = It->second;
			return true;
		}
		return false;
	}

	bool SearchVp(const RSXVertexProgram& rsx_vp, Shader& shader)
	{
		binary2VS::const_iterator It = cacheVS.find((void*)rsx_vp.data.data());
		if (It != cacheVS.end())
		{
			shader = It->second;
			return true;
		}
		return false;
	}

/*	ID3D12PipelineState *GetProg(u32 fp, u32 vp) const
	{
		if (fp == vp)
		{*/
			/*
			LOG_NOTICE(RSX, "Get program (%d):", fp);
			LOG_NOTICE(RSX, "*** prog id = %d", m_buf[fp].prog_id);
			LOG_NOTICE(RSX, "*** vp id = %d", m_buf[fp].vp_id);
			LOG_NOTICE(RSX, "*** fp id = %d", m_buf[fp].fp_id);

			LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[fp].vp_shader.wx_str());
			LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[fp].fp_shader.wx_str());
			*/
/*			return m_buf[fp].prog_id;
		}

		for (u32 i = 0; i<m_buf.size(); ++i)
		{
			if (i == fp || i == vp) continue;*/

//			if (CmpVP(vp, i) && CmpFP(fp, i))
//			{
				/*
				LOG_NOTICE(RSX, "Get program (%d):", i);
				LOG_NOTICE(RSX, "*** prog id = %d", m_buf[i].prog_id);
				LOG_NOTICE(RSX, "*** vp id = %d", m_buf[i].vp_id);
				LOG_NOTICE(RSX, "*** fp id = %d", m_buf[i].fp_id);

				LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[i].vp_shader.wx_str());
				LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[i].fp_shader.wx_str());
				*/
/*				return m_buf[i].prog_id;
			}
		}

		return 0;
	}*/

	void AddVertexProgram(const Shader& vp, RSXVertexProgram& rsx_vp)
	{
		size_t actualVPSize = rsx_vp.data.size() * 4;
		void *fpShadowCopy = malloc(actualVPSize);
		memcpy(fpShadowCopy, rsx_vp.data.data(), actualVPSize);
		cacheVS.insert(std::make_pair(fpShadowCopy, vp));
	}

	void AddFragmentProgram(const Shader& fp, RSXFragmentProgram& rsx_fp)
	{
		size_t actualFPSize = getFPBinarySize(vm::get_ptr<u8>(rsx_fp.addr));
		void *fpShadowCopy = malloc(actualFPSize);
		memcpy(fpShadowCopy, vm::get_ptr<u8>(rsx_fp.addr), actualFPSize);
		cacheFS.insert(std::make_pair(fpShadowCopy, fp));
	}

	void Add(ID3D12PipelineState *prog, Shader& fp, RSXFragmentProgram& rsx_fp, Shader& vp, RSXVertexProgram& rsx_vp)
	{
/*		LOG_NOTICE(RSX, "Add program (%d):", m_buf.size());
		LOG_NOTICE(RSX, "*** prog id = %x", prog);
		LOG_NOTICE(RSX, "*** vp id = %d", vp.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fp.id);
		LOG_NOTICE(RSX, "*** vp data size = %d", rsx_vp.data.size() * 4);
		LOG_NOTICE(RSX, "*** fp data size = %d", rsx_fp.size);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vp.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fp.shader.c_str());*/
	}
};

static ProgramBuffer g_cachedProgram;

D3D12PipelineState::D3D12PipelineState(ID3D12Device *device, RSXVertexProgram *vertexShader, RSXFragmentProgram *fragmentShader)
{
	Shader m_vertex_prog, m_fragment_prog;
	bool m_fp_buf_num = g_cachedProgram.SearchFp(*fragmentShader, m_fragment_prog);
	bool m_vp_buf_num = g_cachedProgram.SearchVp(*vertexShader, m_vertex_prog);

	if (!m_fp_buf_num)
	{
		LOG_WARNING(RSX, "FP not found in buffer!");
//		Decompile(*fragmentShader);
		m_fragment_prog.Compile(SHADER_TYPE::SHADER_TYPE_FRAGMENT);
		g_cachedProgram.AddFragmentProgram(m_fragment_prog, *fragmentShader);

		// TODO: This shouldn't use current dir
		//fs::file("./FragmentProgram.txt", o_write | o_create | o_trunc).write(m_fragment_prog.shader.c_str(), m_fragment_prog.shader.size());
	}

	if (!m_vp_buf_num)
	{
		LOG_WARNING(RSX, "VP not found in buffer!");
//		m_vertex_prog.Decompile(*vertexShader);
		m_vertex_prog.Compile(SHADER_TYPE::SHADER_TYPE_VERTEX);
		g_cachedProgram.AddVertexProgram(m_vertex_prog, *vertexShader);

		// TODO: This shouldn't use current dir
//		fs::file("./VertexProgram.txt", o_write | o_create | o_trunc).write(m_vertex_prog.shader.c_str(), m_vertex_prog.shader.size());
	}

//	if (m_fp_buf_num != -1 && m_vp_buf_num != -1)
	{
//		m_program.id = m_prog_buffer.GetProg(m_fp_buf_num, m_vp_buf_num);
	}

	if (false)//m_program.id)
	{
/*		// RSX Debugger: Check if this program was modified and update it
		if (Ini.GSLogPrograms.GetValue())
		{
			for (auto& program : m_debug_programs)
			{
				if (program.id == m_program.id && program.modified)
				{
					// TODO: This isn't working perfectly. Is there any better/shorter way to update the program
					m_vertex_prog.shader = program.vp_shader;
					m_fragment_prog.shader = program.fp_shader;
					m_vertex_prog.Wait();
					m_vertex_prog.Compile();
					checkForGlError("m_vertex_prog.Compile");
					m_fragment_prog.Wait();
					m_fragment_prog.Compile();
					checkForGlError("m_fragment_prog.Compile");
					glAttachShader(m_program.id, m_vertex_prog.id);
					glAttachShader(m_program.id, m_fragment_prog.id);
					glLinkProgram(m_program.id);
					checkForGlError("glLinkProgram");
					glDetachShader(m_program.id, m_vertex_prog.id);
					glDetachShader(m_program.id, m_fragment_prog.id);
					program.vp_id = m_vertex_prog.id;
					program.fp_id = m_fragment_prog.id;
					program.modified = false;
				}
			}
		}
		m_program.Use();*/
	}
	else
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc = {};

/*		graphicPipelineStateDesc.VS.BytecodeLength = m_vertex_prog.bytecode->GetBufferSize();
		graphicPipelineStateDesc.VS.pShaderBytecode = m_vertex_prog.bytecode->GetBufferPointer();
		graphicPipelineStateDesc.PS.BytecodeLength = m_fragment_prog.bytecode->GetBufferSize();
		graphicPipelineStateDesc.PS.pShaderBytecode = m_fragment_prog.bytecode->GetBufferPointer();
		device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&m_pipelineStateObject));*/
		g_cachedProgram.Add(m_pipelineStateObject, m_fragment_prog, *fragmentShader, m_vertex_prog, *vertexShader);
		/*m_program.Create(m_vertex_prog.id, m_fragment_prog.id);
		checkForGlError("m_program.Create");
		m_prog_buffer.Add(m_program, m_fragment_prog, *m_cur_fragment_prog, m_vertex_prog, *m_cur_vertex_prog);
		checkForGlError("m_prog_buffer.Add");
		m_program.Use();

		// RSX Debugger
		if (Ini.GSLogPrograms.GetValue())
		{
			RSXDebuggerProgram program;
			program.id = m_program.id;
			program.vp_id = m_vertex_prog.id;
			program.fp_id = m_fragment_prog.id;
			program.vp_shader = m_vertex_prog.shader;
			program.fp_shader = m_fragment_prog.shader;
			m_debug_programs.push_back(program);
		}*/
	}
}

D3D12PipelineState::~D3D12PipelineState()
{

}


#endif