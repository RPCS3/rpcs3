#pragma once
#include "Utilities/StrFmt.h"

namespace program_common
{
	namespace interpreter
	{
		std::string get_vertex_interpreter()
		{
			const char* s =
			#include "Interpreter/VertexInterpreter.glsl"
			;
			return s;
		}

		std::string get_fragment_interpreter()
		{
			const char* s =
			#include "Interpreter/FragmentInterpreter.glsl"
			;
			return s;
		}
	}
}
