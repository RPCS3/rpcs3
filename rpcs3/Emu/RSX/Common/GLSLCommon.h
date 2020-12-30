#pragma once
#include <sstream>

#include "GLSLTypes.h"
#include "ShaderParam.h"

namespace rsx
{
	// TODO: Move this somewhere else once more compilers are supported other than glsl
	enum texture_control_bits
	{
		GAMMA_R = 0,
		GAMMA_G,
		GAMMA_B,
		GAMMA_A,
		ALPHAKILL,
		RENORMALIZE,
		EXPAND_A,
		EXPAND_R,
		EXPAND_G,
		EXPAND_B,
		DEPTH_FLOAT,

		GAMMA_CTRL_MASK = (1 << GAMMA_R) | (1 << GAMMA_G) | (1 << GAMMA_B) | (1 << GAMMA_A),
		EXPAND_MASK = (1 << EXPAND_R) | (1 << EXPAND_G) | (1 << EXPAND_B) | (1 << EXPAND_A),
		EXPAND_OFFSET = EXPAND_A
	};
}

namespace program_common
{
	void insert_compare_op(std::ostream& OS, bool low_precision);
	void insert_compare_op_vector(std::ostream& OS);
	void insert_fog_declaration(std::ostream& OS, const std::string& wide_vector_type, const std::string& input_coord, bool declare = false);
}

namespace glsl
{
	std::string getFloatTypeNameImpl(usz elementCount);
	std::string getHalfTypeNameImpl(usz elementCount);
	std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1, bool scalar = false);
	void insert_vertex_input_fetch(std::stringstream& OS, glsl_rules rules, bool glsl4_compliant=true);
	void insert_rop_init(std::ostream& OS);
	void insert_rop(std::ostream& OS, const shader_properties& props);
	void insert_glsl_legacy_function(std::ostream& OS, const shader_properties& props);
	void insert_fog_declaration(std::ostream& OS);
	std::string getFunctionImpl(FUNCTION f);
	void insert_subheader_block(std::ostream& OS);
}
