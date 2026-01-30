#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "Emu/Io/pad_types.h"

class QEvent;
class QWindow;

// Mouse-based motion sensor emulation state.
class mouse_gyro_state
{
private:
	atomic_t<bool> active = false; // Whether right mouse button is currently held (gyro active)
	atomic_t<bool> reset = false;  // One-shot reset request on right mouse button release
	atomic_t<s32> gyro_x = DEFAULT_MOTION_X; // Accumulated from mouse X position relative to center
	atomic_t<s32> gyro_y = DEFAULT_MOTION_Y; // Accumulated from mouse wheel delta
	atomic_t<s32> gyro_z = DEFAULT_MOTION_Z; // Accumulated from mouse Y position relative to center

public:
	void clear();

	bool is_gyro_active() const { return active; }
	bool needs_gyro_reset() const { return reset; }
	s32 get_gyro_x() const { return gyro_x; }
	s32 get_gyro_y() const { return gyro_y; }
	s32 get_gyro_z() const { return gyro_z; }

	void set_gyro_active();
	void set_gyro_reset();
	void set_gyro_xz(s32 off_x, s32 off_y);
	void set_gyro_y(s32 steps);

	void gyro_detect(QEvent* ev, const QWindow& win);
	void apply_gyro(const std::shared_ptr<Pad>& pad);
};
