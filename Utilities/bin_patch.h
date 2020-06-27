#pragma once

#include "BEType.h"
#include <vector>
#include <string>
#include <unordered_map>

#include "util/yaml.hpp"

namespace patch_key
{
	static const std::string all = "All";
}

enum class patch_type
{
	invalid,
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
		std::string original_value; // Used for import consistency (avoid rounding etc.)
		union
		{
			u64 long_value;
			f64 double_value;
		} value { 0 };
	};
	
	using patch_app_versions = std::unordered_map<std::string /*app_version*/, bool /*enabled*/>;
	using patch_serials = std::unordered_map<std::string /*serial*/, patch_app_versions>;
	using patch_titles = std::unordered_map<std::string /*serial*/, patch_serials>;

	struct patch_info
	{
		// Patch information
		std::vector<patch_data> data_list;
		patch_titles titles;
		std::string description;
		std::string patch_version;
		std::string patch_group;
		std::string author;
		std::string notes;
		std::string source_path;

		// Redundant information for accessibility (see patch_container)
		std::string hash;
		std::string version;
		bool is_legacy = false;
		bool is_enabled = false; // only for legacy patches
	};

	struct patch_container
	{
		std::unordered_map<std::string /*description*/, patch_info> patch_info_map;
		std::string hash;
		std::string version;
		bool is_legacy = false;
	};

	using patch_map = std::unordered_map<std::string /*hash*/, patch_container>;

	patch_engine();

	// Returns the directory in which patch_config.yml is located
	static std::string get_patch_config_path();

	// Returns the filepath for the imported_patch.yml
	static std::string get_imported_patch_path();

	// Load from file and append to specified patches map
	static bool load(patch_map& patches, const std::string& path, bool importing = false, std::stringstream* log_messages = nullptr);

	// Read and add a patch node to the patch info
	static bool read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root, std::stringstream* log_messages = nullptr);

	// Get the patch type of a patch node
	static patch_type get_patch_type(YAML::Node node);

	// Add the data of a patch node
	static bool add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root, std::stringstream* log_messages = nullptr);

	// Save to patch_config.yml
	static void save_config(const patch_map& patches_map, bool enable_legacy_patches);

	// Save a patch file
	static bool save_patches(const patch_map& patches, const std::string& path, std::stringstream* log_messages = nullptr);

	// Create or append patches to a file
	static bool import_patches(const patch_map& patches, const std::string& path, size_t& count, size_t& total, std::stringstream* log_messages = nullptr);

	// Remove a patch from a file
	static bool remove_patch(const patch_info& info);

	// Load patch_config.yml
	static patch_map load_config(bool& enable_legacy_patches);

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
