#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::LongKey << "a" << "b";
    out << "a" << "b";
    out << YAML::EndMap;
    out << YAML::EndMap;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
