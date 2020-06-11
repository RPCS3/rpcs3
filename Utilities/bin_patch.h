﻿#pragma once

#include "BEType.h"
#include <vector>
#include <string>
#include <unordered_map>

#include "util/yaml.hpp"

enum class patch_type
{
	load,
	byte,
	le16,
	le32,
	le64,
	lef32,
	lef64,
	be16,
	be32,
	be64,
	bef32,
	bef64,
};

class patch_engine
{
public:
	struct patch_data
	{
		patch_type type = patch_type::load;
		u32 offset = 0;
		u64 value = 0;
	};

	struct patch_info
	{
		// Patch information
		std::vector<patch_engine::patch_data> data_list;
		std::string description;
		std::string patch_version;
		std::string author;
		std::string notes;
		bool enabled = false;

		// Redundant information for accessibility (see patch_title_info)
		std::string hash;
		std::string version;
		std::string title;
		std::string serials;
		bool is_legacy = false;
	};

	struct patch_title_info
	{
		std::unordered_map<std::string /*description*/, patch_engine::patch_info> patch_info_map;
		std::string hash;
		std::string version;
		std::string title;
		std::string serials;
		bool is_legacy = false;
	};

	using patch_map = std::unordered_map<std::string /*hash*/, patch_title_info>;
	using patch_config_map = std::unordered_map<std::string /*hash*/, std::unordered_map<std::string /*description*/, bool /*enabled*/>>;

	patch_engine();

	// Returns the directory in which patch_config.yml is located
	static std::string get_patch_config_path();

	// Load from file and append to specified patches map
	// Example entry:
	//
	// PPU-8007056e52279bea26c15669d1ee08c2df321d00:
	//   Title: Fancy Game
	//   Serials: ABCD12345, SUPA13337 v.1.3
	//   Patches:
	//     60fps:
	//       Author: Batman bin Suparman
	//       Notes: This is super
	//       Patch:
	//         - [ be32, 0x000e522c, 0x995d0072 ]
	//         - [ be32, 0x000e5234, 0x995d0074 ]
	static void load(patch_map& patches, const std::string& path);

	// Read and add a patch node to the patch info
	static void read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root);

	// Get the patch type of a patch node
	static patch_type get_patch_type(YAML::Node node);

	// Add the data of a patch node
	static void add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root);

	// Save to patch_config.yml
	static void save_config(const patch_map& patches_map, bool enable_legacy_patches);

	// Load patch_config.yml
	static patch_config_map load_config(bool& enable_legacy_patches);

	// Load from file and append to member patches map
	void append_global_patches();

	// Load from title relevant files and append to member patches map
	void append_title_patches(const std::string& title_id);

	// Apply patch (returns the number of entries applied)
	std::size_t apply(const std::string& name, u8* dst) const;

	// Apply patch with a check that the address exists in SPU local storage
	std::size_t apply_with_ls_check(const std::string& name, u8* dst, u32 filesz, u32 ls_addr) const;

private:
	// Load from file and append to member patches map
	void append(const std::string& path);

	// Internal: Apply patch (returns the number of entries applied)
	template <bool check_local_storage>
	std::size_t apply_patch(const std::string& name, u8* dst, u32 filesz, u32 ls_addr) const;

	// Database
	patch_map m_map;
};
