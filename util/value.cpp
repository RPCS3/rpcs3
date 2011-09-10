#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value = YAML::Parse("{foo: bar, monkey: value}");
	std::cout << value << "\n";
	
	return 0;
}
