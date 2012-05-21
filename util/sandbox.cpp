#include "yaml-cpp/yaml.h"
#include <iostream>

int main()
{
    YAML::Emitter out;
    out << "foo";
    
    std::cout << out.c_str() << "\n";
    return 0;
}
