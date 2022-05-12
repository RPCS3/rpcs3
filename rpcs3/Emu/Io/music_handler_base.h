#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

class music_handler_base
{
public:
	virtual ~music_handler_base() = default;

	virtual void stop() = 0;
	virtual void play(const std::string& path) = 0;
	virtual void pause() = 0;
	virtual void fast_forward() = 0;
	virtual void fast_reverse() = 0;
	virtual void set_volume(f32 volume) = 0;
	virtual f32 get_volume() const = 0;

	s32 get_state() const
	{
		return m_state;
	}

protected:
	atomic_t<s32> m_state{0};
};
