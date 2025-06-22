#include "../../vkutils/barriers.h"
#include "../../VKHelpers.h"
#include "../../VKResourceManager.h"

#include "../fsr_pass.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#define A_CPU 1
#include "3rdparty/GPUOpen/include/ffx_a.h"
#include "3rdparty/GPUOpen/include/ffx_fsr1.h"
#undef A_CPU

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace vk
{
	namespace FidelityFX
	{
		fsr_pass::fsr_pass(const std::string& config_definitions, u32 push_constants_size_)
		{
			// Just use AMD-provided source with minimal modification
			const char* shader_core =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ubershader.glsl"
			;

			// Replacements
			const char* ffx_a_contents =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ffx_a_flattened.inc"
			;

			const char* ffx_fsr_contents =
				#include "Emu/RSX/Program/Upscalers/FSR1/fsr_ffx_fsr1_flattened.inc"
			;

			const std::pair<std::string_view, std::string> replacement_table[] =
			{
				{ "%FFX_DEFINITIONS%", config_definitions },
				{ "%FFX_A_IMPORT%", ffx_a_contents },
				{ "%FFX_FSR_IMPORT%", ffx_fsr_contents },
				{ "%push_block%", "push_constant" }
			};

			m_src = shader_core;
			m_src = fmt::replace_all(m_src, replacement_table);

			// Fill with 0 to avoid sending incomplete/unused variables to the GPU
			memset(m_constants_buf, 0, sizeof(m_constants_buf));

			// No ssbo usage
			ssbo_count = 0;

			// Enable push constants
			use_push_constants = true;
			push_constants_size = push_constants_size_;

			create();
		}

		std::vector<glsl::program_input> fsr_pass::get_inputs()
		{
			std::vector<vk::glsl::program_input> inputs =
			{
				glsl::program_input::make(
					::glsl::program_domain::glsl_compute_program,
					"InputTexture",
					vk::glsl::input_type_texture,
					0,
					0
				),

				glsl::program_input::make(
					::glsl::program_domain::glsl_compute_program,
					"OutputTexture",
					vk::glsl::input_type_storage_texture,
					0,
					1
				),
			};

			auto result = compute_task::get_inputs();
			result.insert(result.end(), inputs.begin(), inputs.end());
			return result;
		}

		void fsr_pass::bind_resources(const vk::command_buffer& /*cmd*/)
		{
			// Bind relevant stuff
			if (!m_sampler)
			{
				const auto pdev = vk::get_current_renderer();
				m_sampler = std::make_unique<vk::sampler>(*pdev,
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					VK_FALSE, 0.f, 1.f, 0.f, 0.f, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
			}

			m_program->bind_uniform({ m_sampler->value, m_input_image->value, m_input_image->image()->current_layout }, 0, 0);
			m_program->bind_uniform({ VK_NULL_HANDLE, m_output_image->value, m_output_image->image()->current_layout }, 0, 1);
		}

		void fsr_pass::run(const vk::command_buffer& cmd, vk::viewable_image* src, vk::viewable_image* dst, const size2u& input_size, const size2u& output_size)
		{
			m_input_image = src->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY));
			m_output_image = dst->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY));
			m_input_size = input_size;
			m_output_size = output_size;

			if (!m_program)
			{
				load_program(cmd);
			}

			configure(cmd);

			constexpr auto wg_size = 16;
			const auto invocations_x = utils::aligned_div(output_size.width, wg_size);
			const auto invocations_y = utils::aligned_div(output_size.height, wg_size);

			ensure(invocations_x == (output_size.width + (wg_size - 1)) / wg_size);
			ensure(invocations_y == (output_size.height + (wg_size - 1)) / wg_size);
			compute_task::run(cmd, invocations_x, invocations_y, 1);
		}

		easu_pass::easu_pass()
			: fsr_pass(
				"#define SAMPLE_EASU 1\n"
				"#define SAMPLE_RCAS 0\n"
				"#define SAMPLE_BILINEAR 0\n"
				"#define SAMPLE_SLOW_FALLBACK 1",
				80 // 5*VEC4
			)
		{}

		void easu_pass::configure(const vk::command_buffer& cmd)
		{
			auto src_image = m_input_image->image();

			// NOTE: Configuration vector 4 is unused as we do not support HDR natively
			auto con0 = &m_constants_buf[0];
			auto con1 = &m_constants_buf[4];
			auto con2 = &m_constants_buf[8];
			auto con3 = &m_constants_buf[12];

			FsrEasuCon(con0, con1, con2, con3,
				static_cast<f32>(m_input_size.width), static_cast<f32>(m_input_size.height),     // Incoming viewport size to upscale (actual size)
				static_cast<f32>(src_image->width()), static_cast<f32>(src_image->height()),     // Size of the raw image to upscale (in case viewport does not cover it all)
				static_cast<f32>(m_output_size.width), static_cast<f32>(m_output_size.height));  // Size of output viewport (target size)

			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, m_constants_buf);
		}

		rcas_pass::rcas_pass()
			: fsr_pass(
				"#define SAMPLE_RCAS 1\n"
				"#define SAMPLE_EASU 0\n"
				"#define SAMPLE_BILINEAR 0\n"
				"#define SAMPLE_SLOW_FALLBACK 1",
				32 // 2*VEC4
			)
		{}

		void rcas_pass::configure(const vk::command_buffer& cmd)
		{
			// 0 is actually the sharpest with 2 being the chosen limit. Each progressive unit 'halves' the sharpening intensity.
			auto cas_attenuation = 2.f - (g_cfg.video.vk.rcas_sharpening_intensity / 50.f);
			FsrRcasCon(&m_constants_buf[0], cas_attenuation);

			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, m_constants_buf);
		}

	} // Namespace FidelityFX

	void fsr_upscale_pass::dispose_images()
	{
		auto safe_delete = [](auto& data)
		{
			if (data && data->value)
			{
				vk::get_resource_manager()->dispose(data);
			}
			else if (data)
			{
				data.reset();
			}
		};

		safe_delete(m_output_left);
		safe_delete(m_output_right);
		safe_delete(m_intermediate_data);
	}

	void fsr_upscale_pass::initialize_image(u32 output_w, u32 output_h, rsx::flags32_t mode)
	{
		dispose_images();

		const auto pdev = vk::get_current_renderer();
		auto initialize_image_impl = [pdev, output_w, output_h](VkImageUsageFlags usage, VkFormat format)
		{
			return std::make_unique<vk::viewable_image>(
				*pdev,                                               // Owner
				pdev->get_memory_mapping().device_local,             // Must be in device optimal memory
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D,
				format,
				output_w, output_h, 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,  // Dimensions (w, h, d, mips, layers, samples)
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_IMAGE_CREATE_ALLOW_NULL_RPCS3,                          // Allow creation to fail if there is no memory
				VMM_ALLOCATION_POOL_SWAPCHAIN,
				RSX_FORMAT_CLASS_COLOR);
		};

		const VkFlags usage_mask_output = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		const VkFlags usage_mask_intermediate = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		bool failed = true;
		VkFormat data_format = VK_FORMAT_UNDEFINED;

		// Check if it is possible to actually write to the format we want.
		// Fallback to RGBA8 is supported as well
		std::array<VkFormat, 2> supported_formats = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
		for (const auto& format : supported_formats)
		{
			const VkFlags all_required_bits = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
			if ((pdev->get_format_properties(format).optimalTilingFeatures & all_required_bits) == all_required_bits)
			{
				data_format = format;
				failed = false;
				break;
			}
		}

		if ((mode & UPSCALE_LEFT_VIEW) && !failed)
		{
			m_output_left = initialize_image_impl(usage_mask_output, data_format);
			failed |= (m_output_left->value == VK_NULL_HANDLE);
		}

		if ((mode & UPSCALE_RIGHT_VIEW) && !failed)
		{
			m_output_right = initialize_image_impl(usage_mask_output, data_format);
			failed |= (m_output_right->value == VK_NULL_HANDLE);
		}

		if (!failed)
		{
			m_intermediate_data = initialize_image_impl(usage_mask_intermediate, data_format);
			failed |= (m_intermediate_data->value == VK_NULL_HANDLE);
		}

		if (failed)
		{
			if (data_format != VK_FORMAT_UNDEFINED)
			{
				dispose_images();
				rsx_log.warning("FSR is enabled, but the system is out of memory. Will fall back to bilinear upscaling.");
			}
			else
			{
				ensure(!m_output_left && !m_output_right && !m_intermediate_data);
				rsx_log.error("FSR is not supported by this driver and hardware combination.");
			}
		}
	}

	vk::viewable_image* fsr_upscale_pass::scale_output(
		const vk::command_buffer& cmd,
		vk::viewable_image* src,
		VkImage present_surface,
		VkImageLayout present_surface_layout,
		const VkImageBlit& request,
		rsx::flags32_t mode)
	{
		size2u input_size, output_size;
		input_size.width = std::abs(request.srcOffsets[1].x - request.srcOffsets[0].x);
		input_size.height = std::abs(request.srcOffsets[1].y - request.srcOffsets[0].y);
		output_size.width = std::abs(request.dstOffsets[1].x - request.dstOffsets[0].x);
		output_size.height = std::abs(request.dstOffsets[1].y - request.dstOffsets[0].y);

		auto src_image = src;
		auto target_image = present_surface;
		auto target_image_layout = present_surface_layout;
		auto output_request = request;

		if (input_size.width < output_size.width && input_size.height < output_size.height)
		{
			// Cannot upscale both LEFT and RIGHT images at the same time.
			// Default maps to LEFT for simplicity
			ensure((mode & (UPSCALE_LEFT_VIEW | UPSCALE_RIGHT_VIEW)) != (UPSCALE_LEFT_VIEW | UPSCALE_RIGHT_VIEW));

			auto& m_output_data = (mode & UPSCALE_LEFT_VIEW) ? m_output_left : m_output_right;
			if (!m_output_data || m_output_data->width() != output_size.width || m_output_data->height() != output_size.height)
			{
				initialize_image(output_size.width, output_size.height, mode);
			}

			if (m_output_data)
			{
				// Execute the pass here
				auto cs_easu_task = vk::get_compute_task<vk::FidelityFX::easu_pass>();
				auto cs_rcas_task = vk::get_compute_task<vk::FidelityFX::rcas_pass>();

				// Prepare for EASU pass
				src->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				if (m_intermediate_data->current_layout != VK_IMAGE_LAYOUT_GENERAL)
				{
					m_intermediate_data->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
				}
				else
				{
					// R/W CS-CS barrier in case of back-to-back upscales
					vk::insert_image_memory_barrier(cmd,
						m_intermediate_data->value,
						m_intermediate_data->current_layout, m_intermediate_data->current_layout,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_ACCESS_SHADER_READ_BIT,
						VK_ACCESS_SHADER_WRITE_BIT,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
				}

				// EASU
				cs_easu_task->run(cmd, src, m_intermediate_data.get(), input_size, output_size);

				// Prepare for RCAS pass
				m_output_data->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);

				// R/W CS-CS barrier before RCAS
				vk::insert_image_memory_barrier(cmd,
					m_intermediate_data->value,
					m_intermediate_data->current_layout, m_intermediate_data->current_layout,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

				// RCAS
				cs_rcas_task->run(cmd, m_intermediate_data.get(), m_output_data.get(), input_size, output_size);

				// Cleanup
				src->pop_layout(cmd);

				// Swap input for FSR target
				src_image = m_output_data.get();

				// Update output parameters to match expected output
				if (mode & UPSCALE_AND_COMMIT)
				{
					// Explicit CS-Transfer barrier
					vk::insert_image_memory_barrier(cmd,
						m_output_data->value,
						m_output_data->current_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_ACCESS_SHADER_WRITE_BIT,
						VK_ACCESS_TRANSFER_READ_BIT,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

					m_output_data->current_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

					output_request.srcOffsets[0].x = 0;
					output_request.srcOffsets[1].x = output_size.width;
					output_request.srcOffsets[0].y = 0;
					output_request.srcOffsets[1].y = output_size.height;

					// Preserve mirroring/flipping
					if (request.srcOffsets[0].x > request.srcOffsets[1].x)
					{
						std::swap(output_request.srcOffsets[0].x, output_request.srcOffsets[1].x);
					}

					if (request.srcOffsets[0].y > request.srcOffsets[1].y)
					{
						std::swap(output_request.srcOffsets[0].y, output_request.srcOffsets[1].y);
					}
				}
			}
		}

		if (mode & UPSCALE_AND_COMMIT)
		{
			src_image->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			vkCmdBlitImage(cmd, src_image->value, src_image->current_layout, target_image, target_image_layout, 1, &output_request, VK_FILTER_LINEAR);
			src_image->pop_layout(cmd);
			return nullptr;
		}

		return src_image;
	}
}
