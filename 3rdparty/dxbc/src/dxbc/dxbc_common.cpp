#include "dxbc_common.h"

namespace dxvk {
  
  VkShaderStageFlagBits DxbcProgramInfo::shaderStage() const {
    switch (m_type) {
      case DxbcProgramType::PixelShader    : return VK_SHADER_STAGE_FRAGMENT_BIT;
      case DxbcProgramType::VertexShader   : return VK_SHADER_STAGE_VERTEX_BIT;
      case DxbcProgramType::GeometryShader : return VK_SHADER_STAGE_GEOMETRY_BIT;
      case DxbcProgramType::HullShader     : return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
      case DxbcProgramType::DomainShader   : return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
      case DxbcProgramType::ComputeShader  : return VK_SHADER_STAGE_COMPUTE_BIT;
      default: throw DxvkError("DxbcProgramInfo::shaderStage: Unsupported program type");
    }
  }
  
  
  spv::ExecutionModel DxbcProgramInfo::executionModel() const {
    switch (m_type) {
      case DxbcProgramType::PixelShader    : return spv::ExecutionModelFragment;
      case DxbcProgramType::VertexShader   : return spv::ExecutionModelVertex;
      case DxbcProgramType::GeometryShader : return spv::ExecutionModelGeometry;
      case DxbcProgramType::HullShader     : return spv::ExecutionModelTessellationControl;
      case DxbcProgramType::DomainShader   : return spv::ExecutionModelTessellationEvaluation;
      case DxbcProgramType::ComputeShader  : return spv::ExecutionModelGLCompute;
      default: throw DxvkError("DxbcProgramInfo::executionModel: Unsupported program type");
    }
  }
  
}
