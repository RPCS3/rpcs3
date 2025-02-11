#pragma once

#include "GLCompute.h"
#include "GLOverlays.h"

namespace gl
{
	void resolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);
	void unresolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);
	void clear_resolve_helpers();

	struct cs_resolve_base : compute_task
	{
		gl::viewable_image* multisampled = nullptr;
		gl::viewable_image* resolve = nullptr;
		bool is_unresolve = false;

		u32 cs_wave_x = 1;
		u32 cs_wave_y = 1;

		cs_resolve_base()
		{}

		virtual ~cs_resolve_base()
		{}

		void build(const std::string& format_prefix, bool unresolve);

		void bind_resources() override;

		void run(gl::command_context& cmd, gl::viewable_image* msaa_image, gl::viewable_image* resolve_image);
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

	struct ds_resolve_pass_base : overlay_pass
	{
		gl::viewable_image* multisampled = nullptr;
		gl::viewable_image* resolve = nullptr;

		struct
		{
			bool resolve_depth = false;
			bool resolve_stencil = false;
			bool is_unresolve = false;
			color2i sample_count;
		} m_config;

		void build(bool depth, bool stencil, bool unresolve);

		void update_config();

		void run(gl::command_context& cmd, gl::viewable_image* msaa_image, gl::viewable_image* resolve_image);
	};

	struct depth_only_resolver : ds_resolve_pass_base
	{
		depth_only_resolver()
		{
			build(true, false, false);
		}
	};

	struct depth_only_unresolver : ds_resolve_pass_base
	{
		depth_only_unresolver()
		{
			build(true, false, true);
		}
	};

	struct stencil_only_resolver_base : ds_resolve_pass_base
	{
		virtual ~stencil_only_resolver_base() = default;

		void build(bool is_unresolver)
		{
			ds_resolve_pass_base::build(false, true, is_unresolver);
		}

		void emit_geometry(gl::command_context& cmd) override;
	};

	struct stencil_only_resolver : stencil_only_resolver_base
	{
		stencil_only_resolver()
		{
			build(false);
		}
	};

	struct stencil_only_unresolver : stencil_only_resolver_base
	{
		stencil_only_unresolver()
		{
			build(true);
		}
	};

	struct depth_stencil_resolver : ds_resolve_pass_base
	{
		depth_stencil_resolver()
		{
			build(true, true, false);
		}
	};

	struct depth_stencil_unresolver : ds_resolve_pass_base
	{
		depth_stencil_unresolver()
		{
			build(true, true, true);
		}
	};
}
