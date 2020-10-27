#pragma once
#include "GLHelpers.h"
#include "Emu/RSX/display.h"
#include "Utilities/lockless.h"

namespace gl
{
	class pipe_compiler
	{
	public:
		enum op_flags
		{
			COMPILE_DEFAULT = 0,
			COMPILE_INLINE = 1,
			COMPILE_DEFERRED = 2
		};

		using callback_t = std::function<void(std::unique_ptr<glsl::program>&)>;

		pipe_compiler();
		~pipe_compiler();

		void initialize(
			std::function<draw_context_t()> context_create_func,
			std::function<void(draw_context_t)> context_bind_func,
			std::function<void(draw_context_t)> context_destroy_func);

		std::unique_ptr<glsl::program> compile(
			GLuint vp_handle, GLuint fp_handle,
			op_flags flags,
			callback_t post_create_func = {},
			callback_t post_link_func = {},
			callback_t completion_callback = {});

		void operator()();

	private:

		struct pipe_compiler_job
		{
			GLuint vp_handle;
			GLuint fp_handle;
			callback_t post_create_func;
			callback_t post_link_func;
			callback_t completion_callback;

			pipe_compiler_job(GLuint vp, GLuint fp, callback_t post_create, callback_t post_link, callback_t completion)
				: vp_handle(vp), fp_handle(fp), post_create_func(post_create), post_link_func(post_link), completion_callback(completion)
			{}
		};

		lf_queue<pipe_compiler_job> m_work_queue;

		draw_context_t m_context = 0;
		atomic_t<bool> m_context_ready = false;

		std::function<void(draw_context_t context)> m_context_bind_func;
		std::function<void(draw_context_t context)> m_context_destroy_func;

		std::unique_ptr<glsl::program> int_compile_graphics_pipe(
			GLuint vp_handle, GLuint fp_handle, callback_t post_create_func, callback_t post_link_func);
	};

	void initialize_pipe_compiler(
		std::function<draw_context_t()> context_create_func,
		std::function<void(draw_context_t)> context_bind_func,
		std::function<void(draw_context_t)> contextdestroy_func,
		int num_worker_threads = -1);

	void destroy_pipe_compiler();
	pipe_compiler* get_pipe_compiler();
}
