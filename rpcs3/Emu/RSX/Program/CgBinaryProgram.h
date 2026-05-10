#pragma once

#include "util/endian.hpp"
#include "Emu/RSX/Program/RSXVertexProgram.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"
#include "Emu/RSX/Program/ProgramStateCache.h"
#include "Emu/RSX/Program/ShaderParam.h"
#include "Utilities/File.h"

using CGprofile = u32;
using CGbool = s32;
using CGresource = u32;
using CGenum = u32;
using CGtype = u32;
using CGbitfield = u32;
using CGbitfield16 = u16;
using CGint = s32;
using CGuint = u32;

using CgBinaryOffset = CGuint;
using CgBinarySize = CgBinaryOffset;
using CgBinaryEmbeddedConstantOffset = CgBinaryOffset;
using CgBinaryFloatOffset = CgBinaryOffset;
using CgBinaryStringOffset = CgBinaryOffset;
using CgBinaryParameterOffset = CgBinaryOffset;

using CgBinaryParameter = struct CgBinaryParameter;
using CgBinaryEmbeddedConstant = struct CgBinaryEmbeddedConstant;
using CgBinaryVertexProgram = struct CgBinaryVertexProgram;
using CgBinaryFragmentProgram = struct CgBinaryFragmentProgram;
using CgBinaryProgram = struct CgBinaryProgram;

// fragment programs have their constants embedded in the microcode
struct CgBinaryEmbeddedConstant
{
	be_t<u32> ucodeCount;       // occurrences
	be_t<u32> ucodeOffset[1];   // offsets that need to be patched follow
};

// describe a binary program parameter (CgParameter is opaque)
struct CgBinaryParameter
{
	CGtype                          type;          // cgGetParameterType()
	CGresource                      res;           // cgGetParameterResource()
	CGenum                          var;           // cgGetParameterVariability()
	CGint                           resIndex;      // cgGetParameterResourceIndex()
	CgBinaryStringOffset            name;          // cgGetParameterName()
	CgBinaryFloatOffset             defaultValue;  // default constant value
	CgBinaryEmbeddedConstantOffset  embeddedConst; // embedded constant information
	CgBinaryStringOffset            semantic;      // cgGetParameterSemantic()
	CGenum                          direction;     // cgGetParameterDirection()
	CGint                           paramno;       // 0..n: cgGetParameterIndex() -1: globals
	CGbool                          isReferenced;  // cgIsParameterReferenced()
	CGbool                          isShared;	   // cgIsParameterShared()
};

// attributes needed for vshaders
struct CgBinaryVertexProgram
{
	CgBinarySize  instructionCount;       // #instructions
	CgBinarySize  instructionSlot;        // load address (indexed reads!)
	CgBinarySize  registerCount;          // R registers count
	CGbitfield  attributeInputMask;       // attributes vs reads from
	CGbitfield  attributeOutputMask;      // attributes vs writes (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	CGbitfield  userClipMask;             // user clip plane enables (for SET_USER_CLIP_PLANE_CONTROL)
};

typedef enum
{
	CgBinaryPTTNone = 0,
	CgBinaryPTT2x16 = 1,
	CgBinaryPTT1x32 = 2
} CgBinaryPartialTexType;

// attributes needed for pshaders
struct CgBinaryFragmentProgram
{
	CgBinarySize instructionCount;        // #instructions
	CGbitfield   attributeInputMask;      // attributes fp reads (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	CGbitfield   partialTexType;          // texid 0..15 use two bits each marking whether the texture format requires partial load: see CgBinaryPartialTexType
	CGbitfield16 texCoordsInputMask;      // tex coords used by frag prog. (tex<n> is bit n)
	CGbitfield16 texCoords2D;             // tex coords that are 2d        (tex<n> is bit n)
	CGbitfield16 texCoordsCentroid;       // tex coords that are centroid  (tex<n> is bit n)
	u8           registerCount;           // R registers count
	u8           outputFromH0;            // final color from R0 or H0
	u8           depthReplace;            // fp generated z depth value
	u8           pixelKill;               // fp uses kill operations
};

