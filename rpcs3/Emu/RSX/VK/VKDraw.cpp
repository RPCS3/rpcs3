#include "stdafx.h"
#include "VKGSRender.h"
#include "../Common/BufferUtils.h"

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
		default: ASSUME(0);
		};
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
			fmt::throw_exception("Unknown compare op: 0x%x" HERE, static_cast<u32>(op));
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

	float actual_line_width = rsx::method_registers.line_width();
	vkCmdSetLineWidth(*m_current_command_buffer, actual_line_width);

	if (rsx::method_registers.poly_offset_fill_enabled())
	{
		//offset_bias is the constant factor, multiplied by the implementation factor R
		//offst_scale is the slope factor, multiplied by the triangle slope factor M
		vkCmdSetDepthBias(*m_current_command_buffer, rsx::method_registers.poly_offset_bias(), 0.f, rsx::method_registers.poly_offset_scale());
	}
	else
	{
		//Zero bias value - disables depth bias
		vkCmdSetDepthBias(*m_current_command_buffer, 0.f, 0.f, 0.f);
	}

	//Update dynamic state
	if (rsx::method_registers.blend_enabled())
	{
		//Update blend constants
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

	if (m_device->get_depth_bounds_support())
	{
		if (rsx::method_registers.depth_bounds_test_enabled())
		{
			//Update depth bounds min/max
			vkCmdSetDepthBounds(*m_current_command_buffer, rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
		}
		else
		{
			vkCmdSetDepthBounds(*m_current_command_buffer, 0.f, 1.f);
		}
	}

	bind_viewport();

	//TODO: Set up other render-state parameters into the program pipeline

	m_frame_stats.setup_time += m_profiler.duration();
}

void VKGSRender::load_texture_env()
{
	//Load textures
	bool update_framebuffer_sourced = false;
	bool check_for_cyclic_refs = false;

	std::lock_guard lock(m_sampler_mutex);

	if (surface_store_tag != m_rtts.cache_tag) [[unlikely]]
	{
		update_framebuffer_sourced = true;
		surface_store_tag = m_rtts.cache_tag;
	}

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (!fs_sampler_state[i])
			fs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

		if (m_samplers_dirty || m_textures_dirty[i] ||
			(update_framebuffer_sourced && fs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
		{
			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

			if (rsx::method_registers.fragment_textures[i].enabled())
			{
				check_heap_status(VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE);
				*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.fragment_textures[i], m_rtts);

				if (sampler_state->is_cyclic_reference)
				{
					check_for_cyclic_refs |= true;
				}

				bool replace = !fs_sampler_handles[i];
				VkFilter mag_filter;
				vk::minification_filter min_filter;
				f32 min_lod = 0.f, max_lod = 0.f;
				f32 lod_bias = 0.f;

				const u32 texture_format = rsx::method_registers.fragment_textures[i].format() & ~(CELL_GCM_TEXTURE_UN | CELL_GCM_TEXTURE_LN);
				VkBool32 compare_enabled = VK_FALSE;
				VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER;

				if (texture_format >= CELL_GCM_TEXTURE_DEPTH24_D8 && texture_format <= CELL_GCM_TEXTURE_DEPTH16_FLOAT)
				{
					if (m_device->get_formats_support().d24_unorm_s8)
					{
						// NOTE:
						// The nvidia-specific format D24S8 has a special way of doing depth comparison that matches the PS3
						// In case of projected shadow lookup the result of the divide operation has its Z clamped to [0-1] before comparison
						// Most other wide formats (Z bits > 16) do not behave this way and depth greater than 1 is possible due to the use of floating point as storage
						// Compare operations for these formats (such as D32_SFLOAT) are therefore emulated for correct results

						// NOTE2:
						// To improve reusability, DEPTH16 shadow ops are also emulated if D24S8 support is not available

						compare_enabled = VK_TRUE;
						depth_compare_mode = vk::get_compare_func(rsx::method_registers.fragment_textures[i].zfunc(), true);
					}
				}

				const bool aniso_override = !g_cfg.video.strict_rendering_mode && g_cfg.video.anisotropic_level_override > 0;
				const f32 af_level = aniso_override ? g_cfg.video.anisotropic_level_override : vk::max_aniso(rsx::method_registers.fragment_textures[i].max_aniso());
				const auto wrap_s = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_s());
				const auto wrap_t = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_t());
				const auto wrap_r = vk::vk_wrap_mode(rsx::method_registers.fragment_textures[i].wrap_r());
				const auto border_color = vk::get_border_color(rsx::method_registers.fragment_textures[i].border_color());

				// Check if non-point filtering can even be used on this format
				bool can_sample_linear;
				if (sampler_state->format_class == rsx::format_type::color) [[likely]]
				{
					// Most PS3-like formats can be linearly filtered without problem
					can_sample_linear = true;
				}
				else
				{
					// Not all GPUs support linear filtering of depth formats
					const auto vk_format = sampler_state->image_handle ? sampler_state->image_handle->image()->format() :
						vk::get_compatible_sampler_format(m_device->get_formats_support(), sampler_state->external_subresource_desc.gcm_format);

					can_sample_linear = m_device->get_format_properties(vk_format).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
				}

				const auto mipmap_count = rsx::method_registers.fragment_textures[i].get_exact_mipmap_count();
				min_filter = vk::get_min_filter(rsx::method_registers.fragment_textures[i].min_filter());

				if (can_sample_linear)
				{
					mag_filter = vk::get_mag_filter(rsx::method_registers.fragment_textures[i].mag_filter());
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
						min_lod = rsx::method_registers.fragment_textures[i].min_lod();
						max_lod = rsx::method_registers.fragment_textures[i].max_lod();
						lod_bias = rsx::method_registers.fragment_textures[i].bias();

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
					fs_sampler_handles[i] = vk::get_resource_manager()->find_sampler(*m_device, wrap_s, wrap_t, wrap_r, false, lod_bias, af_level, min_lod, max_lod,
						min_filter.filter, mag_filter, min_filter.mipmap_mode, border_color, compare_enabled, depth_compare_mode);
				}
			}
			else
			{
				*sampler_state = {};
			}

			m_textures_dirty[i] = false;
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (!vs_sampler_state[i])
			vs_sampler_state[i] = std::make_unique<vk::texture_cache::sampled_image_descriptor>();

		if (m_samplers_dirty || m_vertex_textures_dirty[i] ||
			(update_framebuffer_sourced && vs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
		{
			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());

			if (rsx::method_registers.vertex_textures[i].enabled())
			{
				check_heap_status(VK_HEAP_CHECK_TEXTURE_UPLOAD_STORAGE);
				*sampler_state = m_texture_cache.upload_texture(*m_current_command_buffer, rsx::method_registers.vertex_textures[i], m_rtts);

				if (sampler_state->is_cyclic_reference || sampler_state->external_subresource_desc.do_not_cache)
				{
					check_for_cyclic_refs |= true;
				}

				bool replace = !vs_sampler_handles[i];
				const VkBool32 unnormalized_coords = !!(rsx::method_registers.vertex_textures[i].format() & CELL_GCM_TEXTURE_UN);
				const auto min_lod = rsx::method_registers.vertex_textures[i].min_lod();
				const auto max_lod = rsx::method_registers.vertex_textures[i].max_lod();
				const auto border_color = vk::get_border_color(rsx::method_registers.vertex_textures[i].border_color());

				if (vs_sampler_handles[i])
				{
					if (!vs_sampler_handles[i]->matches(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
						unnormalized_coords, 0.f, 1.f, min_lod, max_lod, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color))
					{
						replace = true;
					}
				}

				if (replace)
				{
					vs_sampler_handles[i] = vk::get_resource_manager()->find_sampler(
						*m_device,
						VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
						unnormalized_coords,
						0.f, 1.f, min_lod, max_lod,
						VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, border_color);
				}
			}
			else
				*sampler_state = {};

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
}

void VKGSRender::bind_texture_env()
{
	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (current_fp_metadata.referenced_textures_mask & (1 << i))
		{
			vk::image_view* view = nullptr;
			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

			if (rsx::method_registers.fragment_textures[i].enabled() &&
				sampler_state->validate())
			{
				if (view = sampler_state->image_handle; !view)
				{
					//Requires update, copy subresource
					view = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
				}
				else
				{
					switch (auto raw = view->image(); raw->current_layout)
					{
					default:
					//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
						break;
					case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
						verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_dst;
						raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						break;
					case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
						verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_src;
						raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						break;
					case VK_IMAGE_LAYOUT_GENERAL:
						verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage;
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
								*m_current_command_buffer,
								raw->value,
								VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								src_stage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								src_access, VK_ACCESS_SHADER_READ_BIT,
								{ raw->aspect(), 0, 1, 0, 1 });

							raw->current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						}
						break;
					case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
						verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage, !sampler_state->is_cyclic_reference;
						raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						break;
					}
				}
			}

			if (view) [[likely]]
			{
				m_program->bind_uniform({ fs_sampler_handles[i]->value, view->value, view->image()->current_layout },
					i,
					::glsl::program_domain::glsl_fragment_program,
					m_current_frame->descriptor_set);

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					// Stencil mirror required
					auto root_image = static_cast<vk::viewable_image*>(view->image());
					auto stencil_view = root_image->get_view(0xAAE4, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

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
						i,
						::glsl::program_domain::glsl_fragment_program,
						m_current_frame->descriptor_set,
						true);
				}
			}
			else
			{
				const VkImageViewType view_type = vk::get_view_type(current_fragment_program.get_texture_dimension(i));
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					i,
					::glsl::program_domain::glsl_fragment_program,
					m_current_frame->descriptor_set);

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
						i,
						::glsl::program_domain::glsl_fragment_program,
						m_current_frame->descriptor_set,
						true);
				}
			}
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (current_vp_metadata.referenced_textures_mask & (1 << i))
		{
			if (!rsx::method_registers.vertex_textures[i].enabled())
			{
				const auto view_type = vk::get_view_type(current_vertex_program.get_texture_dimension(i));
				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					i,
					::glsl::program_domain::glsl_vertex_program,
					m_current_frame->descriptor_set);

				continue;
			}

			auto sampler_state = static_cast<vk::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
			auto image_ptr = sampler_state->image_handle;

			if (!image_ptr && sampler_state->validate())
			{
				image_ptr = m_texture_cache.create_temporary_subresource(*m_current_command_buffer, sampler_state->external_subresource_desc);
				m_vertex_textures_dirty[i] = true;
			}

			if (!image_ptr)
			{
				rsx_log.error("Texture upload failed to vtexture index %d. Binding null sampler.", i);
				const auto view_type = vk::get_view_type(current_vertex_program.get_texture_dimension(i));

				m_program->bind_uniform({ vk::null_sampler(), vk::null_image_view(*m_current_command_buffer, view_type)->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
					i,
					::glsl::program_domain::glsl_vertex_program,
					m_current_frame->descriptor_set);

				continue;
			}

			switch (auto raw = image_ptr->image(); raw->current_layout)
			{
			default:
			//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_dst;
				raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::blit_engine_src;
				raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage;
				if (!sampler_state->is_cyclic_reference)
				{
					// Custom barrier, see similar block in FS stage
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
						*m_current_command_buffer,
						raw->value,
						VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						src_stage, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						src_access, VK_ACCESS_SHADER_READ_BIT,
						{ raw->aspect(), 0, 1, 0, 1 });

					raw->current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				verify(HERE), sampler_state->upload_context == rsx::texture_upload_context::framebuffer_storage;
				raw->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				break;
			}

			m_program->bind_uniform({ vs_sampler_handles[i]->value, image_ptr->value, image_ptr->image()->current_layout },
				i,
				::glsl::program_domain::glsl_vertex_program,
				m_current_frame->descriptor_set);
		}
	}
}

