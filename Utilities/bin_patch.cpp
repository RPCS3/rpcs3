#include "bin_patch.h"
#include "File.h"
#include "Config.h"

LOG_CHANNEL(patch_log);

static const std::string yml_key_enable_legacy_patches = "Enable Legacy Patches";

template <>
void fmt_class_string<YAML::NodeType::value>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](YAML::NodeType::value value)
	{
		switch (value)
		{
		case YAML::NodeType::Undefined: return "Undefined";
		case YAML::NodeType::Null: return "Null";
		case YAML::NodeType::Scalar: return "Scalar";
		case YAML::NodeType::Sequence: return "Sequence";
		case YAML::NodeType::Map: return "Map";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<patch_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](patch_type value)
	{
		switch (value)
		{
		case patch_type::load: return "load";
		case patch_type::byte: return "byte";
		case patch_type::le16: return "le16";
		case patch_type::le32: return "le32";
		case patch_type::le64: return "le64";
		case patch_type::bef32: return "bef32";
		case patch_type::bef64: return "bef64";
		case patch_type::be16: return "be16";
		case patch_type::be32: return "be32";
		case patch_type::be64: return "be64";
		case patch_type::lef32: return "lef32";
		case patch_type::lef64: return "lef64";
		}

		return unknown;
	});
}

patch_engine::patch_engine()
{
	const std::string patches_path = fs::get_config_dir() + "patches/";

	if (!fs::create_path(patches_path))
	{
		patch_log.fatal("Failed to create path: %s (%s)", patches_path, fs::g_tls_error);
	}
}

std::string patch_engine::get_patch_config_path()
{
#ifdef _WIN32
	return fs::get_config_dir() + "config/patch_config.yml";
#else
	return fs::get_config_dir() + "patch_config.yml";
#endif
}

void patch_engine::load(patch_map& patches_map, const std::string& path)
{
	// Load patch file
	fs::file file{ path };

	if (!file)
	{
		// Do nothing
		return;
	}

	// Interpret yaml nodes
	auto [root, error] = yaml_load(file.to_string());

	if (!error.empty())
	{
		patch_log.fatal("Failed to load patch file %s:\n%s", path, error);
		return;
	}

	// Load patch config to determine which patches are enabled
	bool enable_legacy_patches;
	patch_config_map patch_config = load_config(enable_legacy_patches);

	static const std::string target_version = "1.0";
	std::string version;
	bool is_legacy_patch = false;

	if (const auto version_node = root["Version"])
	{
		version = version_node.Scalar();

		if (version != target_version)
		{
			patch_log.error("Patch engine target version %s does not match file version %s in %s", target_version, version, path);
			return;
		}

		// We don't need the Version node in local memory anymore
		root.remove("Version");
	}
	else
	{
		patch_log.warning("Patch engine version %s: Reading legacy patch file %s", target_version, path);
		is_legacy_patch = true;
	}

	// Go through each main key in the file
	for (auto pair : root)
	{
		const auto& main_key = pair.first.Scalar();

		// Use old logic and yaml layout if this is a legacy patch
		if (is_legacy_patch)
		{
			struct patch_info info{};
			info.hash      = main_key;
			info.enabled   = enable_legacy_patches;
			info.is_legacy = true;

			read_patch_node(info, pair.second, root);

			// Find or create an entry matching the key/hash in our map
			auto& title_info = patches_map[main_key];
			title_info.hash      = main_key;
			title_info.is_legacy = true;
			title_info.patch_info_map["legacy"] = info;
			continue;
		}

		// Use new logic and yaml layout

		if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
		{
			patch_log.error("Skipping key %s: expected Map, found %s (file: %s)", main_key, yml_type, path);
			continue;
		}

		// Skip Anchors
		if (main_key == "Anchors")
		{
			continue;
		}

		std::string title;
		std::string serials;

		if (const auto title_node = pair.second["Title"])
		{
			title = title_node.Scalar();
		}

		if (const auto serials_node = pair.second["Serials"])
		{
			serials = serials_node.Scalar();
		}

		if (const auto patches_node = pair.second["Patches"])
		{
			if (const auto yml_type = patches_node.Type(); yml_type != YAML::NodeType::Map)
			{
				patch_log.error("Skipping Patches: expected Map, found %s (key: %s, file: %s)", yml_type, main_key, path);
				continue;
			}

			// Find or create an entry matching the key/hash in our map
			auto& title_info = patches_map[main_key];
			title_info.is_legacy = false;
			title_info.hash      = main_key;
			title_info.title     = title;
			title_info.serials   = serials;
			title_info.version   = version;

			// Go through each patch
			for (auto patches_entry : patches_node)
			{
				// Each key in "Patches" is also the patch description
				const std::string description = patches_entry.first.Scalar();

				// Find out if this patch was enabled in the patch config
				const bool enabled = patch_config[main_key][description];

				// Compile patch information

				if (const auto yml_type = patches_entry.second.Type(); yml_type != YAML::NodeType::Map)
				{
					patch_log.error("Skipping Patch key %s: expected Map, found %s (key: %s, file: %s)", description, yml_type, main_key, path);
					continue;
				}

				struct patch_info info {};
				info.enabled     = enabled;
				info.description = description;
				info.hash        = main_key;
				info.version     = version;
				info.serials     = serials;
				info.title       = title;

				if (const auto author_node = patches_entry.second["Author"])
				{
					info.author = author_node.Scalar();
				}

				if (const auto patch_version_node = patches_entry.second["Version"])
				{
					info.patch_version = patch_version_node.Scalar();
				}

				if (const auto notes_node = patches_entry.second["Notes"])
				{
					info.notes = notes_node.Scalar();
				}

				if (const auto patch_node = patches_entry.second["Patch"])
				{
					read_patch_node(info, patch_node, root);
				}

				// Insert patch information
				title_info.patch_info_map[description] = info;
			}
		}
	}
}

