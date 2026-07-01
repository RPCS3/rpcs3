#pragma once

#include <util/types.hpp>

namespace program_common
{
	namespace interpreter
	{
		enum compiler_option : u32
		{
			// FS Mix-N-Match
			COMPILER_OPT_ENABLE_TEXTURES       = (1 << 0),
			COMPILER_OPT_ENABLE_DEPTH_EXPORT   = (1 << 1),
			COMPILER_OPT_ENABLE_F32_EXPORT     = (1 << 2),
			COMPILER_OPT_ENABLE_PACKING        = (1 << 3),
			COMPILER_OPT_ENABLE_KIL            = (1 << 4),
			COMPILER_OPT_ENABLE_STIPPLING      = (1 << 5),
			COMPILER_OPT_ENABLE_FLOW_CTRL      = (1 << 6),

			// VS Mix-N-Match
			COMPILER_OPT_ENABLE_INSTANCING     = (1 << 7),
			COMPILER_OPT_ENABLE_VTX_TEXTURES   = (1 << 8),

			// Exclusive bits. Only one can be set at a time
			COMPILER_OPT_ENABLE_ALPHA_TEST_GE  = (1 << 9),
			COMPILER_OPT_ENABLE_ALPHA_TEST_G   = (1 << 10),
			COMPILER_OPT_ENABLE_ALPHA_TEST_LE  = (1 << 11),
			COMPILER_OPT_ENABLE_ALPHA_TEST_L   = (1 << 12),
			COMPILER_OPT_ENABLE_ALPHA_TEST_EQ  = (1 << 13),
			COMPILER_OPT_ENABLE_ALPHA_TEST_NE  = (1 << 14),

			// Meta
			COMPILER_OPT_MAX                   = COMPILER_OPT_ENABLE_ALPHA_TEST_NE,
			COMPILER_OPT_ALPHA_TEST_MASK       = (0b111111 << 9),
			COMPILER_OPT_ALL_VS_MASK           = COMPILER_OPT_ENABLE_INSTANCING | COMPILER_OPT_ENABLE_VTX_TEXTURES,
			COMPILER_OPT_BASE_FS_MASK          = 0b1111111,
			COMPILER_OPT_ALL_FS_MASK           = COMPILER_OPT_BASE_FS_MASK | COMPILER_OPT_ALPHA_TEST_MASK,
			COMPILER_OPT_VS_EXCL_MASK          = COMPILER_OPT_ENABLE_INSTANCING,
			COMPILER_OPT_FS_EXCL_MASK          = COMPILER_OPT_ALPHA_TEST_MASK | COMPILER_OPT_ENABLE_STIPPLING | COMPILER_OPT_ENABLE_DEPTH_EXPORT | COMPILER_OPT_ENABLE_F32_EXPORT,

			// Bounds
			COMPILER_OPT_FS_MAX                = COMPILER_OPT_ENABLE_FLOW_CTRL,
			COMPILER_OPT_FS_MIN                = COMPILER_OPT_ENABLE_TEXTURES,
			COMPILER_OPT_VS_MAX                = COMPILER_OPT_ENABLE_VTX_TEXTURES,
			COMPILER_OPT_VS_MIN                = COMPILER_OPT_ENABLE_INSTANCING,
		};

		enum cached_pipeline_flags : u32
		{
			CACHED_PIPE_UNOPTIMIZED   = (1 << 0),
			CACHED_PIPE_RECOMPILING   = (1 << 1),
			CACHED_PIPE_UNINITIALIZED = (1 << 2),
		};

		[[maybe_unused]] static std::string get_vertex_interpreter()
		{
			const char* s =
			#include "../Program/GLSLInterpreter/VertexInterpreter.glsl"
			;
			return s;
		}

		[[maybe_unused]] static std::string get_fragment_interpreter()
		{
			const char* s =
			#include "../Program/GLSLInterpreter/FragmentInterpreter.glsl"
			;
			return s;
		}

		struct interpreter_shader_variant_t
		{
			u32 shader_opt = 0;
			u32 compatible_shader_opts = 0;
		};

		struct interpreter_pipeline_variant_t
		{
			interpreter_shader_variant_t vs_opts;
			interpreter_shader_variant_t fs_opts;
		};

		struct interpreter_variants_t
		{
			std::vector<interpreter_pipeline_variant_t> pipelines;
			std::vector<std::pair<u32, u32>> base_pipelines;
		};

		interpreter_variants_t get_interpreter_variants();
	}
}
