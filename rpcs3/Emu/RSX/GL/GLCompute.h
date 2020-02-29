#pragma once

#include "Utilities/StrUtil.h"
#include "Emu/IdManager.h"
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
		u32 max_invocations_x = 65535;

		void initialize()
		{
			// Set up optimal kernel size
			const auto& caps = gl::get_driver_caps();
			if (caps.vendor_AMD || caps.vendor_MESA)
			{
				optimal_group_size = 64;
				unroll_loops = false;
			}
			else if (caps.vendor_NVIDIA)
			{
				optimal_group_size = 32;
			}
			else
			{
				optimal_group_size = 128;
			}

			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, reinterpret_cast<GLint*>(&max_invocations_x));
		}

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

			bind_resources();
            m_program.use();
            glDispatchCompute(invocations_x, invocations_y, 1);

            glUseProgram(old_program);
        }

        void run(u32 num_invocations)
        {
			u32 invocations_x, invocations_y;
			if (num_invocations <= max_invocations_x) [[likely]]
			{
				invocations_x = num_invocations;
				invocations_y = 1;
			}
			else
			{
				// Since all the invocations will run, the optimal distribution is sqrt(count)
				const u32 optimal_length = static_cast<u32>(floor(std::sqrt(num_invocations)));
				invocations_x = optimal_length;
				invocations_y = invocations_x;

				if (num_invocations % invocations_x) invocations_y++;
			}

            run(invocations_x, invocations_y);
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
			initialize();

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
                "#define d24x8_to_x8d24(bits) (bits << 8) | (bits >> 24)\n"
                "#define d24x8_to_x8d24_swapped(bits) bswap_u32(d24x8_to_x8d24(bits))\n"
                "#define x8d24_to_d24x8(bits) (bits >> 8) | (bits << 24)\n"
                "#define x8d24_to_d24x8_swapped(bits) x8d24_to_d24x8(bswap_u32(bits))\n"
				"\n"
				"uint linear_invocation_id()\n"
				"{\n"
				"	uint size_in_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);\n"
				"	return (gl_GlobalInvocationID.y * size_in_x) + gl_GlobalInvocationID.x;\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint invocation_id = linear_invocation_id();\n"
				"	uint index = invocation_id * KERNEL_SIZE;\n"
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
            m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_data_length);
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
				rsx_log.error("Inadequate buffer length submitted for a compute operation."
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

    template<bool _SwapBytes = false>
	struct cs_shuffle_d24x8_to_x8d24 : cs_shuffle_base
	{
		cs_shuffle_d24x8_to_x8d24()
		{
            if constexpr (_SwapBytes)
            {
			    cs_shuffle_base::build("d24x8_to_x8d24_swapped");
            }
            else
            {
			    cs_shuffle_base::build("d24x8_to_x8d24");
            }
		}
	};

    template<bool _SwapBytes = false>
	struct cs_shuffle_x8d24_to_d24x8 : cs_shuffle_base
	{
		cs_shuffle_x8d24_to_d24x8()
		{
            if constexpr (_SwapBytes)
            {
			    cs_shuffle_base::build("x8d24_to_d24x8_swapped");
            }
            else
            {
			    cs_shuffle_base::build("x8d24_to_d24x8");
            }
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

	void destroy_compute_tasks();
}