#include "stdafx.h"
#include "../Common/BufferUtils.h"
#include "../rsx_methods.h"

#include "VKAsyncScheduler.h"
#include "VKGSRender.h"
#include "vkutils/buffer_object.h"
#include "vkutils/chip_class.h"
#include <vulkan/vulkan_core.h>

namespace vk
{
	VkImageViewType get_view_type(rsx::texture_dimension_extended type)
	{
		switch (type)
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			return VK_IMAGE_VIEW_TYPE_1D;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			return VK_IMAGE_VIEW_TYPE_2D;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			return VK_IMAGE_VIEW_TYPE_CUBE;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			return VK_IMAGE_VIEW_TYPE_3D;
		default: fmt::throw_exception("Unreachable");
		}
	}

	VkCompareOp get_compare_func(rsx::comparison_function op, bool reverse_direction = false)
	{
		switch (op)
		{
		case rsx::comparison_function::never: return VK_COMPARE_OP_NEVER;
		case rsx::comparison_function::greater: return reverse_direction ? VK_COMPARE_OP_LESS: VK_COMPARE_OP_GREATER;
		case rsx::comparison_function::less: return reverse_direction ? VK_COMPARE_OP_GREATER: VK_COMPARE_OP_LESS;
		case rsx::comparison_function::less_or_equal: return reverse_direction ? VK_COMPARE_OP_GREATER_OR_EQUAL: VK_COMPARE_OP_LESS_OR_EQUAL;
		case rsx::comparison_function::greater_or_equal: return reverse_direction ? VK_COMPARE_OP_LESS_OR_EQUAL: VK_COMPARE_OP_GREATER_OR_EQUAL;
		case rsx::comparison_function::equal: return VK_COMPARE_OP_EQUAL;
		case rsx::comparison_function::not_equal: return VK_COMPARE_OP_NOT_EQUAL;
		case rsx::comparison_function::always: return VK_COMPARE_OP_ALWAYS;
		default:
			fmt::throw_exception("Unknown compare op: 0x%x", static_cast<u32>(op));
		}
	}

	void validate_image_layout_for_read_access(
		vk::command_buffer& cmd,
		vk::image_view* view,
		VkPipelineStageFlags dst_stage,
		const rsx::sampled_image_descriptor_base* sampler_state)
	{
		switch (auto raw = view->image(); +raw->current_layout)
		{
		default:
			//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			//ensure(sampler_state->upload_context == rsx::texture_upload_context::blit_engine_dst);
			raw->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			ensure(sampler_state->upload_context == rsx::texture_upload_context::blit_engine_src);
			raw->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
		case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
			ensure(sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage);
			if (!sampler_state->is_cyclic_reference)
			{
				// This was used in a cyclic ref before, but is missing a barrier
				// No need for a full stall, use a custom barrier instead
				VkPipelineStageFlags src_stage;
				VkAccessFlags src_access;
				if (raw->aspect() == VK_IMAGE_ASPECT_COLOR_BIT)
				{
					src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				}
				else
				{
					src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				}

				vk::insert_image_memory_barrier(
					cmd,
					raw->value,
					raw->current_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					src_stage, dst_stage,
					src_access, VK_ACCESS_SHADER_READ_BIT,
					{ raw->aspect(), 0, 1, 0, 1 });

				raw->current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			ensure(sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage);
			raw->change_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			break;
		}
	}
}

void VKGSRender::begin_render_pass()
{
	vk::begin_renderpass(
		*m_current_command_buffer,
		get_render_pass(),
		m_draw_fbo->value,
		{ positionu{0u, 0u}, sizeu{m_draw_fbo->width(), m_draw_fbo->height()} });
}

void VKGSRender::close_render_pass()
{
	vk::end_renderpass(*m_current_command_buffer);
}

VkRenderPass VKGSRender::get_render_pass()
{
	if (!m_cached_renderpass)
	{
		m_cached_renderpass = vk::get_renderpass(*m_device, m_current_renderpass_key);
	}

	return m_cached_renderpass;
}

