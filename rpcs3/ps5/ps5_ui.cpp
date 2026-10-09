// PS5: what the launcher and the pause menu share (ps5_ui.h).

#include "stdafx.h"
#include "ps5_ui.h"

#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Utilities/Config.h"
#include "Utilities/File.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <map>

LOG_CHANNEL(launcher_log, "Launcher");

extern std::string g_cfg_defaults; // Emu/System.cpp: the config's defaults, as text

namespace rsx::overlays::ps5ui
{
	void style_label(label& target, std::string_view text, u16 font_size, std::string_view font_name, const color4f& color)
	{
		target.set_text(text);
		target.set_font(font_size, font_name);
		target.fore_color = color;
		target.back_color.a = 0.f;
		target.set_padding(0);
		target.auto_resize();
	}

	std::unique_ptr<label> make_label(std::string_view text, u16 font_size, std::string_view font_name, const color4f& color)
	{
		auto result = std::make_unique<label>();
		style_label(*result, text, font_size, font_name, color);
		return result;
	}

	ink measure_ink(font* renderer, std::u32string_view text)
	{
		const std::u32string copy(text);
		const std::vector<vertex> verts = renderer->render_text(copy.c_str());
		ink result{};
		bool first = true;
		for (const vertex& v : verts)
		{
			const f32 x = v.values[0];
			const f32 y = v.values[1];
			result.left = first ? x : std::min(result.left, x);
			result.right = first ? x : std::max(result.right, x);
			result.top = first ? y : std::min(result.top, y);
			result.bottom = first ? y : std::max(result.bottom, y);
			first = false;
		}
		return result;
	}

	// A label's top for its capitals to sit centred on mid_y: the font's 'H'
	// decides, so labels in one line share a baseline whatever their letters
	s16 cap_centred_y(const label& target, f32 mid_y)
	{
		font* renderer = target.get_font();
		const ink cap = measure_ink(renderer, U"H");
		// The label draws its baseline at its top plus the font's pixel size
		return static_cast<s16>(std::lround(mid_y - renderer->get_size_px() - (cap.top + cap.bottom) / 2.f));
	}

	void place(label& target, s16 x, f32 mid_y)
	{
		target.set_pos(x, cap_centred_y(target, mid_y));
	}

	// Shortened with an ellipsis to fit max_w
	void fit_text(label& target, std::string_view text, u16 max_w)
	{
		target.set_text(text);
		target.auto_resize();
		if (target.w <= max_w)
		{
			return;
		}

		std::u32string chars = utf8_to_u32string(text);
		while (!chars.empty())
		{
			chars.pop_back();
			while (!chars.empty() && chars.back() == U' ')
			{
				chars.pop_back();
			}
			target.set_unicode_text(chars + U"…");
			target.auto_resize();
			if (target.w <= max_w)
			{
				return;
			}
		}
	}

	// "WELCOME BACK" with the design's wide letter spacing
	std::string spaced(std::string_view text)
	{
		std::string result;
		for (const char c : text)
		{
			if (!result.empty())
			{
				result += c == ' ' ? "  " : " ";
			}
			if (c != ' ')
			{
				result += c;
			}
		}
		return result;
	}

	std::unique_ptr<image_info> load_image(const std::string& path)
	{
		if (path.empty() || !fs::is_file(path))
		{
			return nullptr;
		}

		auto image = std::make_unique<image_info>(path);
		if (!image->get_data())
		{
			return nullptr;
		}

		// The renderer's texture cache is keyed by address, which a freed image
		// can hand on to the next: upload this one afresh
		image->dirty = true;
		return image;
	}

	std::unique_ptr<memory_image_info> make_glow(std::vector<u8>& pixels)
	{
		constexpr u16 n = 128;
		pixels.resize(usz{n} * n * 4);
		for (u16 yy = 0; yy < n; yy++)
		{
			for (u16 xx = 0; xx < n; xx++)
			{
				const f32 dx = (xx + 0.5f) / n * 2.f - 1.f;
				const f32 dy = (yy + 0.5f) / n * 2.f - 1.f;
				const f32 r = std::min(1.f, std::sqrt(dx * dx + dy * dy));
				const f32 a = (1.f - r) * (1.f - r) * (1.f - r);
				u8* px = &pixels[(usz{yy} * n + xx) * 4];
				px[0] = px[1] = px[2] = 255;
				px[3] = static_cast<u8>(std::lround(255.f * a));
			}
		}
		auto image = std::make_unique<memory_image_info>(n, n, u8{4}, pixels.data());
		image->dirty = true;
		return image;
	}

