// PS5: a game's patches for the launcher's Patches page (ps5_patches.h)

#include "stdafx.h"
#include "ps5_patches.h"

#include "Crypto/utils.h"
#include "Utilities/File.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>

LOG_CHANNEL(patch_log, "PAT");

namespace ps5_patches
{
	namespace
	{
		// A JSON string's text, from its opening quote; empty when it is not one
		bool json_string(std::string_view json, usz at, std::string& out)
		{
			if (at >= json.size() || json[at] != '"')
			{
				return false;
			}

			const auto hex = [&](usz from, u32& code)
			{
				if (from + 4 > json.size())
				{
					return false;
				}
				code = 0;
				for (usz i = from; i < from + 4; i++)
				{
					const char c = json[i];
					code <<= 4;
					if (c >= '0' && c <= '9') code |= c - '0';
					else if (c >= 'a' && c <= 'f') code |= c - 'a' + 10;
					else if (c >= 'A' && c <= 'F') code |= c - 'A' + 10;
					else return false;
				}
				return true;
			};

			out.clear();
			for (usz i = at + 1; i < json.size(); i++)
			{
				const char c = json[i];
				if (c == '"')
				{
					return true;
				}
				if (c != '\\')
				{
					out += c;
					continue;
				}
				if (++i >= json.size())
				{
					return false;
				}
				switch (json[i])
				{
				case '"': out += '"'; break;
				case '\\': out += '\\'; break;
				case '/': out += '/'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'n': out += '\n'; break;
				case 'r': out += '\r'; break;
				case 't': out += '\t'; break;
				case 'u':
				{
					u32 code = 0;
					if (!hex(i + 1, code))
					{
						return false;
					}
					i += 4;
					// A surrogate pair is one character
					if (code >= 0xd800 && code < 0xdc00 && i + 2 < json.size() && json[i + 1] == '\\' && json[i + 2] == 'u')
					{
						u32 low = 0;
						if (hex(i + 3, low) && low >= 0xdc00 && low < 0xe000)
						{
							code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
							i += 6;
						}
					}
					if (code < 0x80)
					{
						out += static_cast<char>(code);
					}
					else if (code < 0x800)
					{
						out += static_cast<char>(0xc0 | (code >> 6));
						out += static_cast<char>(0x80 | (code & 0x3f));
					}
					else if (code < 0x10000)
					{
						out += static_cast<char>(0xe0 | (code >> 12));
						out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
						out += static_cast<char>(0x80 | (code & 0x3f));
					}
					else
					{
						out += static_cast<char>(0xf0 | (code >> 18));
						out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
						out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
						out += static_cast<char>(0x80 | (code & 0x3f));
					}
					break;
				}
				default:
					return false;
				}
			}
			return false;
		}

		// Where a top-level key's value starts, after its colon
		usz json_value(std::string_view json, std::string_view key)
		{
			const std::string quoted = fmt::format("\"%s\"", key);
			for (usz at = json.find(quoted); at != umax; at = json.find(quoted, at + 1))
			{
				usz i = at + quoted.size();
				while (i < json.size() && std::isspace(static_cast<u8>(json[i]))) i++;
				if (i < json.size() && json[i] == ':')
				{
					i++;
					while (i < json.size() && std::isspace(static_cast<u8>(json[i]))) i++;
					return i;
				}
			}
			return umax;
		}

		// The page rpcs3.net answers the desktop's "Download latest patches"
		// with (https://rpcs3.net/compatibility?patch&api=v1&v=1.2), saved
		// from a browser and put in the patches folder over FTP: its patch.yml
		// is checked as the desktop checks it, written in place of the old one
		// (kept as patch.yml.old), and the page removed. Returns the file's
		// name, or what was wrong with it
		std::string unpack_download(const std::string& folder, const std::string& name)
		{
			const std::string path = folder + name;
			const fs::file file(path);
			if (!file || file.size() > 64 * 1024 * 1024)
			{
				return {};
			}

			// A patch file is YAML: only a JSON object is read whole
			std::string start(64, '\0');
			start.resize(file.read(start.data(), start.size()));
			const usz brace = start.find_first_not_of(" \t\r\n\xef\xbb\xbf");
			if (brace == umax || start[brace] != '{')
			{
				return {};
			}
			const std::string json = file.to_string();
			if (json_value(json, "patch") == umax)
			{
				return {};
			}

			const auto fail = [&](std::string_view why)
			{
				patch_log.error("%s: %s", path, why);
				return fmt::format("%s was not unpacked: %s", name, why);
			};

			if (const usz code_at = json_value(json, "return_code"); code_at != umax)
			{
				const s32 code = std::atoi(json.c_str() + code_at);
				if (code < 0)
				{
					return fail(fmt::format("rpcs3.net answered with error %d", code));
				}
			}

			std::string version, sha256, content;
			if (!json_string(json, json_value(json, "version"), version) || version != patch_engine_version)
			{
				return fail(fmt::format("its patches are version '%s', not %s", version, patch_engine_version));
			}
			if (!json_string(json, json_value(json, "patch"), content) || content.empty())
			{
				return fail("it holds no patches");
			}
			if (json_string(json, json_value(json, "sha256"), sha256) && sha256 != sha256_get_hash(content.c_str(), content.size(), true))
			{
				return fail("its patches do not match their checksum");
			}

			patch_engine::patch_map check;
			if (!patch_engine::load(check, name, content, true))
			{
				return fail("RPCS3 could not read its patches");
			}

			const std::string target = folder + "patch.yml";
			if (fs::is_file(target))
			{
				fs::remove_file(target + ".old");
				fs::rename(target, target + ".old", true);
			}
			if (!fs::write_file(target, fs::rewrite, content))
			{
				return fail(fmt::format("patch.yml could not be written (%s)", fs::g_tls_error));
			}
			// (A page saved as patch.yml is patch.yml.old now)
			if (name != "patch.yml")
			{
				fs::remove_file(path);
			}
			patch_log.notice("%s unpacked into patch.yml (%u bytes, patches for %u executables)", path, content.size(), check.size());
			return fmt::format("patch.yml was updated from %s", name);
		}
	}

