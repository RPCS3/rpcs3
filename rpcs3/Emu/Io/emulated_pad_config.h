#pragma once

#include "Utilities/Config.h"
#include "pad_types.h"

#include <array>
#include <map>
#include <optional>

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

	std::map<u32, std::map<u32, T>> buttons;

	std::optional<T> find_button(u32 offset, u32 keycode) const
	{
		if (const auto it = buttons.find(offset); it != buttons.cend())
		{
			if (const auto it2 = it->second.find(keycode); it2 != it->second.cend())
			{
				return it2->second;
			}
		}
	
		return std::nullopt;
	}

	void set_button(const cfg_pad_btn<T>& pbtn, T bbtn)
	{
		const u32 offset = pad_button_offset(pbtn);
		const u32 keycode = pad_button_keycode(pbtn);
		buttons[(offset >> 8) & 0xFF][keycode & 0xFF] = bbtn;
	}

	void set_buttons()
	{
		for (const auto& n : get_nodes())
		{
			const auto& node = static_cast<cfg_pad_btn<T>*>(n);
			set_button(*node, node->btn_id());
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
			player->set_buttons();
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
