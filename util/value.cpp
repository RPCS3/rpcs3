#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value = YAML::Parse("{foo: bar, monkey: value}");
	for(YAML::const_iterator it=value.begin();it!=value.end();++it) {
		std::cout << it->first.as<std::string>() << " -> " << it->second.as<std::string>() << "\n";
	}
	
	return 0;
}
