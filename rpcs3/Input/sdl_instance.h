#pragma once

#ifdef HAVE_SDL3

#include <mutex>

struct sdl_instance
{
public:
	sdl_instance() = default;
	virtual ~sdl_instance();

	static sdl_instance& get_instance()
	{
		static sdl_instance instance {};
		return instance;
	}

	bool initialize();

private:
	bool initialize_impl();

	bool m_initialized = false;
	std::mutex mtx;
};

#endif
