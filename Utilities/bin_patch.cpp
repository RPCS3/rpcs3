#include "bin_patch.h"
#include "File.h"
#include "Config.h"
#include "version.h"
#include "Emu/System.h"

LOG_CHANNEL(patch_log);

static const std::string patch_engine_version = "1.2";
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
		case patch_type::invalid: return "invalid";
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
	const std::string config_dir = fs::get_config_dir() + "config/";
	const std::string patch_path = config_dir + "patch_config.yml";

	if (!fs::create_path(config_dir))
	{
		patch_log.error("Could not create path: %s", patch_path);
	}

	return patch_path;
#else
	return fs::get_config_dir() + "patch_config.yml";
#endif
}

std::string patch_engine::get_imported_patch_path()
{
	return fs::get_config_dir() + "patches/imported_patch.yml";
}

static void append_log_message(std::stringstream* log_messages, const std::string& message)
{
	if (log_messages)
		*log_messages << message << std::endl;
};

bool patch_engine::load(patch_map& patches_map, const std::string& path, bool importing, std::stringstream* log_messages)
{
	// Load patch file
	fs::file file{ path };

	if (!file)
	{
		// Do nothing
		return true;
	}

	// Interpret yaml nodes
	auto [root, error] = yaml_load(file.to_string());

	if (!error.empty() || !root)
	{
		append_log_message(log_messages, "Fatal Error: Failed to load file!");
		patch_log.fatal("Failed to load patch file %s:\n%s", path, error);
		return false;
	}

	// Load patch config to determine which patches are enabled
	bool enable_legacy_patches;
	patch_map patch_config;

	if (!importing)
	{
		patch_config = load_config(enable_legacy_patches);
	}

	std::string version;
	bool is_legacy_patch = false;

	if (const auto version_node = root["Version"])
	{
		version = version_node.Scalar();

		if (version != patch_engine_version)
		{
			append_log_message(log_messages, fmt::format("Error: Patch engine target version %s does not match file version %s", patch_engine_version, version));
			patch_log.error("Patch engine target version %s does not match file version %s in %s", patch_engine_version, version, path);
			return false;
		}

		// We don't need the Version node in local memory anymore
		root.remove("Version");
	}
	else if (importing)
	{
		append_log_message(log_messages, fmt::format("Error: Patch engine target version %s does not match file version %s", patch_engine_version, version));
		patch_log.error("Patch engine version %s: No 'Version' entry found for file %s", patch_engine_version, path);
		return false;
	}
	else
	{
		patch_log.warning("Patch engine version %s: Reading legacy patch file %s", patch_engine_version, path);
		is_legacy_patch = true;
	}

	bool is_valid = true;

	// Go through each main key in the file
	for (auto pair : root)
	{
		const auto& main_key = pair.first.Scalar();

		// Use old logic and yaml layout if this is a legacy patch
		if (is_legacy_patch)
		{
			struct patch_info info{};
			info.hash        = main_key;
			info.is_enabled  = enable_legacy_patches;
			info.is_legacy   = true;
			info.source_path = path;

			if (!read_patch_node(info, pair.second, root, log_messages))
			{
				is_valid = false;
			}

			// Find or create an entry matching the key/hash in our map
			auto& container = patches_map[main_key];
			container.hash      = main_key;
			container.is_legacy = true;
			container.patch_info_map["legacy"] = info;
			continue;
		}

		// Use new logic and yaml layout

		if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
		{
			append_log_message(log_messages, fmt::format("Error: Skipping key %s: expected Map, found %s", main_key, yml_type));
			patch_log.error("Skipping key %s: expected Map, found %s (file: %s)", main_key, yml_type, path);
			is_valid = false;
			continue;
		}

		// Skip Anchors
		if (main_key == "Anchors")
		{
			continue;
		}

		// Find or create an entry matching the key/hash in our map
		auto& container = patches_map[main_key];
		container.is_legacy = false;
		container.hash      = main_key;
		container.version   = version;

		// Go through each patch
		for (auto patches_entry : pair.second)
		{
			// Each key in "Patches" is also the patch description
			const std::string& description = patches_entry.first.Scalar();

			// Compile patch information

			if (const auto yml_type = patches_entry.second.Type(); yml_type != YAML::NodeType::Map)
			{
				append_log_message(log_messages, fmt::format("Error: Skipping Patch key %s: expected Map, found %s (key: %s)", description, yml_type, main_key));
				patch_log.error("Skipping Patch key %s: expected Map, found %s (key: %s, file: %s)", description, yml_type, main_key, path);
				is_valid = false;
				continue;
			}

			struct patch_info info {};
			info.description = description;
			info.hash        = main_key;
			info.version     = version;
			info.source_path = path;

			if (const auto games_node = patches_entry.second["Games"])
			{
				if (const auto yml_type = games_node.Type(); yml_type != YAML::NodeType::Map)
				{
					append_log_message(log_messages, fmt::format("Error: Skipping Games key: expected Map, found %s (patch: %s, key: %s)", yml_type, description, main_key));
					patch_log.error("Skipping Games key: expected Map, found %s (patch: %s, key: %s, file: %s)", yml_type, description, main_key, path);
					is_valid = false;
					continue;
				}

				for (const auto game_node : games_node)
				{
					const std::string& title = game_node.first.Scalar();

					if (const auto yml_type = game_node.second.Type(); yml_type != YAML::NodeType::Map)
					{
						append_log_message(log_messages, fmt::format("Error: Skipping %s: expected Map, found %s (patch: %s, key: %s)", title, yml_type, description, main_key));
						patch_log.error("Skipping %s: expected Map, found %s (patch: %s, key: %s, file: %s)", title, yml_type, description, main_key, path);
						is_valid = false;
						continue;
					}

					for (const auto serial_node : game_node.second)
					{
						const std::string& serial = serial_node.first.Scalar();

						if (const auto yml_type = serial_node.second.Type(); yml_type != YAML::NodeType::Sequence)
						{
							append_log_message(log_messages, fmt::format("Error: Skipping %s: expected Sequence, found %s (title: %s, patch: %s, key: %s)", serial, title, yml_type, description, main_key));
							patch_log.error("Skipping %s: expected Sequence, found %s (title: %s, patch: %s, key: %s, file: %s)", serial, title, yml_type, description, main_key, path);
							is_valid = false;
							continue;
						}

						patch_engine::patch_app_versions app_versions;

						for (const auto version : serial_node.second)
						{
							const auto& app_version = version.Scalar();

							// Find out if this patch was enabled in the patch config
							const bool enabled = patch_config[main_key].patch_info_map[description].titles[title][serial][app_version];

							app_versions.emplace(version.Scalar(), enabled);
						}

						if (app_versions.empty())
						{
							append_log_message(log_messages, fmt::format("Error: Skipping %s: empty Sequence (title: %s, patch: %s, key: %s)", serial, title, description, main_key));
							patch_log.error("Skipping %s: empty Sequence (title: %s, patch: %s, key: %s, file: %s)", serial,title, description, main_key, path);
							is_valid = false;
						}
						else
						{
							info.titles[title][serial] = app_versions;
						}
					}
				}
			}

			if (const auto author_node = patches_entry.second["Author"])
			{
				info.author = author_node.Scalar();
			}

			if (const auto patch_version_node = patches_entry.second["Patch Version"])
			{
				info.patch_version = patch_version_node.Scalar();
			}

			if (const auto notes_node = patches_entry.second["Notes"])
			{
				info.notes = notes_node.Scalar();
			}

			if (const auto patch_group_node = patches_entry.second["Group"])
			{
				info.patch_group = patch_group_node.Scalar();
			}

			if (const auto patch_node = patches_entry.second["Patch"])
			{
				if (!read_patch_node(info, patch_node, root, log_messages))
				{
					is_valid = false;
				}
			}

			// Skip this patch if a higher patch version already exists
			if (container.patch_info_map.find(description) != container.patch_info_map.end())
			{
				bool ok;
				const auto existing_version  = container.patch_info_map[description].patch_version;
				const bool version_is_bigger = utils::compare_versions(info.patch_version, existing_version, ok) > 0;

				if (!ok || !version_is_bigger)
				{
					patch_log.warning("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s (in file %s)", info.patch_version, existing_version, main_key, description, path);
					append_log_message(log_messages, fmt::format("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s (in file %s)", info.patch_version, existing_version, main_key, description, path));
					continue;
				}
				else if (!importing)
				{
					patch_log.warning("A lower patch version was found ('%s' vs '%s') for %s: %s (in file %s)", existing_version, info.patch_version, main_key, description,  container.patch_info_map[description].source_path);
				}
			}

			// Insert patch information
			container.patch_info_map[description] = info;
		}
	}

	return is_valid;
}

