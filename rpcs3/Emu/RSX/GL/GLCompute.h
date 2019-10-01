#pragma once

#include "Utilities/StrUtil.h"
#include "GLHelpers.h"

namespace gl
{
    struct compute_task
    {
        std::string m_src;
        gl::glsl::shader m_shader;
        gl::glsl::program m_program;
        bool compiled = false;

        // Device-specific options
		bool unroll_loops = true;
		u32 optimal_group_size = 1;
		u32 optimal_kernel_size = 1;

        void create()
        {
            if (!compiled)
            {
                m_shader.create(gl::glsl::shader::type::compute);
                m_shader.source(m_src);
                m_shader.compile();

                m_program.create();
                m_program.attach(m_shader);
                m_program.make();

                compiled = true;
            }
        }

        void destroy()
        {
            if (compiled)
            {
                m_program.remove();
                m_shader.remove();

                compiled = false;
            }
        }

        virtual void bind_resources()
        {}

        void run(u32 invocations_x, u32 invocations_y)
        {
            GLint old_program;
            glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);

            m_program.use();
            glDispatchCompute(invocations_x, invocations_y, 1);

            glUseProgram((GLuint)old_program);
        }

        void run(u32 num_invocations)
        {
            run(num_invocations, 1);   
        }
    };

	struct cs_shuffle_base : compute_task
	{
		const gl::buffer* m_data = nullptr;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		std::string uniforms, variables, work_kernel, loop_advance, suffix;

		cs_shuffle_base()
		{
			work_kernel =
				"		value = data[index];\n"
				"		data[index] = %f(value);\n";

			loop_advance =
				"		index++;\n";

			suffix =
				"}\n";
		}

		void build(const char* function_name, u32 _kernel_size = 0)
		{
			// Initialize to allow detecting optimal settings
			create();

			kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

			m_src =
				"#version 430\n"
				"layout(local_size_x=%ws, local_size_y=1, local_size_z=1) in;\n"
				"layout(binding=%loc, std430) buffer ssbo{ uint data[]; };\n"
				"%ub"
				"\n"
				"#define KERNEL_SIZE %ks\n"
				"\n"
				"// Generic swap routines\n"
				"#define bswap_u16(bits)     (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8\n"
				"#define bswap_u32(bits)     (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24\n"
				"#define bswap_u16_u32(bits) (bits & 0xFFFF) << 16 | (bits & 0xFFFF0000) >> 16\n"
				"\n"
				"// Depth format conversions\n"
				"#define d24_to_f32(bits)             floatBitsToUint(float(bits) / 16777215.f)\n"
				"#define f32_to_d24(bits)             uint(uintBitsToFloat(bits) * 16777215.f)\n"
				"#define d24x8_to_f32(bits)           d24_to_f32(bits >> 8)\n"
				"#define d24x8_to_d24x8_swapped(bits) (bits & 0xFF00) | (bits & 0xFF0000) >> 16 | (bits & 0xFF) << 16\n"
				"#define f32_to_d24x8_swapped(bits)   d24x8_to_d24x8_swapped(f32_to_d24(bits))\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint index = gl_GlobalInvocationID.x * KERNEL_SIZE;\n"
				"	uint value;\n"
				"	%vars"
				"\n";

			const std::pair<std::string, std::string> syntax_replace[] =
			{
                { "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%ks", std::to_string(kernel_size) },
				{ "%vars", variables },
				{ "%f", function_name },
				{ "%ub", uniforms },
			};

			m_src = fmt::replace_all(m_src, syntax_replace);
			work_kernel = fmt::replace_all(work_kernel, syntax_replace);

			if (kernel_size <= 1)
			{
				m_src += "	{\n" + work_kernel + "	}\n";
			}
			else if (unroll_loops)
			{
				work_kernel += loop_advance + "\n";

				m_src += std::string
				(
					"	//Unrolled loop\n"
					"	{\n"
				);

				// Assemble body with manual loop unroll to try loweing GPR usage
				for (u32 n = 0; n < kernel_size; ++n)
				{
					m_src += work_kernel;
				}

				m_src += "	}\n";
			}
			else
			{
				m_src += "	for (int loop = 0; loop < KERNEL_SIZE; ++loop)\n";
				m_src += "	{\n";
				m_src += work_kernel;
				m_src += loop_advance;
				m_src += "	}\n";
			}

			m_src += suffix;
		}

		void bind_resources() override
		{
            m_data->bind_range(GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_data_length);
		}

		void run(const gl::buffer* data, u32 data_length, u32 data_offset = 0)
		{
			m_data = data;
			m_data_offset = data_offset;
			m_data_length = data_length;

			const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
			const auto num_bytes_to_process = align(data_length, num_bytes_per_invocation);
			const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

			if ((num_bytes_to_process + data_offset) > data->size())
			{
				// Technically robust buffer access should keep the driver from crashing in OOB situations
				LOG_ERROR(RSX, "Inadequate buffer length submitted for a compute operation."
					"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
			}

			compute_task::run(num_invocations);
		}
	};

	struct cs_shuffle_16 : cs_shuffle_base
	{
		// byteswap ushort
		cs_shuffle_16()
		{
			cs_shuffle_base::build("bswap_u16");
		}
	};

	struct cs_shuffle_32 : cs_shuffle_base
	{
		// byteswap_ulong
		cs_shuffle_32()
		{
			cs_shuffle_base::build("bswap_u32");
		}
	};

	struct cs_shuffle_32_16 : cs_shuffle_base
	{
		// byteswap_ulong + byteswap_ushort
		cs_shuffle_32_16()
		{
			cs_shuffle_base::build("bswap_u16_u32");
		}
	};

	struct cs_shuffle_d24x8_f32 : cs_shuffle_base
	{
		// convert d24x8 to f32
		cs_shuffle_d24x8_f32()
		{
			cs_shuffle_base::build("d24x8_to_f32");
		}
	};

	struct cs_shuffle_se_f32_d24x8 : cs_shuffle_base
	{
		// convert f32 to d24x8 and swap endianness
		cs_shuffle_se_f32_d24x8()
		{
			cs_shuffle_base::build("f32_to_d24x8_swapped");
		}
	};

	struct cs_shuffle_se_d24x8 : cs_shuffle_base
	{
		// swap endianness of d24x8
		cs_shuffle_se_d24x8()
		{
			cs_shuffle_base::build("d24x8_to_d24x8_swapped");
		}
	};

	// NOTE: D24S8 layout has the stencil in the MSB! Its actually S8|D24|S8|D24 starting at offset 0
	struct cs_interleave_task : cs_shuffle_base
	{
		cs_interleave_task()
		{
            uniforms =
            "   uniform uint block_length;\n"
            "   uniform uint z_offset;\n"
            "   uniform uint s_offset;\n";

			variables =
				"	uint depth;\n"
				"	uint stencil;\n"
				"	uint stencil_shift;\n"
				"	uint stencil_offset;\n";
		}

		void run(const gl::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset)
		{
            m_program.uniforms["block_length"] = data_length;
            m_program.uniforms["z_offset"] = zeta_offset - data_offset;
            m_program.uniforms["s_offset"] = stencil_offset - data_offset;
			cs_shuffle_base::run(data, data_length, data_offset);
		}
	};

	template<bool _SwapBytes = false>
	struct cs_gather_d24x8 : cs_interleave_task
	{
		cs_gather_d24x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		depth = data[index + z_offset] & 0x00FFFFFF;\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = data[stencil_offset + s_offset];\n"
				"		stencil = (stencil >> stencil_shift) & 0xFF;\n"
				"		value = (depth << 8) | stencil;\n";

			if constexpr (!_SwapBytes)
			{
				work_kernel +=
				"		data[index] = value;\n";
			}
			else
			{
				work_kernel +=
				"		data[index] = bswap_u32(value);\n";
			}

			cs_shuffle_base::build("");
		}
	};

	template<bool _SwapBytes = false>
	struct cs_gather_d32x8 : cs_interleave_task
	{
		cs_gather_d32x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		depth = f32_to_d24(data[index + z_offset]);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = data[stencil_offset + s_offset];\n"
				"		stencil = (stencil >> stencil_shift) & 0xFF;\n"
				"		value = (depth << 8) | stencil;\n";

			if constexpr (!_SwapBytes)
			{
				work_kernel +=
				"		data[index] = value;\n";
			}
			else
			{
				work_kernel +=
				"		data[index] = bswap_u32(value);\n";
			}

			cs_shuffle_base::build("");
		}
	};

	struct cs_scatter_d24x8 : cs_interleave_task
	{
		cs_scatter_d24x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		value = data[index];\n"
				"		data[index + z_offset] = (value >> 8);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = (value & 0xFF) << stencil_shift;\n"
				"		data[stencil_offset + s_offset] |= stencil;\n";

			cs_shuffle_base::build("");
		}
	};

	struct cs_scatter_d32x8 : cs_interleave_task
	{
		cs_scatter_d32x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		value = data[index];\n"
				"		data[index + z_offset] = d24_to_f32(value >> 8);\n"
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = (value & 0xFF) << stencil_shift;\n"
				"		data[stencil_offset + s_offset] |= stencil;\n";

			cs_shuffle_base::build("");
		}
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<gl::compute_task>> g_compute_tasks;

	template<class T>
	T* get_compute_task()
	{
		u32 index = id_manager::typeinfo::get_index<T>();
		auto &e = g_compute_tasks[index];

		if (!e)
		{
			e = std::make_unique<T>();
			e->create();
		}

		return static_cast<T*>(e.get());
	}
}