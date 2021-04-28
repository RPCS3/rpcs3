#pragma once
#include "VKPipelineCompiler.h"
#include "vkutils/descriptors.hpp"
#include "vkutils/buffer_object.h"

#include "Emu/IdManager.h"

#include "Utilities/StrUtil.h"
#include "util/asm.hpp"

#include <unordered_map>

namespace vk
{
	struct compute_task
	{
		std::string m_src;
		vk::glsl::shader m_shader;
		std::unique_ptr<vk::glsl::program> m_program;
		std::unique_ptr<vk::buffer> m_param_buffer;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		bool initialized = false;
		bool unroll_loops = true;
		bool use_push_constants = false;
		u32 ssbo_count = 1;
		u32 push_constants_size = 0;
		u32 optimal_group_size = 1;
		u32 optimal_kernel_size = 1;
		u32 max_invocations_x = 65535;

		virtual std::vector<std::pair<VkDescriptorType, u8>> get_descriptor_layout();

		void init_descriptors();

		void create();
		void destroy();

		void free_resources();

		virtual void bind_resources() {}
		virtual void declare_inputs() {}

		void load_program(VkCommandBuffer cmd);

		void run(VkCommandBuffer cmd, u32 invocations_x, u32 invocations_y, u32 invocations_z);
		void run(VkCommandBuffer cmd, u32 num_invocations);
	};

	struct cs_shuffle_base : compute_task
	{
		const vk::buffer* m_data;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		std::string variables, work_kernel, loop_advance, suffix;
		std::string method_declarations;

		cs_shuffle_base();

		void build(const char* function_name, u32 _kernel_size = 0);

		void bind_resources() override;

		void set_parameters(VkCommandBuffer cmd, const u32* params, u8 count);

		void run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_length, u32 data_offset = 0);
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
		u32 m_ssbo_length = 0;

		cs_interleave_task();

		void bind_resources() override;

		void run(VkCommandBuffer cmd, const vk::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset);
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

	template<bool _SwapBytes = false, bool _DepthFloat = false>
	struct cs_gather_d32x8 : cs_interleave_task
	{
		cs_gather_d32x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n";

			if constexpr (!_DepthFloat)
			{
				work_kernel +=
				"		depth = f32_to_d24(data[index + z_offset]);\n";
			}
			else
			{
				work_kernel +=
				"		depth = f32_to_d24f(data[index + z_offset]);\n";
			}

			work_kernel +=
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
		cs_scatter_d24x8();
	};

	template<bool _DepthFloat = false>
	struct cs_scatter_d32x8 : cs_interleave_task
	{
		cs_scatter_d32x8()
		{
			work_kernel =
				"		if (index >= block_length)\n"
				"			return;\n"
				"\n"
				"		value = data[index];\n";

			if constexpr (!_DepthFloat)
			{
				work_kernel +=
				"		data[index + z_offset] = d24_to_f32(value >> 8);\n";
			}
			else
			{
				work_kernel +=
				"		data[index + z_offset] = d24f_to_f32(value >> 8);\n";
			}

			work_kernel +=
				"		stencil_offset = (index / 4);\n"
				"		stencil_shift = (index % 4) * 8;\n"
				"		stencil = (value & 0xFF) << stencil_shift;\n"
				"		atomicOr(data[stencil_offset + s_offset], stencil);\n";

			cs_shuffle_base::build("");
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
			use_push_constants = true;
			push_constants_size = 16;

			variables =
				"	uint block_length = params[0].x >> 2;\n"
				"	uint in_offset = params[0].y >> 2;\n"
				"	uint out_offset = params[0].z >> 2;\n"
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
			m_program->bind_buffer({ m_data->value, m_data_offset, m_ssbo_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
		}

		void run(VkCommandBuffer cmd, const vk::buffer* data, u32 src_offset, u32 src_length, u32 dst_offset)
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

			u32 parameters[4] = { src_length, src_offset - data_offset, dst_offset - data_offset, 0 };
			set_parameters(cmd, parameters, 4);
			cs_shuffle_base::run(cmd, data, src_length, data_offset);
		}
	};

