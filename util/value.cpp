#include "yaml-cpp/value.h"
#include <map>

int main()
{
	YAML::Value value(YAML::ValueType::Sequence);
	for(int i=0;i<5;i++)
		value.append(i);
	
	for(YAML::const_iterator it=value.begin();it!=value.end();++it) {
		std::cout << it->as<int>() << "\n";
	}
	
	return 0;
}
