#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginDoc;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::EndSeq;
    out << YAML::EndDoc;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