struct CgBinaryProgram
{
	// vertex/pixel shader identification (BE/LE as well)
	CGprofile profile;

	// binary revision (used to verify binary and driver structs match)
	CgBinarySize binaryFormatRevision;

	// total size of this struct including profile and totalSize field
	CgBinarySize totalSize;

	// parameter usually queried using cgGet[First/Next]LeafParameter
	CgBinarySize parameterCount;
	CgBinaryParameterOffset parameterArray;

	// depending on profile points to a CgBinaryVertexProgram or CgBinaryFragmentProgram struct
	CgBinaryOffset program;

	// raw ucode data
	CgBinarySize    ucodeSize;
	CgBinaryOffset  ucode;

	// variable length data follows
	u8 data[1];
};

class CgBinaryDisasm
{
	OPDEST dst;
	SRC0 src0;
	SRC1 src1;
	SRC2 src2;

	D0 d0;
	D1 d1;
	D2 d2;
	D3 d3;
	SRC src[3];

	std::string m_path; // used for FP decompiler thread, delete this later

	std::vector<char> m_buffer;
	std::string m_arb_shader;
	std::string m_glsl_shader;
	std::string m_dst_reg_name;

	// FP members
	u32 m_offset = 0;
	u32 m_opcode = 0;
	u32 m_step = 0;
	u32 m_size = 0;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;
	std::vector<u32> m_loop_end_offsets;

	// VP members
	u32 m_sca_opcode = 0;
	u32 m_vec_opcode = 0;
	static constexpr usz m_max_instr_count = 512;
	usz m_instr_count = 0;
	std::vector<u32> m_data;

public:
	std::string GetArbShader() const { return m_arb_shader; }
	std::string GetGlslShader() const { return m_glsl_shader; }

	// FP functions
	std::string GetMask() const;
	void AddCodeAsm(const std::string& code);
	std::string AddRegDisAsm(u32 index, int fp16) const;
	std::string AddConstDisAsm();
	std::string AddTexDisAsm() const;
	std::string FormatDisAsm(const std::string& code);
	std::string GetCondDisAsm() const;
	template<typename T> std::string GetSrcDisAsm(T src);

	// VP functions
	std::string GetMaskDisasm(bool is_sca) const;
	std::string GetVecMaskDisasm() const;
	std::string GetScaMaskDisasm() const;
	std::string GetDSTDisasm(bool is_sca = false) const;
	std::string GetSRCDisasm(u32 n) const;
	static std::string GetTexDisasm();
	std::string GetCondDisasm() const;
	std::string AddAddrMaskDisasm() const;
	std::string AddAddrRegDisasm() const;
	u32 GetAddrDisasm() const;
	std::string FormatDisasm(const std::string& code) const;
	void AddScaCodeDisasm(const std::string& code = "");
	void AddVecCodeDisasm(const std::string& code = "");
	void AddCodeCondDisasm(const std::string& dst, const std::string& src);
	void AddCodeDisasm(const std::string& code);
	void SetDSTDisasm(bool is_sca, const std::string& value);
	void SetDSTVecDisasm(const std::string& code);
	void SetDSTScaDisasm(const std::string& code);


	CgBinaryDisasm(const std::string& path);

	template <typename T>
	CgBinaryDisasm(const std::span<T>& data)
		: m_path("<raw>")
	{
		m_buffer.resize(data.size_bytes());
		std::memcpy(m_buffer.data(), data.data(), data.size_bytes());
	}

	~CgBinaryDisasm() = default;

	template<typename T>
	T& GetCgRef(const u32 offset)
	{
		return reinterpret_cast<T&>(m_buffer[offset]);
	}

	static std::string GetCgParamType(u32 type);
	std::string GetCgParamName(u32 offset) const;
	std::string GetCgParamRes(u32 /*offset*/) const;
	std::string GetCgParamSemantic(u32 offset) const;
	std::string GetCgParamValue(u32 offset, u32 end_offset) const;

	void ConvertToLE(CgBinaryProgram& prog);
	void BuildShaderBody(bool include_glsl = true);

	static u32 GetData(const u32 d) { return d << 16 | d >> 16; }
	void TaskFP();
	void TaskVP();
};
