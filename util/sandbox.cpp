#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::Flow;
    out << YAML::BeginSeq;
    out << "a" << "b";
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
