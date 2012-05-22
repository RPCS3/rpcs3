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
    out << "a" << "b" << "c" << "d";
    out << YAML::EndMap;
    out << YAML::LocalTag("hi") << YAML::BeginSeq;
    out << "a" << "b";
    out << YAML::EndSeq;
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
