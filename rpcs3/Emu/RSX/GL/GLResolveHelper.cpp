#include "stdafx.h"
#include "GLResolveHelper.h"
#include "GLTexture.h"

#include <unordered_map>
#include <stack>

namespace gl
{
	std::unordered_map<texture::internal_format, std::unique_ptr<cs_resolve_task>> g_resolve_helpers;
	std::unordered_map<texture::internal_format, std::unique_ptr<cs_unresolve_task>> g_unresolve_helpers;
	std::unordered_map<GLuint, std::unique_ptr<ds_resolve_pass_base>> g_depth_resolvers;
	std::unordered_map<GLuint, std::unique_ptr<ds_resolve_pass_base>> g_depth_unresolvers;

	void clear_resolve_helpers()
	{
		g_resolve_helpers.clear();
		g_unresolve_helpers.clear();
		g_depth_resolvers.clear();
		g_depth_unresolvers.clear();
	}

	static const char* get_format_string(gl::texture::internal_format format)
	{
		switch (format)
		{
		case texture::internal_format::rgb565:
			return "r16";
		case texture::internal_format::rgba8:
		case texture::internal_format::bgra8:
			return "rgba8";
		case texture::internal_format::rgba16f:
			return "rgba16f";
		case texture::internal_format::rgba32f:
			return "rgba32f";
		case texture::internal_format::bgr5a1:
			return "r16";
		case texture::internal_format::r8:
			return "r8";
		case texture::internal_format::rg8:
			return "rg8";
		case texture::internal_format::r32f:
			return "r32f";
		default:
			fmt::throw_exception("Unhandled internal format 0x%x", u32(format));
		}
	}

