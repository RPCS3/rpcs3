#pragma once
#include "ShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include <sstream>

// Helper for GPR occupancy tracking
struct temp_register
{
	bool aliased_r0 = false;
	bool aliased_h0 = false;
	bool aliased_h1 = false;
	bool last_write_half[4] = { false, false, false, false };

	u32 real_index = UINT32_MAX;

	u32 h0_writes = 0u;   // Number of writes to the first 64-bits of the register
	u32 h1_writes = 0u;   // Number of writes to the last 64-bits of the register

	void tag(u32 index, bool half_register, bool x, bool y, bool z, bool w)
	{
		if (half_register)
		{
			if (index & 1)
			{
				if (x) last_write_half[2] = true;
				if (y) last_write_half[2] = true;
				if (z) last_write_half[3] = true;
				if (w) last_write_half[3] = true;

				aliased_h1 = true;
				h1_writes++;
			}
			else
			{
				if (x) last_write_half[0] = true;
				if (y) last_write_half[0] = true;
				if (z) last_write_half[1] = true;
				if (w) last_write_half[1] = true;

				aliased_h0 = true;
				h0_writes++;
			}
		}
		else
		{
			if (x) last_write_half[0] = false;
			if (y) last_write_half[1] = false;
			if (z) last_write_half[2] = false;
			if (w) last_write_half[3] = false;

			aliased_r0 = true;

			h0_writes++;
			h1_writes++;
		}

		if (real_index == UINT32_MAX)
		{
			if (half_register)
				real_index = index >> 1;
			else
				real_index = index;
		}
	}

	bool requires_gather(u8 channel) const
	{
		//Data fetched from the single precision register requires merging of the two half registers
		verify(HERE), channel < 4;
		if (aliased_h0 && channel < 2)
		{
			return last_write_half[channel];
		}

		if (aliased_h1 && channel > 1)
		{
			return last_write_half[channel];
		}

		return false;
	}

	bool requires_split(u32 /*index*/) const
	{
		//Data fetched from any of the two half registers requires sync with the full register
		if (!(last_write_half[0] || last_write_half[1]) && aliased_r0)
		{
			//r0 has been written to
			//TODO: Check for specific elements in real32 register
			return true;
		}

		return false;
	}

	std::string gather_r()
	{
		std::string h0 = "h" + std::to_string(real_index << 1);
		std::string h1 = "h" + std::to_string(real_index << 1 | 1);
		std::string reg = "r" + std::to_string(real_index);
		std::string ret = "//Invalid gather";

		if (aliased_h0 && aliased_h1)
			ret = "(gather(" + h0 + ", " + h1 + "))";
		else if (aliased_h0)
			ret = "(gather(" + h0 + "), " + reg + ".zw)";
		else if (aliased_h1)
			ret = "(" + reg + ".xy, gather(" + h1 + "))";

		return ret;
	}
};

/**
 * This class is used to translate RSX Fragment program to GLSL/HLSL code
 * Backend with text based shader can subclass this class and implement :
 * - virtual std::string getFloatTypeName(size_t elementCount) = 0;
 * - virtual std::string getHalfTypeName(size_t elementCount) = 0;
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

	std::string main;
	u32& m_size;
	u32 m_const_index = 0;
	u32 m_offset;
	u32 m_location = 0;

	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	std::array<temp_register, 64> temp_registers;

	std::string GetMask();

	void SetDst(std::string code, u32 flags = 0);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, bool fp16);
	bool HasReg(u32 index, bool fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	void AddFlowOp(const std::string& code);
	std::string Format(const std::string& code, bool ignore_redirects = false);

	//Technically a temporary workaround until we know what type3 is
	std::string AddType3();

	//Support the transform-2d temp result for use with TEXBEM
	std::string AddX2d();

	//Prevents operations from overflowing the desired range (tested with fp_dynamic3 autotest sample, DS2 for src1.input_prec_mod)
	std::string ClampValue(const std::string& code, u32 precision);

	/**
	* Returns true if the dst set is not a vector (i.e only a single component)
	*/
	bool DstExpectsSca();

	void AddCodeCond(const std::string& lhs, const std::string& rhs);
	std::string GetRawCond();
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }

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

	u32 m_2d_sampled_textures = 0;        //Mask of textures sampled as texture2D (conflicts with samplerShadow fetch)
	u32 m_shadow_sampled_textures = 0;    //Mask of textures sampled as boolean shadow comparisons

	/** returns the type name of float vectors.
	 */
	virtual std::string getFloatTypeName(size_t elementCount) = 0;

	/** returns the type name of half vectors.
	 */
	virtual std::string getHalfTypeName(size_t elementCount) = 0;

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
	/** insert helper functin definitions.
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
		u16  in_register_mask = 0;
		bool has_lit_op = false;
		bool has_gather_op = false;
		bool has_no_output = false;
		bool has_discard_op = false;
		bool has_tex_op = false;
		bool has_divsq = false;
		bool has_clamp = false;
		bool has_w_access = false;
	}
	properties;

	struct
	{
		bool has_native_half_support = false;
		bool emulate_depth_compare = false;
	}
	device_props;

	ParamArray m_parr;
	FragmentProgramDecompiler(const RSXFragmentProgram &prog, u32& size);
	FragmentProgramDecompiler(const FragmentProgramDecompiler&) = delete;
	FragmentProgramDecompiler(FragmentProgramDecompiler&&) = delete;
	std::string Decompile();
};
