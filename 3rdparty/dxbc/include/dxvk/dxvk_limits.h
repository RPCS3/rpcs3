#pragma once

#include <cstddef>

namespace dxvk {

  enum DxvkLimits : size_t {
    MaxNumRenderTargets         =     8,
    MaxNumVertexAttributes      =    32,
    MaxNumVertexBindings        =    32,
    MaxNumXfbBuffers            =     4,
    MaxNumXfbStreams            =     4,
    MaxNumViewports             =    16,
    MaxNumResourceSlots         =  1216,
    MaxNumQueuedCommandBuffers  =    32,
    MaxNumQueryCountPerPool     =   128,
    MaxNumSpecConstants         =    12,
    MaxUniformBufferSize        = 65536,
    MaxVertexBindingStride      =  2048,
    MaxPushConstantSize         =   128,
  };

}
