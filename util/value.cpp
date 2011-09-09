#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value;
	value["key"] = "value";
	std::cout << value["key"].as<std::string>() << "\n";
	value["key"]["key"] = "value";
	std::cout << value["key"]["key"].as<std::string>() << "\n";
	value[5] = "monkey";
	std::cout << value[5].as<std::string>() << "\n";
	value["monkey"] = 5;
	std::cout << value["monkey"].as<int>() << "\n";
	
	std::map<int, std::string> names;
	names[1] = "one";
	names[2] = "two";
	names[3] = "three";
	names[4] = "four";
	value["names"] = names;
	
	value["this"] = value;
	value["this"]["change"] = value;
	value["this"]["change"] = 5;
	
	return 0;
}