patch_type patch_engine::get_patch_type(YAML::Node node)
{
	u64 type_val = 0;

	if (!node || !node.IsScalar() || !cfg::try_to_enum_value(&type_val, &fmt_class_string<patch_type>::format, node.Scalar()))
	{
		return patch_type::invalid;
	}

	return static_cast<patch_type>(type_val);
}

bool patch_engine::add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root, std::stringstream* log_messages)
{
	if (!node || !node.IsSequence())
	{
		append_log_message(log_messages, fmt::format("Skipping invalid patch node %s. (key: %s)", info.description, info.hash));
		patch_log.error("Skipping invalid patch node %s. (key: %s)", info.description, info.hash);
		return false;
	}

	const auto type_node  = node[0];
	auto addr_node        = node[1];
	const auto value_node = node[2];

	const auto type = get_patch_type(type_node);

	if (type == patch_type::invalid)
	{
		const auto type_str = type_node && type_node.IsScalar() ? type_node.Scalar() : "";
		append_log_message(log_messages, fmt::format("Skipping patch node %s: type '%s' is invalid. (key: %s)", info.description, type_str, info.hash));
		patch_log.error("Skipping patch node %s: type '%s' is invalid. (key: %s)", info.description, type_str, info.hash);
		return false;
	}

	if (type == patch_type::load)
	{
		// Special syntax: anchors (named sequence)

		// Most legacy patches don't use the anchor syntax correctly, so try to sanitize it.
		if (info.is_legacy)
		{
			if (const auto yml_type = addr_node.Type(); yml_type == YAML::NodeType::Scalar)
			{
				if (!root)
				{
					patch_log.fatal("Trying to parse legacy patch with invalid root."); // Sanity Check
					return false;
				}

				const auto anchor = addr_node.Scalar();
				const auto anchor_node = root[anchor];

				if (anchor_node)
				{
					addr_node = anchor_node;
					append_log_message(log_messages, fmt::format("Incorrect anchor syntax found in legacy patch: %s (key: %s)", anchor, info.hash));
					patch_log.warning("Incorrect anchor syntax found in legacy patch: %s (key: %s)", anchor, info.hash);
				}
				else
				{
					append_log_message(log_messages, fmt::format("Anchor not found in legacy patch: %s (key: %s)", anchor, info.hash));
					patch_log.error("Anchor not found in legacy patch: %s (key: %s)", anchor, info.hash);
					return false;
				}
			}
		}

		// Check if the anchor was resolved.
		if (const auto yml_type = addr_node.Type(); yml_type != YAML::NodeType::Sequence)
		{
			append_log_message(log_messages, fmt::format("Skipping sequence: expected Sequence, found %s (key: %s)", yml_type, info.hash));
			patch_log.error("Skipping sequence: expected Sequence, found %s (key: %s)", yml_type, info.hash);
			return false;
		}

		// Address modifier (optional)
		const u32 mod = value_node.as<u32>(0);

		bool is_valid = true;

		for (const auto& item : addr_node)
		{
			if (!add_patch_data(item, info, mod, root, log_messages))
			{
				is_valid = false;
			}
		}

		return is_valid;
	}

	struct patch_data p_data{};
	p_data.type           = type;
	p_data.offset         = addr_node.as<u32>(0) + modifier;
	p_data.original_value = value_node.IsScalar() ? value_node.Scalar() : "";

	std::string error_message;

	switch (p_data.type)
	{
	case patch_type::bef32:
	case patch_type::lef32:
	case patch_type::bef64:
	case patch_type::lef64:
	{
		p_data.value.double_value = get_yaml_node_value<f64>(value_node, error_message);
		break;
	}
	default:
	{
		p_data.value.long_value = get_yaml_node_value<u64>(value_node, error_message);
		break;
	}
	}

	if (!error_message.empty())
	{
		error_message = fmt::format("Skipping patch data entry: [ %s, 0x%.8x, %s ] (key: %s) %s",
			p_data.type, p_data.offset, p_data.original_value.empty() ? "?" : p_data.original_value, info.hash, error_message);
		append_log_message(log_messages, error_message);
		patch_log.error("%s", error_message);
		return false;
	}

	info.data_list.emplace_back(p_data);

	return true;
}

