#pragma once
#include <sstream>
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GL/GLVertexProgram.h"
#include "Emu/RSX/GL/GLFragmentProgram.h"

using CGprofile = be_t<u32>;
using CGbool = be_t<s32>;
using CGresource = be_t<u32>;
using CGenum = be_t<u32>;
using CGtype = be_t<u32>;

using CgBinaryOffset = be_t<u32>;
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
	be_t<s32>                       resIndex;      // cgGetParameterResourceIndex()
	CgBinaryStringOffset            name;          // cgGetParameterName()
	CgBinaryFloatOffset             defaultValue;  // default constant value
	CgBinaryEmbeddedConstantOffset  embeddedConst; // embedded constant information
	CgBinaryStringOffset            semantic;      // cgGetParameterSemantic()
	CGenum                          direction;     // cgGetParameterDirection()
	be_t<s32>                       paramno;       // 0..n: cgGetParameterIndex() -1: globals
	CGbool                          isReferenced;  // cgIsParameterReferenced()
	CGbool                          isShared;	   // cgIsParameterShared()
};

// attributes needed for vshaders
struct CgBinaryVertexProgram
{
	be_t<u32>  instructionCount;         // #instructions
	be_t<u32>  instructionSlot;          // load address (indexed reads!)
	be_t<u32>  registerCount;            // R registers count
	be_t<u32>  attributeInputMask;       // attributes vs reads from
	be_t<u32>  attributeOutputMask;      // attributes vs writes (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	be_t<u32>  userClipMask;             // user clip plane enables (for SET_USER_CLIP_PLANE_CONTROL)
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
	be_t<u32> instructionCount;        // #instructions
	be_t<u32> attributeInputMask;      // attributes fp reads (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	be_t<u32> partialTexType;          // texid 0..15 use two bits each marking whether the texture format requires partial load: see CgBinaryPartialTexType
	be_t<u16> texCoordsInputMask;      // tex coords used by frag prog. (tex<n> is bit n)
	be_t<u16> texCoords2D;             // tex coords that are 2d        (tex<n> is bit n)
	be_t<u16> texCoordsCentroid;       // tex coords that are centroid  (tex<n> is bit n)
	u8        registerCount;           // R registers count
	u8        outputFromH0;            // final color from R0 or H0
	u8        depthReplace;            // fp generated z depth value
	u8        pixelKill;               // fp uses kill operations
};

struct CgBinaryProgram
{
	// vertex/pixel shader identification (BE/LE as well)
	CGprofile profile;

	// binary revision (used to verify binary and driver structs match)
	be_t<u32> binaryFormatRevision;

	// total size of this struct including profile and totalSize field
	be_t<u32> totalSize;

	// parameter usually queried using cgGet[First/Next]LeafParameter
	be_t<u32> parameterCount;
	CgBinaryParameterOffset parameterArray;

	// depending on profile points to a CgBinaryVertexProgram or CgBinaryFragmentProgram struct
	CgBinaryOffset program;

	// raw ucode data
	be_t<u32>    ucodeSize;
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

	u8* m_buffer = nullptr;
	std::size_t m_buffer_size = 0;
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
	u32 m_sca_opcode;
	u32 m_vec_opcode;
	static const size_t m_max_instr_count = 512;
	size_t m_instr_count;
	std::vector<u32> m_data;

public:
	std::string GetArbShader() const { return m_arb_shader; }
	std::string GetGlslShader() const { return m_glsl_shader; }

	// FP functions
	std::string GetMask();
	void AddCodeAsm(const std::string& code);
	std::string AddRegDisAsm(u32 index, int fp16);
	std::string AddConstDisAsm();
	std::string AddTexDisAsm();
	std::string FormatDisAsm(const std::string& code);
	std::string GetCondDisAsm();
	template<typename T> std::string GetSrcDisAsm(T src);

