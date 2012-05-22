#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::BeginMap;
    out << "a" << "b";
    out << YAML::EndMap;
    out << "c";
    out << YAML::EndMap;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