void VKGSRender::update_draw_state()
{
	m_profiler.start();

	// Update conditional dynamic state
	if (rsx::method_registers.current_draw_clause.primitive >= rsx::primitive_type::points &&   // AMD/AMDVLK driver does not like it if you render points without setting line width for some reason
		rsx::method_registers.current_draw_clause.primitive <= rsx::primitive_type::line_strip)
	{
		const float actual_line_width =
			m_device->get_wide_lines_support() ? rsx::method_registers.line_width() * rsx::get_resolution_scale() : 1.f;
		vkCmdSetLineWidth(*m_current_command_buffer, actual_line_width);
	}

	if (rsx::method_registers.blend_enabled())
	{
		// Update blend constants
		auto blend_colors = rsx::get_constant_blend_colors();
		vkCmdSetBlendConstants(*m_current_command_buffer, blend_colors.data());
	}

	if (rsx::method_registers.stencil_test_enabled())
	{
		const bool two_sided_stencil = rsx::method_registers.two_sided_stencil_test_enabled();
		VkStencilFaceFlags face_flag = (two_sided_stencil) ? VK_STENCIL_FACE_FRONT_BIT : VK_STENCIL_FRONT_AND_BACK;

		vkCmdSetStencilWriteMask(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_mask());
		vkCmdSetStencilCompareMask(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_func_mask());
		vkCmdSetStencilReference(*m_current_command_buffer, face_flag, rsx::method_registers.stencil_func_ref());

		if (two_sided_stencil)
		{
			vkCmdSetStencilWriteMask(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_mask());
			vkCmdSetStencilCompareMask(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_func_mask());
			vkCmdSetStencilReference(*m_current_command_buffer, VK_STENCIL_FACE_BACK_BIT, rsx::method_registers.back_stencil_func_ref());
		}
	}

	// The remaining dynamic state should only be set once and we have signals to enable/disable mid-renderpass
	if (!(m_current_command_buffer->flags & vk::command_buffer::cb_reload_dynamic_state))
	{
		// Dynamic state already set
		m_frame_stats.setup_time += m_profiler.duration();
		return;
	}

	if (rsx::method_registers.poly_offset_fill_enabled())
	{
		// offset_bias is the constant factor, multiplied by the implementation factor R
		// offst_scale is the slope factor, multiplied by the triangle slope factor M
		// R is implementation dependent and has to be derived empirically for supported implementations.
		// Lucky for us, only NVIDIA currently supports fixed-point 24-bit depth buffers.

		const auto polygon_offset_scale = rsx::method_registers.poly_offset_scale();
		auto polygon_offset_bias = rsx::method_registers.poly_offset_bias();

		if (m_draw_fbo->depth_format() == VK_FORMAT_D24_UNORM_S8_UINT && is_NVIDIA(vk::get_chip_family()))
		{
			// Empirically derived to be 0.5 * (2^24 - 1) for fixed type on Pascal. The same seems to apply for other NVIDIA GPUs.
			// RSX seems to be using 2^24 - 1 instead making the biases twice as large when using fixed type Z-buffer on NVIDIA.
			// Note, that the formula for floating point is complicated, but actually works out for us.
			// Since the exponent range for a polygon is around 0, and we have 23 (+1) mantissa bits, R just works out to the same range by chance \o/.
			polygon_offset_bias *= 0.5f;
		}

		vkCmdSetDepthBias(*m_current_command_buffer, polygon_offset_bias, 0.f, polygon_offset_scale);
	}
	else
	{
		// Zero bias value - disables depth bias
		vkCmdSetDepthBias(*m_current_command_buffer, 0.f, 0.f, 0.f);
	}

	if (m_device->get_depth_bounds_support())
	{
		f32 bounds_min, bounds_max;
		if (rsx::method_registers.depth_bounds_test_enabled())
		{
			// Update depth bounds min/max
			bounds_min = rsx::method_registers.depth_bounds_min();
			bounds_max = rsx::method_registers.depth_bounds_max();
		}
		else
		{
			// Avoid special case where min=max and depth bounds (incorrectly) fails
			bounds_min = std::min(0.f, rsx::method_registers.clip_min());
			bounds_max = std::max(1.f, rsx::method_registers.clip_max());
		}

		if (!m_device->get_unrestricted_depth_range_support())
		{
			bounds_min = std::clamp(bounds_min, 0.f, 1.f);
			bounds_max = std::clamp(bounds_max, 0.f, 1.f);
		}

		vkCmdSetDepthBounds(*m_current_command_buffer, bounds_min, bounds_max);
	}

	bind_viewport();

	m_current_command_buffer->flags &= ~vk::command_buffer::cb_reload_dynamic_state;
	m_graphics_state.clear(rsx::pipeline_state::polygon_offset_state_dirty | rsx::pipeline_state::depth_bounds_state_dirty);
	m_frame_stats.setup_time += m_profiler.duration();
}

