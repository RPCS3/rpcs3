#pragma once
#include "ShaderParam.h"
#include "FragmentProgramRegister.h"
#include "RSXFragmentProgram.h"

#include "Assembler/CFG.h"

#include <sstream>
#include <unordered_map>

/**
 * This class is used to translate RSX Fragment program to GLSL/HLSL code
 * Backend with text based shader can subclass this class and implement :
 * - virtual std::string getFloatTypeName(usz elementCount) = 0;
 * - virtual std::string getHalfTypeName(usz elementCount) = 0;
 * - virtual std::string getFunction(enum class FUNCTION) = 0;
 * - virtual std::string saturate(const std::string &code) = 0;
 * - virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) = 0;
 * - virtual void insertHeader(std::stringstream &OS) = 0;
 * - virtual void insertInputs(std::stringstream &OS) = 0;
 * - virtual void insertOutputs(std::stringstream &OS) = 0;
 * - virtual void insertConstants(std::stringstream &OS) = 0;
 * - virtual void insertMainStart(std::stringstream &OS) = 0;
 * - virtual void insertMainEnd(std::stringstream &OS) = 0;
 */
class FragmentProgramDecompiler
{
	enum OPFLAGS
	{
		no_src_mask = 1,
		src_cast_f32 = 2,
		skip_type_cast = 4,
		texture_ref = 8,

		op_extern = src_cast_f32 | skip_type_cast,
	};

	OPDEST dst;
	SRC0 src0;
	SRC1 src1;
	SRC2 src2;
	u32  opflags;

	const rsx::assembler::Instruction* m_instruction;

	std::string main;
	u32& m_size;
	u32 m_const_index = 0;
	u32 m_location = 0;
	bool m_is_valid_ucode = true;

	u32 m_loop_count;
	int m_code_level;
	std::unordered_map<u32, u32> m_constant_offsets;

	std::array<rsx::MixedPrecisionRegister, 64> temp_registers;

	std::string GetMask() const;

	void SetDst(std::string code, u32 flags = 0);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, bool fp16);
	bool HasReg(u32 index, bool fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	void AddFlowOp(const std::string& code);
	std::string Format(const std::string& code, bool ignore_redirects = false);

	// Support the transform-2d temp result for use with TEXBEM
	std::string AddX2d();

	// Prevents operations from overflowing the desired range (tested with fp_dynamic3 autotest sample, DS2 for src1.input_prec_mod)
	std::string ClampValue(const std::string& code, u32 precision);

	/**
	* Returns true if the dst set is not a vector (i.e only a single component)
	*/
	bool DstExpectsSca() const;

	void AddCodeCond(const std::string& lhs, const std::string& rhs);
	std::string GetRawCond();
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	static u32 GetData(const u32 d) { return d << 16 | d >> 16; }

	/**
	 * Emits code if opcode is an SCT/SCB one and returns true,
	 * otherwise do nothing and return false.
	 * NOTE: What does SCT means ???
	 */
	bool handle_sct_scb(u32 opcode);

	/**
	* Emits code if opcode is an TEX SRB one and returns true,
	* otherwise do nothing and return false.
	* NOTE: What does TEX SRB means ???
	*/
	bool handle_tex_srb(u32 opcode);

protected:
	const RSXFragmentProgram &m_prog;
	u32 m_ctrl = 0;

	/** returns the type name of float vectors.
	 */
	virtual std::string getFloatTypeName(usz elementCount) = 0;

	/** returns the type name of half vectors.
	 */
	virtual std::string getHalfTypeName(usz elementCount) = 0;

	/** returns string calling function where arguments are passed via
	 * $0 $1 $2 substring.
	 */
	virtual std::string getFunction(FUNCTION) = 0;

	/** returns string calling comparison function on 2 args passed as strings.
	 */
	virtual std::string compareFunction(COMPARE, const std::string &, const std::string &) = 0;

	/** Insert header of shader file (eg #version, "system constants"...)
	 */
	virtual void insertHeader(std::stringstream &OS) = 0;
	/** Insert global declaration of fragments inputs.
	 */
	virtual void insertInputs(std::stringstream &OS) = 0;
	/** insert global declaration of fragments outputs.
	*/
	virtual void insertOutputs(std::stringstream &OS) = 0;
	/** insert declaration of shader constants.
	*/
	virtual void insertConstants(std::stringstream &OS) = 0;
	/** insert helper function definitions.
	*/
	virtual void insertGlobalFunctions(std::stringstream &OS) = 0;
	/** insert beginning of main (signature, temporary declaration...)
	*/
	virtual void insertMainStart(std::stringstream &OS) = 0;
	/** insert end of main function (return value, output copy...)
	 */
	virtual void insertMainEnd(std::stringstream &OS) = 0;

public:
	enum : u16
	{
		in_wpos = (1 << 0),
		in_diff_color = (1 << 1),
		in_spec_color = (1 << 2),
		in_fogc = (1 << 3),
		in_tc0 = (1 << 4),
		in_tc1 = (1 << 5),
		in_tc2 = (1 << 6),
		in_tc3 = (1 << 7),
		in_tc4 = (1 << 8),
		in_tc5 = (1 << 9),
		in_tc6 = (1 << 10),
		in_tc7 = (1 << 11),
		in_tc8 = (1 << 12),
		in_tc9 = (1 << 13),
		in_ssa = (1 << 14)
	};

	struct
	{
		// Configuration properties (in)
		u16 in_register_mask = 0;

		u16 common_access_sampler_mask = 0;
		u16 shadow_sampler_mask = 0;
		u16 redirected_sampler_mask = 0;
		u16 multisampled_sampler_mask = 0;

		// Decoded properties (out)
		bool has_lit_op = false;
		bool has_gather_op = false;
		bool has_no_output = false;
		bool has_discard_op = false;
		bool has_tex_op = false;
		bool has_divsq = false;
		bool has_clamp = false;
		bool has_w_access = false;
		bool has_exp_tex_op = false;
		bool has_pkg = false;
		bool has_upg = false;
		bool has_dynamic_register_load = false;

		bool has_tex1D = false;
		bool has_tex2D = false;
		bool has_tex3D = false;
		bool has_texShadowProj = false;

		// Literal offsets
		std::vector<u32> constant_offsets;
	}
	properties;

	struct
	{
		bool has_native_half_support = false;
		bool emulate_depth_compare = false;
		bool has_low_precision_rounding = false;
	}
	device_props;

	ParamArray m_parr;
	FragmentProgramDecompiler(const RSXFragmentProgram &prog, u32& size);
	FragmentProgramDecompiler(const FragmentProgramDecompiler&) = delete;
	FragmentProgramDecompiler(FragmentProgramDecompiler&&) = delete;
	std::string Decompile();
};