	// VP functions
	std::string GetMaskDisasm(bool is_sca);
	std::string GetVecMaskDisasm();
	std::string GetScaMaskDisasm();
	std::string GetDSTDisasm(bool is_sca = false);
	std::string GetSRCDisasm(u32 n);
	std::string GetTexDisasm();
	std::string GetCondDisasm();
	std::string AddAddrMaskDisasm();
	std::string AddAddrRegDisasm();
	u32 GetAddrDisasm();
	std::string FormatDisasm(const std::string& code);
	void AddScaCodeDisasm(const std::string& code = "");
	void AddVecCodeDisasm(const std::string& code = "");
	void AddCodeCondDisasm(const std::string& dst, const std::string& src);
	void AddCodeDisasm(const std::string& code);
	void SetDSTDisasm(bool is_sca, const std::string& value);
	void SetDSTVecDisasm(const std::string& code);
	void SetDSTScaDisasm(const std::string& code);


	CgBinaryDisasm(const std::string& path)
		: m_path(path)
	{
		fs::file f(path);
		if (!f) return;

		m_buffer_size = f.size();
		m_buffer = new u8[m_buffer_size];
		f.read(m_buffer, m_buffer_size);
		m_arb_shader += fmt::format("Loading... [%s]\n", path.c_str());
	}

	~CgBinaryDisasm()
	{
		delete[] m_buffer;
	}

	std::string GetCgParamType(u32 type) const
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

	std::string GetCgParamName(u32 offset) const
	{
		std::stringstream str_stream;
		std::string name;
		while (m_buffer[offset] != 0)
		{
			str_stream << m_buffer[offset];
			offset++;
		}
		name += str_stream.str();
		return name;
	}

	std::string GetCgParamRes(u32 /*offset*/) const
	{
		// rsx_log.warning("GetCgParamRes offset 0x%x", offset);
		// TODO
		return "";
	}

	std::string GetCgParamSemantic(u32 offset) const
	{
		std::stringstream str_stream;
		std::string semantic;
		while (m_buffer[offset] != 0)
		{
			str_stream << m_buffer[offset];
			offset++;
		}
		semantic += str_stream.str();
		return semantic;
	}

	std::string GetCgParamValue(u32 offset, u32 end_offset) const
	{
		std::string offsets = "offsets:";

		u32 num = 0;
		offset += 6;
		while (offset < end_offset)
		{
			offsets += fmt::format(" %d,", m_buffer[offset] << 8 | m_buffer[offset + 1]);
			offset += 4;
			num++;
		}

		if (num > 4)
			return "";

		offsets.pop_back();
		return fmt::format("num %d ", num) + offsets;
	}

	template<typename T>
	T& GetCgRef(const u32 offset)
	{
		return reinterpret_cast<T&>(m_buffer[offset]);
	}