	std::string ftp_folder()
	{
		// /app0 is the title's folder: /data/homebrew/<title id>/
		std::string path = patch_engine::get_patches_path();
		if (path.starts_with("/app0/"))
		{
			path = "/data/homebrew/PPSA99200/" + path.substr(6);
		}
		return path;
	}

	game read(const std::string& serial, const std::string& app_version)
	{
		game g;
		g.serial = serial;
		g.app_version = app_version;

		const std::string folder = patch_engine::get_patches_path();
		fs::create_path(folder);

		// Pages saved from rpcs3.net first: what they hold is read below
		std::vector<std::string> names;
		if (fs::dir dir{folder})
		{
			for (const fs::dir_entry& entry : dir)
			{
				if (!entry.is_directory && entry.name != "." && entry.name != "..")
				{
					names.push_back(entry.name);
				}
			}
		}
		std::sort(names.begin(), names.end());
		for (const std::string& name : names)
		{
			if (!name.ends_with(".old") && name != "patch_config.yml")
			{
				if (std::string result = unpack_download(folder, name); !result.empty())
				{
					g.unpacked = std::move(result);
				}
			}
		}

		// As the desktop reads them, and as they are applied at boot
		std::vector<std::string> files{"patch.yml", "imported_patch.yml"};
		if (fs::dir dir{folder})
		{
			std::vector<std::string> others;
			for (const fs::dir_entry& entry : dir)
			{
				if (!entry.is_directory && entry.name.ends_with("_patch.yml") && entry.name != "imported_patch.yml")
				{
					others.push_back(entry.name);
				}
			}
			std::sort(others.begin(), others.end());
			files.insert(files.end(), others.begin(), others.end());
		}

		for (const std::string& name : files)
		{
			if (!fs::is_file(folder + name))
			{
				continue;
			}
			g.files++;
			if (!patch_engine::load(g.map, folder + name))
			{
				g.broken.push_back(name);
				patch_log.error("Errors in patch file '%s'", name);
			}
		}

		// This game's: the serial's own entries, else "All", and the version's
		// own, else "All", as the patch engine picks them at boot
		std::set<std::string> other_versions;
		for (auto& [hash, container] : g.map)
		{
			for (auto& [description, info] : container.patch_info_map)
			{
				g.total++;
				for (auto& [title, serials] : info.titles)
				{
					std::string found_serial;
					if (serials.contains(serial))
					{
						found_serial = serial;
					}
					else if (serials.contains(patch_key::all))
					{
						found_serial = patch_key::all;
					}
					if (found_serial.empty())
					{
						continue;
					}

					auto& app_versions = serials.at(found_serial);
					std::string found_version;
					if (app_versions.contains(app_version))
					{
						found_version = app_version;
					}
					else if (app_versions.contains(patch_key::all))
					{
						found_version = patch_key::all;
					}
					if (found_version.empty())
					{
						for (const auto& [version, config] : app_versions)
						{
							other_versions.insert(version);
						}
						continue;
					}

					const bool every_game = found_serial == patch_key::all;
					auto it = std::find_if(g.patches.begin(), g.patches.end(), [&](const patch& p)
					{
						return p.description == description && p.every_game == every_game;
					});
					if (it == g.patches.end())
					{
						patch p;
						p.description = description;
						p.author = info.author;
						p.notes = info.notes;
						p.patch_version = info.patch_version;
						p.group = info.patch_group;
						p.every_game = every_game;
						p.every_version = true;

						const patch_engine::patch_config_values& config = app_versions.at(found_version);
						p.enabled = config.enabled;
						for (const auto& [key, default_value] : info.default_config_values)
						{
							value v{key, default_value, default_value.value};
							if (const auto saved = config.config_values.find(key); saved != config.config_values.end())
							{
								v.current.set_and_check_value(saved->second.value, key);
							}
							p.values.push_back(std::move(v));
						}
						g.patches.push_back(std::move(p));
						it = g.patches.end() - 1;
					}
					else
					{
						it->enabled = it->enabled || app_versions.at(found_version).enabled;
					}
					it->every_version = it->every_version && found_version == patch_key::all;
					it->targets.push_back({hash, title, found_serial, found_version});
				}
			}
		}

		std::stable_sort(g.patches.begin(), g.patches.end(), [](const patch& a, const patch& b)
		{
			if (a.every_game != b.every_game)
			{
				return !a.every_game;
			}
			return a.description < b.description;
		});

		if (g.patches.empty())
		{
			g.other_versions.assign(other_versions.begin(), other_versions.end());
		}

		patch_log.notice("Patches for %s v%s: %u of %u in %u files%s", serial, app_version, g.patches.size(), g.total, g.files,
			g.broken.empty() ? "" : fmt::format(" (errors in %s)", fmt::merge(g.broken, ", ")));
		return g;
	}