void VKGSRender::load_texture_env()
{
	// Load textures
	bool check_for_cyclic_refs = false;
	auto check_surface_cache_sampler = [&](auto descriptor, const auto& tex)
	{
		if (!m_texture_cache.test_if_descriptor_expired(*m_current_command_buffer, m_rtts, descriptor, tex))
		{
			check_for_cyclic_refs |= descriptor->is_cyclic_reference;
			return true;
		}

		return false;
	};

	auto get_border_color = [&](const rsx::Texture auto& tex)
	{
		return  m_device->get_custom_border_color_support().require_border_color_remap
			? tex.remapped_border_color()
			: rsx::decode_border_color(tex.border_color());
	};

	std::lock_guard lock(m_sampler_mutex);

	for (u32 textures_ref = current_fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
			continue;

		if (!fs_sampler_state[i])
			fs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

		auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());
		const auto& tex = rsx::method_registers.fragment_textures[i];
		const auto previous_format_class = fs_sampler_state[i]->format_class;

		if (m_samplers_dirty || m_textures_dirty[i] || !check_surface_cache_sampler(sampler_state, tex))
		{
			if (tex.enabled())
			{
				*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, tex, m_rtts);
			}
			else
			{
				*sampler_state = {};
			}

			if (sampler_state->validate())
			{
				if (sampler_state->is_cyclic_reference)
				{
					check_for_cyclic_refs |= true;
				}

				if (!m_textures_dirty[i] && sampler_state->format_class != previous_format_class)
				{
					// Host details changed but RSX is not aware
					m_graphics_state |= rsx::fragment_program_state_dirty;
				}

				bool replace = !fs_sampler_handles[i];
				VkFilter mag_filter;
				vk::minification_filter min_filter;
				f32 min_lod = 0.f, max_lod = 0.f;
				f32 lod_bias = 0.f;

				const u32 texture_format = tex.format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
				VkBool32 compare_enabled = VK_FALSE;
				VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER;

				if (texture_format >= CELL_GCM_TEXTURE_DEPTH24_D8 && texture_format <= CELL_GCM_TEXTURE_DEPTH16_FLOAT)
				{
					compare_enabled = VK_TRUE;
					depth_compare_mode = vk::get_compare_func(tex.zfunc(), true);
				}

				const f32 af_level = vk::max_aniso(tex.max_aniso());
				const auto wrap_s = vk::vk_wrap_mode(tex.wrap_s());
				const auto wrap_t = vk::vk_wrap_mode(tex.wrap_t());
				const auto wrap_r = vk::vk_wrap_mode(tex.wrap_r());

				// NOTE: In vulkan, the border color can bypass the sample swizzle stage.
				// Check the device properties to determine whether to pre-swizzle the colors or not.
				const auto border_color = rsx::is_border_clamped_texture(tex)
					? vk::border_color_t(get_border_color(tex))
					: vk::border_color_t(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

				// Check if non-point filtering can even be used on this format
				bool can_sample_linear;
				if (sampler_state->format_class == RSX_FORMAT_CLASS_COLOR) [[likely]]
				{
					// Most PS3-like formats can be linearly filtered without problem
					can_sample_linear = true;
				}
				else if (sampler_state->format_class != rsx::classify_format(texture_format) &&
					(texture_format == CELL_GCM_TEXTURE_A8R8G8B8 || texture_format == CELL_GCM_TEXTURE_D8R8G8B8))
				{
					// Depth format redirected to BGRA8 resample stage. Do not filter to avoid bits leaking
					can_sample_linear = false;
				}
				else
				{
					// Not all GPUs support linear filtering of depth formats
					const auto vk_format = sampler_state->image_handle ? sampler_state->image_handle->image()->format() :
						vk::get_compatible_sampler_format(m_device->get_formats_support(), sampler_state->external_subresource_desc.gcm_format);

					can_sample_linear = m_device->get_format_properties(vk_format).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
				}

				const auto mipmap_count = tex.get_exact_mipmap_count();
				min_filter = vk::get_min_filter(tex.min_filter());

				if (can_sample_linear)
				{
					mag_filter = vk::get_mag_filter(tex.mag_filter());
				}
				else
				{
					mag_filter = VK_FILTER_NEAREST;
					min_filter.filter = VK_FILTER_NEAREST;
				}

				if (min_filter.sample_mipmaps && mipmap_count > 1)
				{
					f32 actual_mipmaps;
					if (sampler_state->upload_context == rsx::texture_upload_context::shader_read)
					{
						actual_mipmaps = static_cast<f32>(mipmap_count);
					}
					else if (sampler_state->external_subresource_desc.op == rsx::deferred_request_command::mipmap_gather)
					{
						// Clamp min and max lod
						actual_mipmaps = static_cast<f32>(sampler_state->external_subresource_desc.sections_to_copy.size());
					}
					else
					{
						actual_mipmaps = 1.f;
					}

					if (actual_mipmaps > 1.f)
					{
						min_lod = tex.min_lod();
						max_lod = tex.max_lod();
						lod_bias = tex.bias();

						min_lod = std::min(min_lod, actual_mipmaps - 1.f);
						max_lod = std::min(max_lod, actual_mipmaps - 1.f);

						if (min_filter.mipmap_mode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
						{
							// Round to nearest 0.5 to work around some broken games
							// Unlike openGL, sampler parameters cannot be dynamically changed on vulkan, leading to many permutations
							lod_bias = std::floor(lod_bias * 2.f + 0.5f) * 0.5f;
						}
					}
					else
					{
						min_lod = max_lod = lod_bias = 0.f;
						min_filter.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
					}
				}

				if (fs_sampler_handles[i] && m_textures_dirty[i])
				{
					if (!fs_sampler_handles[i]->matches(wrap_s, wrap_t, wrap_r, false, lod_bias, af_level, min_lod, max_lod,
						min_filter.filter, mag_filter, min_filter.mipmap_mode, border_color, compare_enabled, depth_compare_mode))
					{
						replace = true;
					}
				}

				if (replace)
				{
					fs_sampler_handles[i] = vk::get_resource_manager()->get_sampler(
						*m_device,
						fs_sampler_handles[i],
						wrap_s, wrap_t, wrap_r,
						false,
						lod_bias, af_level, min_lod, max_lod,
						min_filter.filter, mag_filter, min_filter.mipmap_mode,
						border_color, compare_enabled, depth_compare_mode);
				}
			}

			m_textures_dirty[i] = false;
		}
	}

	for (u32 textures_ref = current_vp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
			continue;

		if (!vs_sampler_state[i])
			vs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

		auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
		const auto& tex = rsx::method_registers.vertex_textures[i];
		const auto previous_format_class = sampler_state->format_class;

		if (m_samplers_dirty || m_vertex_textures_dirty[i] || !check_surface_cache_sampler(sampler_state, tex))
		{
			if (rsx::method_registers.vertex_textures[i].enabled())
			{
				*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, tex, m_rtts);
			}
			else
			{
				*sampler_state = {};
			}

			if (sampler_state->validate())
			{
				if (sampler_state->is_cyclic_reference || sampler_state->external_subresource_desc.do_not_cache)
				{
					check_for_cyclic_refs |= true;
				}

				if (!m_vertex_textures_dirty[i] && sampler_state->format_class != previous_format_class)
				{
					// Host details changed but RSX is not aware
					m_graphics_state |= rsx::vertex_program_state_dirty;
				}

				bool replace = !vs_sampler_handles[i];
				const VkBool32 unnormalized_coords = !!(tex.format() & CELL_GCM_TEXTURE_UN);
				const auto min_lod = tex.min_lod();
				const auto max_lod = tex.max_lod();
				const auto wrap_s = vk::vk_wrap_mode(tex.wrap_s());
				const auto wrap_t = vk::vk_wrap_mode(tex.wrap_t());

				// NOTE: In vulkan, the border color can bypass the sample swizzle stage.
				// Check the device properties to determine whether to pre-swizzle the colors or not.
				const auto border_color = is_border_clamped_texture(tex)
					? vk::border_color_t(get_border_color(tex))
					: vk::border_color_t(VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);

				if (vs_sampler_handles[i])
				{
					if (!vs_sampler_handles[i]->matches(wrap_s, wrap_t, VK_SAMPLER_ADDRESS_MODE_REPEAT,
						unnormalized_coords, 0.f, 1.f, min_lod, max_lod, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color))
					{
						replace = true;
					}
				}

				if (replace)
				{
					vs_sampler_handles[i] = vk::get_resource_manager()->get_sampler(
						*m_device,
						vs_sampler_handles[i],
						wrap_s, wrap_t, VK_SAMPLER_ADDRESS_MODE_REPEAT,
						unnormalized_coords,
						0.f, 1.f, min_lod, max_lod,
						VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color);
				}
			}

			m_vertex_textures_dirty[i] = false;
		}
	}

	m_samplers_dirty.store(false);

	if (check_for_cyclic_refs)
	{
		// Regenerate renderpass key
		if (const auto key = vk::get_renderpass_key(m_fbo_images, m_current_renderpass_key);
			key != m_current_renderpass_key)
		{
			m_current_renderpass_key = key;
			m_cached_renderpass = VK_NULL_HANDLE;
		}
	}

	if (g_cfg.video.vk.asynchronous_texture_streaming)
	{
		// We have to do this here, because we have to assume the CB will be dumped
		auto& async_task_scheduler = g_fxo->get<vk::AsyncTaskScheduler>();

		if (async_task_scheduler.is_recording() &&
			!async_task_scheduler.is_host_mode())
		{
			// Sync any async scheduler tasks
			if (auto ev = async_task_scheduler.get_primary_sync_label())
			{
				ev->gpu_wait(*m_current_command_buffer, m_async_compute_dependency_info);
			}
		}
	}
}

