#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

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
	using cfg::node::node;

	std::vector<cfg_pad_btn<T>*> buttons;
	std::map<u32, std::map<u32, std::set<const cfg_pad_btn<T>*>>> button_map;

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

	pad_button get_pad_button(T id)
	{
		if (cfg_pad_btn<T>* item = get_button(id))
		{
			return item->get();
		}

		return pad_button::pad_button_max_enum;
	}

	pad_button default_pad_button(T id)
	{
		if (cfg_pad_btn<T>* item = get_button(id))
		{
			return item->get_default();
		}

		return pad_button::pad_button_max_enum;
	}

	void set_button(T id, pad_button btn_id)
	{
		if (cfg_pad_btn<T>* item = get_button(id))
		{
			item->set(btn_id);
		}
	}

	void init_button(cfg_pad_btn<T>* pbtn)
	{
		if (!pbtn) return;
		const u32 offset = pad_button_offset(pbtn->get());
		const u32 keycode = pad_button_keycode(pbtn->get());
		button_map[offset][keycode].insert(std::as_const(pbtn));
		buttons.push_back(pbtn);
	}

	void init_buttons()
	{
		for (const auto& n : get_nodes())
		{
			init_button(static_cast<cfg_pad_btn<T>*>(n));
		}
	}

	void handle_input(const std::function<void(T, u16, bool)>& func, u32 offset, u32 keycode, u16 value, bool pressed, bool check_axis) const
	{
		const auto& btns = find_button(offset, keycode);
		if (btns.empty())
		{
			if (check_axis)
			{
				switch (offset)
				{
				case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:
				case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
				case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
				case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
					handle_input(func, offset, static_cast<u32>(axis_direction::both), value, pressed, false);
					break;
				default:
					break;
				}
			}
			return;
		}

		for (const auto& btn : btns)
		{
			if (btn && func)
			{
				func(btn->btn_id(), value, pressed);
			}
		}
	}

	void handle_input(std::shared_ptr<Pad> pad, bool press_only, const std::function<void(T, u16, bool)>& func) const
	{
		if (!pad)
			return;

		for (const Button& button : pad->m_buttons)
		{
			if (button.m_pressed || !press_only)
			{
				handle_input(func, button.m_offset, button.m_outKeyCode, button.m_value, button.m_pressed, true);
			}
		}

		for (const AnalogStick& stick : pad->m_sticks)
		{
			handle_input(func, stick.m_offset, get_axis_keycode(stick.m_offset, stick.m_value), stick.m_value, true, true);
		}
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

	std::string cfg_id;
	std::array<std::shared_ptr<T>, Count> players;

	bool load()
	{
		bool result = false;
		const std::string cfg_name = fmt::format("%sconfig/%s.yml", fs::get_config_dir(), cfg_id);
		cfg_log.notice("Loading %s config: %s", cfg_id, cfg_name);
	
		from_default();
	
		for (std::shared_ptr<T>& player : players)
		{
			player->buttons.clear();
		}
	
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			if (std::string content = cfg_file.to_string(); !content.empty())
			{
				result = from_string(content);
			}
		}
		else
		{
			save();
		}
	
		for (std::shared_ptr<T>& player : players)
		{
			player->init_buttons();
		}
	
		return result;
	}

	void save() const
	{
		const std::string cfg_name = fmt::format("%sconfig/%s.yml", fs::get_config_dir(), cfg_id);
		cfg_log.notice("Saving %s config to '%s'", cfg_id, cfg_name);
	
		if (!fs::create_path(fs::get_parent_dir(cfg_name)))
		{
			cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
		}
	
		fs::pending_file cfg_file(cfg_name);
	
		if (!cfg_file.file || (cfg_file.file.write(to_string()), !cfg_file.commit()))
		{
			cfg_log.error("Failed to save %s config to '%s' (error=%s)", cfg_id, cfg_name, fs::g_tls_error);
		}
	}
};