void VKGSRender::emit_geometry(u32 sub_index)
{
	auto &draw_call = rsx::method_registers.current_draw_clause;
	m_profiler.start();

	if (sub_index == 0)
	{
		analyse_inputs_interleaved(m_vertex_layout);

		if (!m_vertex_layout.validate())
		{
			// No vertex inputs enabled
			// Execute remainining pipeline barriers with NOP draw
			do
			{
				draw_call.execute_pipeline_dependencies();
			}
			while (draw_call.next());

			draw_call.end();
			return;
		}
	}
	else if (draw_call.execute_pipeline_dependencies() & rsx::vertex_base_changed)
	{
		// Rebase vertex bases instead of
		for (auto &info : m_vertex_layout.interleaved_blocks)
		{
			const auto vertex_base_offset = rsx::method_registers.vertex_data_base_offset();
			info.real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(vertex_base_offset, info.base_offset), info.memory_location, HERE);
		}
	}

	const auto old_persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	const auto old_volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;

	// Programs data is dependent on vertex state
	auto upload_info = upload_vertex_data();
	if (!upload_info.vertex_draw_count)
	{
		// Malformed vertex setup; abort
		return;
	}

	m_frame_stats.vertex_upload_time += m_profiler.duration();

	auto persistent_buffer = m_persistent_attribute_storage ? m_persistent_attribute_storage->value : null_buffer_view->value;
	auto volatile_buffer = m_volatile_attribute_storage ? m_volatile_attribute_storage->value : null_buffer_view->value;
	bool update_descriptors = false;

	const auto& binding_table = m_device->get_pipeline_binding_table();

	if (sub_index == 0)
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
				m_current_frame->buffer_views_to_clean.push_back(std::move(m_vertex_layout_storage));
			}

			vk::clear_status_interrupt(vk::heap_changed);
		}
	}
	else if (persistent_buffer != old_persistent_buffer || volatile_buffer != old_volatile_buffer)
	{
		// Need to update descriptors; make a copy for the next draw
		VkDescriptorSet new_descriptor_set = allocate_descriptor_set();
		std::vector<VkCopyDescriptorSet> copy_set(binding_table.total_descriptor_bindings);

		for (u32 n = 0; n < binding_table.total_descriptor_bindings; ++n)
		{
			copy_set[n] =
			{
				VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,   // sType
				nullptr,                                 // pNext
				m_current_frame->descriptor_set,         // srcSet
				n,                                       // srcBinding
				0u,                                      // srcArrayElement
				new_descriptor_set,                      // dstSet
				n,                                       // dstBinding
				0u,                                      // dstArrayElement
				1u                                       // descriptorCount
			};
		}

		vkUpdateDescriptorSets(*m_device, 0, 0, binding_table.total_descriptor_bindings, copy_set.data());
		m_current_frame->descriptor_set = new_descriptor_set;

		update_descriptors = true;
	}

	// Update vertex fetch parameters
	update_vertex_env(sub_index, upload_info);

	verify(HERE), m_vertex_layout_storage;
	if (update_descriptors)
	{
		m_program->bind_uniform(persistent_buffer, binding_table.vertex_buffers_first_bind_slot, m_current_frame->descriptor_set);
		m_program->bind_uniform(volatile_buffer, binding_table.vertex_buffers_first_bind_slot + 1, m_current_frame->descriptor_set);
		m_program->bind_uniform(m_vertex_layout_storage->value, binding_table.vertex_buffers_first_bind_slot + 2, m_current_frame->descriptor_set);
	}

	if (!m_current_subdraw_id++)
	{
		vkCmdBindPipeline(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
		update_draw_state();
		begin_render_pass();

		if (cond_render_ctrl.hw_cond_active && m_device->get_conditional_render_support())
		{
			// It is inconvenient that conditional rendering breaks other things like compute dispatch
			// TODO: If this is heavy, add refactor the resources into global and add checks around compute dispatch
			VkConditionalRenderingBeginInfoEXT info{};
			info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			info.buffer = m_cond_render_buffer->value;

			m_device->cmdBeginConditionalRenderingEXT(*m_current_command_buffer, &info);
			m_current_command_buffer->flags |= vk::command_buffer::cb_has_conditional_render;
		}
	}

	// Bind the new set of descriptors for use with this draw call
	vkCmdBindDescriptorSets(*m_current_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &m_current_frame->descriptor_set, 0, nullptr);

	m_frame_stats.setup_time += m_profiler.duration();

	if (!upload_info.index_info)
	{
		if (draw_call.is_single_draw())
		{
			vkCmdDraw(*m_current_command_buffer, upload_info.vertex_draw_count, 1, 0, 0);
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

		if (rsx::method_registers.current_draw_clause.is_single_draw())
		{
			const u32 index_count = upload_info.vertex_draw_count;
			vkCmdDrawIndexed(*m_current_command_buffer, index_count, 1, 0, 0, 0);
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
	rsx::thread::begin();

	if (skip_current_frame || swapchain_unavailable || cond_render_ctrl.disable_rendering())
		return;

	init_buffers(rsx::framebuffer_creation_context::context_draw);
}

void VKGSRender::end()
{
	if (skip_current_frame || !framebuffer_status_valid || swapchain_unavailable || cond_render_ctrl.disable_rendering())
	{
		execute_nop_draw();
		rsx::thread::end();
		return;
	}

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
		else if (m_current_frame->used_descriptors)
		{
			m_current_frame->descriptor_pool.reset(0);
			m_current_frame->used_descriptors = 0;
		}

		verify(HERE), !m_current_frame->swap_command_buffer;

		m_current_frame->flags &= ~frame_context_state::dirty;
	}

	m_profiler.start();

	// Check for data casts
	// NOTE: This is deprecated and will be removed soon. The memory barrier invoked before rendering does this better
	auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	if (ds && ds->old_contents.size() == 1 &&
		ds->old_contents[0].source->info.format == VK_FORMAT_B8G8R8A8_UNORM)
	{
		auto key = vk::get_renderpass_key(ds->info.format);
		auto render_pass = vk::get_renderpass(*m_device, key);
		verify("Usupported renderpass configuration" HERE), render_pass != VK_NULL_HANDLE;

		VkClearDepthStencilValue clear = { 1.f, 0xFF };
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

		// Initialize source
		auto src = vk::as_rtt(ds->old_contents[0].source);
		src->read_barrier(*m_current_command_buffer);

		switch (src->current_layout)
		{
		case VK_IMAGE_LAYOUT_GENERAL:
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			break;
		//case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		default:
			src->change_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			break;
		}

		// Clear explicitly before starting the inheritance transfer
		const bool preinitialized = (ds->current_layout == VK_IMAGE_LAYOUT_GENERAL);
		if (!preinitialized) ds->push_layout(*m_current_command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdClearDepthStencilImage(*m_current_command_buffer, ds->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
		if (!preinitialized) ds->pop_layout(*m_current_command_buffer);

		// TODO: Stencil transfer
		ds->old_contents[0].init_transfer(ds);
		m_depth_converter->run(*m_current_command_buffer,
			ds->old_contents[0].src_rect(),
			ds->old_contents[0].dst_rect(),
			src->get_view(0xAAE4, rsx::default_remap_vector),
			ds, render_pass);

		// TODO: Flush management to avoid pass running out of ubo space (very unlikely)
		ds->on_write();
	}

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

	// Allocate descriptor set
	check_descriptors();
	m_current_frame->descriptor_set = allocate_descriptor_set();

	// Load program execution environment
	load_program_env();
	m_frame_stats.setup_time += m_profiler.duration();

	bind_texture_env();
	m_texture_cache.release_uncached_temporary_subresources();
	m_frame_stats.textures_upload_time += m_profiler.duration();

	if (m_current_command_buffer->flags & vk::command_buffer::cb_load_occluson_task)
	{
		u32 occlusion_id = m_occlusion_query_pool.find_free_slot();
		if (occlusion_id == UINT32_MAX)
		{
			// Force flush
			rsx_log.error("[Performance Warning] Out of free occlusion slots. Forcing hard sync.");
			ZCULL_control::sync(this);

			occlusion_id = m_occlusion_query_pool.find_free_slot();
			if (occlusion_id == UINT32_MAX)
			{
				//rsx_log.error("Occlusion pool overflow");
				if (m_current_task) m_current_task->result = 1;
			}
		}

		// Begin query
		m_occlusion_query_pool.begin_query(*m_current_command_buffer, occlusion_id);

		auto &data = m_occlusion_map[m_active_query_info->driver_handle];
		data.indices.push_back(occlusion_id);
		data.set_sync_command_buffer(m_current_command_buffer);

		m_current_command_buffer->flags &= ~vk::command_buffer::cb_load_occluson_task;
		m_current_command_buffer->flags |= (vk::command_buffer::cb_has_occlusion_task | vk::command_buffer::cb_has_open_query);
	}

	bool primitive_emulated = false;
	vk::get_appropriate_topology(rsx::method_registers.current_draw_clause.primitive, primitive_emulated);

	// Apply write memory barriers
	if (ds) ds->write_barrier(*m_current_command_buffer);

	for (auto &rtt : m_rtts.m_bound_render_targets)
	{
		if (auto surface = std::get<1>(rtt))
		{
			surface->write_barrier(*m_current_command_buffer);
		}
	}

	// Final heap check...
	check_heap_status(VK_HEAP_CHECK_VERTEX_STORAGE | VK_HEAP_CHECK_VERTEX_LAYOUT_STORAGE);

	u32 sub_index = 0;
	m_current_subdraw_id = 0;

	rsx::method_registers.current_draw_clause.begin();
	do
	{
		emit_geometry(sub_index++);
	}
	while (rsx::method_registers.current_draw_clause.next());

	if (m_current_command_buffer->flags & vk::command_buffer::cb_has_conditional_render)
	{
		m_device->cmdEndConditionalRenderingEXT(*m_current_command_buffer);
		m_current_command_buffer->flags &= ~(vk::command_buffer::cb_has_conditional_render);
	}

	m_rtts.on_write(m_framebuffer_layout.color_write_enabled.data(), m_framebuffer_layout.zeta_write_enabled);

	rsx::thread::end();
}
