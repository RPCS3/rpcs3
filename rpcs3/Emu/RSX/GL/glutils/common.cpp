#include "state_tracker.hpp"
#include "vao.hpp"

#include <util/atomic.hpp>

extern u64 get_system_time();

namespace gl
{
	static thread_local bool s_tls_primary_context_thread = false;
	static gl::driver_state* s_current_state = nullptr;

	// Context loss tracking
	struct driver_context_state_t
	{
		bool reset_tracking_enabled = false;
		atomic_t<bool> is_context_lost = false;
		u64 last_reset_timestamp = 0;

		static constexpr u64 min_time_between_resets_us = 10 * 1000'000ull;

		void init()
		{
			is_context_lost = false;
			reset_tracking_enabled = false;

			if (!get_driver_caps().ARB_robustness_supported || !glGetGraphicsResetStatusARB)
			{
				rsx_log.warning("GL_ARB_robustness is not supported. GPU resets cannot be detected.");
				return;
			}

			GLint strategy = 0;
			glGetIntegerv(GL_RESET_NOTIFICATION_STRATEGY_ARB, &strategy);

			if (strategy != GL_LOSE_CONTEXT_ON_RESET_ARB)
			{
				rsx_log.warning("The OpenGL context does not report resets. GPU resets cannot be detected.");
				return;
			}

			reset_tracking_enabled = true;
		}

		bool query_reset()
		{
			if (!reset_tracking_enabled || is_context_lost)
			{
				return is_context_lost;
			}

			const GLenum status = glGetGraphicsResetStatusARB();
			if (status == GL_NO_ERROR) [[likely]]
			{
				return false;
			}

			// The status is only reported once, the context stays lost
			is_context_lost = true;

			const char* reason;
			switch (status)
			{
			case GL_GUILTY_CONTEXT_RESET_ARB:
				reason = "a GPU reset was caused by this context";
				break;
			case GL_INNOCENT_CONTEXT_RESET_ARB:
				reason = "a GPU reset was caused by another process";
				break;
			default:
				reason = "a GPU reset of unknown origin";
				break;
			}

			// Avoid an endless reset loop
			const u64 now = get_system_time();
			const u64 prev = std::exchange(last_reset_timestamp, now);

			if (prev && (now - prev) < min_time_between_resets_us)
			{
				fmt::throw_exception("OpenGL context lost again shortly after a recovery (%s)", reason);
			}

			rsx_log.error("OpenGL context lost (%s), the renderer will be replaced", reason);
			return true;
		}
	};

	static driver_context_state_t s_context_state;

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

	void command_context::init()
	{
		s_context_state.init();
	}

	bool command_context::is_lost(bool poll)
	{
		return poll
			? s_context_state.query_reset()
			: s_context_state.is_context_lost.load();
	}
}
