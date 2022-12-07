#pragma once
#include <sstream>

#include "GLSLTypes.h"
#include "ShaderParam.h"

namespace rsx
{
	// TODO: Move this somewhere else once more compilers are supported other than glsl
	enum texture_control_bits
	{
		GAMMA_A = 0,
		GAMMA_R,
		GAMMA_G,
		GAMMA_B,
		ALPHAKILL,
		RENORMALIZE,
		EXPAND_A,
		EXPAND_R,
		EXPAND_G,
		EXPAND_B,
		DEPTH_FLOAT,
		DEPTH_COMPARE_OP,
		DEPTH_COMPARE_1,
		DEPTH_COMPARE_2,
		FILTERED_MAG,
		FILTERED_MIN,
		UNNORMALIZED_COORDS,

		GAMMA_CTRL_MASK = (1 << GAMMA_R) | (1 << GAMMA_G) | (1 << GAMMA_B) | (1 << GAMMA_A),
		EXPAND_MASK = (1 << EXPAND_R) | (1 << EXPAND_G) | (1 << EXPAND_B) | (1 << EXPAND_A),
		EXPAND_OFFSET = EXPAND_A
	};

	enum ROP_control_bits : u32
	{
		// Commands. These trigger explicit action.
		ALPHA_TEST_ENABLE_BIT        = 0,
		SRGB_FRAMEBUFFER_BIT         = 1,
		ALPHA_TO_COVERAGE_ENABLE_BIT = 2,
		POLYGON_STIPPLE_ENABLE_BIT   = 3,

		// Auxilliary config
		INT_FRAMEBUFFER_BIT          = 16,
		MSAA_WRITE_ENABLE_BIT        = 17,

		// Data
		ALPHA_FUNC_OFFSET            = 18,
		MSAA_SAMPLE_CTRL_OFFSET      = 21,

		// Data lengths
		ALPHA_FUNC_NUM_BITS          = 3,
		MSAA_SAMPLE_CTRL_NUM_BITS    = 2,

		// Meta
		ROP_CMD_MASK                 = 0xF // Commands are encoded in the lower 16 bits
	};

	struct ROP_control_t
	{
		u32 value = 0;

		void enable_alpha_test() { value |= (1u << ROP_control_bits::ALPHA_TEST_ENABLE_BIT); }
		void enable_framebuffer_sRGB() { value |= (1u << ROP_control_bits::SRGB_FRAMEBUFFER_BIT); }
		void enable_alpha_to_coverage() { value |= (1u << ROP_control_bits::ALPHA_TO_COVERAGE_ENABLE_BIT); }
		void enable_polygon_stipple() { value |= (1u << ROP_control_bits::POLYGON_STIPPLE_ENABLE_BIT); }

		void enable_framebuffer_INT() { value |= (1u << ROP_control_bits::INT_FRAMEBUFFER_BIT); }
		void enable_MSAA_writes() { value |= (1u << ROP_control_bits::MSAA_WRITE_ENABLE_BIT); }

		void set_alpha_test_func(uint func) { value |= (func << ROP_control_bits::ALPHA_FUNC_OFFSET); }
		void set_msaa_control(uint ctrl) { value |= (ctrl << ROP_control_bits::MSAA_SAMPLE_CTRL_OFFSET); }
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
