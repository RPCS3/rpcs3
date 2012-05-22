#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::Comment("Hello");
    out << YAML::BeginSeq;
    out << YAML::Comment("Hello");
    out << YAML::Anchor("a") << YAML::Comment("anchor") << "item 1" << YAML::Comment("a");
    out << YAML::BeginMap << YAML::Comment("b");
    out << "pens" << YAML::Comment("foo") << "a" << YAML::Comment("bar");
    out << "pencils" << "b";
    out << YAML::EndMap;
    out << "item 2";
    out << YAML::EndSeq;
    
    std::cout << out.c_str() << "\n";
    return 0;
}
