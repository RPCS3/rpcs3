#include "bin_patch.h"
#include "File.h"
#include "Config.h"
#include "version.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "util/types.hpp"
#include "util/endian.hpp"
#include "util/asm.hpp"

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
		case patch_type::utf8: return "utf8";
		}

		return unknown;
	});
}

patch_engine::patch_engine()
{
	const std::string patches_path = get_patches_path();

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
		patch_log.error("Could not create path: %s (%s)", patch_path, fs::g_tls_error);
	}

	return patch_path;
#else
	return fs::get_config_dir() + "patch_config.yml";
#endif
}

std::string patch_engine::get_patches_path()
{
	return fs::get_config_dir() + "patches/";
}

std::string patch_engine::get_imported_patch_path()
{
	return get_patches_path() + "imported_patch.yml";
}

static void append_log_message(std::stringstream* log_messages, const std::string& message)
{
	if (log_messages)
		*log_messages << message << std::endl;
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
			append_log_message(log_messages, fmt::format("Error: File version %s does not match patch engine target version %s (file: %s)", version, patch_engine_version, path));
			patch_log.error("File version %s does not match patch engine target version %s (file: %s)", version, patch_engine_version, path);
			return false;
		}

		// We don't need the Version node in local memory anymore
		root.remove(patch_key::version);
	}
	else
	{
		append_log_message(log_messages, fmt::format("Error: No '%s' entry found. Patch engine version = %s (file: %s)", patch_key::version, patch_engine_version, path));
		patch_log.error("No '%s' entry found. Patch engine version = %s (file: %s)", patch_key::version, patch_engine_version, path);
		return false;
	}

	bool is_valid = true;

	// Go through each main key in the file
	for (auto pair : root)
	{
		const auto& main_key = pair.first.Scalar();

		if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
		{
			append_log_message(log_messages, fmt::format("Error: Skipping key %s: expected Map, found %s", main_key, yml_type));
			patch_log.error("Skipping key %s: expected Map, found %s (file: %s)", main_key, yml_type, path);
			is_valid = false;
			continue;
		}

		// Skip Anchors
		if (main_key == patch_key::anchors)
		{
			continue;
		}

		// Find or create an entry matching the key/hash in our map
		auto& container = patches_map[main_key];
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

			if (const auto games_node = patches_entry.second[patch_key::games])
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

					const bool title_is_all_key = title == patch_key::all;

					for (const auto serial_node : game_node.second)
					{
						const std::string& serial = serial_node.first.Scalar();

						if (serial == patch_key::all)
						{
							if (!title_is_all_key)
							{
								append_log_message(log_messages, fmt::format("Error: Using '%s' as serial is not allowed for titles other than '%s' (title: %s, patch: %s, key: %s)", patch_key::all, patch_key::all, title, description, main_key));
								patch_log.error("Error: Using '%s' as serial is not allowed for titles other than '%s' (title: %s, patch: %s, key: %s, file: %s)", patch_key::all, patch_key::all, title, description, main_key, path);
								is_valid = false;
								continue;
							}
						}
						else if (title_is_all_key)
						{
							append_log_message(log_messages, fmt::format("Error: Only '%s' is allowed as serial if the title is '%s' (serial: %s, patch: %s, key: %s)", patch_key::all, patch_key::all, serial, description, main_key));
							patch_log.error("Error: Only '%s' is allowed as serial if the title is '%s' (serial: %s, patch: %s, key: %s, file: %s)", patch_key::all, patch_key::all, serial, description, main_key, path);
							is_valid = false;
							continue;
						}

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
							patch_log.error("Skipping %s: empty Sequence (title: %s, patch: %s, key: %s, file: %s)", serial, title, description, main_key, path);
							is_valid = false;
						}
						else
						{
							info.titles[title][serial] = app_versions;
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
						if (note && note.IsScalar())
						{
							info.notes += note.Scalar();
						}
						else
						{
							append_log_message(log_messages, fmt::format("Error: Skipping sequenced Note (patch: %s, key: %s)", description, main_key));
							patch_log.error("Skipping sequenced Note (patch: %s, key: %s, file: %s)", description, main_key, path);
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

			if (const auto patch_node = patches_entry.second[patch_key::patch])
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
	case patch_type::utf8:
	{
		break;
	}
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
		patch_log.error("Skipping invalid patch node %s. (key: %s)", info.description, info.hash);
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

void patch_engine::append_global_patches()
{
	// Regular patch.yml
	load(m_map, get_patches_path() + "patch.yml");

	// Imported patch.yml
	load(m_map, get_imported_patch_path());
}

void patch_engine::append_title_patches(const std::string& title_id)
{
	if (title_id.empty())
	{
		return;
	}

	// Regular patch.yml
	load(m_map, get_patches_path() + title_id + "_patch.yml");
}

void ppu_register_range(u32 addr, u32 size);
void ppu_register_function_at(u32 addr, u32 size, u64 ptr);
bool ppu_form_branch_to_code(u32 entry, u32 target);

void unmap_vm_area(std::shared_ptr<vm::block_t>& ptr)
{
	if (ptr && ptr->flags & (1ull << 62))
	{
		const u32 addr = ptr->addr;
		ptr.reset();
		vm::unmap(addr, true);
	}
}

// Returns old 'applied' size
static usz apply_modification(std::basic_string<u32>& applied, const patch_engine::patch_info& patch, u8* dst, u32 filesz, u32 min_addr)
{
	const usz old_applied_size = applied.size();

	for (const auto& p : patch.data_list)
	{
		if (p.type != patch_type::alloc) continue;

		// Do not allow null address or if dst is not a VM ptr
		if (const u32 alloc_at = vm::try_get_addr(dst + (p.offset & -4096)).first; alloc_at >> 16)
		{
			const u32 alloc_size = utils::align(static_cast<u32>(p.value.long_value) + alloc_at % 4096, 4096);

			// Allocate map if needed, if allocated flags will indicate that bit 62 is set (unique identifier)
			auto alloc_map = vm::reserve_map(vm::any, alloc_at & -0x10000, utils::align(alloc_size, 0x10000), vm::page_size_64k | vm::preallocated | (1ull << 62));

			u64 flags = vm::page_readable;

			switch (p.offset % patch_engine::mem_protection::mask)
			{
			case patch_engine::mem_protection::wx: flags |= vm::page_writable + vm::page_executable; break;
			case patch_engine::mem_protection::ro: break;
			case patch_engine::mem_protection::rx: flags |= vm::page_executable; break;
			case patch_engine::mem_protection::rw: flags |= vm::page_writable; break;
			default: ensure(false);
			}

			if (alloc_map)
			{
				if ((p.alloc_addr = alloc_map->falloc(alloc_at, alloc_size)))
				{
					vm::page_protect(alloc_at, alloc_size, 0, flags, flags ^ (vm::page_writable + vm::page_readable + vm::page_executable));

					if (flags & vm::page_executable)
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

	for (const auto& p : patch.data_list)
	{
		u32 offset = p.offset;

		if (relocate_instructions_at && vm::read32(relocate_instructions_at) != 0x6000'0000u)
		{
			// No longer points a NOP to be filled, meaning we ran out of instructions
			relocate_instructions_at = 0;
		}

		if (!relocate_instructions_at && (offset < min_addr || offset - min_addr >= filesz))
		{
			// This patch is out of range for this segment
			continue;
		}

		offset -= min_addr;

		auto ptr = dst + offset;

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
			relocate_instructions_at = 0;

			const u32 out_branch = vm::try_get_addr(dst + (offset & -4)).first;

			// Allow only if points to a PPU executable instruction
			if (out_branch < 0x10000 || out_branch >= 0x4000'0000 || !vm::check_addr<4>(out_branch, vm::page_executable))
			{
				continue;
			}

			const u32 alloc_size = utils::align(static_cast<u32>(p.value.long_value + 1) * 4, 0x10000);

			// Always executable
			u64 flags = vm::page_executable | vm::page_readable;

			switch (p.offset % patch_engine::mem_protection::mask)
			{
			case patch_engine::mem_protection::rw:
			case patch_engine::mem_protection::wx:
			{
				flags |= vm::page_writable;
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
			const u32 addr = p.alloc_addr = alloc_map->alloc(alloc_size, nullptr, 0x10000, flags);

			if (!addr)
			{
				patch_log.error("Failed to allocate 0x%x bytes for code (entry=0x%x)", alloc_size, addr, out_branch);
				continue;
			}

			patch_log.success("Allocated 0x%x for code at 0x%x (entry=0x%x)", alloc_size, addr, out_branch);

			// NOP filled
			std::fill_n(vm::get_super_ptr<u32>(addr), p.value.long_value, 0x60000000);

			// Register code
			ppu_register_range(addr, alloc_size);
			ppu_register_function_at(addr, static_cast<u32>(p.value.long_value), 0);

			// Write branch to code
			ppu_form_branch_to_code(out_branch, addr);
			resval = out_branch & -4;

			// Write address of the allocated memory to the code entry
			*vm::get_super_ptr<u32>(resval) = addr;
	
			// Write branch to return to code
			ppu_form_branch_to_code(addr + static_cast<u32>(p.value.long_value) * 4, resval + 4);
			relocate_instructions_at = addr;
			break;
		}
		case patch_type::jump:
		{
			const u32 out_branch = vm::try_get_addr(dst + (offset & -4)).first;
			const u32 dest = static_cast<u32>(p.value.long_value);

			// Allow only if points to a PPU executable instruction
			if (!ppu_form_branch_to_code(out_branch, dest))
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
		case patch_type::utf8:
		{
			std::memcpy(ptr, p.original_value.data(), p.original_value.size());
			break;
		}
		}

		// Possibly an executable instruction
		applied.push_back(resval);
	}

	return old_applied_size;
}

std::basic_string<u32> patch_engine::apply(const std::string& name, u8* dst, u32 filesz, u32 min_addr)
{
	if (m_map.find(name) == m_map.cend())
	{
		return {};
	}

	std::basic_string<u32> applied_total;
	const auto& container = m_map.at(name);
	const auto& serial = Emu.GetTitleID();
	const auto& app_version = Emu.GetAppVersion();

	// Different containers in order to seperate the patches
	std::vector<const patch_info*> patches_for_this_serial_and_this_version;
	std::vector<const patch_info*> patches_for_this_serial_and_all_versions;
	std::vector<const patch_info*> patches_for_all_serials_and_this_version;
	std::vector<const patch_info*> patches_for_all_serials_and_all_versions;

	// Sort patches into different vectors based on their serial and version
	for (const auto& [description, patch] : container.patch_info_map)
	{
		// Find out if this patch is enabled
		for (const auto& [title, serials] : patch.titles)
		{
			bool is_all_serials = false;
			bool is_all_versions = false;

			std::string found_serial;

			if (serials.find(serial) != serials.end())
			{
				found_serial = serial;
			}
			else if (serials.find(patch_key::all) != serials.end())
			{
				found_serial = patch_key::all;
				is_all_serials = true;
			}

			if (found_serial.empty())
			{
				continue;
			}

			const auto& app_versions = serials.at(found_serial);
			std::string found_app_version;

			if (app_versions.find(app_version) != app_versions.end())
			{
				found_app_version = app_version;
			}
			else if (app_versions.find(patch_key::all) != app_versions.end())
			{
				found_app_version = patch_key::all;
				is_all_versions = true;
			}

			if (!found_app_version.empty() && app_versions.at(found_app_version))
			{
				// This patch is enabled
				if (is_all_serials)
				{
					if (is_all_versions)
					{
						patches_for_all_serials_and_all_versions.emplace_back(&patch);
					}
					else
					{
						patches_for_all_serials_and_this_version.emplace_back(&patch);
					}
				}
				else if (is_all_versions)
				{
					patches_for_this_serial_and_all_versions.emplace_back(&patch);
				}
				else
				{
					patches_for_this_serial_and_this_version.emplace_back(&patch);
				}

				break;
			}
		}
	}

	// Apply modifications sequentially
	auto apply_func = [&](const patch_info& patch)
	{
		const usz old_size = apply_modification(applied_total, patch, dst, filesz, min_addr);

		if (applied_total.size() != old_size)
		{
			patch_log.success("Applied patch (hash='%s', description='%s', author='%s', patch_version='%s', file_version='%s') (<- %u)", patch.hash, patch.description, patch.author, patch.patch_version, patch.version, applied_total.size() - old_size);
		}
	};

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
					patch = nullptr;
				}
			}
		}
	}

	for (auto patch_list : patch_super_list)
	{
		for (const patch_info* patch : *patch_list)
		{
			if (patch)
			{
				apply_func(*patch);
			}
		}
	}

	return applied_total;
}

void patch_engine::unload(const std::string& name)
{
	if (m_map.find(name) == m_map.cend())
	{
		return;
	}

	const auto& container = m_map.at(name);

	for (const auto& [description, patch] : container.patch_info_map)
	{
		for (const auto& [title, serials] : patch.titles)
		{
			for (auto& entry : patch.data_list)
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
}

void patch_engine::save_config(const patch_map& patches_map)
{
	const std::string path = get_patch_config_path();
	patch_log.notice("Saving patch config file %s", path);

	fs::file file(path, fs::rewrite);
	if (!file)
	{
		patch_log.fatal("Failed to open patch config file %s (%s)", path, fs::g_tls_error);
		return;
	}

	YAML::Emitter out;
	out << YAML::BeginMap;

	// Save 'enabled' state per hash, description, serial and app_version
	patch_map config_map;

	for (const auto& [hash, container] : patches_map)
	{
		for (const auto& [description, patch] : container.patch_info_map)
		{
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

		if (const auto& enabled_patches = config_map[hash].patch_info_map; !enabled_patches.empty())
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

static void append_patches(patch_engine::patch_map& existing_patches, const patch_engine::patch_map& new_patches, usz& count, usz& total, std::stringstream* log_messages)
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
		patch_log.fatal("save_patches: Failed to open patch file %s (%s)", path, fs::g_tls_error);
		append_log_message(log_messages, fmt::format("Failed to open patch file %s (%s)", path, fs::g_tls_error));
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

					for (const auto& app_version : app_versions)
					{
						out << app_version.first;
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
	patch_engine::patch_map existing_patches;

	if (load(existing_patches, path, "", true, log_messages))
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

patch_engine::patch_map patch_engine::load_config()
{
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
