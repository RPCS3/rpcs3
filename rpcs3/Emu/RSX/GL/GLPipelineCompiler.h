#pragma once
#include "GLHelpers.h"
#include "glutils/program.h"
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

		using storage_callback_t = std::function<void(std::unique_ptr<glsl::program>&)>;
		using build_callback_t = std::function<void(glsl::program*)>;

		pipe_compiler();
		~pipe_compiler();

		void initialize(
			std::function<draw_context_t()> context_create_func,
			std::function<void(draw_context_t)> context_bind_func,
			std::function<void(draw_context_t)> context_destroy_func);

		std::unique_ptr<glsl::program> compile(
			op_flags flags,
			build_callback_t post_create_func = {},
			build_callback_t post_link_func = {},
			storage_callback_t completion_callback = {});

		void operator()();

	private:

		struct pipe_compiler_job
		{
			build_callback_t post_create_func;
			build_callback_t post_link_func;
			storage_callback_t completion_callback;

			pipe_compiler_job(build_callback_t post_create, build_callback_t post_link, storage_callback_t completion)
				: post_create_func(post_create), post_link_func(post_link), completion_callback(completion)
			{}
		};

		lf_queue<pipe_compiler_job> m_work_queue;

		draw_context_t m_context = 0;
		atomic_t<bool> m_context_ready = false;

		std::function<void(draw_context_t context)> m_context_bind_func;
		std::function<void(draw_context_t context)> m_context_destroy_func;

		std::unique_ptr<glsl::program> int_compile_graphics_pipe(
			build_callback_t post_create_func, build_callback_t post_link_func);
	};

	void initialize_pipe_compiler(
		std::function<draw_context_t()> context_create_func,
		std::function<void(draw_context_t)> context_bind_func,
		std::function<void(draw_context_t)> context_destroy_func,
		int num_worker_threads = -1);

	void destroy_pipe_compiler();
	pipe_compiler* get_pipe_compiler();
}
