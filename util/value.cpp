#include "yaml-cpp/yaml.h"
#include <map>

int main()
{
	YAML::Node node = YAML::Parse("{foo: bar, monkey: value}");
	std::cout << node << "\n";
	
	return 0;
}