bool patch_engine::read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root, std::stringstream* log_messages)
{
	if (!node)
	{
		append_log_message(log_messages, fmt::format("Skipping invalid patch node %s. (key: %s)", info.description, info.hash));
		patch_log.error("Skipping invalid patch node %s. (key: %s)" HERE, info.description, info.hash);
		return false;
	}

	if (const auto yml_type = node.Type(); yml_type != YAML::NodeType::Sequence)
	{
		append_log_message(log_messages, fmt::format("Skipping patch node %s: expected Sequence, found %s (key: %s)", info.description, yml_type, info.hash));
		patch_log.error("Skipping patch node %s: expected Sequence, found %s (key: %s)", info.description, yml_type, info.hash);
		return false;
	}

	bool is_valid = true;

	for (auto patch : node)
	{
		if (!add_patch_data(patch, info, 0, root, log_messages))
		{
			is_valid = false;
		}
	}

	return is_valid;
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

	// Imported patch.yml
	load(m_map, get_imported_patch_path());
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
	const auto& container = m_map.at(name);
	const auto serial = Emu.GetTitleID();
	const auto app_version = Emu.GetAppVersion();

	// Only one patch per patch group is allowed
	std::set<std::string> applied_groups;

	// Apply modifications sequentially
	for (const auto& [description, patch] : container.patch_info_map)
	{
		if (patch.is_legacy && !patch.is_enabled)
		{
			continue;
		}

		bool enabled = false;

		for (const auto& [title, serials] : patch.titles)
		{
			std::string found_serial;

			if (serials.find(patch_key::all) != serials.end())
			{
				found_serial = patch_key::all;
			}
			else if (serials.find(serial) != serials.end())
			{
				found_serial = serial;
			}

			if (!found_serial.empty())
			{
				const auto& app_versions = serials.at(found_serial);
				std::string found_app_version;

				if (app_versions.find(patch_key::all) != app_versions.end())
				{
					found_app_version = patch_key::all;
				}
				else if (app_versions.find(app_version) != app_versions.end())
				{
					found_app_version = app_version;
				}

				if (!found_app_version.empty() && app_versions.at(found_app_version))
				{
					enabled = true;
					break;
				}
			}
		}

		if (!enabled)
		{
			continue;
		}

		if (!patch.patch_group.empty())
		{
			if (applied_groups.contains(patch.patch_group))
			{
				continue;
			}

			applied_groups.insert(patch.patch_group);
		}

		size_t applied = 0;

		for (const auto& p : patch.data_list)
		{
			u32 offset = p.offset;

			if constexpr (check_local_storage)
			{
				if (offset < ls_addr || offset >= (ls_addr + filesz))
				{
					// This patch is out of range for this segment
					continue;
				}
				
				offset -= ls_addr;
			}

			auto ptr = dst + offset;

			switch (p.type)
			{
			case patch_type::invalid:
			case patch_type::load:
			{
				// Invalid in this context
				continue;
			}
			case patch_type::byte:
			{
				*ptr = static_cast<u8>(p.value.long_value);
				break;
			}
			case patch_type::le16:
			{
				*reinterpret_cast<le_t<u16, 1>*>(ptr) = static_cast<u16>(p.value.long_value);
				break;
			}
			case patch_type::le32:
			{
				*reinterpret_cast<le_t<u32, 1>*>(ptr) = static_cast<u32>(p.value.long_value);
				break;
			}
			case patch_type::lef32:
			{
				*reinterpret_cast<le_t<u32, 1>*>(ptr) = std::bit_cast<u32, f32>(static_cast<f32>(p.value.double_value));
				break;
			}
			case patch_type::le64:
			{
				*reinterpret_cast<le_t<u64, 1>*>(ptr) = static_cast<u64>(p.value.long_value);
				break;
			}
			case patch_type::lef64:
			{
				*reinterpret_cast<le_t<u64, 1>*>(ptr) = std::bit_cast<u64, f64>(p.value.double_value);
				break;
			}
			case patch_type::be16:
			{
				*reinterpret_cast<be_t<u16, 1>*>(ptr) = static_cast<u16>(p.value.long_value);
				break;
			}
			case patch_type::be32:
			{
				*reinterpret_cast<be_t<u32, 1>*>(ptr) = static_cast<u32>(p.value.long_value);
				break;
			}
			case patch_type::bef32:
			{
				*reinterpret_cast<be_t<u32, 1>*>(ptr) = std::bit_cast<u32, f32>(static_cast<f32>(p.value.double_value));
				break;
			}
			case patch_type::be64:
			{
				*reinterpret_cast<be_t<u64, 1>*>(ptr) = static_cast<u64>(p.value.long_value);
				break;
			}
			case patch_type::bef64:
			{
				*reinterpret_cast<be_t<u64, 1>*>(ptr) = std::bit_cast<u64, f64>(p.value.double_value);
				break;
			}
			}

			++applied;
		}

		if (container.is_legacy)
		{
			patch_log.notice("Applied legacy patch (hash='%s')(<- %d)", name, applied);
		}
		else
		{
			patch_log.notice("Applied patch (hash='%s', description='%s', author='%s', patch_version='%s', file_version='%s') (<- %d)", name, description, patch.author, patch.patch_version, patch.version, applied);
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

	// Save 'enabled' state per hash, description, serial and app_version
	patch_map config_map;

	for (const auto& [hash, container] : patches_map)
	{
		if (container.is_legacy)
		{
			continue;
		}

		for (const auto& [description, patch] : container.patch_info_map)
		{
			if (patch.is_legacy)
			{
				continue;
			}

			for (const auto& [title, serials] : patch.titles)
			{
				for (const auto& [serial, app_versions] : serials)
				{
					for (const auto& [app_version, enabled] : app_versions)
					{
						if (enabled)
						{
							config_map[hash].patch_info_map[description].titles[title][serial][app_version] = true;
						}
					}
				}
			}
		}

		if (const auto& enabled_patches = config_map[hash].patch_info_map; enabled_patches.size() > 0)
		{
			out << hash << YAML::BeginMap;

			for (const auto& [description, patch] : enabled_patches)
			{
				const auto& titles = patch.titles;

				out << description << YAML::BeginMap;

				for (const auto& [title, serials] : titles)
				{
					out << title << YAML::BeginMap;

					for (const auto& [serial, app_versions] : serials)
					{
						out << serial << YAML::BeginMap;

						for (const auto& [app_version, enabled] : app_versions)
						{
							out << app_version << enabled;
						}

						out << YAML::EndMap;
					}

					out << YAML::EndMap;
				}

				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}
	}

	out << YAML::EndMap;

	file.write(out.c_str(), out.size());
}

static void append_patches(patch_engine::patch_map& existing_patches, const patch_engine::patch_map& new_patches, size_t& count, size_t& total, std::stringstream* log_messages)
{
	for (const auto& [hash, new_container] : new_patches)
	{
		total += new_container.patch_info_map.size();

		if (existing_patches.find(hash) == existing_patches.end())
		{
			existing_patches[hash] = new_container;
			count += new_container.patch_info_map.size();
			continue;
		}

		auto& container = existing_patches[hash];

		for (const auto& [description, new_info] : new_container.patch_info_map)
		{
			if (container.patch_info_map.find(description) == container.patch_info_map.end())
			{
				container.patch_info_map[description] = new_info;
				count++;
				continue;
			}

			auto& info = container.patch_info_map[description];

			bool ok;
			const bool version_is_bigger = utils::compare_versions(new_info.patch_version, info.patch_version, ok) > 0;

			if (!ok)
			{
				patch_log.error("Failed to compare patch versions ('%s' vs '%s') for %s: %s", new_info.patch_version, info.patch_version, hash, description);
				append_log_message(log_messages, fmt::format("Failed to compare patch versions ('%s' vs '%s') for %s: %s", new_info.patch_version, info.patch_version, hash, description));
				continue;
			}

			if (!version_is_bigger)
			{
				patch_log.error("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s", new_info.patch_version, info.patch_version, hash, description);
				append_log_message(log_messages, fmt::format("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s", new_info.patch_version, info.patch_version, hash, description));
				continue;
			}

			for (const auto& [title, new_serials] : new_info.titles)
			{
				for (const auto& [serial, new_app_versions] : new_serials)
				{
					if (!new_app_versions.empty())
					{
						info.titles[title][serial].insert(new_app_versions.begin(), new_app_versions.end());
					}
				}
			}

			if (!new_info.patch_version.empty()) info.patch_version = new_info.patch_version;
			if (!new_info.author.empty())        info.author        = new_info.author;
			if (!new_info.notes.empty())         info.notes         = new_info.notes;
			if (!new_info.data_list.empty())     info.data_list     = new_info.data_list;
			if (!new_info.source_path.empty())   info.source_path   = new_info.source_path;

			count++;
		}
	}
}

bool patch_engine::save_patches(const patch_map& patches, const std::string& path, std::stringstream* log_messages)
{
	fs::file file(path, fs::rewrite);
	if (!file)
	{
		patch_log.fatal("save_patches: Failed to open patch file %s", path);
		append_log_message(log_messages, fmt::format("Failed to open patch file %s", path));
		return false;
	}

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << "Version" << patch_engine_version;

	for (const auto& [hash, container] : patches)
	{
		if (container.patch_info_map.empty())
		{
			continue;
		}

		out << YAML::Newline << YAML::Newline;
		out << hash << YAML::BeginMap;
		out << "Patches" << YAML::BeginMap;

		for (const auto& [description, info] : container.patch_info_map)
		{
			out << description << YAML::BeginMap;
			out << "Games" << YAML::BeginMap;

			for (const auto& [title, serials] : info.titles)
			{
				out << title << YAML::BeginMap;

				for (const auto& [serial, app_versions] : serials)
				{
					out << serial << YAML::BeginSeq;

					for (const auto& app_version : serials)
					{
						out << app_version.first;
					}

					out << YAML::EndSeq;
				}

				out << YAML::EndMap;
			}

			out << YAML::EndMap;

			if (!info.author.empty())        out << "Author"        << info.author;
			if (!info.patch_version.empty()) out << "Patch Version" << info.patch_version;
			if (!info.notes.empty())         out << "Notes"         << info.notes;

			out << "Patch" << YAML::BeginSeq;

			for (const auto& data : info.data_list)
			{
				if (data.type == patch_type::invalid || data.type == patch_type::load)
				{
					// Unreachable with current logic
					continue;
				}

				out << YAML::Flow;
				out << YAML::BeginSeq;
				out << fmt::format("%s", data.type);
				out << fmt::format("0x%.8x", data.offset);
				out << data.original_value;
				out << YAML::EndSeq;
			}

			out << YAML::EndSeq;
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
		out << YAML::EndMap;
	}

	out << YAML::EndMap;

	file.write(out.c_str(), out.size());

	return true;
}

bool patch_engine::import_patches(const patch_engine::patch_map& patches, const std::string& path, size_t& count, size_t& total, std::stringstream* log_messages)
{
	patch_engine::patch_map existing_patches;

	if (load(existing_patches, path, true, log_messages))
	{
		append_patches(existing_patches, patches, count, total, log_messages);
		return count == 0 || save_patches(existing_patches, path, log_messages);
	}

	return false;
}

bool patch_engine::remove_patch(const patch_info& info)
{
	patch_engine::patch_map patches;

	if (load(patches, info.source_path))
	{
		if (patches.find(info.hash) != patches.end())
		{
			auto& container = patches[info.hash];

			if (container.patch_info_map.find(info.description) != container.patch_info_map.end())
			{
				container.patch_info_map.erase(info.description);
				return save_patches(patches, info.source_path);
			}
		}
	}

	return false;
}

patch_engine::patch_map patch_engine::load_config(bool& enable_legacy_patches)
{
	enable_legacy_patches = true; // Default to true

	patch_map config_map;

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

		for (const auto pair : root)
		{
			const auto& hash = pair.first.Scalar();

			if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
			{
				patch_log.error("Error loading patch config key %s: expected Map, found %s (file: %s)", hash, yml_type, path);
				continue;
			}

			for (const auto patch : pair.second)
			{
				const auto& description = patch.first.Scalar();

				if (const auto yml_type = patch.second.Type(); yml_type != YAML::NodeType::Map)
				{
					patch_log.error("Error loading patch %s: expected Map, found %s (hash: %s, file: %s)", description, yml_type, hash, path);
					continue;
				}

				for (const auto title_node : patch.second)
				{
					const auto& title = title_node.first.Scalar();

					if (const auto yml_type = title_node.second.Type(); yml_type != YAML::NodeType::Map)
					{
						patch_log.error("Error loading %s: expected Map, found %s (description: %s, hash: %s, file: %s)", title, yml_type, description, hash, path);
						continue;
					}

					for (const auto serial_node : title_node.second)
					{
						const auto& serial = serial_node.first.Scalar();

						if (const auto yml_type = serial_node.second.Type(); yml_type != YAML::NodeType::Map)
						{
							patch_log.error("Error loading %s: expected Map, found %s (title: %s, description: %s, hash: %s, file: %s)", serial, yml_type, title, description, hash, path);
							continue;
						}

						for (const auto app_version_node : serial_node.second)
						{
							const auto& app_version = app_version_node.first.Scalar();
							const bool enabled = app_version_node.second.as<bool>(false);
							config_map[hash].patch_info_map[description].titles[title][serial][app_version] = enabled;
						}
					}
				}
			}
		}
	}

	return config_map;
}
