#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    out << ':';
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
