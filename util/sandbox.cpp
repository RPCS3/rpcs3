#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::Comment("Skills");
    out << YAML::BeginMap;
    out << YAML::Key << "attack" << YAML::Value << 23;
    out << YAML::Key << "intelligence" << YAML::Value << 56;
    out << YAML::EndMap;
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
