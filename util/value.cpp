#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value = YAML::Parse("foo: bar");
	
	return 0;
}
