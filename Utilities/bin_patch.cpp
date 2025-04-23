#include "bin_patch.h"
#include "ppu_patch.h"
#include "File.h"
#include "Config.h"
#include "version.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/VFS.h"

#include "util/types.hpp"
#include "util/asm.hpp"

#include <charconv>
#include <regex>
#include <vector>

LOG_CHANNEL(patch_log, "PAT");

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
void fmt_class_string<patch_configurable_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](patch_configurable_type value)
	{
		switch (value)
		{
		case patch_configurable_type::double_range: return "double_range";
		case patch_configurable_type::double_enum: return "double_enum";
		case patch_configurable_type::long_range: return "long_range";
		case patch_configurable_type::long_enum: return "long_enum";
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
		case patch_type::alloc: return "alloc";
		case patch_type::code_alloc: return "calloc";
		case patch_type::jump: return "jump";
		case patch_type::jump_link: return "jumpl";
		case patch_type::jump_func: return "jumpf";
		case patch_type::load: return "load";
		case patch_type::byte: return "byte";
		case patch_type::le16: return "le16";
		case patch_type::le32: return "le32";
		case patch_type::le64: return "le64";
		case patch_type::bef32: return "bef32";
		case patch_type::bef64: return "bef64";
		case patch_type::be16: return "be16";
		case patch_type::be32: return "be32";
		case patch_type::bd32: return "bd32";
		case patch_type::be64: return "be64";
		case patch_type::bd64: return "bd64";
		case patch_type::lef32: return "lef32";
		case patch_type::lef64: return "lef64";
		case patch_type::bp_exec: return "bpex";
		case patch_type::utf8: return "utf8";
		case patch_type::c_utf8: return "cutf8";
		case patch_type::move_file: return "move_file";
		case patch_type::hide_file: return "hide_file";
		}

		return unknown;
	});
}

void patch_engine::patch_config_value::set_and_check_value(f64 new_value, std::string_view name)
{
	switch (type)
	{
	case patch_configurable_type::double_enum:
	case patch_configurable_type::long_enum:
	{
		if (std::none_of(allowed_values.begin(), allowed_values.end(), [&new_value](const patch_allowed_value& allowed_value){ return allowed_value.value == new_value; }))
		{
			patch_log.error("Can't set configurable enumerated value '%s' to %f. Using default value %f", name, new_value, value);
			return;
		}
		break;
	}
	case patch_configurable_type::double_range:
	case patch_configurable_type::long_range:
	{
		if (new_value < min || new_value > max)
		{
			patch_log.error("Can't set configurable range value '%s' to %f. Using default value %f", name, new_value, value);
			return;
		}
		break;
	}
	}

	value = new_value;
}

patch_engine::patch_engine()
{
}

std::string patch_engine::get_patch_config_path()
{
	const std::string config_dir = fs::get_config_dir(true);
	const std::string patch_path = config_dir + "patch_config.yml";
#ifdef _WIN32
	if (!fs::create_path(config_dir))
	{
		patch_log.error("Could not create path: %s (%s)", patch_path, fs::g_tls_error);
	}
#endif
	return patch_path;
}

std::string patch_engine::get_patches_path()
{
	return fs::get_config_dir() + "patches/";
}

std::string patch_engine::get_imported_patch_path()
{
	return get_patches_path() + "imported_patch.yml";
}

static void append_log_message(std::stringstream* log_messages, std::string_view message, const logs::message* channel = nullptr)
{
	if (channel)
	{
		channel->operator()("%s", message);
	}

	if (log_messages && !message.empty())
	{
		*log_messages << message << std::endl;
	}
};