	// Reverse morton-order block arrangement
	struct cs_deswizzle_base : compute_task
	{
		virtual void run(VkCommandBuffer cmd, const vk::buffer* dst, u32 out_offset, const vk::buffer* src, u32 in_offset, u32 data_length, u32 width, u32 height, u32 depth, u32 mipmaps) = 0;
	};

	template <typename _BlockType, typename _BaseType, bool _SwapBytes>
	struct cs_deswizzle_3d : cs_deswizzle_base
	{
		union params_t
		{
			u32 data[7];

			struct
			{
				u32 width;
				u32 height;
				u32 depth;
				u32 logw;
				u32 logh;
				u32 logd;
				u32 mipmaps;
			};
		}
		params;

		const vk::buffer* src_buffer = nullptr;
		const vk::buffer* dst_buffer = nullptr;
		u32 in_offset = 0;
		u32 out_offset = 0;
		u32 block_length = 0;

		cs_deswizzle_3d()
		{
			ensure((sizeof(_BlockType) & 3) == 0); // "Unsupported block type"

			ssbo_count = 2;
			use_push_constants = true;
			push_constants_size = 28;

			create();

			m_src =
			"#version 450\n"
			"layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;\n\n"

			"layout(set=0, binding=0, std430) buffer ssbo0{ uint data_in[]; };\n"
			"layout(set=0, binding=1, std430) buffer ssbo1{ uint data_out[]; };\n"
			"layout(push_constant) uniform parameters\n"
			"{\n"
			"	uint image_width;\n"
			"	uint image_height;\n"
			"	uint image_depth;\n"
			"	uint image_logw;\n"
			"	uint image_logh;\n"
			"	uint image_logd;\n"
			"	uint lod_count;\n"
			"};\n\n"

			"struct invocation_properties\n"
			"{\n"
			"	uint data_offset;\n"
			"	uvec3 size;\n"
			"	uvec3 size_log2;\n"
			"};\n\n"

			"#define bswap_u16(bits) (bits & 0xFF) << 8 | (bits & 0xFF00) >> 8 | (bits & 0xFF0000) << 8 | (bits & 0xFF000000) >> 8\n"
			"#define bswap_u32(bits) (bits & 0xFF) << 24 | (bits & 0xFF00) << 8 | (bits & 0xFF0000) >> 8 | (bits & 0xFF000000) >> 24\n\n"

			"invocation_properties invocation;\n\n"

			"bool init_invocation_properties(const in uint offset)\n"
			"{\n"
			"	invocation.data_offset = 0;\n"
			"	invocation.size.x = image_width;\n"
			"	invocation.size.y = image_height;\n"
			"	invocation.size.z = image_depth;\n"
			"	invocation.size_log2.x = image_logw;\n"
			"	invocation.size_log2.y = image_logh;\n"
			"	invocation.size_log2.z = image_logd;\n"
			"	uint level_end = image_width * image_height * image_depth;\n"
			"	uint level = 1;\n\n"

			"	while (offset >= level_end && level < lod_count)\n"
			"	{\n"
			"		invocation.data_offset = level_end;\n"
			"		invocation.size.xy /= 2;\n"
			"		invocation.size.xy = max(invocation.size.xy, uvec2(1));\n"
			"		invocation.size_log2.xy = max(invocation.size_log2.xy, uvec2(1));\n"
			"		invocation.size_log2.xy --;\n"
			"		level_end += (invocation.size.x * invocation.size.y * image_depth);\n"
			"		level++;"
			"	}\n\n"

			"	return (offset < level_end);\n"
			"}\n\n"

			"uint get_z_index(const in uint x_, const in uint y_, const in uint z_)\n"
			"{\n"
			"	uint offset = 0;\n"
			"	uint shift = 0;\n"
			"	uint x = x_;\n"
			"	uint y = y_;\n"
			"	uint z = z_;\n"
			"	uint log2w = invocation.size_log2.x;\n"
			"	uint log2h = invocation.size_log2.y;\n"
			"	uint log2d = invocation.size_log2.z;\n"
			"\n"
			"	do\n"
			"	{\n"
			"		if (log2w > 0)\n"
			"		{\n"
			"			offset |= (x & 1) << shift;\n"
			"			shift++;\n"
			"			x >>= 1;\n"
			"			log2w--;\n"
			"		}\n"
			"\n"
			"		if (log2h > 0)\n"
			"		{\n"
			"			offset |= (y & 1) << shift;\n"
			"			shift++;\n"
			"			y >>= 1;\n"
			"			log2h--;\n"
			"		}\n"
			"\n"
			"		if (log2d > 0)\n"
			"		{\n"
			"			offset |= (z & 1) << shift;\n"
			"			shift++;\n"
			"			z >>= 1;\n"
			"			log2d--;\n"
			"		}\n"
			"	}\n"
			"	while(x > 0 || y > 0 || z > 0);\n"
			"\n"
			"	return offset;\n"
			"}\n\n"

			"void main()\n"
			"{\n"
			"	uint invocations_x = (gl_NumWorkGroups.x * gl_WorkGroupSize.x);"
			"	uint texel_id = (gl_GlobalInvocationID.y * invocations_x) + gl_GlobalInvocationID.x;\n"
			"	uint word_count = %_wordcount;\n\n"

			"	if (!init_invocation_properties(texel_id))\n"
			"		return;\n\n"

			"	// Calculations done in texels, not bytes\n"
			"	uint row_length = invocation.size.x;\n"
			"	uint slice_length = (invocation.size.y * row_length);\n"
			"	uint level_offset = (texel_id - invocation.data_offset);\n"
			"	uint slice_offset = (level_offset % slice_length);\n"
			"	uint z = (level_offset / slice_length);\n"
			"	uint y = (slice_offset / row_length);\n"
			"	uint x = (slice_offset % row_length);\n\n"

			"	uint src_texel_id = get_z_index(x, y, z);\n"
			"	uint dst_id = (texel_id * word_count);\n"
			"	uint src_id = (src_texel_id + invocation.data_offset) * word_count;\n\n"

			"	for (uint i = 0; i < word_count; ++i)\n"
			"	{\n"
			"		uint value = data_in[src_id++];\n"
			"		data_out[dst_id++] = %f(value);\n"
			"	}\n\n"

			"}\n";

			std::string transform;
			if constexpr (_SwapBytes)
			{
				if constexpr (sizeof(_BaseType) == 4)
				{
					transform = "bswap_u32";
				}
				else if constexpr (sizeof(_BaseType) == 2)
				{
					transform = "bswap_u16";
				}
				else
				{
					fmt::throw_exception("Unreachable");
				}
			}

			const std::pair<std::string, std::string> syntax_replace[] =
			{
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%_wordcount", std::to_string(sizeof(_BlockType) / 4) },
				{ "%f", transform }
			};

			m_src = fmt::replace_all(m_src, syntax_replace);
		}

