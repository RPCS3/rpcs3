#include "VKOverlays.h"
#include "VKRenderTargets.h"
#include "VKFramebuffer.h"
#include "VKResourceManager.h"
#include "VKRenderPass.h"
#include "VKPipelineCompiler.h"

#include "vkutils/image.h"
#include "vkutils/image_helpers.h"
#include "vkutils/sampler.h"
#include "vkutils/scratch.h"

#include "../Overlays/overlays.h"
#include "../Program/RSXOverlay.h"

#include "util/fnv_hash.hpp"

namespace vk
{
	overlay_pass::overlay_pass()
	{
		// Override-able defaults
		renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	}

	overlay_pass::~overlay_pass()
	{
		m_vao.destroy();
		m_ubo.destroy();
	}

	u64 overlay_pass::get_pipeline_key(VkRenderPass pass)
	{
		u64 key = rpcs3::hash_struct(renderpass_config);
		key ^= reinterpret_cast<uptr>(pass);
		return key;
	}

	void overlay_pass::check_heap()
	{
		if (!m_vao.heap)
		{
			m_vao.create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1 * 0x100000, "overlays VAO", 128);
			m_ubo.create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 8 * 0x100000, "overlays UBO", 128);
		}
	}

	void overlay_pass::init_descriptors()
	{
		rsx::simple_array<VkDescriptorPoolSize> descriptor_pool_sizes =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
		};

		if (m_num_usable_samplers)
		{
			descriptor_pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_num_usable_samplers });
		}

		if (m_num_input_attachments)
		{
			descriptor_pool_sizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_num_input_attachments });
		}

		// Reserve descriptor pools
		m_descriptor_pool.create(*m_device, descriptor_pool_sizes);

		const auto num_bindings = 1 + m_num_usable_samplers + m_num_input_attachments;
		rsx::simple_array<VkDescriptorSetLayoutBinding> bindings(num_bindings);

		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[0].binding = 0;
		bindings[0].pImmutableSamplers = nullptr;

		u32 descriptor_index = 1;
		for (u32 n = 0; n < m_num_usable_samplers; ++n, ++descriptor_index)
		{
			bindings[descriptor_index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[descriptor_index].descriptorCount = 1;
			bindings[descriptor_index].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[descriptor_index].binding = descriptor_index;
			bindings[descriptor_index].pImmutableSamplers = nullptr;
		}

		for (u32 n = 0; n < m_num_input_attachments; ++n, ++descriptor_index)
		{
			bindings[descriptor_index].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			bindings[descriptor_index].descriptorCount = 1;
			bindings[descriptor_index].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[descriptor_index].binding = descriptor_index;
			bindings[descriptor_index].pImmutableSamplers = nullptr;
		}

		ensure(descriptor_index == num_bindings);
		m_descriptor_layout = vk::descriptors::create_layout(bindings);

		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &m_descriptor_layout;

		std::vector<VkPushConstantRange> push_constants = get_push_constants();
		if (!push_constants.empty())
		{
			layout_info.pushConstantRangeCount = u32(push_constants.size());
			layout_info.pPushConstantRanges = push_constants.data();
		}

		CHECK_RESULT(vkCreatePipelineLayout(*m_device, &layout_info, nullptr, &m_pipeline_layout));
	}

	std::vector<vk::glsl::program_input> overlay_pass::get_vertex_inputs()
	{
		check_heap();
		return{};
	}

	std::vector<vk::glsl::program_input> overlay_pass::get_fragment_inputs()
	{
		std::vector<vk::glsl::program_input> fs_inputs;
		fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_uniform_buffer,{},{}, 0, "static_data" });

		u32 binding = 1;
		for (u32 n = 0; n < m_num_usable_samplers; ++n, ++binding)
		{
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_texture,{},{}, binding, "fs" + std::to_string(n) });
		}

		for (u32 n = 0; n < m_num_input_attachments; ++n, ++binding)
		{
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_texture,{},{}, binding, "sp" + std::to_string(n) });
		}

		return fs_inputs;
	}

	vk::glsl::program* overlay_pass::build_pipeline(u64 storage_key, VkRenderPass render_pass)
	{
		if (!compiled)
		{
			m_vertex_shader.create(::glsl::program_domain::glsl_vertex_program, vs_src);
			m_vertex_shader.compile();

			m_fragment_shader.create(::glsl::program_domain::glsl_fragment_program, fs_src);
			m_fragment_shader.compile();

			compiled = true;
		}

		VkPipelineShaderStageCreateInfo shader_stages[2] = {};
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = m_vertex_shader.get_handle();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = m_fragment_shader.get_handle();
		shader_stages[1].pName = "main";

		std::vector<VkDynamicState> dynamic_state_descriptors;
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_SCISSOR);
		get_dynamic_state_entries(dynamic_state_descriptors);

		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = ::size32(dynamic_state_descriptors);
		dynamic_state_info.pDynamicStates = dynamic_state_descriptors.data();

		VkVertexInputBindingDescription vb = { 0, 16, VK_VERTEX_INPUT_RATE_VERTEX };
		VkVertexInputAttributeDescription via = { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 };
		VkPipelineVertexInputStateCreateInfo vi = {};
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &vb;
		vi.vertexAttributeDescriptionCount = 1;
		vi.pVertexAttributeDescriptions = &via;

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.scissorCount = 1;
		vp.viewportCount = 1;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pVertexInputState = &vi;
		info.pInputAssemblyState = &renderpass_config.ia;
		info.pRasterizationState = &renderpass_config.rs;
		info.pColorBlendState = &renderpass_config.cs;
		info.pMultisampleState = &renderpass_config.ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &renderpass_config.ds;
		info.stageCount = 2;
		info.pStages = shader_stages;
		info.pDynamicState = &dynamic_state_info;
		info.layout = m_pipeline_layout;
		info.basePipelineIndex = -1;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.renderPass = render_pass;

		auto compiler = vk::get_pipe_compiler();
		auto program = compiler->compile(info, m_pipeline_layout, vk::pipe_compiler::COMPILE_INLINE, {}, get_vertex_inputs(), get_fragment_inputs());
		auto result = program.get();
		m_program_cache[storage_key] = std::move(program);

		return result;
	}

	void overlay_pass::load_program(vk::command_buffer& cmd, VkRenderPass pass, const std::vector<vk::image_view*>& src)
	{
		vk::glsl::program *program = nullptr;
		const auto key = get_pipeline_key(pass);

		auto found = m_program_cache.find(key);
		if (found != m_program_cache.end())
			program = found->second.get();
		else
			program = build_pipeline(key, pass);

		m_descriptor_set = m_descriptor_pool.allocate(m_descriptor_layout);

		if (!m_sampler && !src.empty())
		{
			m_sampler = std::make_unique<vk::sampler>(*m_device,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VK_FALSE, 0.f, 1.f, 0.f, 0.f, m_sampler_filter, m_sampler_filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
		}

		update_uniforms(cmd, program);

		program->bind_uniform({ m_ubo.heap->value, m_ubo_offset, std::max(m_ubo_length, 4u) }, 0, m_descriptor_set);

		for (uint n = 0; n < src.size(); ++n)
		{
			VkDescriptorImageInfo info = { m_sampler->value, src[n]->value, src[n]->image()->current_layout };
			program->bind_uniform(info, "fs" + std::to_string(n), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_descriptor_set);
		}

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipeline);
		m_descriptor_set.bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout);

		VkBuffer buffers = m_vao.heap->value;
		VkDeviceSize offsets = m_vao_offset;
		vkCmdBindVertexBuffers(cmd, 0, 1, &buffers, &offsets);
	}

	void overlay_pass::create(const vk::render_device& dev)
	{
		if (!initialized)
		{
			m_device = &dev;
			init_descriptors();

			initialized = true;
		}
	}

	void overlay_pass::destroy()
	{
		if (initialized)
		{
			m_vertex_shader.destroy();
			m_fragment_shader.destroy();
			m_program_cache.clear();
			m_sampler.reset();

			vkDestroyDescriptorSetLayout(*m_device, m_descriptor_layout, nullptr);
			vkDestroyPipelineLayout(*m_device, m_pipeline_layout, nullptr);
			m_descriptor_pool.destroy();

			initialized = false;
		}
	}

	void overlay_pass::free_resources()
	{
		// FIXME: Allocation sizes are known, we don't need to use a data_heap structure
		m_vao.reset_allocation_stats();
		m_ubo.reset_allocation_stats();
	}

	vk::framebuffer* overlay_pass::get_framebuffer(vk::image* target, VkRenderPass render_pass)
	{
		VkDevice dev = (*vk::get_current_renderer());
		return vk::get_framebuffer(dev, target->width(), target->height(), m_num_input_attachments > 0, render_pass, { target });
	}

	void overlay_pass::emit_geometry(vk::command_buffer& cmd)
	{
		vkCmdDraw(cmd, num_drawable_elements, 1, first_vertex, 0);
	}

	void overlay_pass::set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h)
	{
		VkViewport vp{};
		vp.x = static_cast<f32>(x);
		vp.y = static_cast<f32>(y);
		vp.width = static_cast<f32>(w);
		vp.height = static_cast<f32>(h);
		vp.minDepth = 0.f;
		vp.maxDepth = 1.f;
		vkCmdSetViewport(cmd, 0, 1, &vp);

		VkRect2D vs = { { static_cast<s32>(x), static_cast<s32>(y) }, { w, h } };
		vkCmdSetScissor(cmd, 0, 1, &vs);
	}

	void overlay_pass::run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* fbo, const std::vector<vk::image_view*>& src, VkRenderPass render_pass)
	{
		// This call clobbers dynamic state
		cmd.flags |= vk::command_buffer::cb_reload_dynamic_state;

		load_program(cmd, render_pass, src);
		set_up_viewport(cmd, viewport.x1, viewport.y1, viewport.width(), viewport.height());

		vk::begin_renderpass(cmd, render_pass, fbo->value, { positionu{0u, 0u}, sizeu{fbo->width(), fbo->height()} });
		emit_geometry(cmd);
	}

	void overlay_pass::run(vk::command_buffer& cmd, const areau& viewport, vk::image* target, const std::vector<vk::image_view*>& src, VkRenderPass render_pass)
	{
		auto fbo = static_cast<vk::framebuffer_holder*>(get_framebuffer(target, render_pass));
		fbo->add_ref();

		run(cmd, viewport, fbo, src, render_pass);
		fbo->release();
	}

	void overlay_pass::run(vk::command_buffer& cmd, const areau& viewport, vk::image* target, vk::image_view* src, VkRenderPass render_pass)
	{
		std::vector<vk::image_view*> views = { src };
		run(cmd, viewport, target, views, render_pass);
	}

	ui_overlay_renderer::ui_overlay_renderer()
		: m_texture_type(rsx::overlays::texture_sampling_mode::none)
	{
		vs_src =
		#include "../Program/GLSLSnippets/OverlayRenderVS.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/OverlayRenderFS.glsl"
		;

		vs_src = fmt::replace_all(vs_src,
		{
			{ "%preprocessor", "// %preprocessor" },
			{ "%push_block", "push_constant" }
		});

		fs_src = fmt::replace_all(fs_src,
		{
			{ "%preprocessor", "// %preprocessor" },
			{ "%push_block_offset", "layout(offset=68)" },
			{ "%push_block", "push_constant" }
		});

		// 2 input textures
		m_num_usable_samplers = 2;

		renderpass_config.set_attachment_count(1);
		renderpass_config.set_color_mask(0, true, true, true, true);
		renderpass_config.set_depth_mask(false);
		renderpass_config.enable_blend(0,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ZERO,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE,
			VK_BLEND_OP_ADD, VK_BLEND_OP_ADD);
	}

	vk::image_view* ui_overlay_renderer::upload_simple_texture(vk::render_device& dev, vk::command_buffer& cmd,
		vk::data_heap& upload_heap, u64 key, u32 w, u32 h, u32 layers, bool font, bool temp, const void* pixel_src, u32 owner_uid)
	{
		const VkFormat format = (font) ? VK_FORMAT_R8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;
		const u32 pitch = (font) ? w : w * 4;
		const u32 data_size = pitch * h * layers;
		const auto offset = upload_heap.alloc<512>(data_size);
		const auto addr = upload_heap.map(offset, data_size);

		const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers };

		auto tex = std::make_unique<vk::image>(dev, dev.get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_TYPE_2D, format, std::max(w, 1u), std::max(h, 1u), 1, 1, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0, VMM_ALLOCATION_POOL_UNDEFINED);

		if (pixel_src && data_size)
			std::memcpy(addr, pixel_src, data_size);
		else if (data_size)
			std::memset(addr, 0, data_size);

		upload_heap.unmap();

		VkBufferImageCopy region;
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layers };
		region.bufferOffset = offset;
		region.bufferRowLength = w;
		region.bufferImageHeight = h;
		region.imageOffset = {};
		region.imageExtent = { static_cast<u32>(w), static_cast<u32>(h), 1u };

		change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
		vkCmdCopyBufferToImage(cmd, upload_heap.heap->value, tex->value, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		change_image_layout(cmd, tex.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range);

		auto view = std::make_unique<vk::image_view>(dev, tex.get());

		auto result = view.get();

		if (!temp || font)
			view_cache[key] = std::move(view);
		else
			temp_view_cache[key] = std::move(view);

		if (font)
			font_cache[key] = std::move(tex);
		else if (!temp)
			resources.push_back(std::move(tex));
		else
			temp_image_cache[key] = std::make_pair(owner_uid, std::move(tex));

		return result;
	}

	void ui_overlay_renderer::init(vk::command_buffer& cmd, vk::data_heap& upload_heap)
	{
		rsx::overlays::resource_config configuration;
		configuration.load_files();

		auto& dev = cmd.get_command_pool().get_owner();
		u64 storage_key = 1;

		for (const auto &res : configuration.texture_raw_data)
		{
			upload_simple_texture(dev, cmd, upload_heap, storage_key++, res->w, res->h, 1, false, false, res->data, -1);
		}

		configuration.free_resources();
	}

	void ui_overlay_renderer::destroy()
	{
		temp_image_cache.clear();
		temp_view_cache.clear();

		resources.clear();
		font_cache.clear();
		view_cache.clear();

		overlay_pass::destroy();
	}

	void ui_overlay_renderer::remove_temp_resources(u32 key)
	{
		std::vector<u64> keys_to_remove;
		for (const auto& temp_image : temp_image_cache)
		{
			if (temp_image.second.first == key)
			{
				keys_to_remove.push_back(temp_image.first);
			}
		}

		for (const auto& _key : keys_to_remove)
		{
			auto& img_data = temp_image_cache[_key];
			auto& view_data = temp_view_cache[_key];

			auto gc = vk::get_resource_manager();
			gc->dispose(img_data.second);
			gc->dispose(view_data);

			temp_image_cache.erase(_key);
			temp_view_cache.erase(_key);
		}
	}

	vk::image_view* ui_overlay_renderer::find_font(rsx::overlays::font* font, vk::command_buffer& cmd, vk::data_heap& upload_heap)
	{
		const auto image_size = font->get_glyph_data_dimensions();

		u64 key = reinterpret_cast<u64>(font);
		auto found = view_cache.find(key);
		if (found != view_cache.end())
		{
			if (const auto raw = found->second->image();
				image_size.width == raw->width() &&
				image_size.height == raw->height() &&
				image_size.depth == raw->layers())
			{
				return found->second.get();
			}
			else
			{
				auto gc = vk::get_resource_manager();
				gc->dispose(font_cache[key]);
				gc->dispose(view_cache[key]);
			}
		}

		// Create font resource
		const std::vector<u8> bytes = font->get_glyph_data();

		return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, image_size.width, image_size.height, image_size.depth,
				true, false, bytes.data(), -1);
	}

	vk::image_view* ui_overlay_renderer::find_temp_image(rsx::overlays::image_info* desc, vk::command_buffer& cmd, vk::data_heap& upload_heap, u32 owner_uid)
	{
		u64 key = reinterpret_cast<u64>(desc);
		auto found = temp_view_cache.find(key);
		if (found != temp_view_cache.end())
			return found->second.get();

		return upload_simple_texture(cmd.get_command_pool().get_owner(), cmd, upload_heap, key, desc->w, desc->h, 1,
				false, true, desc->data, owner_uid);
	}

	std::vector<VkPushConstantRange> ui_overlay_renderer::get_push_constants()
	{
		return
		{
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.offset = 0,
				.size = 68
			},
			{
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 68,
				.size = 12
			}
		};
	}

	void ui_overlay_renderer::update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/)
	{
		// Byte Layout
		// 00: vec4 ui_scale;
		// 16: vec4 albedo;
		// 32: vec4 viewport;
		// 48: vec4 clip_bounds;
		// 64: uint vertex_config;
		// 68: uint fragment_config;
		// 72: float timestamp;
		// 76: float blur_intensity;

		f32 push_buf[32];
		// 1. Vertex config (00 - 63)
		std::memcpy(push_buf, m_scale_offset.rgba, 16);
		std::memcpy(push_buf + 4, m_color.rgba, 16);

		push_buf[8] = m_viewport.width;
		push_buf[9] = m_viewport.height;
		push_buf[10] = m_viewport.x;
		push_buf[11] = m_viewport.y;

		push_buf[12] = m_clip_region.x1;
		push_buf[13] = m_clip_region.y1;
		push_buf[14] = m_clip_region.x2;
		push_buf[15] = m_clip_region.y2;

		rsx::overlays::vertex_options vert_opts;
		const auto vert_config = vert_opts
			.disable_vertex_snap(m_disable_vertex_snap)
			.get();
		push_buf[16] = std::bit_cast<f32>(vert_config);

		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 68, push_buf);

		// 2. Fragment stuff
		rsx::overlays::fragment_options frag_opts;
		const auto frag_config = frag_opts
			.texture_mode(m_texture_type)
			.clip_fragments(m_clip_enabled)
			.pulse_glow(m_pulse_glow)
			.get();

		push_buf[0] = std::bit_cast<f32>(frag_config);
		push_buf[1] = m_time;
		push_buf[2] = m_blur_strength;

		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 68, 12, push_buf);
	}

	void ui_overlay_renderer::set_primitive_type(rsx::overlays::primitive_type type)
	{
		m_current_primitive_type = type;

		switch (type)
		{
			case rsx::overlays::primitive_type::quad_list:
			case rsx::overlays::primitive_type::triangle_strip:
				renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
				break;
			case rsx::overlays::primitive_type::line_list:
				renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
				break;
			case rsx::overlays::primitive_type::line_strip:
				renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
				break;
			case rsx::overlays::primitive_type::triangle_fan:
				renderpass_config.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
				break;
			default:
				fmt::throw_exception("Unexpected primitive type %d", static_cast<s32>(type));
		}
	}

	void ui_overlay_renderer::emit_geometry(vk::command_buffer& cmd)
	{
		if (m_current_primitive_type == rsx::overlays::primitive_type::quad_list)
		{
			// Emulate quads with disjointed triangle strips
			u32 first = 0;
			u32 num_quads = num_drawable_elements / 4;

			for (u32 n = 0; n < num_quads; ++n)
			{
				vkCmdDraw(cmd, 4, 1, first, 0);
				first += 4;
			}
		}
		else
		{
			overlay_pass::emit_geometry(cmd);
		}
	}

	void ui_overlay_renderer::run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* target, VkRenderPass render_pass,
			vk::data_heap& upload_heap, rsx::overlays::overlay& ui)
	{
		m_scale_offset = color4f(ui.virtual_width, ui.virtual_height, 1.f, 1.f);
		m_viewport = { { static_cast<f32>(viewport.x1), static_cast<f32>(viewport.y1) }, { static_cast<f32>(viewport.width()), static_cast<f32>(viewport.height()) } };

		std::vector<vk::image_view*> image_views
		{
			vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D),
			vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D_ARRAY)
		};

		if (ui.status_flags & rsx::overlays::status_bits::invalidate_image_cache)
		{
			remove_temp_resources(ui.uid);
			ui.status_flags.clear(rsx::overlays::status_bits::invalidate_image_cache);
		}

		for (auto& command : ui.get_compiled().draw_commands)
		{
			num_drawable_elements = static_cast<u32>(command.verts.size());

			upload_vertex_data(command.verts.data(), num_drawable_elements);
			set_primitive_type(command.config.primitives);

			m_time = command.config.get_sinus_value();
			m_texture_type = rsx::overlays::texture_sampling_mode::texture2D;
			m_color = command.config.color;
			m_pulse_glow = command.config.pulse_glow;
			m_blur_strength = static_cast<f32>(command.config.blur_strength) * 0.01f;
			m_clip_enabled = command.config.clip_region;
			m_clip_region = command.config.clip_rect;
			m_disable_vertex_snap = command.config.disable_vertex_snap;

			vk::image_view* src = nullptr;
			switch (command.config.texture_ref)
			{
			case rsx::overlays::image_resource_id::game_icon:
			case rsx::overlays::image_resource_id::backbuffer:
				// TODO
			case rsx::overlays::image_resource_id::none:
				m_texture_type = rsx::overlays::texture_sampling_mode::none;
				break;
			case rsx::overlays::image_resource_id::font_file:
				src = find_font(command.config.font_ref, cmd, upload_heap);
				m_texture_type = src->image()->layers() == 1
					? rsx::overlays::texture_sampling_mode::font2D
					: rsx::overlays::texture_sampling_mode::font3D;
				break;
			case rsx::overlays::image_resource_id::raw_image:
				src = find_temp_image(static_cast<rsx::overlays::image_info*>(command.config.external_data_ref), cmd, upload_heap, ui.uid);
				break;
			default:
				src = view_cache[command.config.texture_ref].get();
				break;
			}

			if (src)
			{
				const int res_id = src->image()->layers() > 1 ? 1 : 0;
				image_views[res_id] = src;
			}

			overlay_pass::run(cmd, viewport, target, image_views, render_pass);
		}

		ui.update(get_system_time());
	}

	attachment_clear_pass::attachment_clear_pass()
	{
		vs_src =
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"layout(push_constant) uniform static_data{ vec4 regs[2]; };\n"
			"layout(location=0) out vec4 color;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
			"	color = regs[0];\n"
			"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
			"}\n";

		fs_src =
			"#version 420\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"layout(location=0) in vec4 color;\n"
			"layout(location=0) out vec4 out_color;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	out_color = color;\n"
			"}\n";

		// Disable samplers
		m_num_usable_samplers = 0;

		renderpass_config.set_depth_mask(false);
		renderpass_config.set_color_mask(0, true, true, true, true);
		renderpass_config.set_attachment_count(1);
	}

	std::vector<VkPushConstantRange> attachment_clear_pass::get_push_constants()
	{
		VkPushConstantRange constant;
		constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		constant.offset = 0;
		constant.size = 32;

		return { constant };
	}

	void attachment_clear_pass::update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/)
	{
		f32 data[8];
		data[0] = clear_color.r;
		data[1] = clear_color.g;
		data[2] = clear_color.b;
		data[3] = clear_color.a;
		data[4] = colormask.r;
		data[5] = colormask.g;
		data[6] = colormask.b;
		data[7] = colormask.a;

		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 32, data);
	}

	void attachment_clear_pass::set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h)
	{
		VkViewport vp{};
		vp.x = static_cast<f32>(x);
		vp.y = static_cast<f32>(y);
		vp.width = static_cast<f32>(w);
		vp.height = static_cast<f32>(h);
		vp.minDepth = 0.f;
		vp.maxDepth = 1.f;
		vkCmdSetViewport(cmd, 0, 1, &vp);

		vkCmdSetScissor(cmd, 0, 1, &region);
	}

	void attachment_clear_pass::run(vk::command_buffer& cmd, vk::framebuffer* target, VkRect2D rect, u32 clearmask, color4f color, VkRenderPass render_pass)
	{
		region = rect;

		color4f mask = { 0.f, 0.f, 0.f, 0.f };
		if (clearmask & 0x10) mask.r = 1.f;
		if (clearmask & 0x20) mask.g = 1.f;
		if (clearmask & 0x40) mask.b = 1.f;
		if (clearmask & 0x80) mask.a = 1.f;

		if (mask != colormask || color != clear_color)
		{
			colormask = mask;
			clear_color = color;

			// Update color mask to match request
			renderpass_config.set_color_mask(0, colormask.r, colormask.g, colormask.b, colormask.a);
		}

		// Update renderpass configuration with the real number of samples
		renderpass_config.set_multisample_state(target->samples(), 0xFFFF, false, false, false);

		// Render fullscreen quad
		overlay_pass::run(cmd, { 0, 0, target->width(), target->height() }, target, std::vector<vk::image_view*>{}, render_pass);
	}

	stencil_clear_pass::stencil_clear_pass()
	{
		vs_src =
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"\n"
			"void main()\n"
			"{\n"
			"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
			"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
			"}\n";

		fs_src =
			"#version 420\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"layout(location=0) out vec4 out_color;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	out_color = vec4(0.);\n"
			"}\n";
	}

	void stencil_clear_pass::set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h)
	{
		VkViewport vp{};
		vp.x = static_cast<f32>(x);
		vp.y = static_cast<f32>(y);
		vp.width = static_cast<f32>(w);
		vp.height = static_cast<f32>(h);
		vp.minDepth = 0.f;
		vp.maxDepth = 1.f;
		vkCmdSetViewport(cmd, 0, 1, &vp);

		vkCmdSetScissor(cmd, 0, 1, &region);
	}

	void stencil_clear_pass::run(vk::command_buffer& cmd, vk::render_target* target, VkRect2D rect, u32 stencil_clear, u32 stencil_write_mask, VkRenderPass render_pass)
	{
		region = rect;

		// Stencil setup. Replace all pixels in the scissor region with stencil_clear with the correct write mask.
		renderpass_config.enable_stencil_test(
			VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE,  // Always replace
			VK_COMPARE_OP_ALWAYS,                                                 // Always pass
			0xFF,                                                                 // Full write-through
			stencil_clear);                                                       // Write active bit

		renderpass_config.set_stencil_mask(stencil_write_mask);
		renderpass_config.set_depth_mask(false);

		// Coverage sampling disabled, but actually report correct number of samples
		renderpass_config.set_multisample_state(target->samples(), 0xFFFF, false, false, false);

		overlay_pass::run(cmd, { 0, 0, target->width(), target->height() }, target, std::vector<vk::image_view*>{}, render_pass);
	}

	video_out_calibration_pass::video_out_calibration_pass()
	{
		vs_src =
		#include "../Program/GLSLSnippets/GenericVSPassthrough.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/VideoOutCalibrationPass.glsl"
		;

		std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%sampler_binding", fmt::format("(%d + x)", sampler_location(0)) },
			{ "%set_decorator", "set=0" },
		};
		fs_src = fmt::replace_all(fs_src, repl_list);

		renderpass_config.set_depth_mask(false);
		renderpass_config.set_color_mask(0, true, true, true, true);
		renderpass_config.set_attachment_count(1);

		m_num_usable_samplers = 2;
	}

	std::vector<VkPushConstantRange> video_out_calibration_pass::get_push_constants()
	{
		VkPushConstantRange constant;
		constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		constant.offset = 0;
		constant.size = 16;

		return { constant };
	}

	void video_out_calibration_pass::update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/)
	{
		vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16, config.data);
	}

	void video_out_calibration_pass::run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* target,
		const rsx::simple_array<vk::viewable_image*>& src, f32 gamma, bool limited_rgb, stereo_render_mode_options stereo_mode, VkRenderPass render_pass)
	{
		config.gamma = gamma;
		config.limit_range = limited_rgb? 1 : 0;
		config.stereo_display_mode = static_cast<u8>(stereo_mode);
		config.stereo_image_count = std::min(::size32(src), 2u);

		std::vector<vk::image_view*> views;
		views.reserve(2);

		for (auto& img : src)
		{
			img->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			views.push_back(img->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY)));
		}

		if (views.size() < 2)
		{
			views.push_back(vk::null_image_view(cmd, VK_IMAGE_VIEW_TYPE_2D));
		}

		overlay_pass::run(cmd, viewport, target, views, render_pass);

		for (auto& img : src)
		{
			img->pop_layout(cmd);
		}
	}
}
