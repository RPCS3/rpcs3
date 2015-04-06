#pragma once
#include "Emu/RSX/RSXVertexProgram.h"
#include "gpu_program.h"

namespace gl
{
	namespace vertex_program
	{
		class decompiler : gpu_program_builder<>
		{
			u32* m_data;

			D0 d0;
			D1 d1;
			D2 d2;
			D3 d3;
			SRC src[3];

			bool is_sca = false;

			std::string m_shader;
			uint m_min_constant_id;
			uint m_max_constant_id;

		public:
			decompiler(u32* data);

		private:
			gpu4_program_context::argument arg(SRC src);
			gpu4_program_context::argument arg_dst();
			gpu4_program_context::argument condition();
			gpu4_program_context::argument texture();

			context_t::operation& op(const std::string& instruction)
			{
				return gpu_program_builder<>::op(instruction).condition(context.get(condition()));
			}

			template<typename ...T>
			context_t::operation& op(const std::string& instruction, T... args)
			{
				return gpu_program_builder<>::op(instruction, args...).condition(context.get(condition()));
			}

			void sca_op(const std::string& instruction, gpu4_program_context::argument dst, gpu4_program_context::argument src);

			std::string instr(const std::string& instruction);

		public:
			decompiler& decompile(u32 start, std::unordered_map<u32, color4>& constants);

			std::string shader() const
			{
				return m_shader;
			}
		};
	}
}
