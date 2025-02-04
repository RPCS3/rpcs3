#pragma once

#include "GLCompute.h"
#include "GLOverlays.h"

namespace gl
{
	void resolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);
	void unresolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);

	struct cs_resolve_base : compute_task
	{
		gl::viewable_image* multisampled = nullptr;
		gl::viewable_image* resolve = nullptr;

		u32 cs_wave_x = 1;
		u32 cs_wave_y = 1;

		cs_resolve_base()
		{}

		virtual ~cs_resolve_base()
		{}

		void build(const std::string& format_prefix, bool unresolve);

		void bind_resources() override
		{
			auto msaa_view = multisampled->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_VIEW_MULTISAMPLED));
			auto resolved_view = resolve->get_view(rsx::default_remap_vector.with_encoding(GL_REMAP_IDENTITY));

			glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(0), msaa_view->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, msaa_view->view_format());
			glBindImageTexture(GL_COMPUTE_IMAGE_SLOT(1), resolved_view->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, resolved_view->view_format());
		}

		void run(gl::command_context& cmd, gl::viewable_image* msaa_image, gl::viewable_image* resolve_image)
		{
			ensure(msaa_image->samples() > 1);
			ensure(resolve_image->samples() == 1);

			multisampled = msaa_image;
			resolve = resolve_image;

			const u32 invocations_x = utils::align(resolve_image->width(), cs_wave_x) / cs_wave_x;
			const u32 invocations_y = utils::align(resolve_image->height(), cs_wave_y) / cs_wave_y;

			compute_task::run(cmd, invocations_x, invocations_y);
		}
	};

	struct cs_resolve_task : cs_resolve_base
	{
		cs_resolve_task(const std::string& format_prefix)
		{
			build(format_prefix, false);
		}
	};

	struct cs_unresolve_task : cs_resolve_base
	{
		cs_unresolve_task(const std::string& format_prefix)
		{
			build(format_prefix, true);
		}
	};
}
