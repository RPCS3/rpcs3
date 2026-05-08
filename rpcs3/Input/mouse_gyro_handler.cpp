#include "stdafx.h"
#include "mouse_gyro_handler.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>

LOG_CHANNEL(gui_log, "GUI");

void mouse_gyro_handler::clear()
{
	m_active = false;
	m_reset = false;
	m_gyro_x = DEFAULT_MOTION_X;
	m_gyro_y = DEFAULT_MOTION_Y;
	m_gyro_z = DEFAULT_MOTION_Z;
}

bool mouse_gyro_handler::toggle_enabled()
{
	m_enabled = !m_enabled;
	clear();
	return m_enabled;
}

void mouse_gyro_handler::set_gyro_active()
{
	gui_log.notice("Mouse-based gyro activated");
	m_active = true;
}

void mouse_gyro_handler::set_gyro_reset()
{
	gui_log.notice("Mouse-based gyro deactivated");
	m_active = false;
	m_reset = true;
}

void mouse_gyro_handler::set_gyro_xz(s32 off_x, s32 off_y)
{
	if (!m_active)
		return;

	m_gyro_x = static_cast<u16>(std::clamp(off_x, 0, DEFAULT_MOTION_X * 2 - 1));
	m_gyro_z = static_cast<u16>(std::clamp(off_y, 0, DEFAULT_MOTION_Z * 2 - 1));
}

void mouse_gyro_handler::set_gyro_y(s32 steps)
{
	if (!m_active)
		return;

	m_gyro_y = static_cast<u16>(std::clamp(m_gyro_y + steps, 0, DEFAULT_MOTION_Y * 2 - 1));
}

void mouse_gyro_handler::handle_event(QEvent* ev, const QWindow& win)
{
	if (!m_enabled)
		return;

	// Mouse-based motion input.
	// Captures mouse events while the game window is focused.
	// Updates motion sensor values via mouse position and mouse wheel while RMB is held.
	// Intentionally independent of chosen pad configuration.
	switch (ev->type())
	{
	case QEvent::MouseButtonPress:
	{
		auto* e = static_cast<QMouseEvent*>(ev);
		if (e->button() == Qt::RightButton)
		{
			// Enable mouse-driven gyro emulation while RMB is held.
			set_gyro_active();
		}
		break;
	}
	case QEvent::MouseButtonRelease:
	{
		auto* e = static_cast<QMouseEvent*>(ev);
		if (e->button() == Qt::RightButton)
		{
			// Disable gyro emulation and request a one-shot motion reset.
			set_gyro_reset();
		}
		break;
	}
	case QEvent::MouseMove:
	{
		auto* e = static_cast<QMouseEvent*>(ev);

		// Track cursor offset from window center.
		const QPoint center(win.width() / 2, win.height() / 2);
		const QPoint cur = e->position().toPoint();

		const s32 off_x = cur.x() - center.x() + DEFAULT_MOTION_X;
		const s32 off_y = cur.y() - center.y() + DEFAULT_MOTION_Z;

		// Determine motion from relative mouse position while gyro emulation is active.
		set_gyro_xz(off_x, off_y);

		break;
	}
	case QEvent::Wheel:
	{
		auto* e = static_cast<QWheelEvent*>(ev);

		// Track mouse wheel steps.
		const s32 steps = e->angleDelta().y() / 120;

		// Accumulate mouse wheel steps while gyro emulation is active.
		set_gyro_y(steps);

		break;
	}
	default:
	{
		break;
	}
	}
}

void mouse_gyro_handler::apply_gyro(const std::shared_ptr<Pad>& pad)
{
	if (!m_enabled || !pad || !pad->is_connected())
		return;

	// Inject mouse-based motion sensor values into pad sensors for gyro emulation.
	// The Qt frontend maps cursor offset and wheel input to absolute motion values while RMB is held.
	if (m_reset)
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
		pad->m_sensors[0].m_value = m_gyro_x; // Mouse X → Motion X
		pad->m_sensors[1].m_value = m_gyro_y; // Mouse Wheel → Motion Y
		pad->m_sensors[2].m_value = m_gyro_z; // Mouse Y → Motion Z
	}
}
