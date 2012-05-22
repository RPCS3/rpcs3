#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::Anchor("monkey") << YAML::LocalTag("a");
    out << YAML::BeginSeq;
    out << "foo";
    out << YAML::LocalTag("hi") << "bar";
    out << YAML::Anchor("asdf") << YAML::BeginMap;
    out << "a" << "b" << "c";
    out << YAML::Anchor("a") << YAML::BeginMap;
    out << YAML::Anchor("d") << "a" << "b";
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::LocalTag("hi") << YAML::BeginSeq;
    out << "a" << "b" << YAML::Alias("monkey");
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
