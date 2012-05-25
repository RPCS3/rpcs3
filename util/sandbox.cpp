#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out(std::cout);
    out << YAML::BeginSeq;
    out << "item 1";
    out << YAML::BeginSeq << "foo 1" << "bar 2" << YAML::EndSeq;
    out << YAML::EndSeq;
    return 0;
}
