#include "keyboard_pad_handler.h"
#include "pad_thread.h"
#include "Emu/Io/pad_config.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/interception.h"
#include "Input/product_info.h"
#include "rpcs3qt/gs_frame.h"

#include <QApplication>

bool keyboard_pad_handler::Init()
{
	const steady_clock::time_point now = steady_clock::now();
	m_last_mouse_move_left  = now;
	m_last_mouse_move_right = now;
	m_last_mouse_move_up    = now;
	m_last_mouse_move_down  = now;
	return true;
}

keyboard_pad_handler::keyboard_pad_handler()
	: QObject()
	, PadHandlerBase(pad_handler::keyboard)
{
	init_configs();

	// set capabilities
	b_has_config = true;
}

void keyboard_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = GetKeyName(Qt::Key_A);
	cfg->ls_down.def  = GetKeyName(Qt::Key_S);
	cfg->ls_right.def = GetKeyName(Qt::Key_D);
	cfg->ls_up.def    = GetKeyName(Qt::Key_W);
	cfg->rs_left.def  = GetKeyName(Qt::Key_Delete);
	cfg->rs_down.def  = GetKeyName(Qt::Key_End);
	cfg->rs_right.def = GetKeyName(Qt::Key_PageDown);
	cfg->rs_up.def    = GetKeyName(Qt::Key_Home);
	cfg->start.def    = GetKeyName(Qt::Key_Return);
	cfg->select.def   = GetKeyName(Qt::Key_Space);
	cfg->ps.def       = GetKeyName(Qt::Key_Backspace);
	cfg->square.def   = GetKeyName(Qt::Key_Z);
	cfg->cross.def    = GetKeyName(Qt::Key_X);
	cfg->circle.def   = GetKeyName(Qt::Key_C);
	cfg->triangle.def = GetKeyName(Qt::Key_V);
	cfg->left.def     = GetKeyName(Qt::Key_Left);
	cfg->down.def     = GetKeyName(Qt::Key_Down);
	cfg->right.def    = GetKeyName(Qt::Key_Right);
	cfg->up.def       = GetKeyName(Qt::Key_Up);
	cfg->r1.def       = GetKeyName(Qt::Key_E);
	cfg->r2.def       = GetKeyName(Qt::Key_T);
	cfg->r3.def       = GetKeyName(Qt::Key_G);
	cfg->l1.def       = GetKeyName(Qt::Key_Q);
	cfg->l2.def       = GetKeyName(Qt::Key_R);
	cfg->l3.def       = GetKeyName(Qt::Key_F);

	cfg->pressure_intensity_button.def = GetKeyName(Qt::NoButton);
	cfg->analog_limiter_button.def = GetKeyName(Qt::NoButton);

	cfg->lstick_anti_deadzone.def = 0;
	cfg->rstick_anti_deadzone.def = 0;
	cfg->lstickdeadzone.def       = 0;
	cfg->rstickdeadzone.def       = 0;
	cfg->ltriggerthreshold.def    = 0;
	cfg->rtriggerthreshold.def    = 0;
	cfg->lpadsquircling.def       = 8000;
	cfg->rpadsquircling.def       = 8000;

	// apply defaults
	cfg->from_default();
}