		void bind_resources() override
		{
			m_program->bind_buffer({ src_buffer->value, in_offset, block_length }, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
			m_program->bind_buffer({ dst_buffer->value, out_offset, block_length }, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_descriptor_set);
		}

		void set_parameters(VkCommandBuffer cmd)
		{
			vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, params.data);
		}

		void run(VkCommandBuffer cmd, const vk::buffer* dst, u32 out_offset, const vk::buffer* src, u32 in_offset, u32 data_length, u32 width, u32 height, u32 depth, u32 mipmaps) override
		{
			dst_buffer = dst;
			src_buffer = src;

			this->in_offset = in_offset;
			this->out_offset = out_offset;
			this->block_length = data_length;

			params.width = width;
			params.height = height;
			params.depth = depth;
			params.mipmaps = mipmaps;
			params.logw = rsx::ceil_log2(width);
			params.logh = rsx::ceil_log2(height);
			params.logd = rsx::ceil_log2(depth);
			set_parameters(cmd);

			const u32 num_bytes_per_invocation = (sizeof(_BlockType) * optimal_group_size);
			const u32 linear_invocations = utils::aligned_div(data_length, num_bytes_per_invocation);
			compute_task::run(cmd, linear_invocations);
		}
	};

	struct cs_aggregator : compute_task
	{
		const buffer* src = nullptr;
		const buffer* dst = nullptr;
		u32 block_length = 0;
		u32 word_count = 0;

		cs_aggregator();

		void bind_resources() override;

		void run(VkCommandBuffer cmd, const vk::buffer* dst, const vk::buffer* src, u32 num_words);
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<vk::compute_task>> g_compute_tasks;

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

	void reset_compute_tasks();
}