bool VKGSRender::bind_texture_env()
{
	bool out_of_memory = false;

	for (u32 textures_ref = current_fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
			continue;

		vk::image_view* view = nullptr;
		auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

		if (rsx::method_registers.fragment_textures[i].enabled() &&
			sampler_state->validate())
		{
			if (view = sampler_state->image_handle; !view)
			{
				//Requires update, copy subresource
				if (!(view = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc)))
				{
					out_of_memory = true;
				}
			}
			else
			{
				validate_image_layout_for_read_access(*m_current_command_buffer, view, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, sampler_state);
			}
		}

		if (view) [[likely]]
		{
			m_program->bind_uniform({ fs_sampler_handles[i]->value, view->value, view->image()->current_layout },
				vk::glsl::binding_set_index_fragment,
				m_fs_binding_table->ftex_location[i]);

			if (current_fragment_program.texture_state.redirected_textures & (1 << i))
			{
				// Stencil mirror required
				auto root_image = static_cast<vk::viewable_image*>(view->image());
				auto stencil_view = root_image->get_view(rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

				if (!m_stencil_mirror_sampler)
				{
					m_stencil_mirror_sampler = std::make_unique<vk::sampler>(*m_device,
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
						VK_FALSE, 0.f, 1.f, 0.f, 0.f,
						VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
						VK_BORDER_COLOR_INT_OPAQUE_BLACK);
				}

				m_program->bind_uniform({ m_stencil_mirror_sampler->value, stencil_view->value, stencil_view->image()->current_layout },
					vk::glsl::binding_set_index_fragment,
					m_fs_binding_table->ftex_stencil_location[i]);
			}
		}
		else
		{
			const VkImageViewType view_type = vk::get_view_type(current_fragment_program.get_texture_dimension(i));
			m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				vk::glsl::binding_set_index_fragment,
				m_fs_binding_table->ftex_location[i]);

			if (current_fragment_program.texture_state.redirected_textures & (1 << i))
			{
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					vk::glsl::binding_set_index_fragment,
					m_fs_binding_table->ftex_stencil_location[i]);
			}
		}
	}

	for (u32 textures_ref = current_vp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
			continue;

		if (!rsx::method_registers.vertex_textures[i].enabled())
		{
			const auto view_type = vk::get_view_type(current_vertex_program.get_texture_dimension(i));
			m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				vk::glsl::binding_set_index_vertex,
				m_vs_binding_table->vtex_location[i]);

			continue;
		}

		auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
		auto image_ptr = sampler_state->image_handle;

		if (!image_ptr && sampler_state->validate())
		{
			if (!(image_ptr = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc)))
			{
				out_of_memory = true;
			}
		}

		if (!image_ptr)
		{
			rsx_log.error("Texture upload failed to vtexture index %d. Binding null sampler.", i);
			const auto view_type = vk::get_view_type(current_vertex_program.get_texture_dimension(i));

			m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				vk::glsl::binding_set_index_vertex,
				m_vs_binding_table->vtex_location[i]);

			continue;
		}

		validate_image_layout_for_read_access(*m_current_command_buffer, image_ptr, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, sampler_state);

		m_program->bind_uniform({ vs_sampler_handles[i]->value, image_ptr->value, image_ptr->image()->current_layout },
			vk::glsl::binding_set_index_vertex,
			m_vs_binding_table->vtex_location[i]);
	}

	return out_of_memory;
}