void keyboard_pad_handler::Key(const u32 code, bool pressed, u16 value)
{
	if (!pad::g_enabled)
	{
		return;
	}

	value = Clamp0To255(value);

	for (auto& pad : m_pads_internal)
	{
		const auto register_new_button_value = [&](std::map<u32, u16>& pressed_keys) -> u16
		{
			u16 actual_value = 0;

			// Make sure we keep this button pressed until all related keys are released.
			if (pressed)
			{
				pressed_keys[code] = value;
			}
			else
			{
				pressed_keys.erase(code);
			}

			// Get the max value of all pressed keys for this button
			for (const auto& [key, val] : pressed_keys)
			{
				actual_value = std::max(actual_value, val);
			}

			return actual_value;
		};

		// Find out if special buttons are pressed (introduced by RPCS3).
		// Activate the buttons here if possible since keys don't auto-repeat. This ensures that they are already pressed in the following loop.
		if (pad.m_pressure_intensity_button_index >= 0)
		{
			Button& pressure_intensity_button = pad.m_buttons[pad.m_pressure_intensity_button_index];

			if (pressure_intensity_button.m_key_codes.contains(code))
			{
				const u16 actual_value = register_new_button_value(pressure_intensity_button.m_pressed_keys);

				pressure_intensity_button.m_pressed = actual_value > 0;
				pressure_intensity_button.m_value = actual_value;
			}
		}

		const bool adjust_pressure = pad.get_pressure_intensity_button_active(m_pressure_intensity_toggle_mode, pad.m_player_id);
		const bool adjust_pressure_changed = pad.m_adjust_pressure_last != adjust_pressure;

		if (adjust_pressure_changed)
		{
			pad.m_adjust_pressure_last = adjust_pressure;
		}

		if (pad.m_analog_limiter_button_index >= 0)
		{
			Button& analog_limiter_button = pad.m_buttons[pad.m_analog_limiter_button_index];

			if (analog_limiter_button.m_key_codes.contains(code))
			{
				const u16 actual_value = register_new_button_value(analog_limiter_button.m_pressed_keys);

				analog_limiter_button.m_pressed = actual_value > 0;
				analog_limiter_button.m_value = actual_value;
			}
		}

		const bool analog_limiter_enabled = pad.get_analog_limiter_button_active(m_analog_limiter_toggle_mode, pad.m_player_id);
		const bool analog_limiter_changed = pad.m_analog_limiter_enabled_last != analog_limiter_enabled;
		const u32 l_stick_multiplier = analog_limiter_enabled ? m_l_stick_multiplier : 100;
		const u32 r_stick_multiplier = analog_limiter_enabled ? m_r_stick_multiplier : 100;

		if (analog_limiter_changed)
		{
			pad.m_analog_limiter_enabled_last = analog_limiter_enabled;
		}

		// Handle buttons
		for (usz i = 0; i < pad.m_buttons.size(); i++)
		{
			// Ignore special buttons
			if (static_cast<s32>(i) == pad.m_pressure_intensity_button_index ||
				static_cast<s32>(i) == pad.m_analog_limiter_button_index)
				continue;

			Button& button = pad.m_buttons[i];

			bool update_button = true;

			if (!button.m_key_codes.contains(code))
			{
				// Handle pressure changes anyway
				update_button = adjust_pressure_changed;
			}
			else
			{
				button.m_actual_value = register_new_button_value(button.m_pressed_keys);

				// to get the fastest response time possible we don't wanna use any lerp with factor 1
				if (button.m_analog)
				{
					update_button = m_analog_lerp_factor >= 1.0f;
				}
				else if (button.m_trigger)
				{
					update_button = m_trigger_lerp_factor >= 1.0f;
				}
			}

			if (!update_button)
				continue;

			if (button.m_actual_value > 0)
			{
				// Modify pressure if necessary if the button was pressed
				if (adjust_pressure)
				{
					button.m_value = pad.m_pressure_intensity;
				}
				else if (m_pressure_intensity_deadzone > 0)
				{
					button.m_value = NormalizeDirectedInput(button.m_actual_value, m_pressure_intensity_deadzone, 255);
				}
				else
				{
					button.m_value = button.m_actual_value;
				}

				button.m_pressed = button.m_value > 0;
			}
			else
			{
				button.m_value = 0;
				button.m_pressed = false;
			}
		}

		// Handle sticks
		for (usz i = 0; i < pad.m_sticks.size(); i++)
		{
			AnalogStick& stick = pad.m_sticks[i];

			const bool is_left_stick = i < 2;

			const bool is_max = stick.m_key_codes_max.contains(code);
			const bool is_min = stick.m_key_codes_min.contains(code);

			if (!is_max && !is_min)
			{
				if (!analog_limiter_changed)
					continue;

				// Update already pressed sticks
				const bool is_min_pressed = !stick.m_pressed_keys_min.empty();
				const bool is_max_pressed = !stick.m_pressed_keys_max.empty();

				const u32 stick_multiplier = is_left_stick ? l_stick_multiplier : r_stick_multiplier;

				const u16 actual_min_value = is_min_pressed ? MultipliedInput(255, stick_multiplier) : 255;
				const u16 normalized_min_value = std::ceil(actual_min_value / 2.0);

				const u16 actual_max_value = is_max_pressed ? MultipliedInput(255, stick_multiplier) : 255;
				const u16 normalized_max_value = std::ceil(actual_max_value / 2.0);

				m_stick_min[i] = is_min_pressed ? std::min<u8>(normalized_min_value, 128) : 0;
				m_stick_max[i] = is_max_pressed ? std::min<int>(128 + normalized_max_value, 255) : 128;
			}
			else
			{
				const u16 actual_value = pressed ? MultipliedInput(value, is_left_stick ? l_stick_multiplier : r_stick_multiplier) : value;
				u16 normalized_value = std::ceil(actual_value / 2.0);

				const auto register_new_stick_value = [&](std::map<u32, u16>& pressed_keys, bool is_max)
				{
					// Make sure we keep this stick pressed until all related keys are released.
					if (pressed)
					{
						pressed_keys[code] = normalized_value;
					}
					else
					{
						pressed_keys.erase(code);
					}

					// Get the min/max value of all pressed keys for this stick
					for (const auto& [key, val] : pressed_keys)
					{
						normalized_value = is_max ? std::max(normalized_value, val) : std::min(normalized_value, val);
					}
				};

				if (is_max)
				{
					register_new_stick_value(stick.m_pressed_keys_max, true);

					const bool is_max_pressed = !stick.m_pressed_keys_max.empty();

					m_stick_max[i] = is_max_pressed ? std::min<int>(128 + normalized_value, 255) : 128;
				}

				if (is_min)
				{
					register_new_stick_value(stick.m_pressed_keys_min, false);

					const bool is_min_pressed = !stick.m_pressed_keys_min.empty();

					m_stick_min[i] = is_min_pressed ? std::min<u8>(normalized_value, 128) : 0;
				}
			}

			m_stick_val[i] = m_stick_max[i] - m_stick_min[i];

			const f32 stick_lerp_factor = is_left_stick ? m_l_stick_lerp_factor : m_r_stick_lerp_factor;

			// to get the fastest response time possible we don't wanna use any lerp with factor 1
			if (stick_lerp_factor >= 1.0f)
			{
				stick.m_value = m_stick_val[i];
			}
		}
	}
}

void keyboard_pad_handler::release_all_keys()
{
	for (auto& pad : m_pads_internal)
	{
		for (Button& button : pad.m_buttons)
		{
			button.m_pressed = false;
			button.m_value = 0;
			button.m_actual_value = 0;
		}

		for (usz i = 0; i < pad.m_sticks.size(); i++)
		{
			m_stick_min[i] = 0;
			m_stick_max[i] = 128;
			m_stick_val[i] = 128;
			pad.m_sticks[i].m_value = 128;
		}
	}

	m_keys_released = true;
}

