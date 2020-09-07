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
                m_shader.create(::glsl::program_domain::glsl_compute_program, m_src);
                m_shader.compile();

                m_program.create();
                m_program.attach(m_shader);
                m_program.link();

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

		std::string uniforms, variables, work_kernel, loop_advance, suffix, method_declarations;

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
				"#define d24f_to_f32(bits) (bits << 7)\n"
				"#define f32_to_d24f(bits) (bits >> 7)\n"
				"\n"
				"uint linear_invocation_id()\n"
				"{\n"
				"	uint size_in_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);\n"
				"	return (gl_GlobalInvocationID.y * size_in_x) + gl_GlobalInvocationID.x;\n"
				"}\n"
				"\n"
				"%md"
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
				{ "%md", method_declarations }
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

	struct cs_shuffle_d32fx8_to_x8d24f : cs_shuffle_base
	{
		u32 m_ssbo_length = 0;

		cs_shuffle_d32fx8_to_x8d24f()
		{
			uniforms = "uniform uint in_ptr, out_ptr;\n";

			variables =
				"	uint in_offset = in_ptr >> 2;\n"
				"	uint out_offset = out_ptr >> 2;\n"
				"	uint depth, stencil;\n";

			work_kernel =
				"		depth = data[index * 2 + in_offset];\n"
				"		stencil = data[index * 2 + (in_offset + 1)] & 0xFFu;\n"
				"		value = f32_to_d24f(depth) << 8;\n"
				"		value |= stencil;\n"
				"		data[index + out_ptr] = bswap_u32(value);\n";

			cs_shuffle_base::build("");
		}

		void bind_resources() override
		{
			m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
		}

		void run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
		{
			u32 data_offset;
			if (src_offset > dst_offset)
			{
				data_offset = dst_offset;
				m_ssbo_length = (src_offset + num_texels * 8) - data_offset;
			}
			else
			{
				data_offset = src_offset;
				m_ssbo_length = (dst_offset + num_texels * 4) - data_offset;
			}

			m_program.uniforms["in_ptr"] = src_offset - data_offset;
			m_program.uniforms["out_ptr"] = dst_offset - data_offset;
			cs_shuffle_base::run(data, num_texels * 4, data_offset);
		}
	};

	struct cs_shuffle_x8d24f_to_d32fx8 : cs_shuffle_base
	{
		u32 m_ssbo_length = 0;

		cs_shuffle_x8d24f_to_d32fx8()
		{
			uniforms = "uniform uint texel_count, in_ptr, out_ptr;\n";

			variables =
				"	uint in_offset = in_ptr >> 2;\n"
				"	uint out_offset = out_ptr >> 2;\n"
				"	uint depth, stencil;\n";

			work_kernel =
				"		value = data[index + in_offset];\n"
				"		value = bswap_u32(value);\n"
				"		stencil = (value & 0xFFu);\n"
				"		depth = (value >> 8);\n"
				"		data[index * 2 + out_offset] = d24f_to_f32(depth);\n"
				"		data[index * 2 + (out_offset + 1)] = stencil;\n";

			cs_shuffle_base::build("");
		}

		void bind_resources() override
		{
			m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
		}

		void run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
		{
			u32 data_offset;
			if (src_offset > dst_offset)
			{
				data_offset = dst_offset;
				m_ssbo_length = (src_offset + num_texels * 4) - data_offset;
			}
			else
			{
				data_offset = src_offset;
				m_ssbo_length = (dst_offset + num_texels * 8) - data_offset;
			}

			m_program.uniforms["in_ptr"] = src_offset - data_offset;
			m_program.uniforms["out_ptr"] = dst_offset - data_offset;
			cs_shuffle_base::run(data, num_texels * 4, data_offset);
		}
	};


	template<typename From, typename To, bool _SwapSrc = false, bool _SwapDst = false>
	struct cs_fconvert_task : cs_shuffle_base
	{
		u32 m_ssbo_length = 0;

		void declare_f16_expansion()
		{
			method_declarations +=
				"uvec2 unpack_e4m12_pack16(const in uint value)\n"
				"{\n"
				"	uvec2 result = uvec2(bitfieldExtract(value, 0, 16), bitfieldExtract(value, 16, 16));\n"
				"	result <<= 11;\n"
				"	result += (120 << 23);\n"
				"	return result;\n"
				"}\n\n";
		}

		void declare_f16_contraction()
		{
			method_declarations +=
				"uint pack_e4m12_pack16(const in uvec2 value)\n"
				"{\n"
				"	uvec2 result = (value - (120 << 23)) >> 11;\n"
				"	return (result.x & 0xFFFF) | (result.y << 16);\n"
				"}\n\n";
		}

		cs_fconvert_task()
		{
			uniforms =
				"uniform uint data_length_in_bytes, in_ptr, out_ptr;\n";

			variables =
				"	uint block_length = data_length_in_bytes >> 2;\n"
				"	uint in_offset = in_ptr >> 2;\n"
				"	uint out_offset = out_ptr >> 2;\n"
				"	uvec4 tmp;\n";

			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n";

			if constexpr (sizeof(From) == 4)
			{
				static_assert(sizeof(To) == 2);
				declare_f16_contraction();

				work_kernel +=
					"		const uint src_offset = (index * 2) + in_offset;\n"
					"		const uint dst_offset = index + out_offset;\n"
					"		tmp.x = data[src_offset];\n"
					"		tmp.y = data[src_offset + 1];\n";

				if constexpr (_SwapSrc)
				{
					work_kernel +=
						"		tmp = bswap_u32(tmp);\n";
				}

				// Convert
				work_kernel += "		tmp.z = pack_e4m12_pack16(tmp.xy);\n";

				if constexpr (_SwapDst)
				{
					work_kernel += "		tmp.z = bswap_u16(tmp.z);\n";
				}

				work_kernel += "		data[dst_offset] = tmp.z;\n";
			}
			else
			{
				static_assert(sizeof(To) == 4);
				declare_f16_expansion();

				work_kernel +=
					"		const uint src_offset = index + in_offset;\n"
					"		const uint dst_offset = (index * 2) + out_offset;\n"
					"		tmp.x = data[src_offset];\n";

				if constexpr (_SwapSrc)
				{
					work_kernel +=
						"		tmp.x = bswap_u16(tmp.x);\n";
				}

				// Convert
				work_kernel += "		tmp.yz = unpack_e4m12_pack16(tmp.x);\n";

				if constexpr (_SwapDst)
				{
					work_kernel += "		tmp.yz = bswap_u32(tmp.yz);\n";
				}

				work_kernel +=
					"		data[dst_offset] = tmp.y;\n"
					"		data[dst_offset + 1] = tmp.z;\n";
			}

			cs_shuffle_base::build("");
		}

		void bind_resources() override
		{
			m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
		}

		void run(const gl::buffer* data, u32 src_offset, u32 src_length, u32 dst_offset)
		{
			u32 data_offset;
			if (src_offset > dst_offset)
			{
				m_ssbo_length = (src_offset + src_length) - dst_offset;
				data_offset = dst_offset;
			}
			else
			{
				m_ssbo_length = (dst_offset - src_offset) + (src_length / sizeof(From)) * sizeof(To);
				data_offset = src_offset;
			}

			m_program.uniforms["data_length_in_bytes"] = src_length;
			m_program.uniforms["in_ptr"] = src_offset - data_offset;
			m_program.uniforms["out_ptr"] = dst_offset - data_offset;

			cs_shuffle_base::run(data, src_length, data_offset);
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
