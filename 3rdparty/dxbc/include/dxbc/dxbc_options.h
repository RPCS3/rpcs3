#pragma once

#include <vulkan/vulkan.h>

#include "util_flags.h"

namespace dxvk {

  struct D3D11Options;

  enum class DxbcFloatControlFlag : uint32_t {
    DenormFlushToZero32,
    DenormPreserve64,
    PreserveNan32,
    PreserveNan64,
  };

  using DxbcFloatControlFlags = Flags<DxbcFloatControlFlag>;

  struct DxbcOptions {
    DxbcOptions() {}

    // Clamp oDepth in fragment shaders if the depth
    // clip device feature is not supported
    bool useDepthClipWorkaround = false;

    /// Determines whether format qualifiers
    /// on typed UAV loads are required
    bool supportsTypedUavLoadR32 = false;

    /// Determines whether raw access chains are supported
    bool supportsRawAccessChains = false;

    /// Clear thread-group shared memory to zero
    bool zeroInitWorkgroupMemory = false;

    /// Declare vertex positions as invariant
    bool invariantPosition = false;

    /// Insert memory barriers after TGSM stoes
    bool forceVolatileTgsmAccess = false;

    /// Try to detect hazards in UAV access and insert
    /// barriers when we know control flow is uniform.
    bool forceComputeUavBarriers = false;

    /// Replace ld_ms with ld
    bool disableMsaa = false;

    /// Force sample rate shading by using sample
    /// interpolation for fragment shader inputs
    bool forceSampleRateShading = false;

    // Enable per-sample interlock if supported
    bool enableSampleShadingInterlock = false;

    /// Use tightly packed arrays for immediate
    /// constant buffers if possible
    bool supportsTightIcbPacking = false;

    /// Whether exporting point size is required
    bool needsPointSizeExport = true;

    /// Whether to enable sincos emulation
    bool sincosEmulation = false;

    /// Float control flags
    DxbcFloatControlFlags floatControl;

    /// Minimum storage buffer alignment
    VkDeviceSize minSsboAlignment = 0;
  };
  
}
