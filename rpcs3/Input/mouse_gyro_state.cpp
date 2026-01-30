#include "mouse_gyro_state.h"

void mouse_gyro_state::clear()
{
	active = false;
	reset = false;
	gyro_x = DEFAULT_MOTION_X;
	gyro_y = DEFAULT_MOTION_Y;
	gyro_z = DEFAULT_MOTION_Z;
}

void mouse_gyro_state::set_gyro_active()
{
	active = true;
}

void mouse_gyro_state::set_gyro_reset()
{
	active = false;
	reset = true;
}

void mouse_gyro_state::set_gyro_xz(s32 off_x, s32 off_y)
{
	if (!active)
		return;

	gyro_x = static_cast<u16>(std::clamp(off_x, 0, DEFAULT_MOTION_X * 2 - 1));
	gyro_z = static_cast<u16>(std::clamp(off_y, 0, DEFAULT_MOTION_Z * 2 - 1));
}

void mouse_gyro_state::set_gyro_y(s32 steps)
{
	if (!active)
		return;

	gyro_y = static_cast<u16>(std::clamp(gyro_y + steps, 0, DEFAULT_MOTION_Y * 2 - 1));
}

void mouse_gyro_state::gyro_run(const std::shared_ptr<Pad>& pad)
{
	if (!pad || !pad->is_connected())
		return;

	// Inject mouse-based motion sensor values into pad sensors for gyro emulation.
	// The Qt frontend maps cursor offset and wheel input to absolute motion values while RMB is held.
	if (reset)
	{
		// RMB released → reset motion
		pad->m_sensors[0].m_value = DEFAULT_MOTION_X;
		pad->m_sensors[1].m_value = DEFAULT_MOTION_Y;
		pad->m_sensors[2].m_value = DEFAULT_MOTION_Z;
		clear();
	}
	else
	{
		// RMB held → accumulate motion
		// Axes have been chosen as tested in Sly 4 minigames. Top-down view motion uses X/Z axes.
		pad->m_sensors[0].m_value = gyro_x; // Mouse X → Motion X
		pad->m_sensors[1].m_value = gyro_y; // Mouse Wheel → Motion Y
		pad->m_sensors[2].m_value = gyro_z; // Mouse Y → Motion Z
	}
}