	// The opening's curves: how far a step of it is, at `now` seconds, for a
	// step from `start` lasting `length`
	f32 progress(f32 now, f32 start, f32 length)
	{
		return std::clamp((now - start) / length, 0.f, 1.f);
	}

	f32 ease_out(f32 t)
	{
		return 1.f - (1.f - t) * (1.f - t) * (1.f - t);
	}

	f32 ease_in_out(f32 t)
	{
		return t < 0.5f ? 4.f * t * t * t : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
	}

	// Part of the screen drawn faded and moved, as the opening has it
	void add_animated(compiled_resource& out, const compiled_resource& part, f32 alpha, f32 dx, f32 dy)
	{
		if (alpha <= 0.f)
		{
			return;
		}
		if (alpha >= 1.f && dx == 0.f && dy == 0.f)
		{
			out.add(part);
			return;
		}

		compiled_resource faded = part;
		for (auto& cmd : faded.draw_commands)
		{
			cmd.config.color.a *= alpha;
			cmd.config.sdf_config.border_color.a *= alpha;
		}
		out.add(faded, dx, dy);
	}

	namespace
	{
		const settings_entry c_settings_table[]
		{
#include "ps5_settings_table.inc"
		};
	}

	std::span<const settings_entry> settings_table()
	{
		return c_settings_table;
	}

	// An entry's section in the config ("Video/Performance Overlay") and name
	std::pair<std::string, std::string> setting_path(emu_settings_type type)
	{
		const cfg_location& location = ::at32(settings_location, type);
		std::string section;
		for (usz i = 0; i + 1 < location.size(); i++)
		{
			section += (i ? "/" : "") + location[i];
		}
		return {section, location.back()};
	}

