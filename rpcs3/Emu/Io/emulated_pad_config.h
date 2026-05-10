#pragma once

#include "Utilities/Config.h"
#include "Utilities/mutex.h"
#include "pad_types.h"
#include "MouseHandler.h"

#include <array>
#include <map>
#include <set>
#include <vector>

LOG_CHANNEL(cfg_log, "CFG");

template <typename T>
class cfg_pad_btn : public cfg::_enum<pad_button>
{
public:
	cfg_pad_btn(cfg::node* owner, const std::string& name, T id, pad_button value, bool dynamic = false)
		: cfg::_enum<pad_button>(owner, name, value, dynamic)
		, m_btn_id(id)
	{};

	T btn_id() const
	{
		return m_btn_id;
	}

private:
	T m_btn_id;
};

template <typename T>
struct emulated_pad_config : cfg::node
{
public:
	using cfg::node::node;

	pad_button get_pad_button(T id)
	{
		std::lock_guard lock(m_mutex);

		if (const cfg_pad_btn<T>* item = get_button(id))
		{
			return item->get();
		}

		return pad_button::pad_button_max_enum;
	}

	pad_button default_pad_button(T id)
	{
		std::lock_guard lock(m_mutex);

		if (const cfg_pad_btn<T>* item = get_button(id))
		{
			return item->get_default();
		}

		return pad_button::pad_button_max_enum;
	}

	void set_button(T id, pad_button btn_id)
	{
		std::lock_guard lock(m_mutex);

		if (cfg_pad_btn<T>* item = get_button(id))
		{
			item->set(btn_id);
		}
	}

	void init_buttons()
	{
		std::lock_guard lock(m_mutex);

		for (const auto& n : get_nodes())
		{
			init_button(static_cast<cfg_pad_btn<T>*>(n));
		}
	}

	void clear_buttons()
	{
		std::lock_guard lock(m_mutex);

		buttons.clear();
		button_map.clear();
	}

	void handle_input(std::shared_ptr<Pad> pad, bool press_only, const std::function<void(T, pad_button, u16, bool, bool&)>& func) const
	{
		if (!pad || pad->is_copilot())
			return;

		for (const Button& button : pad->m_buttons_external)
		{
			if (button.m_pressed || !press_only)
			{
				if (handle_input(func, button.m_offset, button.m_outKeyCode, button.m_value, button.m_pressed, true))
				{
					return;
				}
			}
		}

		for (const AnalogStick& stick : pad->m_sticks_external)
		{
			if (handle_input(func, stick.m_offset, get_axis_keycode(stick.m_offset, stick.m_value), stick.m_value, true, true))
			{
				return;
			}
		}
	}

	void handle_input(const Mouse& mouse, const std::function<void(T, pad_button, u16, bool, bool&)>& func) const
	{
		for (int i = 0; i < 8; i++)
		{
			const MouseButtonCodes cell_code = get_mouse_button_code(i);
			if ((mouse.buttons & cell_code))
			{
				const pad_button button = static_cast<pad_button>(static_cast<int>(pad_button::mouse_button_1) + i);
				const u32 offset = pad_button_offset(button);
				const u32 keycode = pad_button_keycode(button);

				if (handle_input(func, offset, keycode, 255, true, true))
				{
					return;
				}
			}
		}
	}

protected:
	mutable shared_mutex m_mutex;
	std::vector<cfg_pad_btn<T>*> buttons;
	std::map<u32, std::map<u32, std::set<const cfg_pad_btn<T>*>>> button_map;

	cfg_pad_btn<T>* get_button(T id)
	{
		for (cfg_pad_btn<T>* item : buttons)
		{
			if (item && item->btn_id() == id)
			{
				return item;
			}
		}

		return nullptr;
	}

	void init_button(cfg_pad_btn<T>* pbtn)
	{
		if (!pbtn) return;
		const u32 offset = pad_button_offset(pbtn->get());
		const u32 keycode = pad_button_keycode(pbtn->get());
		button_map[offset][keycode].insert(std::as_const(pbtn));
		buttons.push_back(pbtn);
	}

	const std::set<const cfg_pad_btn<T>*>& find_button(u32 offset, u32 keycode) const
	{
		if (const auto it = button_map.find(offset); it != button_map.cend())
		{
			if (const auto it2 = it->second.find(keycode); it2 != it->second.cend() && !it2->second.empty())
			{
				return it2->second;
			}
		}

		static const std::set<const cfg_pad_btn<T>*> empty_set;
		return empty_set;
	}

	bool handle_input(const std::function<void(T, pad_button, u16, bool, bool&)>& func, u32 offset, u32 keycode, u16 value, bool pressed, bool check_axis) const
	{
		m_mutex.lock();

		bool abort = false;

		const auto& btns = find_button(offset, keycode);
		if (btns.empty())
		{
			m_mutex.unlock();

			if (check_axis)
			{
				switch (offset)
				{
				case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:
				case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
				case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
				case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
					abort = handle_input(func, offset, static_cast<u32>(axis_direction::both), value, pressed, false);
					break;
				default:
					break;
				}
			}
			return abort;
		}

		for (const auto& btn : btns)
		{
			if (btn && func)
			{
				func(btn->btn_id(), btn->get(), value, pressed, abort);
				if (abort) break;
			}
		}

		m_mutex.unlock();
		return abort;
	}
};

template <typename T, u32 Count>
struct emulated_pads_config : cfg::node
{
	emulated_pads_config(std::string id) : cfg_id(std::move(id))
	{
		for (u32 i = 0; i < Count; i++)
		{
			players.at(i) = std::make_shared<T>(this, fmt::format("Player %d", i + 1));
		}
	}

	shared_mutex m_mutex;
	std::string cfg_id;
	std::array<std::shared_ptr<T>, Count> players;

	bool load()
	{
		m_mutex.lock();

		bool result = false;
		const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), cfg_id);
		cfg_log.notice("Loading %s config: %s", cfg_id, cfg_name);

		from_default();

		for (std::shared_ptr<T>& player : players)
		{
			player->clear_buttons();
		}

		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			if (const std::string content = cfg_file.to_string(); !content.empty())
			{
				result = from_string(content);
			}

			for (std::shared_ptr<T>& player : players)
			{
				player->init_buttons();
			}

			m_mutex.unlock();
		}
		else
		{
			m_mutex.unlock();
			save();
		}

		return result;
	}

	void save()
	{
		std::lock_guard lock(m_mutex);

		const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), cfg_id);
		cfg_log.notice("Saving %s config to '%s'", cfg_id, cfg_name);

		if (!fs::create_path(fs::get_parent_dir(cfg_name)))
		{
			cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
		}

		if (!cfg::node::save(cfg_name))
		{
			cfg_log.error("Failed to save %s config to '%s' (error=%s)", cfg_id, cfg_name, fs::g_tls_error);
		}

		for (std::shared_ptr<T>& player : players)
		{
			player->clear_buttons();
			player->init_buttons();
		}
	}
};
