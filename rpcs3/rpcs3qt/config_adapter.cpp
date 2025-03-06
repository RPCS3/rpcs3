#include "config_adapter.h"
#include "Emu/system_config.h"

LOG_CHANNEL(cfg_log, "CFG");

// Helper methods to interact with YAML and the config settings.
namespace cfg_adapter
{
	static cfg::_base& get_cfg(const cfg::_base& root, const std::string& name)
	{
		if (root.get_type() == cfg::type::node)
		{
			for (const auto& node : static_cast<const cfg::node&>(root).get_nodes())
			{
				if (node->get_name() == name)
				{
					return *node;
				}
			}
		}

		fmt::throw_exception("Node not found: %s", name);
	}

	static cfg::_base& get_cfg(cfg::_base& root, const cfg_location::const_iterator begin, const cfg_location::const_iterator end)
	{
		return begin == end ? root : get_cfg(get_cfg(root, *begin), begin + 1, end);
	}

	YAML::Node get_node(const YAML::Node& node, const cfg_location::const_iterator begin, const cfg_location::const_iterator end)
	{
		if (begin == end)
		{
			return node;
		}

		if (!node || !node.IsMap())
		{
			cfg_log.fatal("Node error. A cfg_location does not match its cfg::node (location: %s)", get_yaml_node_location(node));
			return YAML::Node();
		}

		return get_node(node[*begin], begin + 1, end); // TODO
	}

	YAML::Node get_node(const YAML::Node& node, const cfg_location& location)
	{
		return get_node(node, location.cbegin(), location.cend());
	}

	std::vector<std::string> get_options(const cfg_location& location)
	{
		return cfg_adapter::get_cfg(g_cfg, location.cbegin(), location.cend()).to_list();
	}

	static bool get_is_dynamic(const cfg_location& location)
	{
		return cfg_adapter::get_cfg(g_cfg, location.cbegin(), location.cend()).get_is_dynamic();
	}

	bool get_is_dynamic(emu_settings_type type)
	{
		return get_is_dynamic(::at32(settings_location, type));
	}

	std::string get_setting_name(emu_settings_type type)
	{
		const cfg_location& loc = ::at32(settings_location, type);
		return ::at32(loc, loc.size() - 1);
	}
}