bool VKGSRender::bind_interpreter_texture_env()
{
	if (current_fp_metadata.referenced_textures_mask == 0)
	{
		// Nothing to do
		return false;
	}

	std::array<VkDescriptorImageInfo, 68> texture_env;
	VkDescriptorImageInfo fallback = { vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, VK_IMAGE_VIEW_TYPE_1D)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	auto start = texture_env.begin();
	auto end = start;

	// Fill default values
	// 1D
	std::advance(end, 16);
	std::fill(start, end, fallback);
	// 2D
	start = end;
	fallback.imageView = vk::null_image_view(*m_current_command_buffer, VK_IMAGE_VIEW_TYPE_2D)->value;
	std::advance(end, 16);
	std::fill(start, end, fallback);
	// 3D
	start = end;
	fallback.imageView = vk::null_image_view(*m_current_command_buffer, VK_IMAGE_VIEW_TYPE_3D)->value;
	std::advance(end, 16);
	std::fill(start, end, fallback);
	// CUBE
	start = end;
	fallback.imageView = vk::null_image_view(*m_current_command_buffer, VK_IMAGE_VIEW_TYPE_CUBE)->value;
	std::advance(end, 16);
	std::fill(start, end, fallback);

	bool out_of_memory = false;

	for (u32 textures_ref = current_fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
			continue;

		vk::image_view* view = nullptr;
		auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

		if (rsx::method_registers.fragment_textures[i].enabled() &&
			sampler_state->validate())
		{
			if (view = sampler_state->image_handle; !view)
			{
				//Requires update, copy subresource
				if (!(view = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc)))
				{
					out_of_memory = true;
				}
			}
			else
			{
				validate_image_layout_for_read_access(*m_current_command_buffer, view, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, sampler_state);
			}
		}

		if (view)
		{
			const int offsets[] = { 0, 16, 48, 32 };
			auto& sampled_image_info = texture_env[offsets[static_cast<u32>(sampler_state->image_type)] + i];
			sampled_image_info = { fs_sampler_handles[i]->value, view->value, view->image()->current_layout };
		}
	}

	m_shader_interpreter.update_fragment_textures(texture_env);
	return out_of_memory;
}

