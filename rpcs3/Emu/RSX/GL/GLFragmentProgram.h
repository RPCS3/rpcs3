#pragma once
#include "Emu/RSX/RSXFragmentProgram.h"
#include "gpu_program.h"

namespace gl
{
	namespace fragment_program
	{
		class decompiler : gpu_program_builder<>
		{
			vm::ptr<u32> m_data;

			OPDEST dst;
			SRC0 src0;
			SRC1 src1;
			SRC2 src2;

			bool next_is_constant = false;

			std::unordered_map<u32, std::vector<std::string>> custom_instruction;
			std::string m_shader;

		public:
			decompiler(vm::ptr<u32> data);

		private:
			static u32 swap_bytes(u32 data)
			{
				return (data << 16) | (data >> 16);
			}

			template<typename T>
			gpu4_program_context::argument arg(T src)
			{
				gpu4_program_context::argument result;
				switch (src.reg_type)
				{
				case 0: //template register
					result = variable("TEMP", (src.fp16 ? "H" : "R") + std::to_string(src.tmp_reg_index));
					break;

				case 1: //input register
				{
					static const std::string reg_table[] =
					{
						"fragment.position",
						"fragment.color",
						"fragment.color.secondary",
						"fragment.fogcoord",
						"fragment.texcoord[0]",
						"fragment.texcoord[1]",
						"fragment.texcoord[2]",
						"fragment.texcoord[3]",
						"fragment.texcoord[4]",
						"fragment.texcoord[5]",
						"fragment.texcoord[6]",
						"fragment.texcoord[7]",
						"fragment.texcoord[8]",
						"fragment.texcoord[9]"
					};

					if (dst.src_attr_reg_num < std::distance(std::begin(reg_table), std::end(reg_table)))
					{
						result = predeclared_variable(reg_table[dst.src_attr_reg_num]);
					}
					else
					{
						throw std::runtime_error("Bad src reg num");
					}
				}
				break;

				case 2: //constant
				{
					f32 c0;
					f32 c1;
					f32 c2;
					f32 c3;

					(u32&)c0 = swap_bytes(m_data[0]);
					(u32&)c1 = swap_bytes(m_data[1]);
					(u32&)c2 = swap_bytes(m_data[2]);
					(u32&)c3 = swap_bytes(m_data[3]);
					next_is_constant = true;

					result = constant(c0, c1, c2, c3);
				}
				break;
				}

				std::string mask;
				mask += swizzle_mask[src.swizzle_x];
				mask += swizzle_mask[src.swizzle_y];
				mask += swizzle_mask[src.swizzle_z];
				mask += swizzle_mask[src.swizzle_w];

				if (mask[0] == mask[1] && mask[1] == mask[2] && mask[2] == mask[3])
				{
					mask = std::string(1, mask[0]);
				}

				return result.mask(mask).abs(src.abs).neg(src.neg);
			}

			template<>
			gpu4_program_context::argument arg(OPDEST dst)
			{
				if (dst.no_dest && !dst.set_cond)
				{
					return{};
				}

				std::string mask;

				if (dst.mask_x) mask += "x";
				if (dst.mask_y) mask += "y";
				if (dst.mask_z) mask += "z";
				if (dst.mask_w) mask += "w";

				if (mask[0] == mask[1] && mask[1] == mask[2] && mask[2] == mask[3])
				{
					mask = std::string(1, mask[0]);
				}

				if (dst.no_dest && dst.set_cond)
				{
					return variable("TEMP", "CC" + std::to_string(src0.cond_reg_index)).mask(mask);
				}

				return variable("TEMP", (dst.fp16 ? "H" : "R") + std::to_string(dst.dest_reg)).mask(mask);
			}

			gpu4_program_context::argument condition();
			gpu4_program_context::argument texture();

			enum instr_flag
			{
				F = 1 << 0, //floating - point data type modifiers
				I = 1 << 1, //signed and unsigned integer data type modifiers
				C = 1 << 2, //condition code update modifiers
				S = 1 << 3, //clamping(saturation) modifiers
				H = 1 << 4, //half - precision float data type suffix
			};

			template<typename ...T>
			context_t::operation& op(const std::string& instruction, T... args)
			{
				return gpu_program_builder<>::op(instruction, args...).condition(context.get(condition()));
			}

			std::string instr(std::string instruction, int flags = 0);

		public:
			decompiler& decompile(u32 control);

			std::string shader() const
			{
				return m_shader;
			}
		};
	}
}
