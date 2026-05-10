#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "util/types.hpp"
#include "util/yaml.hpp"

namespace patch_key
{
	static const std::string all = "All";
	static const std::string anchors = "Anchors";
	static const std::string author = "Author";
	static const std::string games = "Games";
	static const std::string group = "Group";
	static const std::string notes = "Notes";
	static const std::string patch = "Patch";
	static const std::string patch_version = "Patch Version";
	static const std::string version = "Version";
	static const std::string enabled = "Enabled";
	static const std::string config_values = "Configurable Values";
	static const std::string value = "Value";
	static const std::string type = "Type";
	static const std::string min = "Min";
	static const std::string max = "Max";
	static const std::string allowed_values = "Allowed Values";
}

inline static const std::string patch_engine_version = "1.2";

enum class patch_type
{
	invalid,
	load,
	alloc, // Allocate memory at address (zeroized executable memory)
	code_alloc,// Allocate memory somewhere, saves branch to memory at specfied address (filled with PPU NOP and branch for returning)
	jump, // Install special 32-bit jump instruction (PPU only atm)
	jump_link, // jump + set link (PPU only atm)
	jump_func, // jump to exported function (PPU only, forever)
	byte,
	le16,
	le32,
	le64,
	lef32,
	lef64,
	be16,
	be32,
	bd32, // be32 with data hint (non-code)
	be64,
	bd64, // be64 with data hint (non-code)
	bef32,
	bef64,
	bp_exec, // Execution Breakpoint 
	utf8, // Text of string (not null-terminated automatically)
	c_utf8, // Text of string (null-terminated automatically)
	move_file, // Move file
	hide_file, // Hide file
};

static constexpr bool patch_type_uses_hex_offset(patch_type type)
{
	return type >= patch_type::alloc && type <= patch_type::c_utf8;
}

enum class patch_configurable_type
{
	double_range,
	double_enum,
	long_range,
	long_enum
};

class patch_engine
{
public:
	struct patch_data
	{
		patch_type type = patch_type::load;
		u32 offset = 0;
		std::string original_offset{}; // Used for specifying paths
		std::string original_value{}; // Used for import consistency (avoid rounding etc.)
		union
		{
			u64 long_value;
			f64 double_value;
		} value{0};
		mutable u32 alloc_addr = 0; // Used to save optional allocation address (if occured)
	};

	struct patch_allowed_value
	{
		std::string label;
		f64 value{};

		bool operator==(const patch_allowed_value& other) const
		{
			return value == other.value && label == other.label;
		}
	};

	struct patch_config_value
	{
		f64 value{};
		f64 min{};
		f64 max{};
		patch_configurable_type type{};
		std::vector<patch_allowed_value> allowed_values;

		bool operator==(const patch_config_value& other) const
		{
			return value == other.value && min == other.min && max == other.max && type == other.type && allowed_values == other.allowed_values;
		}

		void set_and_check_value(f64 new_value, std::string_view name);
	};

	struct patch_config_values
	{
		bool enabled{};
		std::map<std::string, patch_config_value> config_values;
	};

	using patch_app_versions = std::unordered_map<std::string /*app_version*/, patch_config_values>;
	using patch_serials = std::unordered_map<std::string /*serial*/, patch_app_versions>;
	using patch_titles = std::unordered_map<std::string /*serial*/, patch_serials>;

	struct patch_info
	{
		// Patch information
		std::vector<patch_data> data_list{};
		patch_titles titles{};
		std::string description{};
		std::string patch_version{};
		std::string patch_group{};
		std::string author{};
		std::string notes{};
		std::string source_path{};
		std::map<std::string, patch_config_value> default_config_values;

		// Redundant information for accessibility (see patch_container)
		std::string hash{};
		std::string version{};
		std::map<std::string, patch_config_value> actual_config_values;
	};

	struct patch_container
	{
		std::unordered_map<std::string /*description*/, patch_info> patch_info_map{};
		std::string hash{};
		std::string version{};
	};

	enum mem_protection : u8
	{
		wx = 0, // Read + Write + Execute (default)
		ro = 1, // Read
		rx = 2, // Read + Execute
		rw = 3, // Read + Write
		mask = 3,
	};

	using patch_map = std::unordered_map<std::string /*hash*/, patch_container>;

	patch_engine();

	patch_engine(const patch_engine&) = delete;

	patch_engine& operator=(const patch_engine&) = delete;

	// Returns the directory in which patch_config.yml is located
	static std::string get_patch_config_path();

	// Returns the directory in which patches are located
	static std::string get_patches_path();

	// Returns the filepath for the imported_patch.yml
	static std::string get_imported_patch_path();

	// Load from file and append to specified patches map
	static bool load(patch_map& patches, const std::string& path, std::string content = "", bool importing = false, std::stringstream* log_messages = nullptr);

	// Read and add a patch node to the patch info
	static bool read_patch_node(patch_info& info, YAML::Node node, const YAML::Node& root, std::string_view path, std::stringstream* log_messages = nullptr);

	// Get the patch type from a string
	static patch_type get_patch_type(std::string_view text);

	// Get the patch type of a patch node
	static patch_type get_patch_type(YAML::Node node);

	// Add the data of a patch node
	static bool add_patch_data(YAML::Node node, patch_info& info, u32 modifier, const YAML::Node& root, std::string_view path, std::stringstream* log_messages = nullptr);

	// Save to patch_config.yml
	static void save_config(const patch_map& patches_map);

	// Save a patch file
	static bool save_patches(const patch_map& patches, const std::string& path, std::stringstream* log_messages = nullptr);

	// Create or append patches to a file
	static bool import_patches(const patch_map& patches, const std::string& path, usz& count, usz& total, std::stringstream* log_messages = nullptr);

	// Remove a patch from a file
	static bool remove_patch(const patch_info& info);

	// Load patch_config.yml
	static patch_map load_config();

	// Load from file and append to member patches map
	void append_global_patches();

	// Load from title relevant files and append to member patches map
	void append_title_patches(std::string_view title_id);

	// Apply patch (returns the number of entries applied)
	void apply(std::vector<u32>& applied_total, const std::string& name, std::function<u8*(u32, u32)> mem_translate, u32 filesz = -1, u32 min_addr = 0);

	// Deallocate memory used by patches
	void unload(const std::string& name);

private:
	// Database
	patch_map m_map{};

	// Only one patch per patch group can be applied
	std::set<std::string> m_applied_groups{};
};
