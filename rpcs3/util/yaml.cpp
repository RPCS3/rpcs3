#include "util/yaml.hpp"

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