void VKGSRender::emit_geometry(u32 sub_index)
{
	auto &draw_call = rsx::method_registers.current_draw_clause;
	m_profiler.start();

	const rsx::flags32_t vertex_state_mask = rsx::vertex_base_changed | rsx::vertex_arrays_changed;
	const rsx::flags32_t state_flags = (sub_index == 0) ? rsx::vertex_arrays_changed : draw_call.execute_pipeline_dependencies(m_ctx);

	if (state_flags & rsx::vertex_arrays_changed)
	{
		m_draw_processor.analyse_inputs_interleaved(m_vertex_layout, current_vp_metadata);
	}
	else if (state_flags & rsx::vertex_base_changed)
	{
		// Rebase vertex bases instead of
		for (auto& info : m_vertex_layout.interleaved_blocks)
		{
			info->vertex_range.second = 0;
			const auto vertex_base_offset = rsx::method_registers.vertex_data_base_offset();
			info->real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(vertex_base_offset, info->base_offset), info->memory_location);
		}
	}
	else
	{
		// Discard cached results
		for (auto& info : m_vertex_layout.interleaved_blocks)
		{
			info->vertex_range.second = 0;
		}
	}

	if ((state_flags & vertex_state_mask) && !m_vertex_layout.validate())
	{
		// No vertex inputs enabled
		// Execute remainining pipeline barriers with NOP draw
		do
		{
			draw_call.execute_pipeline_dependencies(m_ctx);
		}
		while (draw_call.next());

		draw_call.end();
		return;
	}

	// Programs data is dependent on vertex state
	auto upload_info = upload_vertex_data();
	if (!upload_info.vertex_draw_count)
	{
		// Malformed vertex setup; abort
		return;
	}

	m_frame_stats.vertex_upload_time += m_profiler.duration();

	// Faults are allowed during vertex upload. Ensure consistent CB state after uploads.
	// Queries are spawned and closed outside render pass scope for consistency reasons.
	if (m_current_command_buffer->flags & vk::command_buffer::cb_load_occluson_task)
	{
		u32 occlusion_id = m_occlusion_query_manager->allocate_query(*m_current_command_buffer);
		if (occlusion_id == umax)
		{
			// Force flush
			rsx_log.warning("[Performance Warning] Out of free occlusion slots. Forcing hard sync.");
			ZCULL_control::sync(this);

			occlusion_id = m_occlusion_query_manager->allocate_query(*m_current_command_buffer);
			if (occlusion_id == umax)
			{
				//rsx_log.error("Occlusion pool overflow");
				if (m_current_task) m_current_task->result = 1;
			}
		}

		// Before starting a query, we need to match RP scope (VK_1_0 rules).
		// We always want our queries to start outside a renderpass whenever possible.
		// We ignore this for performance reasons whenever possible of course and only do this for sensitive drivers.
		if (vk::use_strict_query_scopes() &&
			vk::is_renderpass_open(*m_current_command_buffer))
		{
			vk::end_renderpass(*m_current_command_buffer);
			emergency_query_cleanup(m_current_command_buffer);
		}

		// Begin query
		m_occlusion_query_manager->begin_query(*m_current_command_buffer, occlusion_id);

		auto& data = m_occlusion_map[m_active_query_info->driver_handle];
		data.indices.push_back(occlusion_id);
		data.set_sync_command_buffer(m_current_command_buffer);

		m_current_command_buffer->flags &= ~vk::command_buffer::cb_load_occluson_task;
		m_current_command_buffer->flags |= (vk::command_buffer::cb_has_occlusion_task | vk::command_buffer::cb_has_open_query);
	}

	auto persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	auto volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;
	bool update_descriptors = false;

	if (m_current_draw.subdraw_id == 0)
	{
		update_descriptors = true;

		// Allocate stream layout memory for this batch
		m_vertex_layout_stream_info.range = rsx::method_registers.current_draw_clause.pass_count() * 128;
		m_vertex_layout_stream_info.offset = m_vertex_layout_ring_info.alloc<256>(m_vertex_layout_stream_info.range);

		if (vk::test_status_interrupt(vk::heap_changed))
		{
			if (m_vertex_layout_storage &&
				m_vertex_layout_storage->info.buffer != m_vertex_layout_ring_info.heap->value)
			{
				vk::get_resource_manager()->dispose(m_vertex_layout_storage);
			}

			vk::clear_status_interrupt(vk::heap_changed);
		}
	}

	// Update vertex fetch parameters
	update_vertex_env(sub_index, upload_info);

	ensure(m_vertex_layout_storage);
	if (update_descriptors)
	{
		m_program->bind_uniform(persistent_buffer, vk::glsl::binding_set_index_vertex, m_vs_binding_table->vertex_buffers_location);
		m_program->bind_uniform(volatile_buffer, vk::glsl::binding_set_index_vertex, m_vs_binding_table->vertex_buffers_location + 1);
		m_program->bind_uniform(m_vertex_layout_storage->value, vk::glsl::binding_set_index_vertex, m_vs_binding_table->vertex_buffers_location + 2);
	}

	bool reload_state = (!m_current_draw.subdraw_id++);
	vk::renderpass_op(*m_current_command_buffer, [&](const vk::command_buffer& cmd, VkRenderPass pass, VkFramebuffer fbo)
	{
		if (get_render_pass() == pass && m_draw_fbo->value == fbo)
		{
			// Nothing to do
			return;
		}

		if (pass)
		{
			// Subpass mismatch, end it before proceeding
			vk::end_renderpass(cmd);
		}

		// Starting a new renderpass should clobber dynamic state
		m_current_command_buffer->flags |= vk::command_buffer::cb_reload_dynamic_state;

		reload_state = true;
	});

	// Bind both pipe and descriptors in one go
	// FIXME: We only need to rebind the pipeline when reload state is set. Flags?
	m_program->bind(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

	if (reload_state)
	{
		update_draw_state();
		begin_render_pass();

		if (cond_render_ctrl.hw_cond_active && m_device->get_conditional_render_support())
		{
			// It is inconvenient that conditional rendering breaks other things like compute dispatch
			// TODO: If this is heavy, add refactor the resources into global and add checks around compute dispatch
			VkConditionalRenderingBeginInfoEXT info{};
			info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			info.buffer = m_cond_render_buffer->value;

			_vkCmdBeginConditionalRenderingEXT(*m_current_command_buffer, &info);
			m_current_command_buffer->flags |= vk::command_buffer::cb_has_conditional_render;
		}
	}

	// Bind the new set of descriptors for use with this draw call
	m_frame_stats.setup_time += m_profiler.duration();

	if (!upload_info.index_info)
	{
		if (draw_call.is_trivial_instanced_draw)
		{
			vkCmdDraw(*m_current_command_buffer, upload_info.vertex_draw_count, draw_call.pass_count(), 0, 0);
		}
		else if (draw_call.is_single_draw())
		{
			vkCmdDraw(*m_current_command_buffer, upload_info.vertex_draw_count, 1, 0, 0);
		}
		else if (m_device->get_multidraw_support())
		{
			const auto subranges = draw_call.get_subranges();
			auto ptr = utils::bless<const VkMultiDrawInfoEXT>(& subranges.front().first);
			_vkCmdDrawMultiEXT(*m_current_command_buffer, ::size32(subranges), ptr, 1, 0, sizeof(rsx::draw_range_t));
		}
		else
		{
			u32 vertex_offset = 0;
			const auto subranges = draw_call.get_subranges();
			for (const auto &range : subranges)
			{
				vkCmdDraw(*m_current_command_buffer, range.count, 1, vertex_offset, 0);
				vertex_offset += range.count;
			}
		}
	}
	else
	{
		const VkIndexType index_type = std::get<1>(*upload_info.index_info);
		const VkDeviceSize offset = std::get<0>(*upload_info.index_info);

		vkCmdBindIndexBuffer(*m_current_command_buffer, m_index_buffer_ring_info.heap->value, offset, index_type);

		if (draw_call.is_trivial_instanced_draw)
		{
			vkCmdDrawIndexed(*m_current_command_buffer, upload_info.vertex_draw_count, draw_call.pass_count(), 0, 0, 0);
		}
		else if (rsx::method_registers.current_draw_clause.is_single_draw())
		{
			vkCmdDrawIndexed(*m_current_command_buffer, upload_info.vertex_draw_count, 1, 0, 0, 0);
		}
		else if (m_device->get_multidraw_support())
		{
			const auto subranges = draw_call.get_subranges();
			const auto subranges_count = ::size32(subranges);
			const auto allocation_size = subranges_count * sizeof(VkMultiDrawIndexedInfoEXT);

			m_multidraw_parameters_buffer.resize(allocation_size);
			auto base_ptr = utils::bless<VkMultiDrawIndexedInfoEXT>(m_multidraw_parameters_buffer.data());

			u32 vertex_offset = 0;
			auto _ptr = base_ptr;

			for (const auto& range : subranges)
			{
				const auto count = get_index_count(draw_call.primitive, range.count);
				_ptr->firstIndex = vertex_offset;
				_ptr->indexCount = count;
				_ptr->vertexOffset = 0;

				_ptr++;
				vertex_offset += count;
			}
			_vkCmdDrawMultiIndexedEXT(*m_current_command_buffer, subranges_count, base_ptr, 1, 0, sizeof(VkMultiDrawIndexedInfoEXT), nullptr);
		}
		else
		{
			u32 vertex_offset = 0;
			const auto subranges = draw_call.get_subranges();
			for (const auto &range : subranges)
			{
				const auto count = get_index_count(draw_call.primitive, range.count);
				vkCmdDrawIndexed(*m_current_command_buffer, count, 1, vertex_offset, 0, 0);
				vertex_offset += count;
			}
		}
	}

	m_frame_stats.draw_exec_time += m_profiler.duration();
}