	namespace
	{
		// Every target of the patch takes its state and values
		void store(game& g, const patch& p)
		{
			for (const target& t : p.targets)
			{
				auto& container = g.map[t.hash];
				auto& config = container.patch_info_map[p.description].titles[t.title][t.serial][t.app_version];
				config.enabled = p.enabled;
				config.config_values.clear();
				for (const value& v : p.values)
				{
					config.config_values[v.key] = v.current;
				}
			}
			g.changed = true;
		}
	}

	void set_enabled(game& g, usz index, bool enabled)
	{
		if (index >= g.patches.size() || g.patches[index].enabled == enabled)
		{
			return;
		}

		patch& p = g.patches[index];
		p.enabled = enabled;
		store(g, p);
		patch_log.notice("Patch '%s' %s for %s", p.description, enabled ? "on" : "off", g.serial);

		// As the desktop does: one of a group at a time
		if (enabled && !p.group.empty())
		{
			for (usz i = 0; i < g.patches.size(); i++)
			{
				patch& other = g.patches[i];
				if (i != index && other.enabled && other.group == p.group && other.every_game == p.every_game)
				{
					other.enabled = false;
					store(g, other);
				}
			}
		}
	}

	void step_value(game& g, usz index, usz value_index, s32 direction, bool big)
	{
		if (index >= g.patches.size() || value_index >= g.patches[index].values.size())
		{
			return;
		}

		patch& p = g.patches[index];
		value& v = p.values[value_index];
		auto& current = v.current;
		f64 next = current.value;

		switch (current.type)
		{
		case patch_configurable_type::double_enum:
		case patch_configurable_type::long_enum:
		{
			const auto& allowed = current.allowed_values;
			if (allowed.empty())
			{
				return;
			}
			const auto it = std::find_if(allowed.begin(), allowed.end(), [&](const patch_engine::patch_allowed_value& a) { return a.value == current.value; });
			const s32 count = static_cast<s32>(allowed.size());
			const s32 at = it == allowed.end() ? 0 : static_cast<s32>(it - allowed.begin());
			next = allowed[(at + (direction < 0 ? count - 1 : 1)) % count].value;
			break;
		}
		case patch_configurable_type::long_range:
		case patch_configurable_type::double_range:
		{
			// A step of about a hundredth of the range, rounded to 1, 2 or 5
			// of a power of ten (whole for whole ranges)
			const f64 span = current.max - current.min;
			f64 step = span / 100.;
			if (step > 0.)
			{
				const f64 power = std::pow(10., std::floor(std::log10(step)));
				const f64 m = step / power;
				step = (m < 1.5 ? 1. : m < 3.5 ? 2. : m < 7.5 ? 5. : 10.) * power;
			}
			if (current.type == patch_configurable_type::long_range)
			{
				step = std::max(1., std::round(step));
			}
			if (step <= 0.)
			{
				step = 1.;
			}
			next = std::clamp(current.value + direction * step * (big ? 10. : 1.), current.min, current.max);
			break;
		}
		}

		if (next != current.value)
		{
			current.value = next;
			store(g, p);
		}
	}

	void reset_value(game& g, usz index, usz value_index)
	{
		if (index >= g.patches.size() || value_index >= g.patches[index].values.size())
		{
			return;
		}

		patch& p = g.patches[index];
		value& v = p.values[value_index];
		if (v.current.value != v.default_value)
		{
			v.current.value = v.default_value;
			store(g, p);
		}
	}

	void write(game& g)
	{
		if (!g.changed)
		{
			return;
		}
		patch_engine::save_config(g.map);
		g.changed = false;
	}

	std::string value_text(const value& v, f64 number)
	{
		for (const auto& allowed : v.current.allowed_values)
		{
			if (allowed.value == number)
			{
				return allowed.label;
			}
		}
		if (v.current.type == patch_configurable_type::long_range || v.current.type == patch_configurable_type::long_enum || number == std::floor(number))
		{
			return fmt::format("%d", static_cast<s64>(number));
		}
		std::string text = fmt::format("%.4f", number);
		while (text.ends_with('0')) text.pop_back();
		if (text.ends_with('.')) text.pop_back();
		return text;
	}

	std::string value_text(const value& v)
	{
		return value_text(v, v.current.value);
	}
}