patch_type patch_engine::get_patch_type(YAML::Node node)
{
	u64 type_val = 0;
	cfg::try_to_enum_value(&type_val, &fmt_class_string<patch_type>::format, node.Scalar());
	return static_cast<patch_type>(type_val);
}

void patch_engine::add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root)
{
	const auto type_node  = node[0];
	auto addr_node        = node[1];
	const auto value_node = node[2];

	struct patch_data p_data{};
	p_data.type   = get_patch_type(type_node);
	p_data.offset = addr_node.as<u32>(0) + modifier;

	switch (p_data.type)
	{
	case patch_type::load:
	{
		// Special syntax: anchors (named sequence)

		// Most legacy patches don't use the anchor syntax correctly, so try to sanitize it.
		if (info.is_legacy)
		{
			if (const auto yml_type = addr_node.Type(); yml_type == YAML::NodeType::Scalar)
			{
				const auto anchor = addr_node.Scalar();
				const auto anchor_node = root[anchor];

				if (anchor_node)
				{
					addr_node = anchor_node;
					patch_log.warning("Incorrect anchor syntax found in legacy patch: %s (key: %s)", anchor, info.hash);
				}
				else
				{
					patch_log.error("Anchor not found in legacy patch: %s (key: %s)", anchor, info.hash);
					return;
				}
			}
		}

		// Check if the anchor was resolved.
		if (const auto yml_type = addr_node.Type(); yml_type != YAML::NodeType::Sequence)
		{
			patch_log.error("Skipping sequence: expected Sequence, found %s (key: %s)", yml_type, info.hash);
			return;
		}

		// Address modifier (optional)
		const u32 mod = value_node.as<u32>(0);

		for (const auto& item : addr_node)
		{
			add_patch_data(item, info, mod, root);
		}

		return;
	}
	case patch_type::bef32:
	case patch_type::lef32:
	{
		p_data.value = std::bit_cast<u32>(value_node.as<f32>());
		break;
	}
	case patch_type::bef64:
	case patch_type::lef64:
	{
		p_data.value = std::bit_cast<u64>(value_node.as<f64>());
		break;
	}
	default:
	{
		p_data.value = value_node.as<u64>();
	}
	}

	info.data_list.emplace_back(p_data);
}

void patch_engine::read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root)
{
	if (const auto yml_type = node.Type(); yml_type != YAML::NodeType::Sequence)
	{
		patch_log.error("Skipping patch node %s: expected Sequence, found %s (key: %s)", info.description, yml_type, info.hash);
		return;
	}

	for (auto patch : node)
	{
		add_patch_data(patch, info, 0, root);
	}
}

void patch_engine::append(const std::string& patch)
{
	load(m_map, patch);
}

void patch_engine::append_global_patches()
{
	// Legacy patch.yml
	load(m_map, fs::get_config_dir() + "patch.yml");

	// New patch.yml
	load(m_map, fs::get_config_dir() + "patches/patch.yml");
}

void patch_engine::append_title_patches(const std::string& title_id)
{
	if (title_id.empty())
	{
		return;
	}

	// Legacy patch.yml
	load(m_map, fs::get_config_dir() + "data/" + title_id + "/patch.yml");

	// New patch.yml
	load(m_map, fs::get_config_dir() + "patches/" +  title_id + "_patch.yml");
}

std::size_t patch_engine::apply(const std::string& name, u8* dst) const
{
	return apply_patch<false>(name, dst, 0, 0);
}

std::size_t patch_engine::apply_with_ls_check(const std::string& name, u8* dst, u32 filesz, u32 ls_addr) const
{
	return apply_patch<true>(name, dst, filesz, ls_addr);
}

