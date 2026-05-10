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
T get_yaml_node_value(const YAML::Node& node, std::string& error_message)
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

std::string get_yaml_node_location(const YAML::Node& node)
{
	try
	{
		const auto mark = node.Mark();

		if (mark.is_null())
			return "unknown";

		return fmt::format("line %d, column %d", mark.line, mark.column); // Don't need the pos. It's not really useful.
	}
	catch (const std::exception& e)
	{
		return e.what();
	}
}

std::string get_yaml_node_location(const YAML::detail::iterator_value& it)
{
	return get_yaml_node_location(it.first);
}

template u8 get_yaml_node_value<u8>(const YAML::Node&, std::string&);
template s8 get_yaml_node_value<s8>(const YAML::Node&, std::string&);
template u16 get_yaml_node_value<u16>(const YAML::Node&, std::string&);
template s16 get_yaml_node_value<s16>(const YAML::Node&, std::string&);
template u32 get_yaml_node_value<u32>(const YAML::Node&, std::string&);
template s32 get_yaml_node_value<s32>(const YAML::Node&, std::string&);
template u64 get_yaml_node_value<u64>(const YAML::Node&, std::string&);
template s64 get_yaml_node_value<s64>(const YAML::Node&, std::string&);
template f32 get_yaml_node_value<f32>(const YAML::Node&, std::string&);
template f64 get_yaml_node_value<f64>(const YAML::Node&, std::string&);
template std::string get_yaml_node_value<std::string>(const YAML::Node&, std::string&);
template cheat_info get_yaml_node_value<cheat_info>(const YAML::Node&, std::string&);