void VKGSRender::begin()
{
	// Save shader state now before prefetch and loading happens
	m_interpreter_state = (m_graphics_state.load() & rsx::pipeline_state::invalidate_pipeline_bits);

	rsx::thread::begin();

	if (skip_current_frame ||
		swapchain_unavailable ||
		cond_render_ctrl.disable_rendering())
	{
		return;
	}

	init_buffers(rsx::framebuffer_creation_context::context_draw);

	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		// Shaders need to be reloaded.
		m_prev_program = m_program;
		m_program = nullptr;
	}
}

void VKGSRender::end()
{
	if (skip_current_frame || !m_graphics_state.test(rsx::rtt_config_valid) || swapchain_unavailable || cond_render_ctrl.disable_rendering())
	{
		execute_nop_draw();
		rsx::thread::end();
		return;
	}

	m_profiler.start();

	// Check for frame resource status here because it is possible for an async flip to happen between begin/end
	if (m_current_frame->flags & frame_context_state::dirty) [[unlikely]]
	{
		check_present_status();

		if (m_current_frame->swap_command_buffer) [[unlikely]]
		{
			// Borrow time by using the auxilliary context
			m_aux_frame_context.grab_resources(*m_current_frame);
			m_current_frame = &m_aux_frame_context;
		}

		ensure(!m_current_frame->swap_command_buffer);

		m_current_frame->flags &= ~frame_context_state::dirty;
	}

	analyse_current_rsx_pipeline();

	m_frame_stats.setup_time += m_profiler.duration();

	load_texture_env();
	m_frame_stats.textures_upload_time += m_profiler.duration();

	if (!load_program())
	{
		// Program is not ready, skip drawing this
		std::this_thread::yield();
		execute_nop_draw();
		// m_rtts.on_write(); - breaks games for obvious reasons
		rsx::thread::end();
		return;
	}

	// Load program execution environment
	load_program_env();
	m_frame_stats.setup_time += m_profiler.duration();

	// Apply write memory barriers
	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil))
	{
		ds->write_barrier(*m_current_command_buffer);
	}

	for (auto &rtt : m_rtts.m_bound_render_targets)
	{
		if (auto surface = std::get<1>(rtt))
		{
			surface->write_barrier(*m_current_command_buffer);
		}
	}

	m_frame_stats.setup_time += m_profiler.duration();

	// Now bind the shader resources. It is important that this takes place after the barriers so that we don't end up with stale descriptors
	for (int retry = 0; retry < 3; ++retry)
	{
		if (retry > 0 && m_samplers_dirty) [[ unlikely ]]
		{
			// Reload texture env if referenced objects were invalidated during OOM handling.
			load_texture_env();

			// Do not trust fragment/vertex texture state after a texture state reset.
			// NOTE: We don't want to change the program - it's too late for that now. We just need to harmonize the state.
			m_graphics_state |= rsx::vertex_program_state_dirty | rsx::fragment_program_state_dirty;
			get_current_fragment_program(fs_sampler_state);
			get_current_vertex_program(vs_sampler_state);
			m_graphics_state.clear(rsx::pipeline_state::invalidate_pipeline_bits);
		}

		const bool out_of_memory = m_shader_interpreter.is_interpreter(m_program)
			? bind_interpreter_texture_env()
			: bind_texture_env();

		if (!out_of_memory)
		{
			break;
		}

		// Handle OOM
		if (!on_vram_exhausted(rsx::problem_severity::fatal))
		{
			// It is not possible to free memory. Just use placeholder textures. Can cause graphics glitches but shouldn't crash otherwise
			break;
		}
	}

	m_texture_cache.release_uncached_temporary_subresources();
	m_frame_stats.textures_upload_time += m_profiler.duration();

	u32 sub_index = 0;               // RSX subdraw ID
	m_current_draw.subdraw_id = 0;   // Host subdraw ID. Invalid RSX subdraws do not increment this value

	if (m_graphics_state & rsx::pipeline_state::invalidate_vk_dynamic_state)
	{
		m_current_command_buffer->flags |= vk::command_buffer::cb_reload_dynamic_state;
	}

	auto& draw_call = rsx::method_registers.current_draw_clause;
	draw_call.begin();
	do
	{
		emit_geometry(sub_index++);

		if (draw_call.is_trivial_instanced_draw)
		{
			// We already completed. End the draw.
			draw_call.end();
		}
	}
	while (draw_call.next());

	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_conditional_render)
	{
		_vkCmdEndConditionalRenderingEXT(*m_current_command_buffer);
		m_current_command_buffer->flags &= ~(vk::command_buffer::cb_has_conditional_render);
	}

	m_rtts.on_write(m_framebuffer_layout.color_write_enabled, m_framebuffer_layout.zeta_write_enabled);

	rsx::thread::end();
}
