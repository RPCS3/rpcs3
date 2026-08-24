#include "dxbc_util.h"

namespace dxvk {
  
  uint32_t primitiveVertexCount(DxbcPrimitive primitive) {
    static const std::array<uint32_t, 8> s_vertexCounts = {
       0, // Undefined
       1, // Point
       2, // Line
       3, // Triangle
       0, // Undefined
       0, // Undefined
       4, // Line with adjacency
       6, // Triangle with adjacency
    };
    
    if (primitive >= DxbcPrimitive::Patch1) {
      return uint32_t(primitive)
           - uint32_t(DxbcPrimitive::Patch1)
           + 1u;
    } else {
      return s_vertexCounts.at(uint32_t(primitive));
    }
  }
  
}