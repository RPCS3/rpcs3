#pragma once

enum class cfg_mode
{
	custom,           // Prefer regular custom config. Fall back to global config.
	custom_selection, // Use user-selected custom config. Fall back to global config.
	global,           // Use global config.
	config_override,  // Use config override. This does not use the global VFS settings! Fall back to global config.
	continuous,       // Use same config as on last boot. Fall back to global config.
	default_config    // Use the default values of the config entries.
};
