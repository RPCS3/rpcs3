#pragma once

#include "dxbc_include.h"

namespace dxvk {
  
  /**
   * \brief DXBC Program type
   * 
   * Defines the shader stage that a DXBC
   * module has been compiled form.
   */
  enum class DxbcProgramType : uint16_t {
    PixelShader     = 0,
    VertexShader    = 1,
    GeometryShader  = 2,
    HullShader      = 3,
    DomainShader    = 4,
    ComputeShader   = 5,

    Count
  };

  using DxbcProgramTypeFlags = Flags<DxbcProgramType>;
  
  
  /**
   * \brief DXBC shader info
   * 
   * Stores the shader program type.
   */
  class DxbcProgramInfo {
    
  public:
    
    DxbcProgramInfo() { }
    DxbcProgramInfo(DxbcProgramType type)
    : m_type(type) { }
    
    /**
     * \brief Program type
     * \returns Program type
     */
    DxbcProgramType type() const {
      return m_type;
    }
    
    /**
     * \brief Vulkan shader stage
     * 
     * The \c VkShaderStageFlagBits constant
     * that corresponds to the program type.
     * \returns Vulkan shaer stage
     */
    VkShaderStageFlagBits shaderStage() const;
    
    /**
     * \brief SPIR-V execution model
     * 
     * The execution model that corresponds
     * to the Vulkan shader stage.
     * \returns SPIR-V execution model
     */
    spv::ExecutionModel executionModel() const;
    
  private:
    
    DxbcProgramType m_type  = DxbcProgramType::PixelShader;
    
  };
  
}
