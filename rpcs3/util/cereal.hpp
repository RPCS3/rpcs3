#pragma once

#include <string>

template <typename T>
std::string cereal_serialize(const T&);

template <typename T>
void cereal_deserialize(T& out, const std::string& data);
