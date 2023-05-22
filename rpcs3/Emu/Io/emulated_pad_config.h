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
};

template <typename T>
struct emulated_pads_config : cfg::node
{
	emulated_pads_config(std::string id) : cfg_id(std::move(id)) {}

	std::string cfg_id;

	T player1{ this, "Player 1" };
	T player2{ this, "Player 2" };
	T player3{ this, "Player 3" };
	T player4{ this, "Player 4" };
	T player5{ this, "Player 5" };
	T player6{ this, "Player 6" };
	T player7{ this, "Player 7" };

	std::array<T*, 7> players{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load()
	{
		bool result = false;
		const std::string cfg_name = fmt::format("%sconfig/%s.yml", fs::get_config_dir(), cfg_id);
		cfg_log.notice("Loading %s config: %s", cfg_id, cfg_name);
	
		from_default();
	
		for (T* player : players)
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
	
		for (T* player : players)
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
