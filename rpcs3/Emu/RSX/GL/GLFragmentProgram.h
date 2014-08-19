#pragma once
#include "GLShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"

struct GLFragmentDecompilerThread : public ThreadBase
{
	union OPDEST
	{
		u32 HEX;

		struct
		{
			u32 end                 : 1; // Set to 1 if this is the last instruction
			u32 dest_reg            : 6; // Destination register index
			u32 fp16                : 1; // Destination is a half register (H0 to H47)
			u32 set_cond            : 1; // Condition Code Registers (CC0 and CC1) are updated
			u32 mask_x              : 1;
			u32 mask_y              : 1;
			u32 mask_z              : 1;
			u32 mask_w              : 1;
			u32 src_attr_reg_num    : 4;
			u32 tex_num             : 4;
			u32 exp_tex             : 1; // _bx2
			u32 prec                : 2;
			u32 opcode              : 6;
			u32 no_dest             : 1;
			u32 saturate            : 1; // _sat
		};
	} dst;

	union SRC0
	{
		u32 HEX;

		struct
		{
			u32 reg_type            : 2;
			u32 tmp_reg_index       : 6;
			u32 fp16                : 1;
			u32 swizzle_x           : 2;
			u32 swizzle_y           : 2;
			u32 swizzle_z           : 2;
			u32 swizzle_w           : 2;
			u32 neg                 : 1;
			u32 exec_if_lt          : 1;
			u32 exec_if_eq          : 1;
			u32 exec_if_gr          : 1;
			u32 cond_swizzle_x      : 2;
			u32 cond_swizzle_y      : 2;
			u32 cond_swizzle_z      : 2;
			u32 cond_swizzle_w      : 2;
			u32 abs                 : 1;
			u32 cond_mod_reg_index  : 1;
			u32 cond_reg_index      : 1;
		};
	} src0;

	union SRC1
	{
		u32 HEX;

		struct
		{
			u32 reg_type            : 2;
			u32 tmp_reg_index       : 6;
			u32 fp16                : 1;
			u32 swizzle_x           : 2;
			u32 swizzle_y           : 2;
			u32 swizzle_z           : 2;
			u32 swizzle_w           : 2;
			u32 neg                 : 1;
			u32 abs                 : 1;
			u32 input_mod_src0      : 3;
			u32                     : 6;
			u32 scale               : 3;
			u32 opcode_is_branch    : 1;
		};

		struct
		{
			u32 else_offset : 31;
			u32             : 1;
		};

		// LOOP, REP
		struct
		{
			u32              : 2;
			u32 end_counter  : 8; // End counter value for LOOP or rep count for REP
			u32 init_counter : 8; // Initial counter value for LOOP
			u32              : 1;
			u32 increment    : 8; // Increment value for LOOP
		};
	} src1;

	union SRC2
	{
		u32 HEX;

		u32 end_offset;

		struct
		{
			u32 reg_type         : 2;
			u32 tmp_reg_index    : 6;
			u32 fp16             : 1;
			u32 swizzle_x        : 2;
			u32 swizzle_y        : 2;
			u32 swizzle_z        : 2;
			u32 swizzle_w        : 2;
			u32 neg              : 1;
			u32 abs              : 1;
			u32 addr_reg         : 11;
			u32 use_index_reg    : 1;
			u32 perspective_corr : 1;
		};
	} src2;

	std::string main;
	std::string& m_shader;
	GLParamArray& m_parr;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;
	u32 m_ctrl;
	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	GLFragmentDecompilerThread(std::string& shader, GLParamArray& parr, u32 addr, u32& size, u32 ctrl)
		: ThreadBase("Fragment Shader Decompiler Thread")
		, m_shader(shader)
		, m_parr(parr)
		, m_addr(addr)
		, m_size(size) 
		, m_const_index(0)
		, m_location(0)
		, m_ctrl(ctrl)
	{
		m_size = 0;
	}

	std::string GetMask();

	void SetDst(std::string code, bool append_mask = true);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	std::string Format(const std::string& code);

	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	virtual void Task();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
};

/** Storage for an Fragment Program in the process of of recompilation.
 *  This class calls OpenGL functions and should only be used from the RSX/Graphics thread.
 */
class GLShaderProgram
{
public:
	GLShaderProgram();
	~GLShaderProgram();

	/**
	 * Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	 * @param prog RSXShaderProgram specifying the location and size of the shader in memory
	 */
	void Decompile(RSXShaderProgram& prog);

	/**
	* Asynchronously decompile a fragment shader located in the PS3's Memory.
	* When this function is called you must call Wait() before GetShaderText() will return valid data.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	void DecompileAsync(RSXShaderProgram& prog);

	/** Wait for the decompiler task to complete decompilation. */
	void Wait();

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile();

	/** Get the source text for this shader */
	inline const std::string& GetShaderText() const { return m_shader; }

	/**
	 * Set the source text for this shader
	 * @param shaderText supplied shader text
	 */
	inline void SetShaderText(const std::string& shaderText) { m_shader = shaderText; }

	/** Get the OpenGL id this shader is bound to */
	inline u32 GetId() const { return m_id; }

	/** 
	 * Set the OpenGL id this shader is bound to
	 * @param id supplied id
	 */
	inline void SetId(const u32 id) { m_id = id; }

private:
	/** Threaded fragment shader decompiler responsible for decompiling this program */
	GLFragmentDecompilerThread* m_decompiler_thread;

	/** Shader parameter storage */
	GLParamArray m_parr;

	/** Text of our decompiler shader */
	std::string m_shader;

	/** OpenGL id this shader is bound to */
	u32 m_id;

	/** Deletes the shader and any stored information */
	void Delete();
};
