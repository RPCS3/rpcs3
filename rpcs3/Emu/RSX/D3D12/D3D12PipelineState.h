#pragma once
#if defined (DX12_SUPPORT)

#include <d3d12.h>
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include <wrl/client.h>


enum class SHADER_TYPE
{
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_FRAGMENT
};

struct PipelineProperties
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology;
};

/** Storage for a shader
*   Embeds the D3DBlob corresponding to
*/
class Shader
{
public:
	Shader() : bytecode(nullptr) {}
	~Shader() {}

	u32 Id;
	Microsoft::WRL::ComPtr<ID3DBlob> bytecode;

	/**
	* Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	//	void Decompile(RSXFragmentProgram& prog)

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile(const std::string &code, SHADER_TYPE st);
};



namespace ProgramHashUtil
{
	// Based on
	// https://github.com/AlexAltea/nucleus/blob/master/nucleus/gpu/rsx_pgraph.cpp
	union qword
	{
		u64 dword[2];
		u32 word[4];
	};

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

	struct FragmentHashUtil
	{
		/**
		* RSX fragment program constants are inlined inside shader code.
		* This function takes an instruction from a fragment program and
		* returns an equivalent instruction where inlined constants
		* are masked.
		* This allows to hash/compare fragment programs even if their
		* inlined constants are modified inbetween
		*/
		static qword fragmentMaskConstant(const qword &initialQword)
		{
			qword result = initialQword;
			u64 dword0Mask = 0, dword1Mask = 0;;
			// Check if there is a constant and mask word if there is
			SRC0 s0 = { initialQword.word[1] };
			SRC1 s1 = { initialQword.word[2] };
			SRC2 s2 = { initialQword.word[3] };
			if (s0.reg_type == 2)
				result.word[1] = 0;
			if (s1.reg_type == 2)
				result.word[2] = 0;
			if (s2.reg_type == 2)
				result.word[3] = 0;
			return result;
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
				const qword& maskedInst = FragmentHashUtil::fragmentMaskConstant(inst);

				hash ^= maskedInst.dword[0];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				hash ^= maskedInst.dword[1];
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
				instIndex++;
			}
			return 0;
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

				const qword& maskedInst1 = FragmentHashUtil::fragmentMaskConstant(inst1);
				const qword& maskedInst2 = FragmentHashUtil::fragmentMaskConstant(inst2);

				if (maskedInst1.dword[0] != maskedInst2.dword[0] || maskedInst1.dword[1] != maskedInst2.dword[1])
					return false;
				instIndex++;
			}
		}
	};

}

typedef std::unordered_map<void *, Shader, ProgramHashUtil::HashVertexProgram, ProgramHashUtil::VertexProgramCompare> binary2VS;
typedef std::unordered_map<void *, Shader, ProgramHashUtil::HashFragmentProgram, ProgramHashUtil::FragmentProgramCompare> binary2FS;

struct PSOKey
{
	u32 vpIdx;
	u32 fpIdx;
	PipelineProperties properties;
};

struct PSOKeyHash
{
	size_t operator()(const PSOKey &key) const
	{
		size_t hashValue = 0;
		hashValue ^= std::hash<unsigned>()(key.vpIdx);
		return hashValue;
	}
};

struct PSOKeyCompare
{
	size_t operator()(const PSOKey &key1, const PSOKey &key2) const
	{
		return (key1.vpIdx == key2.vpIdx) && (key1.fpIdx == key2.fpIdx) && (key1.properties.Topology == key2.properties.Topology);
	}
};

/**
 * Cache for shader blobs and Pipeline state object
 * The class is responsible for creating the object so the state only has to call getGraphicPipelineState
 */
class PipelineStateObjectCache
{
private:
	size_t m_currentShaderId;
	binary2VS m_cacheVS;
	binary2FS m_cacheFS;

	std::unordered_map<PSOKey, ID3D12PipelineState *, PSOKeyHash, PSOKeyCompare> m_cachePSO;

	bool SearchFp(const RSXFragmentProgram& rsx_fp, Shader& shader);
	bool SearchVp(const RSXVertexProgram& rsx_vp, Shader& shader);
	ID3D12PipelineState *GetProg(const PSOKey &psoKey) const;
	void AddVertexProgram(Shader& vp, RSXVertexProgram& rsx_vp);
	void AddFragmentProgram(Shader& fp, RSXFragmentProgram& rsx_fp);
	void Add(ID3D12PipelineState *prog, const PSOKey& PSOKey);
public:
	PipelineStateObjectCache();
	~PipelineStateObjectCache();
	// Note: the last param is not taken into account if the PSO is not regenerated
	ID3D12PipelineState *getGraphicPipelineState(
		ID3D12Device *device,
		ID3D12RootSignature *rootSignature,
		RSXVertexProgram *vertexShader,
		RSXFragmentProgram *fragmentShader,
		const PipelineProperties &pipelineProperties,
		const std::vector<D3D12_INPUT_ELEMENT_DESC> &IASet
	);
};



#endif