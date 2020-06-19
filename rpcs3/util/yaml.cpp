#include "util/yaml.hpp"
#include "Utilities/types.h"

std::pair<YAML::Node, std::string> yaml_load(const std::string& from)
{
	YAML::Node result;

	try
	{
		result = YAML::Load(from);
	}
	catch(const std::exception& e)
	{
		return{YAML::Node(), std::string("YAML exception:\n") + e.what()};
	}

	return{result, ""};
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

template u64 get_yaml_node_value<u64>(YAML::Node, std::string&);
template f64 get_yaml_node_value<f64>(YAML::Node, std::string&);
