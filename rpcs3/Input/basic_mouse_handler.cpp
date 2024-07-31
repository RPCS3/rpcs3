#include <QApplication>
#include <QCursor>

#include "util/types.hpp"
#include "util/logs.hpp"

#include "basic_mouse_handler.h"
#include "keyboard_pad_handler.h"
#include "rpcs3qt/gs_frame.h"
#include "Emu/Io/interception.h"
#include "Emu/Io/mouse_config.h"

mouse_config g_cfg_mouse;

void basic_mouse_handler::Init(const u32 max_connect)
{
	if (m_info.max_connect > 0)
	{
		// Already initialized
		return;
	}

	g_cfg_mouse.from_default();
	g_cfg_mouse.load();

	reload_config();

	m_mice.clear();
	m_mice.emplace_back(Mouse());

	m_info = {};
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(::size32(m_mice), max_connect);
	m_info.info = input::g_mice_intercepted ? CELL_MOUSE_INFO_INTERCEPTED : 0; // Ownership of mouse data: 0=Application, 1=System
	for (u32 i = 1; i < max_connect; i++)
	{
		m_info.status[i] = CELL_MOUSE_STATUS_DISCONNECTED;
		m_info.mode[i] = CELL_MOUSE_INFO_TABLET_MOUSE_MODE;
		m_info.tablet_is_supported[i] = CELL_MOUSE_INFO_TABLET_NOT_SUPPORTED;
	}
	m_info.status[0] = CELL_MOUSE_STATUS_CONNECTED; // (TODO: Support for more mice)
	m_info.vendor_id[0] = 0x1234;
	m_info.product_id[0] = 0x1234;

	type = mouse_handler::basic;
}

void basic_mouse_handler::reload_config()
{
	m_buttons[CELL_MOUSE_BUTTON_1] = get_mouse_button(g_cfg_mouse.mouse_button_1);
	m_buttons[CELL_MOUSE_BUTTON_2] = get_mouse_button(g_cfg_mouse.mouse_button_2);
	m_buttons[CELL_MOUSE_BUTTON_3] = get_mouse_button(g_cfg_mouse.mouse_button_3);
	m_buttons[CELL_MOUSE_BUTTON_4] = get_mouse_button(g_cfg_mouse.mouse_button_4);
	m_buttons[CELL_MOUSE_BUTTON_5] = get_mouse_button(g_cfg_mouse.mouse_button_5);
	m_buttons[CELL_MOUSE_BUTTON_6] = get_mouse_button(g_cfg_mouse.mouse_button_6);
	m_buttons[CELL_MOUSE_BUTTON_7] = get_mouse_button(g_cfg_mouse.mouse_button_7);
	m_buttons[CELL_MOUSE_BUTTON_8] = get_mouse_button(g_cfg_mouse.mouse_button_8);
}

/* Sets the target window for the event handler, and also installs an event filter on the target. */
void basic_mouse_handler::SetTargetWindow(QWindow* target)
{
	if (target)
	{
		m_target = target;
		target->installEventFilter(this);
	}
	else
	{
		// If this is hit, it probably means that some refactoring occurs because currently a gsframe is created in Load.
		// We still want events so filter from application instead since target is null.
		QApplication::instance()->installEventFilter(this);
		input_log.error("Trying to set mouse handler to a null target window.");
	}
}

bool basic_mouse_handler::eventFilter(QObject* target, QEvent* ev)
{
	if (!ev)
	{
		return false;
	}

	// !m_target is for future proofing when gsrender isn't automatically initialized on load to ensure events still occur
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible() || target == m_target)
	{
		if (g_cfg_mouse.reload_requested.exchange(false))
		{
			reload_config();
		}

		switch (ev->type())
		{
		case QEvent::MouseButtonPress:
			MouseButtonDown(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseButtonRelease:
			MouseButtonUp(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseMove:
			MouseMove(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::Wheel:
			MouseScroll(static_cast<QWheelEvent*>(ev));
			break;
		default:
			return false;
		}
	}
	return false;
}

void basic_mouse_handler::MouseButtonDown(QMouseEvent* event)
{
	if (!event) return;

	const int button = event->button();
	if (const auto it = std::find_if(m_buttons.cbegin(), m_buttons.cend(), [button](const auto& entry){ return entry.second == button; });
		it != m_buttons.cend())
	{
		MouseHandlerBase::Button(0, it->first, true);
	}
}

void basic_mouse_handler::MouseButtonUp(QMouseEvent* event)
{
	if (!event) return;

	const int button = event->button();
	if (const auto it = std::find_if(m_buttons.cbegin(), m_buttons.cend(), [button](const auto& entry){ return entry.second == button; });
		it != m_buttons.cend())
	{
		MouseHandlerBase::Button(0, it->first, false);
	}
}

void basic_mouse_handler::MouseScroll(QWheelEvent* event)
{
	if (!event) return;

	const QPoint delta = event->angleDelta();
	const s8 x = std::clamp(delta.x() / 120, -128, 127);
	const s8 y = std::clamp(delta.y() / 120, -128, 127);
	MouseHandlerBase::Scroll(0, x, y);
}

bool basic_mouse_handler::get_mouse_lock_state() const
{
	if (auto game_frame = dynamic_cast<gs_frame*>(m_target))
		return game_frame->get_mouse_lock_state();
	return false;
}

int basic_mouse_handler::get_mouse_button(const cfg::string& button)
{
	const std::string name = button.to_string();
	const auto it = std::find_if(mouse_list.cbegin(), mouse_list.cend(), [&name](const auto& entry){ return entry.second == name; });

	if (it != mouse_list.cend())
	{
		return it->first;
	}

	return Qt::MouseButton::NoButton;
}

void basic_mouse_handler::MouseMove(QMouseEvent* event)
{
	if (!event) return;

	if (is_time_for_update())
	{
		// get the screen dimensions
		const QSize screen = m_target->size();
		const QPoint e_pos = event->pos();

		if (m_target && m_target->isActive() && get_mouse_lock_state())
		{
			// get the center of the screen in global coordinates
			QPoint p_center = m_target->geometry().topLeft() + QPoint(screen.width() / 2, screen.height() / 2);

			// reset the mouse to the center for consistent results since edge movement won't be registered
			QCursor::setPos(m_target->screen(), p_center);

			// convert the center into screen coordinates
			p_center = m_target->mapFromGlobal(p_center);

			// current mouse position, starting at the center
			static QPoint p_real(p_center);

			// get the delta of the mouse position to the screen center
			const QPoint p_delta = e_pos - p_center;

			// update the current position without leaving the screen borders
			p_real.setX(std::clamp(p_real.x() + p_delta.x(), 0, screen.width()));
			p_real.setY(std::clamp(p_real.y() + p_delta.y(), 0, screen.height()));

			// pass the 'real' position and the current delta to the screen center
			MouseHandlerBase::Move(0, p_real.x(), p_real.y(), screen.width(), screen.height(), true, p_delta.x(), p_delta.y());
		}
		else
		{
			// pass the absolute position
			MouseHandlerBase::Move(0, e_pos.x(), e_pos.y(), screen.width(), screen.height());
		}
	}
}
