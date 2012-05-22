#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::Comment("Hello");
    out << YAML::BeginSeq;
    out << "item 1";
    out << YAML::BeginMap;
    out << "pens" << "a";
    out << "pencils" << "b";
    out << YAML::EndMap;
    out << "item 2";
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