	void BuildShaderBody()
	{
		ParamArray param_array;

		auto& prog = GetCgRef<CgBinaryProgram>(0);

		if (prog.profile == 7004u)
		{
			auto& fprog = GetCgRef<CgBinaryFragmentProgram>(prog.program);
			m_arb_shader += "\n";
			m_arb_shader += fmt::format("# binaryFormatRevision 0x%x\n", prog.binaryFormatRevision);
			m_arb_shader += fmt::format("# profile sce_fp_rsx\n");
			m_arb_shader += fmt::format("# parameterCount %d\n", prog.parameterCount);
			m_arb_shader += fmt::format("# instructionCount %d\n", fprog.instructionCount);
			m_arb_shader += fmt::format("# attributeInputMask 0x%x\n", fprog.attributeInputMask);
			m_arb_shader += fmt::format("# registerCount %d\n\n", fprog.registerCount);

			CgBinaryParameterOffset offset = prog.parameterArray;
			for (u32 i = 0; i < prog.parameterCount; i++)
			{
				auto& fparam = GetCgRef<CgBinaryParameter>(offset);

				std::string param_type = GetCgParamType(fparam.type) + " ";
				std::string param_name = GetCgParamName(fparam.name) + " ";
				std::string param_res = GetCgParamRes(fparam.res) + " ";
				std::string param_semantic = GetCgParamSemantic(fparam.semantic) + " ";
				std::string param_const = GetCgParamValue(fparam.embeddedConst, fparam.name);

				m_arb_shader += fmt::format("#%d ", i) + param_type + param_name + param_semantic + param_const + "\n";

				offset += u32{sizeof(CgBinaryParameter)};
			}

			m_arb_shader += "\n";
			m_offset = prog.ucode;
			TaskFP();

			// reload binary data in the virtual memory, temporary solution
			{
				u32 ptr;
				{
					fs::file f(m_path);
					if (!f) return;

					size_t size = f.size();
					vm::init();
					ptr = vm::alloc(static_cast<u32>(size), vm::main);
					f.read(vm::base(ptr), size);
				}

				auto& vmprog = vm::_ref<CgBinaryProgram>(ptr);
				auto& vmfprog = vm::_ref<CgBinaryFragmentProgram>(ptr + vmprog.program);
				u32 size;
				u32 ctrl = (vmfprog.outputFromH0 ? 0 : 0x40) | (vmfprog.depthReplace ? 0xe : 0);
				std::vector<rsx::texture_dimension_extended> td;
				RSXFragmentProgram prog;
				prog.ucode_length = 0, prog.addr = vm::base(ptr + vmprog.ucode), prog.offset = 0, prog.ctrl = ctrl;
				GLFragmentDecompilerThread(m_glsl_shader, param_array, prog, size).Task();
				vm::close();
			}
		}

		else
		{
			auto& vprog = GetCgRef<CgBinaryVertexProgram>(prog.program);
			m_arb_shader += "\n";
			m_arb_shader += fmt::format("# binaryFormatRevision 0x%x\n", prog.binaryFormatRevision);
			m_arb_shader += fmt::format("# profile sce_vp_rsx\n");
			m_arb_shader += fmt::format("# parameterCount %d\n", prog.parameterCount);
			m_arb_shader += fmt::format("# instructionCount %d\n", vprog.instructionCount);
			m_arb_shader += fmt::format("# registerCount %d\n", vprog.registerCount);
			m_arb_shader += fmt::format("# attributeInputMask 0x%x\n", vprog.attributeInputMask);
			m_arb_shader += fmt::format("# attributeOutputMask 0x%x\n\n", vprog.attributeOutputMask);

			CgBinaryParameterOffset offset = prog.parameterArray;
			for (u32 i = 0; i < prog.parameterCount; i++)
			{
				auto& vparam = GetCgRef<CgBinaryParameter>(offset);

				std::string param_type = GetCgParamType(vparam.type) + " ";
				std::string param_name = GetCgParamName(vparam.name) + " ";
				std::string param_res = GetCgParamRes(vparam.res) + " ";
				std::string param_semantic = GetCgParamSemantic(vparam.semantic) + " ";
				std::string param_const = GetCgParamValue(vparam.embeddedConst, vparam.name);

				m_arb_shader += fmt::format("#%d ", i) + param_type + param_name + param_semantic + param_const + "\n";

				offset += u32{sizeof(CgBinaryParameter)};
			}

			m_arb_shader += "\n";
			m_offset = prog.ucode;

			u32* vdata = reinterpret_cast<u32*>(&m_buffer[m_offset]);
			verify(HERE), (m_buffer_size - m_offset) % sizeof(u32) == 0;
			for (u32 i = 0; i < (m_buffer_size - m_offset) / sizeof(u32); i++)
			{
				vdata[i] = std::bit_cast<u32, be_t<u32>>(vdata[i]);
			}

			for (u32 i = 0; i < prog.ucodeSize / sizeof(u32); i++)
			{
				m_data.push_back(vdata[i]);
			}

			TaskVP();
			RSXVertexProgram prog;
			prog.data = m_data;
			GLVertexDecompilerThread(prog, m_glsl_shader, param_array).Task();
		}
	}

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
	void TaskFP();
	void TaskVP();
};