bool patch_engine::load(patch_map& patches_map, const std::string& path, std::string content, bool importing, std::stringstream* log_messages)
{
	if (content.empty())
	{
		// Load patch file
		fs::file file{path};

		if (!file)
		{
			// Do nothing
			return true;
		}

		content = file.to_string();
	}

	// Interpret yaml nodes
	auto [root, error] = yaml_load(content);

	if (!error.empty() || !root)
	{
		append_log_message(log_messages, "Fatal Error: Failed to load file!");
		patch_log.fatal("Failed to load patch file %s:\n%s", path, error);
		return false;
	}

	// Load patch config to determine which patches are enabled
	patch_map patch_config;

	if (!importing)
	{
		patch_config = load_config();
	}

	std::string version;

	if (const auto version_node = root[patch_key::version])
	{
		version = version_node.Scalar();

		if (version != patch_engine_version)
		{
			append_log_message(log_messages, fmt::format("Error: File version %s does not match patch engine target version %s (location: %s, file: %s)", version, patch_engine_version, get_yaml_node_location(version_node), path), &patch_log.error);
			return false;
		}

		// We don't need the Version node in local memory anymore
		root.remove(patch_key::version);
	}
	else
	{
		append_log_message(log_messages, fmt::format("Error: No '%s' entry found. Patch engine version = %s (file: %s)", patch_key::version, patch_engine_version, path), &patch_log.error);
		return false;
	}

	bool is_valid = true;

	// Go through each main key in the file
	for (auto pair : root)
	{
		const std::string& main_key = pair.first.Scalar();

		if (main_key.empty())
		{
			append_log_message(log_messages, fmt::format("Error: Skipping empty key (location: %s, file: %s)", get_yaml_node_location(pair.first), path), &patch_log.error);
			is_valid = false;
			continue;
		}

		if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
		{
			append_log_message(log_messages, fmt::format("Error: Skipping key %s: expected Map, found %s (location: %s, file: %s)", main_key, yml_type, get_yaml_node_location(pair.second), path), &patch_log.error);
			is_valid = false;
			continue;
		}

		// Skip Anchors
		if (main_key == patch_key::anchors)
		{
			continue;
		}

		// Find or create an entry matching the key/hash in our map
		patch_container& container = patches_map[main_key];
		container.hash    = main_key;
		container.version = version;

		// Go through each patch
		for (auto patches_entry : pair.second)
		{
			// Each key in "Patches" is also the patch description
			const std::string& description = patches_entry.first.Scalar();

			if (description.empty())
			{
				append_log_message(log_messages, fmt::format("Error: Empty patch name (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(patches_entry.first), path), &patch_log.error);
				is_valid = false;
				continue;
			}

			// Compile patch information

			if (const auto yml_type = patches_entry.second.Type(); yml_type != YAML::NodeType::Map)
			{
				append_log_message(log_messages, fmt::format("Error: Skipping Patch key %s: expected Map, found %s (key: %s, location: %s, file: %s)", description, yml_type, main_key, get_yaml_node_location(patches_entry.second), path), &patch_log.error);
				is_valid = false;
				continue;
			}

			struct patch_info info {};
			info.description = description;
			info.hash        = main_key;
			info.version     = version;
			info.source_path = path;

			if (const auto games_node = patches_entry.second[patch_key::games])
			{
				if (const auto yml_type = games_node.Type(); yml_type != YAML::NodeType::Map)
				{
					append_log_message(log_messages, fmt::format("Error: Skipping Games key: expected Map, found %s (patch: %s, key: %s, location: %s, file: %s)", yml_type, description, main_key, get_yaml_node_location(games_node), path), &patch_log.error);
					is_valid = false;
					continue;
				}

				for (const auto game_node : games_node)
				{
					const std::string& title = game_node.first.Scalar();

					if (title.empty())
					{
						append_log_message(log_messages, fmt::format("Error: Empty game title (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(game_node), path), &patch_log.error);
						is_valid = false;
						continue;
					}

					if (const auto yml_type = game_node.second.Type(); yml_type != YAML::NodeType::Map)
					{
						append_log_message(log_messages, fmt::format("Error: Skipping game %s: expected Map, found %s (patch: %s, key: %s, location: %s, file: %s)", title, yml_type, description, main_key, get_yaml_node_location(game_node), path), &patch_log.error);
						is_valid = false;
						continue;
					}

					const bool title_is_all_key = title == patch_key::all;

					for (const auto serial_node : game_node.second)
					{
						const std::string& serial = serial_node.first.Scalar();

						if (serial.empty())
						{
							append_log_message(log_messages, fmt::format("Error: Using empty serial (title: %s, patch: %s, key: %s, location: %s, file: %s)", title, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
							is_valid = false;
							continue;
						}

						if (serial == patch_key::all)
						{
							if (!title_is_all_key)
							{
								append_log_message(log_messages, fmt::format("Error: Using '%s' as serial is not allowed for titles other than '%s' (title: %s, patch: %s, key: %s, location: %s, file: %s)", patch_key::all, patch_key::all, title, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
								is_valid = false;
								continue;
							}
						}
						else if (title_is_all_key)
						{
							append_log_message(log_messages, fmt::format("Error: Only '%s' is allowed as serial if the title is '%s' (serial: %s, patch: %s, key: %s, location: %s, file: %s)", patch_key::all, patch_key::all, serial, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
							is_valid = false;
							continue;
						}
						else if (serial.size() != 9 || !std::all_of(serial.begin(), serial.end(), [](char c) { return std::isalnum(c); }))
						{
							append_log_message(log_messages, fmt::format("Error: Serial '%s' invalid (patch: %s, key: %s, location: %s, file: %s)", serial, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
							is_valid = false;
							continue;
						}

						if (const auto yml_type = serial_node.second.Type(); yml_type != YAML::NodeType::Sequence)
						{
							append_log_message(log_messages, fmt::format("Error: Skipping %s: expected Sequence, found %s (title: %s, patch: %s, key: %s, location: %s, file: %s)", serial, title, yml_type, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
							is_valid = false;
							continue;
						}

						patch_app_versions app_versions;

						for (const auto version : serial_node.second)
						{
							const std::string& app_version = version.Scalar();

							static const std::regex app_ver_regexp("^([0-9]{2}\\.[0-9]{2})$");

							if (app_version != patch_key::all && (app_version.size() != 5 || !std::regex_match(app_version, app_ver_regexp)))
							{
								append_log_message(log_messages, fmt::format("Error: Skipping invalid app version '%s' (title: %s, serial: %s, patch: %s, key: %s, location: %s, file: %s)", app_version, title, serial, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
								is_valid = false;
								continue;
							}

							if (app_versions.contains(app_version))
							{
								append_log_message(log_messages, fmt::format("Error: Skipping duplicate app version '%s' (title: %s, serial: %s, patch: %s, key: %s, location: %s, file: %s)", app_version, title, serial, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
								is_valid = false;
								continue;
							}

							// Get this patch's config values
							const patch_config_values& config_values = patch_config[main_key].patch_info_map[description].titles[title][serial][app_version];

							app_versions[app_version] = config_values;
						}

						if (app_versions.empty())
						{
							append_log_message(log_messages, fmt::format("Error: Skipping %s: empty Sequence (title: %s, patch: %s, key: %s, location: %s, file: %s)", serial, title, description, main_key, get_yaml_node_location(serial_node), path), &patch_log.error);
							is_valid = false;
						}
						else
						{
							info.titles[title][serial] = std::move(app_versions);
						}
					}
				}
			}

			if (const auto author_node = patches_entry.second[patch_key::author])
			{
				info.author = author_node.Scalar();
			}

			if (const auto patch_version_node = patches_entry.second[patch_key::patch_version])
			{
				info.patch_version = patch_version_node.Scalar();
			}

			if (const auto notes_node = patches_entry.second[patch_key::notes])
			{
				if (notes_node.IsSequence())
				{
					for (const auto note : notes_node)
					{
						if (note)
						{
							if (note.IsScalar())
							{
								info.notes += note.Scalar();
							}
							else
							{
								append_log_message(log_messages, fmt::format("Error: Skipping sequenced Note (patch: %s, key: %s, location: %s, file: %s)", description, main_key, get_yaml_node_location(note), path), &patch_log.error);
								is_valid = false;
							}
						}
						else
						{
							append_log_message(log_messages, fmt::format("Error: Skipping sequenced Note (patch: %s, key: %s, location: %s, file: %s)", description, main_key, get_yaml_node_location(notes_node), path), &patch_log.error);
							is_valid = false;
						}
					}
				}
				else
				{
					info.notes = notes_node.Scalar();
				}
			}

			if (const auto patch_group_node = patches_entry.second[patch_key::group])
			{
				info.patch_group = patch_group_node.Scalar();
			}

			if (const auto config_values_node = patches_entry.second[patch_key::config_values])
			{
				if (const auto yml_type = config_values_node.Type(); yml_type != YAML::NodeType::Map || config_values_node.size() == 0)
				{
					append_log_message(log_messages, fmt::format("Error: Skipping configurable values: expected Map, found %s (patch: %s, key: %s, location: %s, file: %s)", yml_type, description, main_key, get_yaml_node_location(config_values_node), path), &patch_log.error);
					is_valid = false;
				}
				else
				{
					for (const auto config_value_node : config_values_node)
					{
						const std::string& value_key = config_value_node.first.Scalar();

						if (const auto yml_type = config_value_node.second.Type(); yml_type != YAML::NodeType::Map)
						{
							append_log_message(log_messages, fmt::format("Error: Skipping configurable value %s: expected Map, found %s (patch: %s, key: %s, location: %s, file: %s)", value_key, yml_type, description, main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
							is_valid = false;
						}
						else
						{
							patch_config_value& config_value = info.default_config_values[value_key];

							if (const auto config_value_type_node = config_value_node.second[patch_key::type]; config_value_type_node && config_value_type_node.IsScalar())
							{
								const std::string& str_type = config_value_type_node.Scalar();
								bool is_valid_type = false;

								for (patch_configurable_type type : { patch_configurable_type::double_range, patch_configurable_type::double_enum, patch_configurable_type::long_range, patch_configurable_type::long_enum })
								{
									if (str_type == fmt::format("%s", type))
									{
										config_value.type = type;
										is_valid_type = true;
										break;
									}
								}

								if (is_valid_type)
								{
									const auto get_and_check_config_value = [&](const YAML::Node& node)
									{
										std::string err;
										f64 val{};

										switch (config_value.type)
										{
										case patch_configurable_type::double_range:
										case patch_configurable_type::double_enum:
											val = get_yaml_node_value<f64>(node, err);
											break;
										case patch_configurable_type::long_range:
										case patch_configurable_type::long_enum:
											val = static_cast<f64>(get_yaml_node_value<s64>(node, err));
											break;
										}

										if (!err.empty())
										{
											append_log_message(log_messages, fmt::format("Error: Invalid data type found in configurable value: %s (key: %s, location: %s, file: %s)", err, main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}

										return val;
									};

									if (const auto config_value_value_node = config_value_node.second[patch_key::value]; config_value_value_node && config_value_value_node.IsScalar())
									{
										config_value.value = get_and_check_config_value(config_value_value_node);
									}
									else
									{
										append_log_message(log_messages, fmt::format("Error: Invalid or missing configurable value (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
										is_valid = false;
									}

									switch (config_value.type)
									{
									case patch_configurable_type::double_range:
									case patch_configurable_type::long_range:
									{
										if (const auto config_value_min_node = config_value_node.second[patch_key::min]; config_value_min_node && config_value_min_node.IsScalar())
										{
											config_value.min = get_and_check_config_value(config_value_min_node);
										}
										else
										{
											append_log_message(log_messages, fmt::format("Error: Invalid or missing configurable min (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}

										if (const auto config_value_max_node = config_value_node.second[patch_key::max]; config_value_max_node && config_value_max_node.IsScalar())
										{
											config_value.max = get_and_check_config_value(config_value_max_node);
										}
										else
										{
											append_log_message(log_messages, fmt::format("Error: Invalid or missing configurable max (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}

										if (config_value.min >= config_value.max)
										{
											append_log_message(log_messages, fmt::format("Error: Configurable max has to be larger than configurable min (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}

										if (config_value.value < config_value.min || config_value.value > config_value.max)
										{
											append_log_message(log_messages, fmt::format("Error: Configurable value out of range (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}
										break;
									}
									case patch_configurable_type::double_enum:
									case patch_configurable_type::long_enum:
									{
										if (const auto config_value_allowed_values_node = config_value_node.second[patch_key::allowed_values]; config_value_allowed_values_node && config_value_allowed_values_node.IsMap())
										{
											config_value.allowed_values.clear();

											for (const auto allowed_value : config_value_allowed_values_node)
											{
												if (allowed_value.second && allowed_value.second.IsScalar())
												{
													patch_allowed_value new_allowed_value{};
													new_allowed_value.label = allowed_value.first.Scalar();
													new_allowed_value.value = get_and_check_config_value(allowed_value.second);

													if (std::any_of(config_value.allowed_values.begin(), config_value.allowed_values.end(), [&new_allowed_value](const patch_allowed_value& other){ return new_allowed_value.value == other.value || new_allowed_value.label == other.label; }))
													{
														append_log_message(log_messages, fmt::format("Error: Skipping configurable allowed value. Another entry with the same label or value already exists. (patch: %s, key: %s, location: %s, file: %s)", description, main_key, get_yaml_node_location(allowed_value), path), &patch_log.error);
														is_valid = false;
													}
													else
													{
														config_value.allowed_values.push_back(std::move(new_allowed_value));
													}
												}
												else
												{
													append_log_message(log_messages, fmt::format("Error: Skipping configurable allowed value (patch: %s, key: %s, location: %s, file: %s)", description, main_key, get_yaml_node_location(allowed_value), path), &patch_log.error);
													is_valid = false;
												}
											}

											if (config_value.allowed_values.size() < 2)
											{
												append_log_message(log_messages, fmt::format("Error: Configurable allowed values need at least 2 entries (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_allowed_values_node), path), &patch_log.error);
												is_valid = false;
											}

											if (std::none_of(config_value.allowed_values.begin(), config_value.allowed_values.end(), [&config_value](const patch_allowed_value& other){ return other.value == config_value.value; }))
											{
												append_log_message(log_messages, fmt::format("Error: Configurable value was not found in allowed values (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_allowed_values_node), path), &patch_log.error);
												is_valid = false;
											}
										}
										else
										{
											append_log_message(log_messages, fmt::format("Error: Invalid or missing configurable allowed values (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
											is_valid = false;
										}
										break;
									}
									}
								}
								else
								{
									append_log_message(log_messages, fmt::format("Error: Invalid configurable type (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_type_node), path), &patch_log.error);
									is_valid = false;
								}
							}
							else
							{
								append_log_message(log_messages, fmt::format("Error: Invalid or missing configurable type (key: %s, location: %s, file: %s)", main_key, get_yaml_node_location(config_value_node), path), &patch_log.error);
								is_valid = false;
							}
						}
					}
				}
			}

			if (const auto patch_node = patches_entry.second[patch_key::patch])
			{
				if (!read_patch_node(info, patch_node, root, path, log_messages))
				{
					for (const auto& it : patches_entry.second)
					{
						if (it.first.Scalar() == patch_key::patch)
						{
							append_log_message(log_messages, fmt::format("Skipping invalid patch node %s: (key: %s, location: %s, file: %s)", info.description, main_key, get_yaml_node_location(it.first), path), &patch_log.error);
							break;
						}
					}
					is_valid = false;
				}
			}

			// Skip this patch if a higher patch version already exists
			if (container.patch_info_map.contains(description))
			{
				bool ok;
				const std::string& existing_version = container.patch_info_map[description].patch_version;
				const bool version_is_bigger = utils::compare_versions(info.patch_version, existing_version, ok) > 0;

				if (!ok || !version_is_bigger)
				{
					append_log_message(log_messages, fmt::format("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s (in file %s)", info.patch_version, existing_version, main_key, description, path), &patch_log.warning);
					continue;
				}

				if (!importing)
				{
					patch_log.warning("A lower patch version was found ('%s' vs '%s') for %s: %s (in file %s)", existing_version, info.patch_version, main_key, description,  container.patch_info_map[description].source_path);
				}
			}

			// Insert patch information
			container.patch_info_map[description] = std::move(info);
		}
	}

	return is_valid;
}

patch_type patch_engine::get_patch_type(std::string_view text)
{
	u64 type_val = 0;

	if (!cfg::try_to_enum_value(&type_val, &fmt_class_string<patch_type>::format, text))
	{
		return patch_type::invalid;
	}

	return static_cast<patch_type>(type_val);
}

patch_type patch_engine::get_patch_type(YAML::Node node)
{
	if (!node || !node.IsScalar())
	{
		return patch_type::invalid;
	}

	return get_patch_type(node.Scalar());
}

bool patch_engine::add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root, std::string_view path, std::stringstream* log_messages)
{
	if (!node || !node.IsSequence())
	{
		append_log_message(log_messages, fmt::format("Skipping invalid patch node %s. (key: %s, location: %s, file: %s)", info.description, info.hash, get_yaml_node_location(node), path), &patch_log.error);
		return false;
	}

	const auto type_node  = node[0];
	const auto addr_node  = node[1];
	const auto value_node = node[2];

	const auto type = get_patch_type(type_node);

	if (type == patch_type::invalid)
	{
		const auto type_str = type_node && type_node.IsScalar() ? type_node.Scalar() : "";
		append_log_message(log_messages, fmt::format("Skipping patch node %s: type '%s' is invalid. (key: %s, location: %s, file: %s)", info.description, type_str, info.hash, get_yaml_node_location(node), path), &patch_log.error);
		return false;
	}

	if (type == patch_type::load)
	{
		// Special syntax: anchors (named sequence)

		// Check if the anchor was resolved.
		if (const auto yml_type = addr_node.Type(); yml_type != YAML::NodeType::Sequence)
		{
			append_log_message(log_messages, fmt::format("Skipping patch node %s: expected Sequence, found %s (key: %s, location: %s, file: %s)", info.description, yml_type, info.hash, get_yaml_node_location(node), path), &patch_log.error);
			return false;
		}

		// Address modifier (optional)
		const u32 mod = value_node.as<u32>(0);

		bool is_valid = true;

		for (const auto& item : addr_node)
		{
			if (!add_patch_data(item, info, mod, root, path, log_messages))
			{
				is_valid = false;
			}
		}

		return is_valid;
	}

	if (const auto yml_type = value_node.Type(); yml_type != YAML::NodeType::Scalar)
	{
		append_log_message(log_messages, fmt::format("Skipping patch node %s. Value element has wrong type %s. (key: %s, location: %s, file: %s)", info.description, yml_type, info.hash, get_yaml_node_location(node), path), &patch_log.error);
		return false;
	}

	if (patch_type_uses_hex_offset(type) && !addr_node.Scalar().starts_with("0x"))
	{
		append_log_message(log_messages, fmt::format("Skipping patch node %s. Address element has wrong format %s. (key: %s, location: %s, file: %s)", info.description, addr_node.Scalar(), info.hash, get_yaml_node_location(node), path), &patch_log.error);
		return false;
	}

	patch_data p_data{};
	p_data.type            = type;
	p_data.offset          = addr_node.as<u32>(0) + modifier;
	p_data.original_offset = addr_node.Scalar();
	p_data.original_value  = value_node.Scalar();

	const bool is_config_value = info.default_config_values.contains(p_data.original_value);
	const patch_config_value config_value = is_config_value ? ::at32(info.default_config_values, p_data.original_value) : patch_config_value{};

	std::string error_message;

	// Validate offset
	switch (p_data.type)
	{
	case patch_type::move_file:
	case patch_type::hide_file:
		break;
	default:
	{
		[[maybe_unused]] const u32 offset = get_yaml_node_value<u32>(addr_node, error_message);
		if (!error_message.empty())
		{
			error_message = fmt::format("Skipping patch data entry: [ %s, 0x%.8x, %s ] (key: %s, location: %s, file: %s) Invalid patch offset '%s' (not a valid u32 or overflow)",
				p_data.type, p_data.offset, p_data.original_value.empty() ? "?" : p_data.original_value, info.hash, get_yaml_node_location(node), path, p_data.original_offset);
			append_log_message(log_messages, error_message, &patch_log.error);
			return false;
		}
		if ((0xFFFFFFFF - modifier) < p_data.offset)
		{
			error_message = fmt::format("Skipping patch data entry: [ %s, 0x%.8x, %s ] (key: %s, location: %s, file: %s) Invalid combination of patch offset 0x%.8x and modifier 0x%.8x (overflow)",
				p_data.type, p_data.offset, p_data.original_value.empty() ? "?" : p_data.original_value, info.hash, get_yaml_node_location(node), path, p_data.offset, modifier);
			append_log_message(log_messages, error_message, &patch_log.error);
			return false;
		}
		break;
	}
	}

	// Validate value
	const auto get_node_value = [&]<typename U, typename S>(U, S) // Add unused params. The lambda doesn't compile otherwise.
	{
		p_data.value.long_value = is_config_value ? static_cast<U>(config_value.value) : get_yaml_node_value<U>(value_node, error_message);

		if (error_message.find("bad conversion") != std::string::npos)
		{
			error_message.clear();
			p_data.value.long_value = get_yaml_node_value<S>(value_node, error_message);
		}
	};

	switch (p_data.type)
	{
	case patch_type::invalid:
	case patch_type::load:
	{
		fmt::throw_exception("Unreachable patch type: %s", p_data.type);
	}
	case patch_type::bp_exec:
	case patch_type::utf8:
	case patch_type::c_utf8:
	case patch_type::jump_func:
	case patch_type::move_file:
	case patch_type::hide_file:
	{
		break;
	}
	case patch_type::bef32:
	case patch_type::lef32:
	{
		p_data.value.double_value = is_config_value ? config_value.value : get_yaml_node_value<f32>(value_node, error_message);
		break;
	}
	case patch_type::bef64:
	case patch_type::lef64:
	{
		p_data.value.double_value = is_config_value ? config_value.value : get_yaml_node_value<f64>(value_node, error_message);
		break;
	}
	case patch_type::byte:
	{
		get_node_value(u8{}, s8{});
		break;
	}
	case patch_type::le16:
	case patch_type::be16:
	{
		get_node_value(u16{}, s16{});
		break;
	}
	case patch_type::le32:
	case patch_type::be32:
	case patch_type::bd32:
	{
		get_node_value(u32{}, s32{});
		break;
	}
	case patch_type::alloc:
	case patch_type::code_alloc:
	case patch_type::jump:
	case patch_type::jump_link:
	case patch_type::le64:
	case patch_type::be64:
	case patch_type::bd64:
	{
		get_node_value(u64{}, s64{});
		break;
	}
	}

	if (!error_message.empty())
	{
		error_message = fmt::format("Skipping patch data entry: [ %s, 0x%.8x, %s ] (key: %s, location: %s, file: %s) %s",
			p_data.type, p_data.offset, p_data.original_value.empty() ? "?" : p_data.original_value, info.hash, get_yaml_node_location(node), path, error_message);
		append_log_message(log_messages, error_message, &patch_log.error);
		return false;
	}

	info.data_list.emplace_back(std::move(p_data));

	return true;
}

bool patch_engine::read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root, std::string_view path, std::stringstream* log_messages)
{
	if (!node)
	{
		append_log_message(log_messages, fmt::format("Skipping invalid patch node %s. (key: %s, location: %s)", info.description, info.hash, get_yaml_node_location(node)), &patch_log.error);
		return false;
	}

	if (const auto yml_type = node.Type(); yml_type != YAML::NodeType::Sequence)
	{
		append_log_message(log_messages, fmt::format("Skipping patch node %s: expected Sequence, found %s (key: %s, location: %s)", info.description, yml_type, info.hash, get_yaml_node_location(node)), &patch_log.error);
		return false;
	}

	bool is_valid = true;

	for (auto patch : node)
	{
		if (!add_patch_data(patch, info, 0, root, path, log_messages))
		{
			is_valid = false;
		}
	}

	return is_valid;
}

void patch_engine::append_global_patches()
{
	// Regular patch.yml
	load(m_map, get_patches_path() + "patch.yml");

	// Imported patch.yml
	load(m_map, get_imported_patch_path());
}

void patch_engine::append_title_patches(std::string_view title_id)
{
	if (title_id.empty())
	{
		return;
	}

	// Regular patch.yml
	load(m_map, fmt::format("%s%s_patch.yml", get_patches_path(), title_id));
}

void unmap_vm_area(std::shared_ptr<vm::block_t>& ptr)
{
	if (ptr && ptr->flags & (1ull << 62))
	{
		vm::unmap(0, true, &ptr);
	}
}

// Returns old 'applied' size
static usz apply_modification(std::vector<u32>& applied, patch_engine::patch_info& patch, std::function<u8*(u32, u32)> mem_translate, u32 filesz, u32 min_addr)
{
	const usz old_applied_size = applied.size();

	// Update configurable values
	for (const auto& [key, config_value] : patch.actual_config_values)
	{
		for (usz i = 0; i < patch.data_list.size(); i++)
		{
			patch_engine::patch_data& p = ::at32(patch.data_list, i);

			if (p.original_value == key)
			{
				switch (p.type)
				{
				case patch_type::bef32:
				case patch_type::lef32:
				case patch_type::bef64:
				case patch_type::lef64:
				{
					p.value.double_value = config_value.value;
					patch_log.notice("Using configurable value (key='%s', value=%f, index=%d, hash='%s', description='%s', author='%s', patch_version='%s', file_version='%s')",
						key, p.value.double_value, i, patch.hash, patch.description, patch.author, patch.patch_version, patch.version);
					break;
				}
				default:
				{
					p.value.long_value = static_cast<u64>(config_value.value);
					patch_log.notice("Using configurable value (key='%s', value=0x%x=%d, index=%d, hash='%s', description='%s', author='%s', patch_version='%s', file_version='%s')",
						key, p.value.long_value, p.value.long_value, i, patch.hash, patch.description, patch.author, patch.patch_version, patch.version);
					break;
				}
				}
			}
		}
	}

	for (const patch_engine::patch_data& p : patch.data_list)
	{
		if (p.type != patch_type::alloc) continue;

		// Do not allow null address or if resultant ptr is not a VM ptr
		if (const u32 alloc_at = (p.offset & -4096); alloc_at >> 16)
		{
			const u32 alloc_size = utils::align(static_cast<u32>(p.value.long_value) + alloc_at % 4096, 4096);

			// Allocate map if needed, if allocated flags will indicate that bit 62 is set (unique identifier)
			auto alloc_map = vm::reserve_map(vm::any, alloc_at & -0x10000, utils::align(alloc_size, 0x10000), vm::page_size_64k | (1ull << 62));

			u64 flags = vm::alloc_unwritable;

			switch (p.offset % patch_engine::mem_protection::mask)
			{
			case patch_engine::mem_protection::wx: flags = vm::alloc_executable; break;
			case patch_engine::mem_protection::ro: break;
			case patch_engine::mem_protection::rx: flags |= vm::alloc_executable; break;
			case patch_engine::mem_protection::rw: flags &= ~vm::alloc_unwritable; break;
			default: ensure(false);
			}

			if (alloc_map)
			{
				if (alloc_map->falloc(alloc_at, alloc_size, nullptr, flags))
				{
					if (vm::check_addr(alloc_at, vm::page_1m_size))
					{
						p.alloc_addr = alloc_at & -0x100000;
					}
					else if (vm::check_addr(alloc_at, vm::page_64k_size))
					{
						p.alloc_addr = alloc_at & -0x10000;
					}
					else
					{
						p.alloc_addr = alloc_at & -0x1000;
					}

					if (flags & vm::alloc_executable)
					{
						ppu_register_range(alloc_at, alloc_size);
					}

					applied.push_back(::narrow<u32>(&p - patch.data_list.data())); // Remember index in case of failure to allocate any memory
					continue;
				}

				// Revert if allocated map before failure
				unmap_vm_area(alloc_map);
			}
		}

		// Revert in case of failure
		std::for_each(applied.begin() + old_applied_size, applied.end(), [&](u32 index)
		{
			const u32 addr = std::exchange(patch.data_list[index].alloc_addr, 0);

			vm::dealloc(addr);

			auto alloc_map = vm::get(vm::any, addr);
			unmap_vm_area(alloc_map);
		});

		applied.resize(old_applied_size);
		return old_applied_size;
	}

	// Fixup values from before
	std::fill(applied.begin() + old_applied_size, applied.end(), u32{umax});

	u32 relocate_instructions_at = 0;

	for (const patch_engine::patch_data& p : patch.data_list)
	{
		u32 offset = p.offset;

		if (relocate_instructions_at && vm::read32(relocate_instructions_at) != 0x6000'0000u)
		{
			// No longer points a NOP to be filled, meaning we ran out of instructions
			relocate_instructions_at = 0;
		}

		u32 memory_size = 0;

		switch (p.type)
		{
		case patch_type::invalid:
		case patch_type::load:
		{
			// Invalid in this context
			break;
		}
		case patch_type::alloc:
		{
			// Applied before
			memory_size = 0;
			continue;
		}
		case patch_type::byte:
		{
			memory_size = sizeof(u8);
			break;
		}
		case patch_type::le16:
		case patch_type::be16:
		{
			memory_size = sizeof(u16);
			break;
		}
		case patch_type::code_alloc:
		case patch_type::jump:
		case patch_type::jump_link:
		case patch_type::jump_func:
		case patch_type::bp_exec:
		case patch_type::le32:
		case patch_type::lef32:
		case patch_type::bd32:
		case patch_type::be32:
		case patch_type::bef32:
		{
			memory_size = sizeof(u32);
			break;
		}
		case patch_type::lef64:
		case patch_type::le64:
		case patch_type::bd64:
		case patch_type::be64:
		case patch_type::bef64:
		{
			memory_size = sizeof(u64);
			break;
		}
		case patch_type::utf8:
		{
			memory_size = ::size32(p.original_value);
			break;
		}
		case patch_type::c_utf8:
		{
			memory_size = utils::add_saturate<u32>(::size32(p.original_value), 1);
			break;
		}
		case patch_type::move_file:
		case patch_type::hide_file:
		{
			memory_size = 0;
			break;
		}
		}

		if (memory_size != 0 && !relocate_instructions_at && (filesz < memory_size || offset < min_addr || offset - min_addr > filesz - memory_size))
		{
			// This patch is out of range for this segment
			continue;
		}

		offset -= min_addr;

		auto ptr = mem_translate(offset, memory_size);

		if (!ptr && memory_size != 0 && !relocate_instructions_at)
		{
			// Memory translation failed
			continue;
		}

		if (relocate_instructions_at)
		{
			offset = relocate_instructions_at;
			ptr = vm::get_super_ptr<u8>(relocate_instructions_at);
			relocate_instructions_at += 4; // Advance to the next instruction on dynamic memory
		}

		u32 resval = umax;

		switch (p.type)
		{
		case patch_type::invalid:
		case patch_type::load:
		{
			// Invalid in this context
			continue;
		}
		case patch_type::alloc:
		{
			// Applied before
			continue;
		}
		case patch_type::code_alloc:
		{
			const u32 out_branch = vm::try_get_addr(relocate_instructions_at ? vm::get_super_ptr<u8>(offset & -4) : mem_translate(offset & -4, 4)).first;

			// Allow only if points to a PPU executable instruction
			if (out_branch < 0x10000 || out_branch >= 0x4000'0000 || !vm::check_addr<4>(out_branch, vm::page_executable))
			{
				continue;
			}

			const u32 alloc_size = utils::align(static_cast<u32>(p.value.long_value + 1) * 4, 0x10000);

			// Check if should maybe reuse previous code cave allocation (0 size)
			if (alloc_size - 4 != 0)
			{
				// Nope
				relocate_instructions_at = 0;
			}

			// Always executable
			u64 flags = vm::alloc_executable | vm::alloc_unwritable;

			switch (p.offset & patch_engine::mem_protection::mask)
			{
			case patch_engine::mem_protection::rw:
			case patch_engine::mem_protection::wx:
			{
				flags &= ~vm::alloc_unwritable;
				break;
			}
			case patch_engine::mem_protection::ro:
			case patch_engine::mem_protection::rx:
			{
				break;
			}
			default: ensure(false);
			}

			const auto alloc_map = ensure(vm::get(vm::any, out_branch));

			// Range allowed for absolute branches to operate at
			// It takes into account that we need to put a branch for return at the end of memory space
			const u32 addr = p.alloc_addr = (relocate_instructions_at ? relocate_instructions_at : alloc_map->alloc(alloc_size, nullptr, 0x10000, flags));

			if (!addr)
			{
				patch_log.error("Failed to allocate 0x%x bytes for code (entry=0x%x)", alloc_size, addr, out_branch);
				continue;
			}

			patch_log.success("Allocated 0x%x for code at 0x%x (entry=0x%x)", alloc_size, addr, out_branch);

			// NOP filled
			std::fill_n(vm::get_super_ptr<u32>(addr), p.value.long_value, 0x60000000);

			// Check if already registered by previous code allocation
			if (relocate_instructions_at != addr)
			{
				// Register code
				ppu_register_range(addr, alloc_size);
			}

			resval = out_branch & -4;

			// Write branch to return to code
			if (!ppu_form_branch_to_code(addr + static_cast<u32>(p.value.long_value) * 4, resval + 4))
			{
				patch_log.error("Failed to write return jump at 0x%x", addr + static_cast<u32>(p.value.long_value) * 4);
				ensure(alloc_map->dealloc(addr));
				continue;
			}

			// Write branch to code
			if (!ppu_form_branch_to_code(out_branch, addr))
			{
				patch_log.error("Failed to jump to code cave at 0x%x", out_branch);
				ensure(alloc_map->dealloc(addr));
				continue;
			}

			// Record the insertion point as a faux block.
			g_fxo->need<ppu_patch_block_registry_t>();
			g_fxo->get<ppu_patch_block_registry_t>().block_addresses.insert(resval);

			relocate_instructions_at = addr;
			break;
		}
		case patch_type::jump:
		case patch_type::jump_link:
		{
			const u32 out_branch = vm::try_get_addr(relocate_instructions_at ? vm::get_super_ptr<u8>(offset & -4) : mem_translate(offset & -4, 4)).first;
			const u32 dest = static_cast<u32>(p.value.long_value);

			// Allow only if points to a PPU executable instruction
			if (!ppu_form_branch_to_code(out_branch, dest, p.type == patch_type::jump_link))
			{
				continue;
			}

			resval = out_branch & -4;
			break;
		}
		case patch_type::jump_func:
		{
			const std::string& str = p.original_value;

			const u32 out_branch = vm::try_get_addr(relocate_instructions_at ? vm::get_super_ptr<u8>(offset & -4) : mem_translate(offset & -4, 4)).first;
			const usz sep_pos = str.find_first_of(':');

			// Must contain only a single ':' or none
			// If ':' is found: Left string is the module name, right string is the function name
			// If ':' is not found: The entire string is a direct address of the function's descriptor in hexadecimal
			if (str.size() <= 2 || !sep_pos || sep_pos == str.size() - 1 || sep_pos != str.find_last_of(":"))
			{
				continue;
			}

			const std::string_view func_name{std::string_view(str).substr(sep_pos + 1)};
			u32 id = 0;

			if (func_name.starts_with("0x"sv))
			{
				// Raw hexadecimal-formatted FNID (real function name cannot contain a digit at the start, derived from C/CPP which were used in PS3 development)
				const auto result = std::from_chars(func_name.data() + 2, func_name.data() + func_name.size() - 2, id, 16);

				if (result.ec != std::errc() || str.data() + sep_pos != result.ptr)
				{
					continue;
				}
			}
			else
			{
				if (sep_pos == umax)
				{
					continue;
				}

				// Generate FNID using function name
				id = ppu_generate_id(func_name);
			}

			// Allow only if points to a PPU executable instruction
			// FNID/OPD-address is placed at target
			if (!ppu_form_branch_to_code(out_branch, id, true, true, std::string{str.data(), sep_pos != umax ? sep_pos : 0}))
			{
				continue;
			}

			resval = out_branch & -4;
			break;
		}
		case patch_type::byte:
		{
			*ptr = static_cast<u8>(p.value.long_value);
			break;
		}
		case patch_type::le16:
		{
			le_t<u16> val = static_cast<u16>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::le32:
		{
			le_t<u32> val = static_cast<u32>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::lef32:
		{
			le_t<f32> val = static_cast<f32>(p.value.double_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::le64:
		{
			le_t<u64> val = static_cast<u64>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::lef64:
		{
			le_t<f64> val = p.value.double_value;
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::be16:
		{
			be_t<u16> val = static_cast<u16>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::bd32:
		{
			be_t<u32> val = static_cast<u32>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::be32:
		{
			be_t<u32> val = static_cast<u32>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			if (offset % 4 == 0)
				resval = offset;
			break;
		}
		case patch_type::bef32:
		{
			be_t<f32> val = static_cast<f32>(p.value.double_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::bd64:
		{
			be_t<u64> val = static_cast<u64>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::be64:
		{
			be_t<u64> val = static_cast<u64>(p.value.long_value);
			std::memcpy(ptr, &val, sizeof(val));

			if (offset % 4)
			{
				break;
			}

			resval = offset;
			applied.push_back((offset + 7) & -4); // Two 32-bit locations
			break;
		}
		case patch_type::bef64:
		{
			be_t<f64> val = p.value.double_value;
			std::memcpy(ptr, &val, sizeof(val));
			break;
		}
		case patch_type::bp_exec:
		{
			const u32 exec_addr = vm::try_get_addr(relocate_instructions_at ? vm::get_super_ptr<u8>(offset & -4) : mem_translate(offset & -4, 4)).first;

			if (exec_addr)
			{
				Emu.GetCallbacks().add_breakpoint(exec_addr);
			}

			break;
		}
		case patch_type::utf8:
		{
			std::memcpy(ptr, p.original_value.data(), p.original_value.size());
			break;
		}
		case patch_type::c_utf8:
		{
			std::memcpy(ptr, p.original_value.data(), p.original_value.size() + 1);
			break;
		}
		case patch_type::move_file:
		case patch_type::hide_file:
		{
			const bool is_hide = p.type == patch_type::hide_file;
			std::string original_vfs_path = p.original_offset;
			std::string dest_vfs_path = p.original_value;

			if (original_vfs_path.empty())
			{
				patch_log.error("Failed to patch file: original path is empty", original_vfs_path);
				continue;
			}

			if (!is_hide && dest_vfs_path.empty())
			{
				patch_log.error("Failed to patch file: destination path is empty", dest_vfs_path);
				continue;
			}

			if (!original_vfs_path.starts_with("/dev_"))
			{
				original_vfs_path.insert(0, "/dev_");
			}

			if (!is_hide && !dest_vfs_path.starts_with("/dev_"))
			{
				dest_vfs_path.insert(0, "/dev_");
			}

			const std::string dest_path = is_hide ? fs::get_config_dir() + "delete_this_dir.../delete_this..." : vfs::get(dest_vfs_path);

			if (dest_path.empty())
			{
				patch_log.error("Failed to patch file path at '%s': destination is not mounted", original_vfs_path, dest_vfs_path);
				continue;
			}

			if (!vfs::mount(original_vfs_path, dest_path))
			{
				patch_log.error("Failed to patch file path at '%s': vfs::mount(dest='%s') failed", original_vfs_path, dest_vfs_path);
				continue;
			}

			break;
		}
		}

		// Possibly an executable instruction
		applied.push_back(resval);
	}

	return old_applied_size;
}

void patch_engine::apply(std::vector<u32>& applied_total, const std::string& name, std::function<u8*(u32, u32)> mem_translate, u32 filesz, u32 min_addr)
{
	// applied_total may be non-empty, do not clear it

	if (!m_map.contains(name))
	{
		return;
	}

	const patch_container& container = ::at32(m_map, name);
	const std::string& serial = Emu.GetTitleID();
	const std::string& app_version = Emu.GetAppVersion();

	// Different containers in order to separate the patches
	std::vector<std::shared_ptr<patch_info>> patches_for_this_serial_and_this_version;
	std::vector<std::shared_ptr<patch_info>> patches_for_this_serial_and_all_versions;
	std::vector<std::shared_ptr<patch_info>> patches_for_all_serials_and_this_version;
	std::vector<std::shared_ptr<patch_info>> patches_for_all_serials_and_all_versions;

	// Sort patches into different vectors based on their serial and version
	for (const auto& [description, patch] : container.patch_info_map)
	{
		// Find out if this patch is enabled
		for (const auto& [title, serials] : patch.titles)
		{
			bool is_all_serials = false;
			bool is_all_versions = false;

			std::string found_serial;

			if (serials.contains(serial))
			{
				found_serial = serial;
			}
			else if (serials.contains(patch_key::all))
			{
				found_serial = patch_key::all;
				is_all_serials = true;
			}

			if (found_serial.empty())
			{
				continue;
			}

			const patch_app_versions& app_versions = ::at32(serials, found_serial);
			std::string found_app_version;

			if (app_versions.contains(app_version))
			{
				found_app_version = app_version;
			}
			else if (app_versions.contains(patch_key::all))
			{
				found_app_version = patch_key::all;
				is_all_versions = true;
			}

			if (found_app_version.empty())
			{
				continue;
			}

			const patch_config_values& config_values = ::at32(app_versions, found_app_version);

			// Check if this patch is enabled
			if (config_values.enabled)
			{
				// Make copy of this patch
				std::shared_ptr<patch_info> p_ptr = std::make_shared<patch_info>(patch);

				// Move configurable values to special container for readability
				p_ptr->actual_config_values = p_ptr->default_config_values;

				// Update configurable values
				for (auto& [key, config_value] : config_values.config_values)
				{
					if (p_ptr->actual_config_values.contains(key))
					{
						patch_config_value& actual_config_value = ::at32(p_ptr->actual_config_values, key);
						actual_config_value.set_and_check_value(config_value.value, key);
					}
				}

				// Sort the patch by priority
				if (is_all_serials)
				{
					if (is_all_versions)
					{
						patches_for_all_serials_and_all_versions.emplace_back(p_ptr);
					}
					else
					{
						patches_for_all_serials_and_this_version.emplace_back(p_ptr);
					}
				}
				else if (is_all_versions)
				{
					patches_for_this_serial_and_all_versions.emplace_back(p_ptr);
				}
				else
				{
					patches_for_this_serial_and_this_version.emplace_back(p_ptr);
				}

				break;
			}
		}
	}

	// Sort specific patches after global patches
	// So they will determine the end results
	const auto patch_super_list =
	{
		&patches_for_all_serials_and_all_versions,
		&patches_for_all_serials_and_this_version,
		&patches_for_this_serial_and_all_versions,
		&patches_for_this_serial_and_this_version
	};

	// Filter by patch group (reverse so specific patches will be prioritized over globals)
	for (auto it = std::rbegin(patch_super_list); it != std::rend(patch_super_list); it++)
	{
		for (auto& patch : *it.operator*())
		{
			if (!patch->patch_group.empty())
			{
				if (!m_applied_groups.insert(patch->patch_group).second)
				{
					patch.reset();
				}
			}
		}
	}

	// Apply modifications sequentially
	for (auto patch_list : patch_super_list)
	{
		for (const std::shared_ptr<patch_info>& patch : *patch_list)
		{
			if (patch)
			{
				const usz old_size = apply_modification(applied_total, *patch, mem_translate, filesz, min_addr);

				if (applied_total.size() != old_size)
				{
					patch_log.success("Applied patch (hash='%s', description='%s', author='%s', patch_version='%s', file_version='%s') (<- %u)",
						patch->hash, patch->description, patch->author, patch->patch_version, patch->version, applied_total.size() - old_size);
				}
			}
		}
	}

	// Ensure consistent order
	std::sort(applied_total.begin(), applied_total.end());
}

void patch_engine::unload(const std::string& name)
{
	if (!m_map.contains(name))
	{
		return;
	}

	const patch_container& container = ::at32(m_map, name);

	for (const auto& [description, patch] : container.patch_info_map)
	{
		for (const patch_data& entry : patch.data_list)
		{
			// Deallocate used memory
			if (u32 addr = std::exchange(entry.alloc_addr, 0))
			{
				vm::dealloc(addr);

				auto alloc_map = vm::get(vm::any, addr);
				unmap_vm_area(alloc_map);
			}
		}
	}
}

void patch_engine::save_config(const patch_map& patches_map)
{
	const std::string path = get_patch_config_path();
	patch_log.notice("Saving patch config file %s", path);

	YAML::Emitter out;
	out << YAML::BeginMap;

	// Save values per hash, description, serial and app_version
	patch_map config_map;

	for (const auto& [hash, container] : patches_map)
	{
		for (const auto& [description, patch] : container.patch_info_map)
		{
			for (const auto& [title, serials] : patch.titles)
			{
				for (const auto& [serial, app_versions] : serials)
				{
					for (const auto& [app_version, config_values] : app_versions)
					{
						const bool config_values_dirty = !patch.default_config_values.empty() && !config_values.config_values.empty() && patch.default_config_values != config_values.config_values;

						if (config_values.enabled || config_values_dirty)
						{
							config_map[hash].patch_info_map[description].titles[title][serial][app_version] = config_values;
						}
					}
				}
			}
		}

		if (const auto& patches_to_save = config_map[hash].patch_info_map; !patches_to_save.empty())
		{
			out << hash << YAML::BeginMap;

			for (const auto& [description, patch] : patches_to_save)
			{
				out << description << YAML::BeginMap;

				for (const auto& [title, serials] : patch.titles)
				{
					out << title << YAML::BeginMap;

					for (const auto& [serial, app_versions] : serials)
					{
						out << serial << YAML::BeginMap;

						for (const auto& [app_version, config_values] : app_versions)
						{
							const auto& default_config_values = ::at32(container.patch_info_map, description).default_config_values;
							const bool config_values_dirty = !default_config_values.empty() && !config_values.config_values.empty() && default_config_values != config_values.config_values;

							if (config_values.enabled || config_values_dirty)
							{
								out << app_version << YAML::BeginMap;

								if (config_values.enabled)
								{
									out << patch_key::enabled << config_values.enabled;
								}

								if (config_values_dirty)
								{
									out << patch_key::config_values << YAML::BeginMap;

									for (const auto& [name, config_value] : config_values.config_values)
									{
										out << name << config_value.value;
									}

									out << YAML::EndMap;
								}

								out << YAML::EndMap;
							}
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

	fs::pending_file file(path);

	if (!file.file || file.file.write(out.c_str(), out.size()) < out.size() || !file.commit())
	{
		patch_log.error("Failed to create patch config file %s (error=%s)", path, fs::g_tls_error);
	}
}

static void append_patches(patch_engine::patch_map& existing_patches, const patch_engine::patch_map& new_patches, usz& count, usz& total, std::stringstream* log_messages, std::string_view path)
{
	for (const auto& [hash, new_container] : new_patches)
	{
		total += new_container.patch_info_map.size();

		if (!existing_patches.contains(hash))
		{
			existing_patches[hash] = new_container;
			count += new_container.patch_info_map.size();
			continue;
		}

		patch_engine::patch_container& container = existing_patches[hash];

		for (const auto& [description, new_info] : new_container.patch_info_map)
		{
			if (!container.patch_info_map.contains(description))
			{
				container.patch_info_map[description] = new_info;
				count++;
				continue;
			}

			patch_engine::patch_info& info = container.patch_info_map[description];

			bool ok;
			const bool version_is_bigger = utils::compare_versions(new_info.patch_version, info.patch_version, ok) > 0;

			if (!ok)
			{
				append_log_message(log_messages, fmt::format("Failed to compare patch versions ('%s' vs '%s') for %s: %s (file: %s)", new_info.patch_version, info.patch_version, hash, description, path), &patch_log.error);
				continue;
			}

			if (!version_is_bigger)
			{
				append_log_message(log_messages, fmt::format("A higher or equal patch version already exists ('%s' vs '%s') for %s: %s (file: %s)", new_info.patch_version, info.patch_version, hash, description, path), &patch_log.error);
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
		append_log_message(log_messages, fmt::format("Failed to open patch file %s (%s)", path, fs::g_tls_error), &patch_log.fatal);
		return false;
	}

	YAML::Emitter out;
	out << YAML::BeginMap;
	out << patch_key::version << patch_engine_version;

	for (const auto& [hash, container] : patches)
	{
		if (container.patch_info_map.empty())
		{
			continue;
		}

		out << YAML::Newline << YAML::Newline;
		out << hash << YAML::BeginMap;

		for (const auto& [description, info] : container.patch_info_map)
		{
			out << description << YAML::BeginMap;
			out << patch_key::games << YAML::BeginMap;

			for (const auto& [title, serials] : info.titles)
			{
				out << title << YAML::BeginMap;

				for (const auto& [serial, app_versions] : serials)
				{
					out << serial << YAML::BeginSeq;

					for (const auto& [app_version, patch_config] : app_versions)
					{
						out << app_version;
					}

					out << YAML::EndSeq;
				}

				out << YAML::EndMap;
			}

			out << YAML::EndMap;

			if (!info.author.empty())        out << patch_key::author        << info.author;
			if (!info.patch_version.empty()) out << patch_key::patch_version << info.patch_version;
			if (!info.patch_group.empty())   out << patch_key::group         << info.patch_group;
			if (!info.notes.empty())         out << patch_key::notes         << info.notes;

			if (!info.default_config_values.empty())
			{
				out << patch_key::config_values << YAML::BeginMap;

				for (const auto& [key, config_value] : info.default_config_values)
				{
					out << key << YAML::BeginMap;
					out << patch_key::type << fmt::format("%s", config_value.type);
					out << patch_key::value << config_value.value;

					switch (config_value.type)
					{
					case patch_configurable_type::double_range:
					case patch_configurable_type::long_range:
						out << patch_key::min << config_value.min;
						out << patch_key::max << config_value.max;
						break;
					case patch_configurable_type::double_enum:
					case patch_configurable_type::long_enum:
						out << patch_key::allowed_values << YAML::BeginMap;

						for (const auto& allowed_value : config_value.allowed_values)
						{
							out << allowed_value.label << allowed_value.value;
						}

						out << YAML::EndMap;
						break;
					}

					out << YAML::EndMap;
				}

				out << YAML::EndMap;
			}

			out << patch_key::patch << YAML::BeginSeq;

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
	}

	out << YAML::EndMap;

	file.write(out.c_str(), out.size());

	return true;
}

bool patch_engine::import_patches(const patch_engine::patch_map& patches, const std::string& path, usz& count, usz& total, std::stringstream* log_messages)
{
	patch_map existing_patches;

	if (load(existing_patches, path, "", true, log_messages))
	{
		append_patches(existing_patches, patches, count, total, log_messages, path);
		return count == 0 || save_patches(existing_patches, path, log_messages);
	}

	return false;
}

bool patch_engine::remove_patch(const patch_info& info)
{
	patch_map patches;

	if (load(patches, info.source_path))
	{
		if (patches.contains(info.hash))
		{
			patch_container& container = patches[info.hash];

			if (container.patch_info_map.contains(info.description))
			{
				container.patch_info_map.erase(info.description);
				return save_patches(patches, info.source_path);
			}
		}
	}

	return false;
}

patch_engine::patch_map patch_engine::load_config()
{
	patch_map config_map{};

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

		for (const auto pair : root)
		{
			const std::string& hash = pair.first.Scalar();

			if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
			{
				patch_log.error("Error loading patch config key %s: expected Map, found %s (file: %s)", hash, yml_type, path);
				continue;
			}

			for (const auto patch : pair.second)
			{
				const std::string& description = patch.first.Scalar();

				if (const auto yml_type = patch.second.Type(); yml_type != YAML::NodeType::Map)
				{
					patch_log.error("Error loading patch %s: expected Map, found %s (hash: %s, file: %s)", description, yml_type, hash, path);
					continue;
				}

				for (const auto title_node : patch.second)
				{
					const std::string& title = title_node.first.Scalar();

					if (const auto yml_type = title_node.second.Type(); yml_type != YAML::NodeType::Map)
					{
						patch_log.error("Error loading %s: expected Map, found %s (description: %s, hash: %s, file: %s)", title, yml_type, description, hash, path);
						continue;
					}

					for (const auto serial_node : title_node.second)
					{
						const std::string& serial = serial_node.first.Scalar();

						if (const auto yml_type = serial_node.second.Type(); yml_type != YAML::NodeType::Map)
						{
							patch_log.error("Error loading %s: expected Map, found %s (title: %s, description: %s, hash: %s, file: %s)", serial, yml_type, title, description, hash, path);
							continue;
						}

						for (const auto app_version_node : serial_node.second)
						{
							const auto& app_version = app_version_node.first.Scalar();
							auto& config_values = config_map[hash].patch_info_map[description].titles[title][serial][app_version];

							if (app_version_node.second.IsMap())
							{
								if (const auto enable_node = app_version_node.second[patch_key::enabled])
								{
									config_values.enabled = enable_node.as<bool>(false);
								}

								if (const auto config_values_node = app_version_node.second[patch_key::config_values])
								{
									for (const auto config_value_node : config_values_node)
									{
										patch_config_value& config_value = config_values.config_values[config_value_node.first.Scalar()];
										config_value.value = config_value_node.second.as<f64>(0.0);
									}
								}
							}
							else
							{
								// Legacy
								config_values.enabled = app_version_node.second.as<bool>(false);
							}
						}
					}
				}
			}
		}
	}

	return config_map;
}
