#pragma once
#include "Emu/RSX/VK/VKProgramPipeline.h"
#include "vkutils/descriptors.h"
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

		bool initialized = false;
		bool unroll_loops = true;
		bool use_push_constants = false;
		u32 ssbo_count = 1;
		u32 push_constants_size = 0;
		u32 optimal_group_size = 1;
		u32 optimal_kernel_size = 1;
		u32 max_invocations_x = 65535;

		compute_task() = default;
		virtual ~compute_task() { destroy(); }

		void create();
		void destroy();

		virtual std::vector<glsl::program_input> get_inputs();
		virtual void bind_resources(const vk::command_buffer& /*cmd*/) {}

		void load_program(const vk::command_buffer& cmd);

		void run(const vk::command_buffer& cmd, u32 invocations_x, u32 invocations_y, u32 invocations_z);
		void run(const vk::command_buffer& cmd, u32 num_invocations);
	};

	struct cs_shuffle_base : compute_task
	{
		const vk::buffer* m_data;
		u32 m_data_offset = 0;
		u32 m_data_length = 0;
		u32 kernel_size = 1;

		rsx::simple_array<u32> m_params;

		std::string variables, work_kernel, loop_advance, suffix;
		std::string method_declarations;

		cs_shuffle_base();

		void build(const char* function_name, u32 _kernel_size = 0);

		void bind_resources(const vk::command_buffer& cmd) override;

		void set_parameters(const vk::command_buffer& cmd);

		void run(const vk::command_buffer& cmd, const vk::buffer* data, u32 data_length, u32 data_offset = 0);
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

		void bind_resources(const vk::command_buffer& cmd) override;

		void run(const vk::command_buffer& cmd, const vk::buffer* data, u32 data_offset, u32 data_length, u32 zeta_offset, u32 stencil_offset);
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

		void bind_resources(const vk::command_buffer& cmd) override
		{
			set_parameters(cmd);
			m_program->bind_uniform({ *m_data, m_data_offset, m_ssbo_length }, 0, 0);
		}

		void run(const vk::command_buffer& cmd, const vk::buffer* data, u32 src_offset, u32 src_length, u32 dst_offset)
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

			m_params = { src_length, src_offset - data_offset, dst_offset - data_offset, 0 };
			cs_shuffle_base::run(cmd, data, src_length, data_offset);
		}
	};

	// Reverse morton-order block arrangement
	struct cs_deswizzle_base : compute_task
	{
		virtual void run(const vk::command_buffer& cmd, const vk::buffer* dst, u32 out_offset, const vk::buffer* src, u32 in_offset, u32 data_length, u32 width, u32 height, u32 depth, u32 mipmaps) = 0;
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
			ssbo_count = 2;
			use_push_constants = true;
			push_constants_size = 28;

			create();

			m_src =
			#include "../Program/GLSLSnippets/GPUDeswizzle.glsl"
			;

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

			const std::pair<std::string_view, std::string> syntax_replace[] =
			{
				{ "%loc", "0" },
				{ "%set", "set = 0" },
				{ "%push_block", "push_constant" },
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%_wordcount", std::to_string(std::max<u32>(sizeof(_BlockType) / 4u, 1u)) },
				{ "%f", transform },
				{ "%_8bit", sizeof(_BlockType) == 1 ? "1" : "0" },
				{ "%_16bit", sizeof(_BlockType) == 2 ? "1" : "0" },
			};

			m_src = fmt::replace_all(m_src, syntax_replace);
		}

		void bind_resources(const vk::command_buffer& cmd) override
		{
			set_parameters(cmd);

			m_program->bind_uniform({ *src_buffer, in_offset, block_length }, 0, 0);
			m_program->bind_uniform({ *dst_buffer, out_offset, block_length }, 0, 1);
		}

		void set_parameters(const vk::command_buffer& cmd)
		{
			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, params.data);
		}

		void run(const vk::command_buffer& cmd, const vk::buffer* dst, u32 out_offset, const vk::buffer* src, u32 in_offset, u32 data_length, u32 width, u32 height, u32 depth, u32 mipmaps) override
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

			const u32 word_count_per_invocation = std::max<u32>(sizeof(_BlockType) / 4u, 1u);
			const u32 num_bytes_per_invocation = (word_count_per_invocation * 4u * optimal_group_size);
			const u32 workgroup_invocations = utils::aligned_div(data_length, num_bytes_per_invocation);
			compute_task::run(cmd, workgroup_invocations);
		}
	};

	struct cs_aggregator : compute_task
	{
		const buffer* src = nullptr;
		const buffer* dst = nullptr;
		u32 block_length = 0;
		u32 word_count = 0;

		cs_aggregator();

		void bind_resources(const vk::command_buffer& cmd) override;

		void run(const vk::command_buffer& cmd, const vk::buffer* dst, const vk::buffer* src, u32 num_words);
	};

	enum RSX_detiler_op
	{
		decode = 0,
		encode = 1
	};

	struct RSX_detiler_config
	{
		u32 tile_base_address;
		u32 tile_base_offset;
		u32 tile_rw_offset;
		u32 tile_size;
		u32 tile_pitch;
		u32 bank;

		const vk::buffer* dst;
		u32 dst_offset;
		const vk::buffer* src;
		u32 src_offset;

		u16 image_width;
		u16 image_height;
		u32 image_pitch;
		u8  image_bpp;
	};

	template <RSX_detiler_op Op>
	struct cs_tile_memcpy : compute_task
	{
#pragma pack (push, 1)
		struct
		{
			u32 prime;
			u32 factor;
			u32 num_tiles_per_row;
			u32 tile_base_address;
			u32 tile_size;
			u32 tile_address_offset;
			u32 tile_rw_offset;
			u32 tile_pitch;
			u32 tile_bank;
			u32 image_width;
			u32 image_height;
			u32 image_pitch;
			u32 image_bpp;
		} params;
#pragma pack (pop)

		const vk::buffer* src_buffer = nullptr;
		const vk::buffer* dst_buffer = nullptr;
		u32 in_offset = 0;
		u32 out_offset = 0;
		u32 in_block_length = 0;
		u32 out_block_length = 0;

		cs_tile_memcpy()
		{
			ssbo_count = 2;
			use_push_constants = true;
			push_constants_size = sizeof(params);

			create();

			m_src =
			#include "../Program/GLSLSnippets/RSXMemoryTiling.glsl"
				;

			const std::pair<std::string_view, std::string> syntax_replace[] =
			{
				{ "%loc", "0" },
				{ "%set", "set = 0" },
				{ "%push_block", "push_constant" },
				{ "%ws", std::to_string(optimal_group_size) },
				{ "%op", std::to_string(Op) }
			};

			m_src = fmt::replace_all(m_src, syntax_replace);
		}

		void bind_resources(const vk::command_buffer& cmd) override
		{
			set_parameters(cmd);

			const auto op = static_cast<u32>(Op);
			m_program->bind_uniform({ *src_buffer, in_offset, in_block_length }, 0u, 0u ^ op);
			m_program->bind_uniform({ *dst_buffer, out_offset, out_block_length }, 0u, 1u ^ op);
		}

		void set_parameters(const vk::command_buffer& cmd)
		{
			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, &params);
		}

		void run(const vk::command_buffer& cmd, const RSX_detiler_config& config)
		{
			dst_buffer = config.dst;
			src_buffer = config.src;

			this->in_offset = config.src_offset;
			this->out_offset = config.dst_offset;

			const auto tile_aligned_height = std::min(
				utils::align<u32>(config.image_height, 64),
				utils::aligned_div(config.tile_size - config.tile_base_offset, config.tile_pitch)
			);

			if constexpr (Op == RSX_detiler_op::decode)
			{
				this->in_block_length = tile_aligned_height * config.tile_pitch;
				this->out_block_length = config.image_height * config.image_pitch;
			}
			else
			{
				this->in_block_length = config.image_height * config.image_pitch;
				this->out_block_length = tile_aligned_height * config.tile_pitch;
			}

			auto get_prime_factor = [](u32 pitch) -> std::pair<u32, u32>
			{
				const u32 base = (pitch >> 8);
				if ((pitch & (pitch - 1)) == 0)
				{
					return { 1u, base };
				}

				for (const auto prime : { 3, 5, 7, 11, 13 })
				{
					if ((base % prime) == 0)
					{
						return { prime, base / prime };
					}
				}

				rsx_log.error("Unexpected pitch value 0x%x", pitch);
				return {};
			};

			const auto [prime, factor] = get_prime_factor(config.tile_pitch);
			const u32 tiles_per_row = prime * factor;

			params.prime = prime;
			params.factor = factor;
			params.num_tiles_per_row = tiles_per_row;
			params.tile_base_address = config.tile_base_address;
			params.tile_rw_offset = config.tile_rw_offset;
			params.tile_size = config.tile_size;
			params.tile_address_offset = config.tile_base_offset;
			params.tile_pitch = config.tile_pitch;
			params.tile_bank = config.bank;
			params.image_width = config.image_width;
			params.image_height = (Op == RSX_detiler_op::decode) ? tile_aligned_height : config.image_height;
			params.image_pitch = config.image_pitch;
			params.image_bpp = config.image_bpp;

			const u32 subtexels_per_invocation = (config.image_bpp < 4) ? (4 / config.image_bpp) : 1;
			const u32 virtual_width = config.image_width / subtexels_per_invocation;
			const u32 invocations_x = utils::aligned_div(virtual_width, optimal_group_size);
			compute_task::run(cmd, invocations_x, config.image_height, 1);
		}
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<vk::compute_task>> g_compute_tasks;

	template<class T>
	T* get_compute_task()
	{
		u32 index = stx::typeindex<id_manager::typeinfo, T>();
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
