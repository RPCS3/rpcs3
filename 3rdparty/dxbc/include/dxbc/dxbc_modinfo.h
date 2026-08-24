#pragma once

#include "dxbc_options.h"

namespace dxvk {

  /**
   * \brief Tessellation info
   * 
   * Stores the maximum tessellation factor
   * to export from tessellation shaders.
   */
  struct DxbcTessInfo {
    float maxTessFactor;
  };

  /**
   * \brief Xfb capture entry
   * 
   * Stores an output variable to capture,
   * as well as the buffer to write it to.
   */
  struct DxbcXfbEntry {
    const char* semanticName;
    uint32_t    semanticIndex;
    uint32_t    componentIndex;
    uint32_t    componentCount;
    uint32_t    streamId;
    uint32_t    bufferId;
    uint32_t    offset;
  };

  /**
   * \brief Xfb info
   * 
   * Stores capture entries and output buffer
   * strides. This structure must only be
   * defined if \c entryCount is non-zero.
   */
  struct DxbcXfbInfo {
    uint32_t      entryCount;
    DxbcXfbEntry  entries[128];
    uint32_t      strides[4];
    int32_t       rasterizedStream;
  };

  /**
   * \brief Shader module info
   * 
   * Stores information which may affect shader compilation.
   * This data can be supplied by the client API implementation.
   */
  struct DxbcModuleInfo {
    DxbcOptions   options;
    DxbcTessInfo* tess;
    DxbcXfbInfo*  xfb;
  };

}