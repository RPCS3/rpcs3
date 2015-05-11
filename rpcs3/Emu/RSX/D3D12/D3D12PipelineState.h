#pragma once
#if defined (DX12_SUPPORT)

#include <d3d12.h>
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Emu/RSX/RSXVertexProgram.h"


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

	u32 Id;
	ID3DBlob *bytecode;

	/**
	* Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	//	void Decompile(RSXFragmentProgram& prog)

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile(SHADER_TYPE st);
};

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

class PipelineStateObjectCache
{
private:
	size_t currentShaderId;
	binary2VS cacheVS;
	binary2FS cacheFS;
	// Key is vertex << 32 | fragment ids
	std::unordered_map<u64, ID3D12PipelineState *> cachePSO;

	bool SearchFp(const RSXFragmentProgram& rsx_fp, Shader& shader);
	bool SearchVp(const RSXVertexProgram& rsx_vp, Shader& shader);
	ID3D12PipelineState *GetProg(u32 fp, u32 vp) const;
	void AddVertexProgram(Shader& vp, RSXVertexProgram& rsx_vp);
	void AddFragmentProgram(Shader& fp, RSXFragmentProgram& rsx_fp);
	void Add(ID3D12PipelineState *prog, Shader& fp, Shader& vp);
public:
	PipelineStateObjectCache();
	ID3D12PipelineState *getGraphicPipelineState(ID3D12Device *device, RSXVertexProgram *vertexShader, RSXFragmentProgram *fragmentShader);
};



#endif