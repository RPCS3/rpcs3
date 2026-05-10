#include "state_tracker.hpp"
#include "vao.hpp"

namespace gl
{
	static thread_local bool s_tls_primary_context_thread = false;
	static gl::driver_state* s_current_state = nullptr;

	void set_primary_context_thread(bool value)
	{
		s_tls_primary_context_thread = value;
	}

	bool is_primary_context_thread()
	{
		return s_tls_primary_context_thread;
	}

	void set_command_context(gl::command_context& ctx)
	{
		s_current_state = ctx.operator->();
	}

	void set_command_context(gl::driver_state& ctx)
	{
		s_current_state = &ctx;
	}

	gl::command_context get_command_context()
	{
		return { *s_current_state };
	}

	attrib_t vao::operator[](u32 index) const noexcept
	{
		return attrib_t(index);
	}
}
