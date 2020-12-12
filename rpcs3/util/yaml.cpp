#include "util/yaml.hpp"
#include "util/types.hpp"
#include "Utilities/cheat_info.h"
#include "Utilities/Config.h"

namespace YAML
{
	template <>
	struct convert<cheat_info>
	{
		static bool decode(const Node& node, cheat_info& rhs)
		{
			if (node.size() != 3)
			{
				return false;
			}

			rhs.description = node[0].Scalar();
			u64 type64      = 0;
			if (!cfg::try_to_enum_value(&type64, &fmt_class_string<cheat_type>::format, node[1].Scalar()))
				return false;
			if (type64 >= cheat_type_max)
				return false;
			rhs.type       = cheat_type{::narrow<u8>(type64)};
			rhs.red_script = node[2].Scalar();
			return true;
		}
	};
} // namespace YAML

std::pair<YAML::Node, std::string> yaml_load(const std::string& from)
{
	YAML::Node result;

	try
	{
		result = YAML::Load(from);
	}
	catch (const std::exception& e)
	{
		return {YAML::Node(), std::string("YAML exception: ") + e.what()};
	}

	return {result, ""};
}

template <typename T>
T get_yaml_node_value(YAML::Node node, std::string& error_message)
{
	try
	{
		return node.as<T>();
	}
	catch (const std::exception& e)
	{
		error_message = e.what();
	}

	return {};
}

template u32 get_yaml_node_value<u32>(YAML::Node, std::string&);
template u64 get_yaml_node_value<u64>(YAML::Node, std::string&);
template f64 get_yaml_node_value<f64>(YAML::Node, std::string&);
template cheat_info get_yaml_node_value<cheat_info>(YAML::Node, std::string&);