template <bool check_local_storage>
std::size_t patch_engine::apply_patch(const std::string& name, u8* dst, u32 filesz, u32 ls_addr) const
{
	if (m_map.find(name) == m_map.cend())
	{
		return 0;
	}

	size_t applied_total = 0;
	const auto& title_info = m_map.at(name);

	// Apply modifications sequentially
	for (const auto& [description, patch] : title_info.patch_info_map)
	{
		if (!patch.enabled)
		{
			continue;
		}

		size_t applied = 0;

		for (const auto& p : patch.data_list)
		{
			u32 offset = p.offset;

			if constexpr (check_local_storage)
			{
				offset -= ls_addr;

				if (offset < ls_addr || offset >= (ls_addr + filesz))
				{
					// This patch is out of range for this segment
					continue;
				}
			}

			auto ptr = dst + offset;

			switch (p.type)
			{
			case patch_type::load:
			{
				// Invalid in this context
				continue;
			}
			case patch_type::byte:
			{
				*ptr = static_cast<u8>(p.value);
				break;
			}
			case patch_type::le16:
			{
				*reinterpret_cast<le_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
				break;
			}
			case patch_type::le32:
			case patch_type::lef32:
			{
				*reinterpret_cast<le_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
				break;
			}
			case patch_type::le64:
			case patch_type::lef64:
			{
				*reinterpret_cast<le_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
				break;
			}
			case patch_type::be16:
			{
				*reinterpret_cast<be_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
				break;
			}
			case patch_type::be32:
			case patch_type::bef32:
			{
				*reinterpret_cast<be_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
				break;
			}
			case patch_type::be64:
			case patch_type::bef64:
			{
				*reinterpret_cast<be_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
				break;
			}
			}

			++applied;
		}

		if (title_info.is_legacy)
		{
			patch_log.notice("Applied legacy patch (<- %d)", applied);
		}
		else
		{
			patch_log.notice("Applied patch (description='%s', author='%s', patch_version='%s', file_version='%s') (<- %d)", description, patch.author, patch.patch_version, patch.version, applied);
		}

		applied_total += applied;
	}

	return applied_total;
}

void patch_engine::save_config(const patch_map& patches_map, bool enable_legacy_patches)
{
	const std::string path = get_patch_config_path();
	patch_log.notice("Saving patch config file %s", path);

	fs::file file(path, fs::rewrite);
	if (!file)
	{
		patch_log.fatal("Failed to open patch config file %s", path);
		return;
	}

	YAML::Emitter out;
	out << YAML::BeginMap;

	// Save "Enable Legacy Patches"
	out << yml_key_enable_legacy_patches << enable_legacy_patches;

	// Save 'enabled' state per hash and description
	patch_config_map config_map;

	for (const auto& [hash, title_info] : patches_map)
	{
		if (title_info.is_legacy)
		{
			continue;
		}

		for (const auto& [description, patch] : title_info.patch_info_map)
		{
			config_map[hash][description] = patch.enabled;
		}

		if (config_map[hash].size() > 0)
		{
			out << hash;
			out << YAML::BeginMap;

			for (const auto& [description, enabled] : config_map[hash])
			{
				out << description;
				out << enabled;
			}

			out << YAML::EndMap;
		}
	}
	out << YAML::EndMap;

	file.write(out.c_str(), out.size());
}

patch_engine::patch_config_map patch_engine::load_config(bool& enable_legacy_patches)
{
	enable_legacy_patches = true; // Default to true

	patch_config_map config_map;

	const std::string path = get_patch_config_path();
	patch_log.notice("Loading patch config file %s", path);

	if (fs::file f{ path })
	{
		auto [root, error] = yaml_load(f.to_string());

		if (!error.empty())
		{
			patch_log.fatal("Failed to load patch config file %s:\n%s", path, error);
			return config_map;
		}

		// Try to load "Enable Legacy Patches" (default to true)
		if (auto enable_legacy_node = root[yml_key_enable_legacy_patches])
		{
			enable_legacy_patches = enable_legacy_node.as<bool>(true);
			root.remove(yml_key_enable_legacy_patches); // Remove the node in order to skip it in the next part
		}

		for (auto pair : root)
		{
			auto& hash = pair.first.Scalar();
			auto& data = config_map[hash];

			if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
			{
				patch_log.error("Error loading patch config key %s: expected Map, found %s (file: %s)", hash, yml_type, path);
				continue;
			}

			for (auto patch : pair.second)
			{
				const auto description = patch.first.Scalar();
				const auto enabled     = patch.second.as<bool>(false);

				data[description] = enabled;
			}
		}
	}

	return config_map;
}
