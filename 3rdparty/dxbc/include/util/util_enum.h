#pragma once

#define ENUM_NAME(name) \
  case name: return os << #name

#define ENUM_DEFAULT(name) \
  default: return os << static_cast<int32_t>(e)
