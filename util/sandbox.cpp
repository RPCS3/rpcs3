#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << YAML::DoubleQuoted << "Hello, World!\n";
    
    std::cout << out.c_str() << "\n";
    return 0;
}
