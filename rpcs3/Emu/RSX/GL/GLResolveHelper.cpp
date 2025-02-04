#include "stdafx.h"
#include "GLResolveHelper.h"

#include <unordered_map>

namespace gl
{
	std::unordered_map<texture::internal_format, std::unique_ptr<cs_resolve_task>> g_resolve_helpers;
	std::unordered_map<texture::internal_format, std::unique_ptr<cs_unresolve_task>> g_unresolve_helpers;

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
		ensure(src->format_class() == RSX_FORMAT_CLASS_COLOR); // TODO
		{
			auto& job = g_resolve_helpers[src->get_internal_format()];
			if (!job)
			{
				const auto fmt = get_format_string(src->get_internal_format());
				job.reset(new cs_resolve_task(fmt));
			}

			job->run(cmd, src, dst);
		}
	}

	void unresolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src)
	{
		ensure(src->format_class() == RSX_FORMAT_CLASS_COLOR); // TODO
		{
			auto& job = g_unresolve_helpers[src->get_internal_format()];
			if (!job)
			{
				const auto fmt = get_format_string(src->get_internal_format());
				job.reset(new cs_unresolve_task(fmt));
			}

			job->run(cmd, dst, src);
		}
	}

	void cs_resolve_base::build(const std::string& format_prefix, bool unresolve)
	{
		create();

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
			#include "Emu/RSX/Program/MSAA/ColorResolvePass.glsl"
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
	}
}
