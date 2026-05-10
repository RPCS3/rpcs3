#pragma once

#include <utility>
#include <string>

#ifdef _MSC_VER
#pragma warning(push, 0)
#include "yaml-cpp/yaml.h"
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Weffc++"
#include "yaml-cpp/yaml.h"
#pragma GCC diagnostic pop
#endif

// Load from string and consume exception
std::pair<YAML::Node, std::string> yaml_load(const std::string& from);

// Use try/catch in YAML::Node::as<T>() instead of YAML::Node::as<T>(fallback) in order to get an error message
template <typename T>
T get_yaml_node_value(const YAML::Node& node, std::string& error_message);

// Get the location of the node in the document
std::string get_yaml_node_location(const YAML::Node& node);
std::string get_yaml_node_location(const YAML::detail::iterator_value& it);
