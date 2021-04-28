#pragma once

#include <string>

template <typename T>
std::string cereal_serialize(const T&);

template <typename T>
void cereal_deserialize(T& data, const std::string& src);
