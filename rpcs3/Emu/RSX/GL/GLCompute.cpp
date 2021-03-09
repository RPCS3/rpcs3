#include "GLCompute.h"
#include "Utilities/StrUtil.h"

namespace gl
{
	void compute_task::initialize()
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

	void compute_task::create()
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

	void compute_task::destroy()
	{
		if (compiled)
		{
			m_program.remove();
			m_shader.remove();

			compiled = false;
		}
	}

	void compute_task::run(u32 invocations_x, u32 invocations_y)
	{
		GLint old_program;
		glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);

		bind_resources();
		m_program.use();
		glDispatchCompute(invocations_x, invocations_y, 1);

		glUseProgram(old_program);
	}

	void compute_task::run(u32 num_invocations)
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

	cs_shuffle_base::cs_shuffle_base()
	{
		work_kernel =
			"		value = data[index];\n"
			"		data[index] = %f(value);\n";

		loop_advance =
			"		index++;\n";

		suffix =
			"}\n";
	}

	void cs_shuffle_base::build(const char* function_name, u32 _kernel_size)
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

	void cs_shuffle_base::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_data_length);
	}

	void cs_shuffle_base::run(const gl::buffer* data, u32 data_length, u32 data_offset)
	{
		m_data = data;
		m_data_offset = data_offset;
		m_data_length = data_length;

		const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
		const auto num_bytes_to_process = utils::align(data_length, num_bytes_per_invocation);
		const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

		if ((num_bytes_to_process + data_offset) > data->size())
		{
			// Technically robust buffer access should keep the driver from crashing in OOB situations
			rsx_log.error("Inadequate buffer length submitted for a compute operation."
				"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
		}

		compute_task::run(num_invocations);
	}

	cs_shuffle_d32fx8_to_x8d24f::cs_shuffle_d32fx8_to_x8d24f()
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

	void cs_shuffle_d32fx8_to_x8d24f::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
	}

	void cs_shuffle_d32fx8_to_x8d24f::run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
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

	cs_shuffle_x8d24f_to_d32fx8::cs_shuffle_x8d24f_to_d32fx8()
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

	void cs_shuffle_x8d24f_to_d32fx8::bind_resources()
	{
		m_data->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), m_data_offset, m_ssbo_length);
	}

	void cs_shuffle_x8d24f_to_d32fx8::run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels)
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
}
