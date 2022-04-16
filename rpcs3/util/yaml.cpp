#include "util/yaml.hpp"
#include "util/types.hpp"
#include "Utilities/Config.h"

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

std::string get_yaml_node_location(YAML::Node node)
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

template u32 get_yaml_node_value<u32>(YAML::Node, std::string&);
template u64 get_yaml_node_value<u64>(YAML::Node, std::string&);
template s64 get_yaml_node_value<s64>(YAML::Node, std::string&);
template f64 get_yaml_node_value<f64>(YAML::Node, std::string&);
