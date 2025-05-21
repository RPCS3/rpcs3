#pragma once

#ifdef HAVE_SDL3

#include <mutex>

struct sdl_instance
{
public:
	sdl_instance() = default;
	virtual ~sdl_instance();

	static sdl_instance& get_instance();

	bool initialize();
	void pump_events();

private:
	void set_hint(const char* name, const char* value);
	bool initialize_impl();

	bool m_initialized = false;
	std::mutex m_instance_mutex;
};

#endif
