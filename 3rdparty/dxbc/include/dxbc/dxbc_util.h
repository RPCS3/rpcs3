#pragma once

#include "dxbc_common.h"
#include "dxbc_enums.h"

namespace dxvk {

  /**
   * \brief Push constant struct
   */
  struct DxbcPushConstants {
    uint32_t rasterizerSampleCount;
  };


  /**
   * \brief Binding numbers and properties
   */
  enum DxbcBindingProperties : uint32_t {
    DxbcConstBufBindingIndex  = 0,
    DxbcConstBufBindingCount  = 16,
    DxbcSamplerBindingIndex   = DxbcConstBufBindingIndex
                              + DxbcConstBufBindingCount,
    DxbcSamplerBindingCount   = 16,
    DxbcResourceBindingIndex  = DxbcSamplerBindingIndex
                              + DxbcSamplerBindingCount,
    DxbcResourceBindingCount  = 128,
    DxbcStageBindingCount     = DxbcConstBufBindingCount
                              + DxbcSamplerBindingCount
                              + DxbcResourceBindingCount,
    DxbcUavBindingIndex       = DxbcStageBindingCount * 6,
    DxbcUavBindingCount       = 64,
  };


  /**
   * \brief Shader binding mask
   *
   * Stores a bit masks of resource bindings
   * that are accessed by any given shader.
   */
  struct DxbcBindingMask {
    uint32_t cbvMask      = 0u;
    uint32_t samplerMask  = 0u;
    uint64_t uavMask      = 0u;
    std::array<uint64_t, 2> srvMask = { };

    void reset() {
      cbvMask = 0u;
      samplerMask = 0u;
      uavMask = 0u;
      srvMask = { };
    }

    bool empty() const {
      uint64_t mask = (uint64_t(cbvMask) | uint64_t(samplerMask) << 32u)
                    | (uavMask | srvMask[0] | srvMask[1]);
      return !mask;
    }

    DxbcBindingMask operator & (const DxbcBindingMask& other) const {
      DxbcBindingMask result = *this;
      result.cbvMask      &= other.cbvMask;
      result.samplerMask  &= other.samplerMask;
      result.uavMask      &= other.uavMask;
      result.srvMask[0]   &= other.srvMask[0];
      result.srvMask[1]   &= other.srvMask[1];
      return result;
    }
  };


  /**
   * \brief Computes first binding index for a given stage
   *
   * \param [in] stage The shader stage
   * \returns Index of first binding
   */
  inline uint32_t computeStageBindingOffset(DxbcProgramType stage) {
    return DxbcStageBindingCount * uint32_t(stage);
  }


  /**
   * \brief Computes first UAV binding index offset for a given stage
   *
   * \param [in] stage The shader stage
   * \returns Index of first UAV binding
   */
  inline uint32_t computeStageUavBindingOffset(DxbcProgramType stage) {
    return DxbcUavBindingIndex
         + DxbcUavBindingCount * (stage == DxbcProgramType::ComputeShader ? 2 : 0);
  }


  /**
   * \brief Computes constant buffer binding index
   * 
   * \param [in] stage Shader stage
   * \param [in] index Constant buffer index
   * \returns Binding index
   */
  inline uint32_t computeConstantBufferBinding(DxbcProgramType stage, uint32_t index) {
    return computeStageBindingOffset(stage) + DxbcConstBufBindingIndex + index;
  }


  /**
   * \brief Computes sampler binding index
   * 
   * \param [in] stage Shader stage
   * \param [in] index Sampler index
   * \returns Binding index
   */
  inline uint32_t computeSamplerBinding(DxbcProgramType stage, uint32_t index) {
    return computeStageBindingOffset(stage) + DxbcSamplerBindingIndex + index;
  }


  /**
   * \brief Computes resource binding index
   * 
   * \param [in] stage Shader stage
   * \param [in] index Resource index
   * \returns Binding index
   */
  inline uint32_t computeSrvBinding(DxbcProgramType stage, uint32_t index) {
    return computeStageBindingOffset(stage) + DxbcResourceBindingIndex + index;
  }


  /**
   * \brief Computes UAV binding offset
   *
   * \param [in] stage Shader stage
   * \param [in] index UAV index
   * \returns Binding index
   */
  inline uint32_t computeUavBinding(DxbcProgramType stage, uint32_t index) {
    return computeStageUavBindingOffset(stage) + index;
  }
  
  
  /**
   * \brief Computes UAV counter binding offset
   *
   * \param [in] stage Shader stage
   * \param [in] index UAV index
   * \returns Binding index
   */
  inline uint32_t computeUavCounterBinding(DxbcProgramType stage, uint32_t index) {
    return computeStageUavBindingOffset(stage) + DxbcUavBindingCount + index;
  }
  
  /**
   * \brief Primitive vertex count
   * 
   * Calculates the number of vertices
   * for a given primitive type.
   */
  uint32_t primitiveVertexCount(
          DxbcPrimitive   primitive);
  
}
