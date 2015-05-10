#include "stdafx.h"
#if defined (DX12_SUPPORT)

#include "D3D12PipelineState.h"
#include "Emu/Memory/vm.h"
#include "Utilities/Log.h"
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <unordered_map>

#pragma comment (lib, "d3dcompiler.lib")

std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob> > CachedShader;

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

// Copied from GL implementation

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

//	GLParamArray parr;
	u32 id;
	std::string shader;
	Microsoft::WRL::ComPtr<ID3DBlob> bytecode;

	/**
	* Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
//	void Decompile(RSXFragmentProgram& prog);

	/**
	* Asynchronously decompile a fragment shader located in the PS3's Memory.
	* When this function is called you must call Wait() before GetShaderText() will return valid data.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
//	void DecompileAsync(RSXFragmentProgram& prog);

	/** Wait for the decompiler task to complete decompilation. */
//	void Wait();

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

private:
	/** Threaded fragment shader decompiler responsible for decompiling this program */
//	GLFragmentDecompilerThread* m_decompiler_thread;

	/** Deletes the shader and any stored information */
//	void Delete();
};

// Could be improved with an (un)ordered map ?
class ProgramBuffer
{
	std::vector<GLBufferInfo> m_buf;
public:
	int SearchFp(const RSXFragmentProgram& rsx_fp, Shader& shader)
	{
		int n = m_buf.size();
		for (int i = 0; i < m_buf.size(); ++i)
		{
			if (memcmp(&m_buf[i].fp_data[0], vm::get_ptr<void>(rsx_fp.addr), m_buf[i].fp_data.size()) != 0) continue;

			shader.id = m_buf[i].fp_id;
			shader.shader = m_buf[i].fp_shader.c_str();
			shader.bytecode = m_buf[i].fp_bytecode;

			return i;
		}

		return -1;
	}

	int SearchVp(const RSXVertexProgram& rsx_vp, Shader& shader)
	{
		for (u32 i = 0; i < m_buf.size(); ++i)
		{
			if (m_buf[i].vp_data.size() != rsx_vp.data.size()) continue;
			if (memcmp(m_buf[i].vp_data.data(), rsx_vp.data.data(), rsx_vp.data.size() * 4) != 0) continue;

			shader.id = m_buf[i].vp_id;
			shader.shader = m_buf[i].vp_shader.c_str();
			shader.bytecode = m_buf[i].vp_bytecode;

			return i;
		}

		return -1;
	}

	ID3D12PipelineState *GetProg(u32 fp, u32 vp) const
	{
		if (fp == vp)
		{
			/*
			LOG_NOTICE(RSX, "Get program (%d):", fp);
			LOG_NOTICE(RSX, "*** prog id = %d", m_buf[fp].prog_id);
			LOG_NOTICE(RSX, "*** vp id = %d", m_buf[fp].vp_id);
			LOG_NOTICE(RSX, "*** fp id = %d", m_buf[fp].fp_id);

			LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[fp].vp_shader.wx_str());
			LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[fp].fp_shader.wx_str());
			*/
			return m_buf[fp].prog_id;
		}

		for (u32 i = 0; i<m_buf.size(); ++i)
		{
			if (i == fp || i == vp) continue;

//			if (CmpVP(vp, i) && CmpFP(fp, i))
			{
				/*
				LOG_NOTICE(RSX, "Get program (%d):", i);
				LOG_NOTICE(RSX, "*** prog id = %d", m_buf[i].prog_id);
				LOG_NOTICE(RSX, "*** vp id = %d", m_buf[i].vp_id);
				LOG_NOTICE(RSX, "*** fp id = %d", m_buf[i].fp_id);

				LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[i].vp_shader.wx_str());
				LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[i].fp_shader.wx_str());
				*/
				return m_buf[i].prog_id;
			}
		}

		return 0;
	}


	void Add(ID3D12PipelineState *prog, Shader& fp, RSXFragmentProgram& rsx_fp, Shader& vp, RSXVertexProgram& rsx_vp)
	{
		GLBufferInfo new_buf = {};

		LOG_NOTICE(RSX, "Add program (%d):", m_buf.size());
		LOG_NOTICE(RSX, "*** prog id = %x", prog);
		LOG_NOTICE(RSX, "*** vp id = %d", vp.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fp.id);
		LOG_NOTICE(RSX, "*** vp data size = %d", rsx_vp.data.size() * 4);
		LOG_NOTICE(RSX, "*** fp data size = %d", rsx_fp.size);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vp.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fp.shader.c_str());


		new_buf.prog_id = prog;
		new_buf.vp_id = vp.id;
		new_buf.fp_id = fp.id;
		new_buf.fp_bytecode = fp.bytecode;

		new_buf.fp_data.insert(new_buf.fp_data.end(), vm::get_ptr<u8>(rsx_fp.addr), vm::get_ptr<u8>(rsx_fp.addr + rsx_fp.size));
		new_buf.vp_data = rsx_vp.data;
		new_buf.vp_bytecode = vp.bytecode;

		new_buf.vp_shader = vp.shader;
		new_buf.fp_shader = fp.shader;

		m_buf.resize(m_buf.size() + 1);
		m_buf.push_back(new_buf);
	}
};

static ProgramBuffer g_cachedProgram;

D3D12PipelineState::D3D12PipelineState(ID3D12Device *device, RSXVertexProgram *vertexShader, RSXFragmentProgram *fragmentShader)
{
	Shader m_vertex_prog, m_fragment_prog;
	int m_fp_buf_num = g_cachedProgram.SearchFp(*fragmentShader, m_fragment_prog);
	int m_vp_buf_num = g_cachedProgram.SearchVp(*vertexShader, m_vertex_prog);

	if (m_fp_buf_num == -1)
	{
		LOG_WARNING(RSX, "FP not found in buffer!");
//		m_fragment_prog.Decompile(*fragmentShader);
		m_fragment_prog.Compile(SHADER_TYPE::SHADER_TYPE_FRAGMENT);

		// TODO: This shouldn't use current dir
//		fs::file("./FragmentProgram.txt", o_write | o_create | o_trunc).write(m_fragment_prog.shader.c_str(), m_fragment_prog.shader.size());
	}

	if (m_vp_buf_num == -1)
	{
		LOG_WARNING(RSX, "VP not found in buffer!");
//		m_vertex_prog.Decompile(*vertexShader);
		m_vertex_prog.Compile(SHADER_TYPE::SHADER_TYPE_VERTEX);

		// TODO: This shouldn't use current dir
//		fs::file("./VertexProgram.txt", o_write | o_create | o_trunc).write(m_vertex_prog.shader.c_str(), m_vertex_prog.shader.size());
	}

	if (m_fp_buf_num != -1 && m_vp_buf_num != -1)
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

		graphicPipelineStateDesc.VS.BytecodeLength = m_vertex_prog.bytecode->GetBufferSize();
		graphicPipelineStateDesc.VS.pShaderBytecode = m_vertex_prog.bytecode->GetBufferPointer();
		graphicPipelineStateDesc.PS.BytecodeLength = m_fragment_prog.bytecode->GetBufferSize();
		graphicPipelineStateDesc.PS.pShaderBytecode = m_fragment_prog.bytecode->GetBufferPointer();
		device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&m_pipelineStateObject));
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