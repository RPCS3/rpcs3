#pragma once
#include "ShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include <sstream>

/**
 * This class is used to translate RSX Fragment program to GLSL/HLSL code
 * Backend with text based shader can subclass this class and implement :
 * - virtual std::string getFloatTypeName(size_t elementCount) = 0;
 * - virtual std::string getFunction(enum class FUNCTION) = 0;
 * - virtual std::string saturate(const std::string &code) = 0;
 * - virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) = 0;
 * - virtual void insertHeader(std::stringstream &OS) = 0;
 * - virtual void insertIntputs(std::stringstream &OS) = 0;
 * - virtual void insertOutputs(std::stringstream &OS) = 0;
 * - virtual void insertConstants(std::stringstream &OS) = 0;
 * - virtual void insertMainStart(std::stringstream &OS) = 0;
 * - virtual void insertMainEnd(std::stringstream &OS) = 0;
 */
class FragmentProgramDecompiler
{
	struct temp_register
	{
		bool aliased_r0 = false;
		bool aliased_h0 = false;
		bool aliased_h1 = false;
		bool last_write_half = false;

		u32 real_index = UINT32_MAX;

		void tag(u32 index, bool half_register)
		{
			if (half_register)
			{
				last_write_half = true;

				if (index & 1)
					aliased_h1 = true;
				else
					aliased_h0 = true;
			}
			else
			{
				aliased_r0 = true;
				last_write_half = false;
			}

			if (real_index == UINT32_MAX)
			{
				if (half_register)
					real_index = index >> 1;
				else
					real_index = index;
			}
		}

		bool requires_gather(bool xy, bool zw) const
		{
			//Data fetched from the single precision register requires merging of the two half registers
			//TODO: Check individual swizzle channels
			if (aliased_h0 && xy || aliased_h1 && zw)
				return last_write_half;

			return false;
		}

		bool requires_split(u32 /*index*/) const
		{
			//Data fetched from any of the two half registers requires sync with the full register
			if (!last_write_half && aliased_r0)
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
				ret = reg + " = gather(" + h0 + ", " + h1 + ");";
			else if (aliased_h0)
				ret = reg + ".xy = gather(" + h0 + ");";
			else if (aliased_h1)
				ret = reg + ".zw = gather(" + h1 + ");";

			last_write_half = false;
			aliased_h0 = false;
			aliased_h1 = false;
			return ret;
		}
	};

	OPDEST dst;
	SRC0 src0;
	SRC1 src1;
	SRC2 src2;

	std::string main;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;

	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	std::array<temp_register, 24> temp_registers;

	std::string GetMask();

	void SetDst(std::string code, bool append_mask = true);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	void AddFlowOp(std::string code);
	std::string Format(const std::string& code);

	//Technically a temporary workaround until we know what type3 is
	std::string AddType3();

	//Prevent division by zero by catching denormals
	//Simpler variant where input and output are expected to be positive
	std::string NotZero(const std::string& code);
	std::string NotZeroPositive(const std::string& code);
	
	//Prevents operations from overflowing the desired range (tested with fp_dynamic3 autotest sample, DS2 for src1.input_prec_mod)
	std::string ClampValue(const std::string& code, u32 precision);

	/**
	* Returns true if the dst set is not a vector (i.e only a single component)
	*/
	bool DstExpectsSca();

	void AddCodeCond(const std::string& dst, const std::string& src);
	std::string GetRawCond();
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }

	/**
	 * Emits code if opcode is an SCT one and returns true,
	 * otherwise do nothing and return false.
	 * NOTE: What does SCT means ???
	 */
	bool handle_sct(u32 opcode);

	/**
	* Emits code if opcode is an SCB one and returns true,
	* otherwise do nothing and return false.
	* NOTE: What does SCB means ???
	*/
	bool handle_scb(u32 opcode);

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

	/** returns string calling function where arguments are passed via
	 * $0 $1 $2 substring.
	 */
	virtual std::string getFunction(FUNCTION) = 0;

	/** returns string calling saturate function.
	*/
	virtual std::string saturate(const std::string &code) = 0;
	/** returns string calling comparison function on 2 args passed as strings.
	 */
	virtual std::string compareFunction(COMPARE, const std::string &, const std::string &) = 0;

	/** Insert header of shader file (eg #version, "system constants"...)
	 */
	virtual void insertHeader(std::stringstream &OS) = 0;
	/** Insert global declaration of fragments inputs.
	 */
	virtual void insertIntputs(std::stringstream &OS) = 0;
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
	ParamArray m_parr;
	FragmentProgramDecompiler(const RSXFragmentProgram &prog, u32& size);
	FragmentProgramDecompiler(const FragmentProgramDecompiler&) = delete;
	FragmentProgramDecompiler(FragmentProgramDecompiler&&) = delete;
	std::string Decompile();
};
