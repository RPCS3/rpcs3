#include "yaml-cpp/value.h"

int main()
{
	YAML::Value value;
	value["key"] = "value";
	std::cout << value["key"].as<std::string>() << "\n";
	value["key"]["key"] = "value";
	std::cout << value["key"]["key"].as<std::string>() << "\n";
//	value[5] = "monkey";
//	std::cout << value[5].as<std::string>() << "\n";
	
	return 0;
}
