#pragma once
#include "Utilities/StrFmt.h"

namespace program_common
{
	namespace interpreter
	{
		enum compiler_option
		{
			COMPILER_OPT_ENABLE_TEXTURES = 1,
			COMPILER_OPT_ENABLE_DEPTH_EXPORT = 2,
			COMPILER_OPT_ENABLE_F32_EXPORT = 4,
			COMPILER_OPT_ENABLE_ALPHA_TEST_GE = 8,
			COMPILER_OPT_ENABLE_ALPHA_TEST_G = 16,
			COMPILER_OPT_ENABLE_ALPHA_TEST_LE = 32,
			COMPILER_OPT_ENABLE_ALPHA_TEST_L = 64,
			COMPILER_OPT_ENABLE_ALPHA_TEST_EQ = 128,
			COMPILER_OPT_ENABLE_ALPHA_TEST_NE = 256,
			COMPILER_OPT_ENABLE_FLOW_CTRL = 512,
			COMPILER_OPT_ENABLE_PACKING = 1024,
			COMPILER_OPT_ENABLE_KIL = 2048,
			COMPILER_OPT_ENABLE_STIPPLING = 4096
		};

		static std::string get_vertex_interpreter()
		{
			const char* s =
			#include "Interpreter/VertexInterpreter.glsl"
			;
			return s;
		}

		static std::string get_fragment_interpreter()
		{
			const char* s =
			#include "Interpreter/FragmentInterpreter.glsl"
			;
			return s;
		}
	}
}