	// A setting's values to choose from. RPCS3's on/off settings list none
	// (cfg::_bool has no to_list), and with only the current value to
	// cycle through, CROSS changed nothing; numbers list only their ends,
	// so they step between them in round numbers, some 40 stops at most
	std::vector<std::string> setting_options(const cfg::_base& setting, std::string_view key)
	{
		// Fewer than the setting has: what the console cannot run, and
		// ranges better in a few steps
		static const std::map<std::string_view, std::vector<std::string>> offered
		{
			// The shader interpreter froze the console
			{"Shader Mode", {"Async Recompiler (multi-threaded)", "Legacy Recompiler (single-threaded)"}},
			{"Anisotropic Filter Override", {"0", "2", "4", "8", "16"}},
			{"Resolution Scale", {"50", "75", "100", "125", "150", "200", "250", "300"}},
			// Adjusted 20 to 40 µs at a time, as RPCS3's tooltip says
			{"Driver Wake-Up Delay", {"0", "20", "40", "60", "80", "100", "150", "200", "300", "400", "500", "750", "1000", "1500", "2000", "3000", "5000"}},
			{"Vblank Rate", {"30", "50", "60", "75", "90", "100", "120", "144", "165", "180", "240", "300", "360", "480", "600"}},
			{"Late Audio Wait", {"0", "10", "20", "30", "40", "50", "60", "80", "100"}},
			{"Menu Sound Volume", {"0", "10", "25", "40", "55", "70", "85", "100"}},
			{"Frame Generation Frames", {"0", "1", "2", "3"}},
			{"Frame Generation Quality", {"0", "1", "2"}},
			{"Frame Generation Step Aside", {"0", "1", "2"}},
			{"Max LLVM Compile Threads", {"0", "1", "2", "3", "4", "6", "8", "10", "12", "16"}},
		};
		if (const auto it = offered.find(key); it != offered.end())
		{
			return it->second;
		}

		if (setting.get_type() == cfg::type::_bool)
		{
			return {"false", "true"};
		}

		std::vector<std::string> list = setting.to_list();
		const cfg::type type = setting.get_type();
		if ((type != cfg::type::_int && type != cfg::type::uint) || list.size() != 2)
		{
			return list;
		}

		// Integers, or reals (cfg::_float is typed _int, and lists its ends
		// with decimals), written as the setting writes them
		const bool real = list[0].find('.') != umax;
		const auto text = [real](long double number) -> std::string
		{
			if (!real)
			{
				// Whole multiples of a whole step: exact as they are
				return number < 0 ? std::to_string(static_cast<s64>(number)) : std::to_string(static_cast<u64>(number));
			}
			std::array<char, 32> buffer{};
			const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), static_cast<f64>(number), std::chars_format::fixed);
			return error == std::errc() ? std::string(buffer.data(), end) : std::string("0");
		};
		const long double low = std::stold(list[0]);
		const long double high = std::stold(list[1]);
		if (real)
		{
			list = {text(low), text(high)};
		}

		static constexpr long double steps[] = {0.01L, 0.05L, 0.1L, 0.25L, 0.5L, 1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 2500, 5000, 10000,
			25000, 50000, 100000, 250000, 500000, 1e6L, 1e7L, 1e8L, 1e9L, 1e10L, 1e12L, 1e15L, 1e18L};
		long double step = steps[std::size(steps) - 1];
		for (const long double candidate : steps)
		{
			if ((real || candidate >= 1) && (high - low) / candidate <= 40)
			{
				step = candidate;
				break;
			}
		}

		std::vector<std::pair<long double, std::string>> values{{low, list[0]}, {high, list[1]}};
		for (long double number = std::ceil(low / step) * step; number < high; number += step)
		{
			if (number > low)
			{
				values.emplace_back(number, text(number));
			}
		}
		for (const std::string& extra : {setting.def_to_string(), setting.to_string()})
		{
			values.emplace_back(std::stold(extra), extra);
		}
		std::sort(values.begin(), values.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

		std::vector<std::string> result;
		long double last = 0;
		for (const auto& [number, written] : values)
		{
			if (!result.empty() && number == last)
			{
				// The same number written two ways: the setting's own way
				if (written == setting.to_string() || written == setting.def_to_string())
				{
					result.back() = written;
				}
				continue;
			}
			result.push_back(written);
			last = number;
		}
		return result;
	}

	// A setting by its section ("Video", or "Video/Performance Overlay") and name
	cfg::_base* find_setting(cfg::node& root, std::string_view section, std::string_view key)
	{
		cfg::node* node = &root;
		while (!section.empty())
		{
			const usz slash = section.find('/');
			const std::string_view name = section.substr(0, slash);
			section = slash == umax ? std::string_view{} : section.substr(slash + 1);
			cfg::node* next = nullptr;
			for (cfg::_base* child : node->get_nodes())
			{
				if (child->get_type() == cfg::type::node && child->get_name() == name)
				{
					next = static_cast<cfg::node*>(child);
					break;
				}
			}
			if (!next)
			{
				return nullptr;
			}
			node = next;
		}
		for (cfg::_base* setting : node->get_nodes())
		{
			if (setting->get_name() == key)
			{
				return setting;
			}
		}
		return nullptr;
	}

	std::string setting_text(const std::string& key, const std::string& value)
	{
		if (value == "true") return "On";
		if (value == "false") return "Off";
		if (value == "0" && (key == "Preferred SPU Threads" || key == "Anisotropic Filter Override" || key == "Max LLVM Compile Threads" ||
			key == "Shader Compiler Threads")) return "Auto";
		if (key == "Resolution Scale" || key == "Master Volume" || key == "Clocks scale" || key == "Time Stretching Threshold") return value + "%";
		if (key == "Anisotropic Filter Override") return value + "x";
		if (key == "Desired Audio Buffer Duration" || key == "Metrics update interval (ms)") return value + " ms";
		if (key == "Driver Wake-Up Delay") return value + " µs";
		if (key == "Vblank Rate") return value + " Hz";
		if (key == "Late Audio Wait") return value == "0" ? std::string("Off") : value + " ms";
		if (key == "Menu Sound Volume") return value == "0" ? std::string("Off") : value + "%";
		if (key == "Frame Generation Frames") return value == "0" ? std::string("Auto") : value;
		if (key == "Frame Generation Quality") return value == "0" ? std::string("Fast") : value == "2" ? std::string("Quality") : std::string("Balanced");
		if (key == "Frame Generation Step Aside") return value == "0" ? std::string("Never") : value == "2" ? std::string("Most moves") : std::string("Quick moves");
		if (key == "Opacity (%)") return value + "%";
		return value;
	}

	// The global config, as a game's boot starts from: the defaults, then config.yml
	std::unique_ptr<cfg_root> load_global_config()
	{
		auto root = std::make_unique<cfg_root>();
		root->from_string(g_cfg_defaults);
		if (fs::file file{fs::get_config_dir(true) + "config.yml"})
		{
			root->from_string(file.to_string());
		}
		return root;
	}

	std::string yaml_quoted(std::string_view text)
	{
		std::string result = "\"";
		for (const char c : text)
		{
			if (c == '"' || c == '\\') result += '\\';
			result += c;
		}
		return result + "\"";
	}

	// The entries of `config` that differ from `base`, as YAML: what a custom
	// config needs to hold, and nothing the global config already says
	bool write_differences(std::string& out, const cfg::node& config, const cfg::node& base, int depth)
	{
		bool any = false;
		const auto& nodes = config.get_nodes();
		const auto& base_nodes = base.get_nodes();
		for (usz i = 0; i < nodes.size() && i < base_nodes.size(); i++)
		{
			const cfg::_base* entry = nodes[i];
			const cfg::_base* base_entry = base_nodes[i];
			const std::string indent(depth * 2, ' ');

			switch (entry->get_type())
			{
			case cfg::type::node:
			{
				std::string inner;
				if (write_differences(inner, *static_cast<const cfg::node*>(entry), *static_cast<const cfg::node*>(base_entry), depth + 1))
				{
					out += indent + yaml_quoted(entry->get_name()) + ":\n" + inner;
					any = true;
				}
				break;
			}
			case cfg::type::_bool:
			case cfg::type::_enum:
			case cfg::type::_int:
			case cfg::type::uint:
			case cfg::type::string:
			{
				if (const std::string value = entry->to_string(); value != base_entry->to_string())
				{
					out += indent + yaml_quoted(entry->get_name()) + ": " + yaml_quoted(value) + "\n";
					any = true;
				}
				break;
			}
			default:
				break;
			}
		}
		return any;
	}

	// A setting's default as this title has it: the shader interpreter, the
	// emulator's own default, froze the console and is not offered
	std::string usable_default(const cfg::_base& setting, const std::vector<std::string>& options)
	{
		const std::string value = setting.def_to_string();
		if (std::find(options.begin(), options.end(), value) != options.end())
		{
			return value;
		}
		return options.empty() ? value : options.front();
	}

	std::vector<setting_row> read_game_settings(const std::string& serial, bool headings)
	{
		// What the game gets today: the global config, then its custom config
		const auto global = load_global_config();
		const auto game = load_global_config();
		if (fs::file file{rpcs3::utils::get_custom_config_path(serial)})
		{
			game->from_string(file.to_string());
		}

		std::vector<setting_row> rows;
		std::string_view tab;
		for (const settings_entry& entry : settings_table())
		{
			if (headings && entry.tab != tab)
			{
				tab = entry.tab;
				setting_row heading;
				heading.label = entry.tab;
				heading.heading = true;
				rows.push_back(std::move(heading));
			}

			const auto [section, key] = setting_path(entry.type);
			cfg::_base* global_setting = find_setting(*global, section, key);
			cfg::_base* game_setting = find_setting(*game, section, key);
			if (!global_setting || !game_setting)
			{
				launcher_log.error("Game settings: no setting %s/%s", section, key);
				continue;
			}

			setting_row row;
			row.section = section;
			row.key = key;
			row.label = *entry.label ? entry.label : key;
			row.help = entry.help;
			row.global = global_setting->to_string();
			row.value = game_setting->to_string();
			row.options = setting_options(*global_setting, key);
			for (const std::string& value : {row.global, row.value})
			{
				if (std::find(row.options.begin(), row.options.end(), value) == row.options.end())
				{
					row.options.insert(row.options.begin(), value);
				}
			}
			rows.push_back(std::move(row));
		}
		return rows;
	}

	void write_game_settings(const std::string& serial, const std::vector<setting_row>& rows)
	{
		const auto global = load_global_config();
		const auto game = load_global_config();
		const std::string path = rpcs3::utils::get_custom_config_path(serial);
		if (fs::file file{path})
		{
			game->from_string(file.to_string());
		}
		for (const setting_row& row : rows)
		{
			if (!row.heading)
			{
				if (cfg::_base* setting = find_setting(*game, row.section, row.key))
				{
					setting->from_string(row.value);
				}
			}
		}

		std::string yaml;
		if (write_differences(yaml, *game, *global, 0))
		{
			fs::create_path(rpcs3::utils::get_custom_config_dir());
			fs::pending_file temp(path);
			if (temp.file)
			{
				temp.file.write(yaml);
			}
			if (!temp.file || !temp.commit())
			{
				launcher_log.error("Game settings: could not write %s (%s)", path, fs::g_tls_error);
			}
			else
			{
				launcher_log.notice("Game settings: saved %s", path);
			}
		}
		else if (fs::is_file(path))
		{
			// Nothing of its own left: the game follows the global config
			fs::remove_file(path);
			launcher_log.notice("Game settings: removed %s", path);
		}
	}
}