bool keyboard_pad_handler::eventFilter(QObject* target, QEvent* ev)
{
	if (!ev) [[unlikely]]
	{
		return false;
	}

	if (input::g_active_mouse_and_keyboard != input::active_mouse_and_keyboard::pad)
	{
		if (!m_keys_released)
		{
			release_all_keys();
		}
		return false;
	}

	m_keys_released = false;

	// !m_target is for future proofing when gsrender isn't automatically initialized on load.
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible()|| target == m_target)
	{
		switch (ev->type())
		{
		case QEvent::KeyPress:
			keyPressEvent(static_cast<QKeyEvent*>(ev));
			break;
		case QEvent::KeyRelease:
			keyReleaseEvent(static_cast<QKeyEvent*>(ev));
			break;
		case QEvent::MouseButtonPress:
			mousePressEvent(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseButtonRelease:
			mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseMove:
			mouseMoveEvent(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::Wheel:
			mouseWheelEvent(static_cast<QWheelEvent*>(ev));
			break;
		case QEvent::FocusOut:
			release_all_keys();
			break;
		default:
			break;
		}
	}
	return false;
}

/* Sets the target window for the event handler, and also installs an event filter on the target. */
void keyboard_pad_handler::SetTargetWindow(QWindow* target)
{
	if (target != nullptr)
	{
		m_target = target;
		target->installEventFilter(this);
	}
	else
	{
		QApplication::instance()->installEventFilter(this);
		// If this is hit, it probably means that some refactoring occurs because currently a gsframe is created in Load.
		// We still want events so filter from application instead since target is null.
		input_log.error("Trying to set pad handler to a null target window.");
	}
}

void keyboard_pad_handler::processKeyEvent(QKeyEvent* event, bool pressed)
{
	if (!event) [[unlikely]]
	{
		return;
	}

	if (event->isAutoRepeat())
	{
		event->ignore();
		return;
	}

	const auto handle_key = [this, pressed, event]()
	{
		QStringList list = GetKeyNames(event);
		if (list.isEmpty())
			return;

		const bool is_num_key = list.removeAll("Num") > 0;
		const QString name = QString::fromStdString(GetKeyName(event, true));

		// TODO: Edge case: switching numlock keeps numpad keys pressed due to now different modifier

		// Handle every possible key combination, for example: ctrl+A -> {ctrl, A, ctrl+A}
		for (const QString& keyname : list)
		{
			// skip the 'original keys' when handling numpad keys
			if (is_num_key && !keyname.contains("Num"))
				continue;
			// skip held modifiers when handling another key
			if (keyname != name && list.count() > 1 && (keyname == "Alt" || keyname == "AltGr" || keyname == "Ctrl" || keyname == "Meta" || keyname == "Shift"))
				continue;
			Key(GetKeyCode(keyname), pressed);
		}
	};

	handle_key();
	event->ignore();
}

void keyboard_pad_handler::keyPressEvent(QKeyEvent* event)
{
	if (!event) [[unlikely]]
	{
		return;
	}

	if (event->modifiers() & Qt::AltModifier)
	{
		switch (event->key())
		{
		case Qt::Key_I:
			m_deadzone_y = std::min(m_deadzone_y + 1, 255);
			input_log.success("mouse move adjustment: deadzone y = %d", m_deadzone_y);
			event->ignore();
			return;
		case Qt::Key_U:
			m_deadzone_y = std::max(0, m_deadzone_y - 1);
			input_log.success("mouse move adjustment: deadzone y = %d", m_deadzone_y);
			event->ignore();
			return;
		case Qt::Key_Y:
			m_deadzone_x = std::min(m_deadzone_x + 1, 255);
			input_log.success("mouse move adjustment: deadzone x = %d", m_deadzone_x);
			event->ignore();
			return;
		case Qt::Key_T:
			m_deadzone_x = std::max(0, m_deadzone_x - 1);
			input_log.success("mouse move adjustment: deadzone x = %d", m_deadzone_x);
			event->ignore();
			return;
		case Qt::Key_K:
			m_multi_y = std::min(m_multi_y + 0.1, 5.0);
			input_log.success("mouse move adjustment: multiplier y = %d", static_cast<int>(m_multi_y * 100));
			event->ignore();
			return;
		case Qt::Key_J:
			m_multi_y = std::max(0.0, m_multi_y - 0.1);
			input_log.success("mouse move adjustment: multiplier y = %d", static_cast<int>(m_multi_y * 100));
			event->ignore();
			return;
		case Qt::Key_H:
			m_multi_x = std::min(m_multi_x + 0.1, 5.0);
			input_log.success("mouse move adjustment: multiplier x = %d", static_cast<int>(m_multi_x * 100));
			event->ignore();
			return;
		case Qt::Key_G:
			m_multi_x = std::max(0.0, m_multi_x - 0.1);
			input_log.success("mouse move adjustment: multiplier x = %d", static_cast<int>(m_multi_x * 100));
			event->ignore();
			return;
		default:
			break;
		}
	}
	processKeyEvent(event, true);
}

void keyboard_pad_handler::keyReleaseEvent(QKeyEvent* event)
{
	processKeyEvent(event, false);
}

void keyboard_pad_handler::mousePressEvent(QMouseEvent* event)
{
	if (!event) [[unlikely]]
	{
		return;
	}

	Key(event->button(), true);
	event->ignore();
}

void keyboard_pad_handler::mouseReleaseEvent(QMouseEvent* event)
{
	if (!event) [[unlikely]]
	{
		return;
	}

	Key(event->button(), false, 0);
	event->ignore();
}

bool keyboard_pad_handler::get_mouse_lock_state() const
{
	if (gs_frame* game_frame = dynamic_cast<gs_frame*>(m_target))
		return game_frame->get_mouse_lock_state();
	return false;
}

void keyboard_pad_handler::mouseMoveEvent(QMouseEvent* event)
{
	if (!m_mouse_move_used || !event)
	{
		event->ignore();
		return;
	}

	static int movement_x = 0;
	static int movement_y = 0;

	if (m_target && m_target->isActive() && get_mouse_lock_state())
	{
		// get the screen dimensions
		const QSize screen = m_target->size();

		// get the center of the screen in global coordinates
		QPoint p_center = m_target->geometry().topLeft() + QPoint(screen.width() / 2, screen.height() / 2);

		// reset the mouse to the center for consistent results since edge movement won't be registered
		QCursor::setPos(m_target->screen(), p_center);

		// convert the center into screen coordinates
		p_center = m_target->mapFromGlobal(p_center);

		// get the delta of the mouse position to the screen center
		const QPoint p_delta = event->pos() - p_center;

		if (m_mouse_movement_mode == mouse_movement_mode::relative)
		{
			movement_x = p_delta.x();
			movement_y = p_delta.y();
		}
		else
		{
			// current mouse position, starting at the center
			static QPoint p_real(p_center);

			// update the current position without leaving the screen borders
			p_real.setX(std::clamp(p_real.x() + p_delta.x(), 0, screen.width()));
			p_real.setY(std::clamp(p_real.y() + p_delta.y(), 0, screen.height()));

			// get the delta of the real mouse position to the screen center
			const QPoint p_real_delta = p_real - p_center;

			movement_x = p_real_delta.x();
			movement_y = p_real_delta.y();
		}
	}
	else if (m_mouse_movement_mode == mouse_movement_mode::relative)
	{
		static int last_pos_x = 0;
		static int last_pos_y = 0;
		const QPoint e_pos = event->pos();

		movement_x = e_pos.x() - last_pos_x;
		movement_y = e_pos.y() - last_pos_y;

		last_pos_x = e_pos.x();
		last_pos_y = e_pos.y();
	}
	else if (m_target && m_target->isActive())
	{
		// get the screen dimensions
		const QSize screen = m_target->size();

		// get the center of the screen in global coordinates
		QPoint p_center = m_target->geometry().topLeft() + QPoint(screen.width() / 2, screen.height() / 2);

		// convert the center into screen coordinates
		p_center = m_target->mapFromGlobal(p_center);

		// get the delta of the mouse position to the screen center
		const QPoint p_delta = event->pos() - p_center;

		movement_x = p_delta.x();
		movement_y = p_delta.y();
	}

	movement_x *= m_multi_x;
	movement_y *= m_multi_y;

	int deadzone_x = 0;
	int deadzone_y = 0;

	if (movement_x == 0 && movement_y != 0)
	{
		deadzone_y = m_deadzone_y;
	}
	else if (movement_y == 0 && movement_x != 0)
	{
		deadzone_x = m_deadzone_x;
	}
	else if (movement_x != 0 && movement_y != 0 && m_deadzone_x != 0 && m_deadzone_y != 0)
	{
		// Calculate the point on our deadzone ellipsis intersected with the line (0, 0)(movement_x, movement_y)
		// Ellipsis: 1 = (x²/a²) + (y²/b²)     ; where: a = m_deadzone_x and b = m_deadzone_y
		// Line:     y = mx + t                ; where: t = 0 and m = (movement_y / movement_x)
		// Combined: x = +-(a*b)/sqrt(a²m²+b²) ; where +- is always +, since we only want the magnitude

		const double a = m_deadzone_x;
		const double b = m_deadzone_y;
		const double m = static_cast<double>(movement_y) / static_cast<double>(movement_x);

		deadzone_x = a * b / std::sqrt(std::pow(a, 2) * std::pow(m, 2) + std::pow(b, 2));
		deadzone_y = std::abs(m * deadzone_x);
	}

	if (movement_x < 0)
	{
		Key(mouse::move_right, false);
		Key(mouse::move_left, true, std::min(deadzone_x + std::abs(movement_x), 255));
		m_last_mouse_move_left = steady_clock::now();
	}
	else if (movement_x > 0)
	{
		Key(mouse::move_left, false);
		Key(mouse::move_right, true, std::min(deadzone_x + movement_x, 255));
		m_last_mouse_move_right = steady_clock::now();
	}

	// in Qt mouse up is equivalent to movement_y < 0
	if (movement_y < 0)
	{
		Key(mouse::move_down, false);
		Key(mouse::move_up, true, std::min(deadzone_y + std::abs(movement_y), 255));
		m_last_mouse_move_up = steady_clock::now();
	}
	else if (movement_y > 0)
	{
		Key(mouse::move_up, false);
		Key(mouse::move_down, true, std::min(deadzone_y + movement_y, 255));
		m_last_mouse_move_down = steady_clock::now();
	}

	event->ignore();
}

void keyboard_pad_handler::mouseWheelEvent(QWheelEvent* event)
{
	if (!m_mouse_wheel_used || !event)
	{
		return;
	}

	const QPoint direction = event->angleDelta();

	if (direction.isNull())
	{
		// Scrolling started/ended event, no direction given
		return;
	}

	if (const int x = direction.x())
	{
		const bool to_left = event->inverted() ? x < 0 : x > 0;

		if (to_left)
		{
			Key(mouse::wheel_left, true);
			m_last_wheel_move_left = steady_clock::now();
		}
		else
		{
			Key(mouse::wheel_right, true);
			m_last_wheel_move_right = steady_clock::now();
		}
	}
	if (const int y = direction.y())
	{
		const bool to_up = event->inverted() ? y < 0 : y > 0;

		if (to_up)
		{
			Key(mouse::wheel_up, true);
			m_last_wheel_move_up = steady_clock::now();
		}
		else
		{
			Key(mouse::wheel_down, true);
			m_last_wheel_move_down = steady_clock::now();
		}
	}
}

std::vector<pad_list_entry> keyboard_pad_handler::list_devices()
{
	std::vector<pad_list_entry> list_devices;
	list_devices.emplace_back(std::string(pad::keyboard_device_name), false);
	return list_devices;
}

std::string keyboard_pad_handler::GetMouseName(const QMouseEvent* event)
{
	return GetMouseName(event->button());
}

std::string keyboard_pad_handler::GetMouseName(u32 button)
{
	if (const auto it = mouse_list.find(button); it != mouse_list.cend())
		return it->second;
	return "FAIL";
}

QStringList keyboard_pad_handler::GetKeyNames(const QKeyEvent* keyEvent)
{
	QStringList list;

	if (keyEvent->modifiers() & Qt::ShiftModifier)
	{
		list.append("Shift");
		list.append(QKeySequence(keyEvent->key() | Qt::ShiftModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::AltModifier)
	{
		list.append("Alt");
		list.append(QKeySequence(keyEvent->key() | Qt::AltModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::ControlModifier)
	{
		list.append("Ctrl");
		list.append(QKeySequence(keyEvent->key() | Qt::ControlModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::MetaModifier)
	{
		list.append("Meta");
		list.append(QKeySequence(keyEvent->key() | Qt::MetaModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::KeypadModifier)
	{
		list.append("Num"); // helper object, not used as actual key
		list.append(QKeySequence(keyEvent->key() | Qt::KeypadModifier).toString(QKeySequence::NativeText));
	}

	// Handle special cases
	if (const std::string name = native_scan_code_to_string(keyEvent->nativeScanCode()); !name.empty())
	{
		list.append(QString::fromStdString(name));
	}

	switch (keyEvent->key())
	{
	case Qt::Key_Alt:
		list.append("Alt");
		break;
	case Qt::Key_AltGr:
		list.append("AltGr");
		break;
	case Qt::Key_Shift:
		list.append("Shift");
		break;
	case Qt::Key_Control:
		list.append("Ctrl");
		break;
	case Qt::Key_Meta:
		list.append("Meta");
		break;
	default:
		list.append(QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText));
		break;
	}

	list.removeDuplicates();
	return list;
}

std::string keyboard_pad_handler::GetKeyName(const QKeyEvent* keyEvent, bool with_modifiers)
{
	// Handle special cases first
	if (std::string name = native_scan_code_to_string(keyEvent->nativeScanCode()); !name.empty())
	{
		return name;
	}

	switch (keyEvent->key())
	{
	case Qt::Key_Alt: return "Alt";
	case Qt::Key_AltGr: return "AltGr";
	case Qt::Key_Shift: return "Shift";
	case Qt::Key_Control: return "Ctrl";
	case Qt::Key_Meta: return "Meta";
	case Qt::Key_NumLock: return QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText).toStdString();
#ifdef __APPLE__
	// On macOS, the arrow keys are considered to be part of the keypad;
	// since most Mac keyboards lack a keypad to begin with,
	// we change them to regular arrows to avoid confusion
	case Qt::Key_Left: return "←";
	case Qt::Key_Up: return "↑";
	case Qt::Key_Right: return "→";
	case Qt::Key_Down: return "↓";
#endif
	default:
		break;
	}

	if (with_modifiers)
	{
		return QKeySequence(keyEvent->key() | keyEvent->modifiers()).toString(QKeySequence::NativeText).toStdString();
	}

	return QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText).toStdString();
}

std::string keyboard_pad_handler::GetKeyName(const u32& keyCode)
{
	return QKeySequence(keyCode).toString(QKeySequence::NativeText).toStdString();
}

std::set<u32> keyboard_pad_handler::GetKeyCodes(const cfg::string& cfg_string)
{
	std::set<u32> key_codes;
	for (const std::string& key_name : cfg_pad::get_buttons(cfg_string))
	{
		if (u32 code = GetKeyCode(QString::fromStdString(key_name)); code != Qt::NoButton)
		{
			key_codes.insert(code);
		}
	}
	return key_codes;
}

u32 keyboard_pad_handler::GetKeyCode(const QString& keyName)
{
	if (keyName.isEmpty()) return Qt::NoButton;
	if (const int native_scan_code = native_scan_code_from_string(keyName.toStdString()); native_scan_code >= 0)
		return Qt::Key_unknown + native_scan_code; // Special cases that can't be expressed with Qt::Key
	if (keyName == "Alt") return Qt::Key_Alt;
	if (keyName == "AltGr") return Qt::Key_AltGr;
	if (keyName == "Shift") return Qt::Key_Shift;
	if (keyName == "Ctrl") return Qt::Key_Control;
	if (keyName == "Meta") return Qt::Key_Meta;
#ifdef __APPLE__
	// QKeySequence doesn't work properly for the arrow keys on macOS
	if (keyName == "Num←") return Qt::Key_Left;
	if (keyName == "Num↑") return Qt::Key_Up;
	if (keyName == "Num→") return Qt::Key_Right;
	if (keyName == "Num↓") return Qt::Key_Down;
#endif

	const QKeySequence seq(keyName);
	u32 key_code = Qt::NoButton;

	if (seq.count() == 1 && seq[0].key() != Qt::Key_unknown)
		key_code = seq[0].key();
	else
		input_log.notice("GetKeyCode(%s): seq.count() = %d", keyName, seq.count());

	return key_code;
}

int keyboard_pad_handler::native_scan_code_from_string([[maybe_unused]] const std::string& key)
{
	// NOTE: Qt throws a Ctrl key at us when using Alt Gr first, so right Alt does not work at the moment
	if (key == "Shift Left") return native_key::shift_l;
	if (key == "Shift Right") return native_key::shift_r;
	if (key == "Ctrl Left") return native_key::ctrl_l;
	if (key == "Ctrl Right") return native_key::ctrl_r;
	if (key == "Alt Left") return native_key::alt_l;
	if (key == "Alt Right") return native_key::alt_r;
	if (key == "Meta Left") return native_key::meta_l;
	if (key == "Meta Right") return native_key::meta_r;
#ifdef _WIN32
	if (key == "Num+0" || key == "Num+Ins") return 82;
	if (key == "Num+1" || key == "Num+End") return 79;
	if (key == "Num+2" || key == "Num+Down") return 80;
	if (key == "Num+3" || key == "Num+PgDown") return 81;
	if (key == "Num+4" || key == "Num+Left") return 75;
	if (key == "Num+5" || key == "Num+Clear") return 76;
	if (key == "Num+6" || key == "Num+Right") return 77;
	if (key == "Num+7" || key == "Num+Home") return 71;
	if (key == "Num+8" || key == "Num+Up") return 72;
	if (key == "Num+9" || key == "Num+PgUp") return 73;
	if (key == "Num+," || key == "Num+Del") return 83;
	if (key == "Num+/") return 309;
	if (key == "Num+*") return 55;
	if (key == "Num+-") return 74;
	if (key == "Num++") return 78;
	if (key == "Num+Enter") return 284;
#else
	// TODO
#endif
	return -1;
}

std::string keyboard_pad_handler::native_scan_code_to_string(int native_scan_code)
{
	// NOTE: the other Qt function "nativeVirtualKey" does not distinguish between VK_SHIFT and VK_RSHIFT key in Qt at the moment
	// NOTE: Qt throws a Ctrl key at us when using Alt Gr first, so right Alt does not work at the moment
	// NOTE: for MacOs: nativeScanCode may not work
	switch (native_scan_code)
	{
	case native_key::shift_l: return "Shift Left";
	case native_key::shift_r: return "Shift Right";
	case native_key::ctrl_l: return "Ctrl Left";
	case native_key::ctrl_r: return "Ctrl Right";
	case native_key::alt_l: return "Alt Left";
	case native_key::alt_r: return "Alt Right";
	case native_key::meta_l: return "Meta Left";
	case native_key::meta_r: return "Meta Right";
#ifdef _WIN32
	case 82: return "Num+0"; // Also "Num+Ins" depending on numlock
	case 79: return "Num+1"; // Also "Num+End" depending on numlock
	case 80: return "Num+2"; // Also "Num+Down" depending on numlock
	case 81: return "Num+3"; // Also "Num+PgDown" depending on numlock
	case 75: return "Num+4"; // Also "Num+Left" depending on numlock
	case 76: return "Num+5"; // Also "Num+Clear" depending on numlock
	case 77: return "Num+6"; // Also "Num+Right" depending on numlock
	case 71: return "Num+7"; // Also "Num+Home" depending on numlock
	case 72: return "Num+8"; // Also "Num+Up" depending on numlock
	case 73: return "Num+9"; // Also "Num+PgUp" depending on numlock
	case 83: return "Num+,"; // Also "Num+Del" depending on numlock
	case 309: return "Num+/";
	case 55: return "Num+*";
	case 74: return "Num+-";
	case 78: return "Num++";
	case 284: return "Num+Enter";
#else
	// TODO
#endif
	default: return "";
	}
}

bool keyboard_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad)
{
	if (!pad || pad->m_player_id >= g_cfg_input.player.size())
		return false;

	const cfg_player* player_config = g_cfg_input.player[pad->m_player_id];
	if (!player_config || player_config->device.to_string() != pad::keyboard_device_name)
		return false;

	m_pad_configs[pad->m_player_id].from_string(player_config->config.to_string());
	const cfg_pad* cfg = &m_pad_configs[pad->m_player_id];
	if (cfg == nullptr)
		return false;

	m_mouse_movement_mode = cfg->mouse_move_mode;
	m_mouse_move_used = false;
	m_mouse_wheel_used = false;
	m_deadzone_x = cfg->mouse_deadzone_x;
	m_deadzone_y = cfg->mouse_deadzone_y;
	m_multi_x = cfg->mouse_acceleration_x / 100.0;
	m_multi_y = cfg->mouse_acceleration_y / 100.0;
	m_l_stick_lerp_factor = cfg->l_stick_lerp_factor / 100.0f;
	m_r_stick_lerp_factor = cfg->r_stick_lerp_factor / 100.0f;
	m_analog_lerp_factor  = cfg->analog_lerp_factor / 100.0f;
	m_trigger_lerp_factor = cfg->trigger_lerp_factor / 100.0f;
	m_l_stick_multiplier = cfg->lstickmultiplier;
	m_r_stick_multiplier = cfg->rstickmultiplier;
	m_analog_limiter_toggle_mode = cfg->analog_limiter_toggle_mode.get();
	m_pressure_intensity_toggle_mode = cfg->pressure_intensity_toggle_mode.get();
	m_pressure_intensity_deadzone = cfg->pressure_intensity_deadzone.get();

	const auto find_keys = [this](const cfg::string& name)
	{
		std::set<u32> keys = FindKeyCodes<u32, u32>(mouse_list, name, false);
		for (const u32& key : GetKeyCodes(name)) keys.insert(key);

		if (!keys.empty())
		{
			if (!m_mouse_move_used && (keys.contains(mouse::move_left) || keys.contains(mouse::move_right) || keys.contains(mouse::move_up) || keys.contains(mouse::move_down)))
			{
				m_mouse_move_used = true;
			}
			else if (!m_mouse_wheel_used && (keys.contains(mouse::wheel_left) || keys.contains(mouse::wheel_right) || keys.contains(mouse::wheel_up) || keys.contains(mouse::wheel_down)))
			{
				m_mouse_wheel_used = true;
			}
		}
		return keys;
	};

	u32 pclass_profile = 0x0;

	for (const input::product_info& product : input::get_products_by_class(cfg->device_class_type))
	{
		if (product.vendor_id == cfg->vendor_id && product.product_id == cfg->product_id)
		{
			pclass_profile = product.pclass_profile;
		}
	}

	// Fixed assign change, default is both sensor and press off
	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD,
		cfg->device_class_type,
		pclass_profile,
		cfg->vendor_id,
		cfg->product_id,
		cfg->pressure_intensity
	);

	if (b_has_pressure_intensity_button)
	{
		pad->m_buttons.emplace_back(special_button_offset, find_keys(cfg->pressure_intensity_button), special_button_value::pressure_intensity);
		pad->m_pressure_intensity_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;
	}

	if (b_has_analog_limiter_button)
	{
		pad->m_buttons.emplace_back(special_button_offset, find_keys(cfg->analog_limiter_button), special_button_value::analog_limiter);
		pad->m_analog_limiter_button_index = static_cast<s32>(pad->m_buttons.size()) - 1;
	}

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_keys(cfg->ps),       CELL_PAD_CTRL_PS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_keys(cfg->l2),       CELL_PAD_CTRL_L2);

	if (pad->m_class_type == CELL_PAD_PCLASS_TYPE_SKATEBOARD)
	{
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->ir_nose), CELL_PAD_CTRL_PRESS_TRIANGLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->ir_tail), CELL_PAD_CTRL_PRESS_CIRCLE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->ir_left), CELL_PAD_CTRL_PRESS_CROSS);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->ir_right), CELL_PAD_CTRL_PRESS_SQUARE);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->tilt_left), CELL_PAD_CTRL_PRESS_L1);
		pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK, find_keys(cfg->tilt_right), CELL_PAD_CTRL_PRESS_R1);
	}

	pad->m_sticks[0] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  find_keys(cfg->ls_left), find_keys(cfg->ls_right));
	pad->m_sticks[1] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  find_keys(cfg->ls_up),   find_keys(cfg->ls_down));
	pad->m_sticks[2] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, find_keys(cfg->rs_left), find_keys(cfg->rs_right));
	pad->m_sticks[3] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, find_keys(cfg->rs_up),   find_keys(cfg->rs_down));

	pad->m_sensors[0] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_X, 0, 0, 0, DEFAULT_MOTION_X);
	pad->m_sensors[1] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Y, 0, 0, 0, DEFAULT_MOTION_Y);
	pad->m_sensors[2] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Z, 0, 0, 0, DEFAULT_MOTION_Z);
	pad->m_sensors[3] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_G, 0, 0, 0, DEFAULT_MOTION_G);

	pad->m_vibrateMotors[0] = VibrateMotor(true, 0);
	pad->m_vibrateMotors[1] = VibrateMotor(false, 0);

	m_bindings.emplace_back(pad, nullptr, nullptr);
	m_pads_internal.push_back(*pad);

	return true;
}

