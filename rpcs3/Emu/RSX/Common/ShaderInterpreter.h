#pragma once
#include "Utilities/StrFmt.h"

namespace program_common
{
	namespace interpreter
	{
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