	void resolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src)
	{
		ensure(src->samples() > 1 && dst->samples() == 1);

		if (src->aspect() == gl::image_aspect::color) [[ likely ]]
		{
			auto& job = g_resolve_helpers[src->get_internal_format()];
			if (!job)
			{
				const auto fmt = get_format_string(src->get_internal_format());
				job.reset(new cs_resolve_task(fmt));
			}

			job->run(cmd, src, dst);
			return;
		}

		auto get_resolver_pass = [](GLuint aspect_bits) -> std::unique_ptr<ds_resolve_pass_base>&
		{
			auto& pass = g_depth_resolvers[aspect_bits];
			if (!pass)
			{
				ds_resolve_pass_base* ptr = nullptr;
				switch (aspect_bits)
				{
				case gl::image_aspect::depth:
					ptr = new depth_only_resolver();
					break;
				case gl::image_aspect::stencil:
					ptr = new stencil_only_resolver();
					break;
				case (gl::image_aspect::depth | gl::image_aspect::stencil):
					ptr = new depth_stencil_resolver();
					break;
				default:
					fmt::throw_exception("Unreachable");
				}

				pass.reset(ptr);
			}

			return pass;
		};

		if (src->aspect() == (gl::image_aspect::depth | gl::image_aspect::stencil) &&
			!gl::get_driver_caps().ARB_shader_stencil_export_supported)
		{
			// Special case, NVIDIA-only fallback
			auto& depth_pass = get_resolver_pass(gl::image_aspect::depth);
			depth_pass->run(cmd, src, dst);

			auto& stencil_pass = get_resolver_pass(gl::image_aspect::stencil);
			stencil_pass->run(cmd, src, dst);
			return;
		}

		auto& pass = get_resolver_pass(src->aspect());
		pass->run(cmd, src, dst);
	}

	void unresolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src)
	{
		ensure(dst->samples() > 1 && src->samples() == 1);

		if (src->aspect() == gl::image_aspect::color) [[ likely ]]
		{
			auto& job = g_unresolve_helpers[src->get_internal_format()];
			if (!job)
			{
				const auto fmt = get_format_string(src->get_internal_format());
				job.reset(new cs_unresolve_task(fmt));
			}

			job->run(cmd, dst, src);
			return;
		}

		auto get_unresolver_pass = [](GLuint aspect_bits) -> std::unique_ptr<ds_resolve_pass_base>&
		{
			auto& pass = g_depth_unresolvers[aspect_bits];
			if (!pass)
			{
				ds_resolve_pass_base* ptr = nullptr;
				switch (aspect_bits)
				{
				case gl::image_aspect::depth:
					ptr = new depth_only_unresolver();
					break;
				case gl::image_aspect::stencil:
					ptr = new stencil_only_unresolver();
					break;
				case (gl::image_aspect::depth | gl::image_aspect::stencil):
					ptr = new depth_stencil_unresolver();
					break;
				default:
					fmt::throw_exception("Unreachable");
				}

				pass.reset(ptr);
			}

			return pass;
		};

		if (src->aspect() == (gl::image_aspect::depth | gl::image_aspect::stencil) &&
			!gl::get_driver_caps().ARB_shader_stencil_export_supported)
		{
			// Special case, NVIDIA-only fallback
			auto& depth_pass = get_unresolver_pass(gl::image_aspect::depth);
			depth_pass->run(cmd, dst, src);

			auto& stencil_pass = get_unresolver_pass(gl::image_aspect::stencil);
			stencil_pass->run(cmd, dst, src);
			return;
		}

		auto& pass = get_unresolver_pass(src->aspect());
		pass->run(cmd, dst, src);
	}

	// Implementation

	void cs_resolve_base::build(const std::string& format_prefix, bool unresolve)
	{
		is_unresolve = unresolve;

		switch (optimal_group_size)
		{
		default:
		case 64:
			cs_wave_x = 8;
			cs_wave_y = 8;
			break;
		case 32:
			cs_wave_x = 8;
			cs_wave_y = 4;
			break;
		}

		static const char* resolve_kernel =
			#include "Emu/RSX/Program/MSAA/ColorResolvePass.glsl"
			;

		static const char* unresolve_kernel =
			#include "Emu/RSX/Program/MSAA/ColorUnresolvePass.glsl"
			;

		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%WORKGROUP_SIZE_X", std::to_string(cs_wave_x) },
			{ "%WORKGROUP_SIZE_Y", std::to_string(cs_wave_y) },
			{ "%IMAGE_FORMAT", format_prefix },
			{ "%BGRA_SWAP", "0" }
		};

		m_src = unresolve ? unresolve_kernel : resolve_kernel;
		m_src = fmt::replace_all(m_src, syntax_replace);

		rsx_log.notice("Resolve shader:\n%s", m_src);

		create();
	}

	void cs_resolve_base::bind_resources()
	{
		auto msaa_view = multisampled->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_VIEW_MULTISAMPLED));
		auto resolved_view = resolve->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY));

		glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(0), msaa_view->id(), 0, GL_FALSE, 0, is_unresolve ? GL_WRITE_ONLY : GL_READ_ONLY, msaa_view->view_format());
		glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(1), resolved_view->id(), 0, GL_FALSE, 0, is_unresolve ? GL_READ_ONLY : GL_WRITE_ONLY, resolved_view->view_format());
	}

	void cs_resolve_base::run(gl::command_context& cmd, gl::viewable_image* msaa_image, gl::viewable_image* resolve_image)
	{
		ensure(msaa_image->samples() > 1);
		ensure(resolve_image->samples() == 1);

		multisampled = msaa_image;
		resolve = resolve_image;

		const u32 invocations_x = utils::align(resolve_image->width(), cs_wave_x) / cs_wave_x;
		const u32 invocations_y = utils::align(resolve_image->height(), cs_wave_y) / cs_wave_y;

		compute_task::run(cmd, invocations_x, invocations_y);
	}

	void ds_resolve_pass_base::build(bool depth, bool stencil, bool unresolve)
	{
		m_config.resolve_depth = depth;
		m_config.resolve_stencil = stencil;
		m_config.is_unresolve = unresolve;

		vs_src =
#include "Emu/RSX/Program/GLSLSnippets/GenericVSPassthrough.glsl"
			;

		static const char* depth_resolver =
#include "Emu/RSX/Program/MSAA/DepthResolvePass.glsl"
			;

		static const char* depth_unresolver =
#include "Emu/RSX/Program/MSAA/DepthUnresolvePass.glsl"
			;

		static const char* stencil_resolver =
#include "Emu/RSX/Program/MSAA/StencilResolvePass.glsl"
			;

		static const char* stencil_unresolver =
#include "Emu/RSX/Program/MSAA/StencilUnresolvePass.glsl"
			;

		static const char* depth_stencil_resolver =
#include "Emu/RSX/Program/MSAA/DepthStencilResolvePass.glsl"
			;

		static const char* depth_stencil_unresolver =
#include "Emu/RSX/Program/MSAA/DepthStencilUnresolvePass.glsl"
			;

		if (m_config.resolve_depth && m_config.resolve_stencil)
		{
			fs_src = m_config.is_unresolve ? depth_stencil_unresolver : depth_stencil_resolver;
			m_write_aspect_mask = gl::image_aspect::depth | gl::image_aspect::stencil;
		}
		else if (m_config.resolve_depth)
		{
			fs_src = m_config.is_unresolve ? depth_unresolver : depth_resolver;
			m_write_aspect_mask = gl::image_aspect::depth;
		}
		else if (m_config.resolve_stencil)
		{
			fs_src = m_config.is_unresolve ? stencil_unresolver : stencil_resolver;
			m_write_aspect_mask = gl::image_aspect::stencil;
		}

		enable_depth_writes = m_config.resolve_depth;
		enable_stencil_writes = m_config.resolve_stencil;


		create();

		rsx_log.notice("Resolve shader:\n%s", fs_src);
	}

	void ds_resolve_pass_base::update_config()
	{
		ensure(multisampled && multisampled->samples() > 1);
		switch (multisampled->samples())
		{
		case 2:
			m_config.sample_count.x = 2;
			m_config.sample_count.y = 1;
			break;
		case 4:
			m_config.sample_count.x = m_config.sample_count.y = 2;
			break;
		default:
			fmt::throw_exception("Unsupported sample count %d", multisampled->samples());
		}

		program_handle.uniforms["sample_count"] = m_config.sample_count;
	}

	void ds_resolve_pass_base::run(gl::command_context& cmd, gl::viewable_image* msaa_image, gl::viewable_image* resolve_image)
	{
		multisampled = msaa_image;
		resolve = resolve_image;
		update_config();

		const auto read_resource = m_config.is_unresolve ? resolve_image : msaa_image;
		const auto write_resource = m_config.is_unresolve ? msaa_image : resolve_image;

		// Resource binding
		std::stack<int> bind_slots;
		std::vector<std::unique_ptr<saved_sampler_state>> saved_sampler_states;
		auto allocate_slot = [&]() -> int
		{
			ensure(!bind_slots.empty());
			const int slot = bind_slots.top();
			bind_slots.pop();
			saved_sampler_states.emplace_back(std::make_unique<gl::saved_sampler_state>(slot, m_sampler));
			return slot;
		};

		// Reserve 2 slots max
		bind_slots.push(GL_TEMP_IMAGE_SLOT(1));
		bind_slots.push(GL_TEMP_IMAGE_SLOT(0));

		if (m_config.resolve_depth)
		{
			const int bind_slot = allocate_slot();
			cmd->bind_texture(bind_slot, static_cast<GLenum>(read_resource->get_target()), read_resource->id(), GL_TRUE);
		}

		if (m_config.resolve_stencil)
		{
			const int bind_slot = allocate_slot();
			auto stencil_view = read_resource->get_view(rsx::default_remap_vector.with_encoding(gl::GL_REMAP_IDENTITY), gl::image_aspect::stencil);
			cmd->bind_texture(bind_slot, static_cast<GLenum>(read_resource->get_target()), stencil_view->id(), GL_TRUE);
		}

		areau viewport{};
		viewport.x2 = write_resource->width();
		viewport.y2 = write_resource->height();
		overlay_pass::run(cmd, viewport, write_resource->id(), m_write_aspect_mask, false);
	}

	void stencil_only_resolver_base::emit_geometry(gl::command_context& cmd)
	{
		// Modified version of the base overlay pass to emit 8 draws instead of 1
		int old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
		m_vao.bind();

		// Clear the target
		gl::clear_cmd_info clear_info
		{
			.aspect_mask = gl::image_aspect::stencil,
			.clear_stencil = {
				.mask = 0xFF,
				.value = 0
			}
		};
		gl::clear_attachments(cmd, clear_info);

		// Override stencil settings. Always pass, reference is all one, compare mask doesn't matter.
		// For each pass the write mask will be overriden to commit output bitwise
		cmd->stencil_func(GL_ALWAYS, 0xFF, 0xFF);
		cmd->stencil_op(GL_REPLACE, GL_REPLACE, GL_REPLACE);

		// Start our inner loop
		for (s32 write_mask = 0x1; write_mask <= 0x80; write_mask <<= 1)
		{
			program_handle.uniforms["stencil_mask"] = write_mask;
			cmd->stencil_mask(write_mask);

			glDrawArrays(primitives, 0, num_drawable_elements);
		}

		glBindVertexArray(old_vao);
	}
}
