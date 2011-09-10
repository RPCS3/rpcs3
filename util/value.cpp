#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value;
	value["seq"] = YAML::Value(YAML::ValueType::Sequence);
	for(int i=0;i<5;i++)
		value["seq"].append(i);
	value["map"]["one"] = "I";
	value["map"]["two"] = "II";
	value["map"]["three"] = "III";
	value["map"]["four"] = "IV";
	
	for(YAML::const_iterator it=value["seq"].begin();it!=value["seq"].end();++it) {
		std::cout << it->as<int>() << "\n";
	}

	for(YAML::const_iterator it=value["map"].begin();it!=value["map"].end();++it) {
		std::cout << it->first.as<std::string>() << " -> " << it->second.as<std::string>() << "\n";
	}
	
	return 0;
}