void keyboard_pad_handler::process()
{
	constexpr double stick_interval = 10.0;
	constexpr double button_interval = 10.0;

	const auto now = steady_clock::now();

	const double elapsed_stick = std::chrono::duration_cast<std::chrono::microseconds>(now - m_stick_time).count() / 1000.0;
	const double elapsed_button = std::chrono::duration_cast<std::chrono::microseconds>(now - m_button_time).count() / 1000.0;

	const bool update_sticks = elapsed_stick > stick_interval;
	const bool update_buttons = elapsed_button > button_interval;

	if (update_sticks)
	{
		m_stick_time = now;
	}

	if (update_buttons)
	{
		m_button_time = now;
	}

	if (m_mouse_move_used && m_mouse_movement_mode == mouse_movement_mode::relative)
	{
		constexpr double mouse_interval = 30.0;

		const double elapsed_left  = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_mouse_move_left).count() / 1000.0;
		const double elapsed_right = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_mouse_move_right).count() / 1000.0;
		const double elapsed_up    = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_mouse_move_up).count() / 1000.0;
		const double elapsed_down  = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_mouse_move_down).count() / 1000.0;

		// roughly 1-2 frames to process the next mouse move
		if (elapsed_left > mouse_interval)
		{
			Key(mouse::move_left, false);
			m_last_mouse_move_left = now;
		}
		if (elapsed_right > mouse_interval)
		{
			Key(mouse::move_right, false);
			m_last_mouse_move_right = now;
		}
		if (elapsed_up > mouse_interval)
		{
			Key(mouse::move_up, false);
			m_last_mouse_move_up = now;
		}
		if (elapsed_down > mouse_interval)
		{
			Key(mouse::move_down, false);
			m_last_mouse_move_down = now;
		}
	}

	const auto get_lerped = [](f32 v0, f32 v1, f32 lerp_factor)
	{
		// linear interpolation from the current value v0 to the desired value v1
		const f32 res = std::lerp(v0, v1, lerp_factor);

		// round to the correct direction to prevent sticky values on small factors
		return (v0 <= v1) ? std::ceil(res) : std::floor(res);
	};

	for (uint i = 0; i < m_pads_internal.size(); i++)
	{
		auto& pad = m_pads_internal[i];

		if (last_connection_status[i] == false)
		{
			ensure(m_bindings[i].pad);
			m_bindings[i].pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;
			m_bindings[i].pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[i] = true;
			connected_devices++;
		}
		else
		{
			if (update_sticks)
			{
				for (usz j = 0; j < pad.m_sticks.size(); j++)
				{
					const f32 stick_lerp_factor = (j < 2) ? m_l_stick_lerp_factor : m_r_stick_lerp_factor;

					// we already applied the following values on keypress if we used factor 1
					if (stick_lerp_factor < 1.0f)
					{
						const f32 v0 = static_cast<f32>(pad.m_sticks[j].m_value);
						const f32 v1 = static_cast<f32>(m_stick_val[j]);
						const f32 res = get_lerped(v0, v1, stick_lerp_factor);

						pad.m_sticks[j].m_value = static_cast<u16>(res);
					}
				}
			}

			if (update_buttons)
			{
				for (Button& button : pad.m_buttons)
				{
					if (button.m_analog)
					{
						// we already applied the following values on keypress if we used factor 1
						if (m_analog_lerp_factor < 1.0f)
						{
							const f32 v0 = static_cast<f32>(button.m_value);
							const f32 v1 = static_cast<f32>(button.m_actual_value);
							const f32 res = get_lerped(v0, v1, m_analog_lerp_factor);

							button.m_value = static_cast<u16>(res);
							button.m_pressed = button.m_value > 0;
						}
					}
					else if (button.m_trigger)
					{
						// we already applied the following values on keypress if we used factor 1
						if (m_trigger_lerp_factor < 1.0f)
						{
							const f32 v0 = static_cast<f32>(button.m_value);
							const f32 v1 = static_cast<f32>(button.m_actual_value);
							const f32 res = get_lerped(v0, v1, m_trigger_lerp_factor);

							button.m_value = static_cast<u16>(res);
							button.m_pressed = button.m_value > 0;
						}
					}
				}
			}
		}
	}

	if (m_mouse_wheel_used)
	{
		// Releases the wheel buttons 0,1 sec after they've been triggered
		// Next activation is set to distant future to avoid activating this on every proc
		const auto update_threshold = now - std::chrono::milliseconds(100);
		const auto distant_future = now + std::chrono::hours(24);

		if (update_threshold >= m_last_wheel_move_up)
		{
			Key(mouse::wheel_up, false);
			m_last_wheel_move_up = distant_future;
		}
		if (update_threshold >= m_last_wheel_move_down)
		{
			Key(mouse::wheel_down, false);
			m_last_wheel_move_down = distant_future;
		}
		if (update_threshold >= m_last_wheel_move_left)
		{
			Key(mouse::wheel_left, false);
			m_last_wheel_move_left = distant_future;
		}
		if (update_threshold >= m_last_wheel_move_right)
		{
			Key(mouse::wheel_right, false);
			m_last_wheel_move_right = distant_future;
		}
	}

	for (uint i = 0; i < m_bindings.size(); i++)
	{
		auto& pad = m_bindings[i].pad;
		ensure(pad);

		const cfg_pad* cfg = &m_pad_configs[pad->m_player_id];
		ensure(cfg);

		const Pad& pad_internal = m_pads_internal[i];

		// Normalize and apply pad squircling
		// Copy sticks first. We don't want to modify the raw internal values
		std::array<AnalogStick, 4> squircled_sticks = pad_internal.m_sticks;

		// Apply squircling
		if (cfg->lpadsquircling != 0)
		{
			u16& lx = squircled_sticks[0].m_value;
			u16& ly = squircled_sticks[1].m_value;

			ConvertToSquirclePoint(lx, ly, cfg->lpadsquircling);
		}

		if (cfg->rpadsquircling != 0)
		{
			u16& rx = squircled_sticks[2].m_value;
			u16& ry = squircled_sticks[3].m_value;

			ConvertToSquirclePoint(rx, ry, cfg->rpadsquircling);
		}

		pad->m_buttons = pad_internal.m_buttons;
		pad->m_sticks = squircled_sticks; // Don't use std::move here. We assign values lockless, so std::move can lead to segfaults.
	}
}
