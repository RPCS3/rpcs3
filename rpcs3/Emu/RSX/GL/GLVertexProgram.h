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
			bool m_need_label = false;
			int m_label = 0;

		public:
			decompiler(u32* data);

		private:
			gpu4_program_context::argument arg(SRC src);
			gpu4_program_context::argument arg_dst(bool is_address = false);
			gpu4_program_context::argument label();
			gpu4_program_context::argument condition();
			gpu4_program_context::argument texture();

			context_t::operation& op(const std::string& instruction)
			{
				return gpu_program_builder<>::op(instruction).condition(context.get(condition()));
			}

			template<typename ...T>
			context_t::operation& op(const std::string& instruction, T... args)
			{
				auto &result = gpu_program_builder<>::op(instruction, args...).condition(context.get(condition()));

				if (m_need_label)
				{
					m_need_label = false;
					//result.label(fmt::format("label%u", m_label));
				}

				return result;
			}

			void sca_op(const std::string& instruction, gpu4_program_context::argument dst, gpu4_program_context::argument src);

			std::string instr(std::string instruction);

		public:
			decompiler& decompile(u32 start, std::unordered_map<u32, color4>& constants);

			std::string shader() const
			{
				return m_shader;
			}
		};
	}
}
