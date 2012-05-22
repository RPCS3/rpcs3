#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    out << "foo";
    out << "bar";
    out << YAML::BeginMap;
    out << "a" << "b" << "c" << "d";
    out << YAML::EndMap;
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
