#pragma once

#include "Emu/IdManager.h"
#include "GLHelpers.h"

#include <unordered_map>

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

		void initialize();
		void create();
		void destroy();

		virtual void bind_resources() {}

		void run(u32 invocations_x, u32 invocations_y);
		void run(u32 num_invocations);
	};

	struct cs_shuffle_base : compute_task
	{
		const gl::buffer* m_data = nullptr;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		std::string uniforms, variables, work_kernel, loop_advance, suffix, method_declarations;

		cs_shuffle_base();

		void build(const char* function_name, u32 _kernel_size = 0);

		void bind_resources() override;

		void run(const gl::buffer* data, u32 data_length, u32 data_offset = 0);
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

		cs_shuffle_d32fx8_to_x8d24f();

		void bind_resources() override;

		void run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels);
	};

	struct cs_shuffle_x8d24f_to_d32fx8 : cs_shuffle_base
	{
		u32 m_ssbo_length = 0;

		cs_shuffle_x8d24f_to_d32fx8();

		void bind_resources() override;

		void run(const gl::buffer* data, u32 src_offset, u32 dst_offset, u32 num_texels);
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
